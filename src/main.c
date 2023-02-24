/**
 * Copyright by Benjamin Joseph Correia.
 * Date: 2022-08-11
 * License: MIT
 *
 * Description:
 * Systolic benchmark
 */

#include <errno.h>
#include <fcntl.h>
#include <hps.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "errstack.h"
#define soc_cv_av
#include "hwlib.h"
#include "socal/hps.h"
#include "socal/socal.h"
#include "util.h"

#define HPS2FPGA_BASE (0xC0000000)
//#define HPS2FPGA_BASE (0xFF200000)
#define HPS2FPGA_SPAN (0x00200000)

/*
#define HPS2FPGA_SPAN (0x00200000)

#define HW_REGS_BASE (ALT_STM_OFST)
#define HW_REGS_SPAN (0x04000000)
#define HW_REGS_MASK (HW_REGS_SPAN - 1)
*/
#define FIFO_IS_FULL(csr)    !!(*(csr + 1) & 0b000001)
#define FIFO_IS_EMPTY(csr)   !!(*(csr + 1) & 0b000010)
#define FIFO_FILL_LEVEL(csr) (*csr)
#define FIFO_DEPTH           (128)

#define N_TRANSFERS  (512 * 1024)
#define VERIFY_VALUE 0

struct prog_state_s
{
	int fd_dev_mem;
	void *virtual_base;
	int64_t expected;
	int64_t value;

	int32_t send_count;
	int32_t recv_count;

	volatile int64_t *fifo_to_copro_data;
	volatile int32_t *fifo_to_copro_csr;
	volatile int64_t *fifo_to_hps_data;
	volatile int32_t *fifo_to_hps_csr;
	volatile int32_t *pio_status;
};

static void _state_cleanup(UNUSED struct prog_state_s *state)
{
	if (state->virtual_base) {
		munmap(state->virtual_base, HPS2FPGA_SPAN);
		state->virtual_base = NULL;
	}
	if (state->fd_dev_mem >= 0) {
again:
		int res = close(state->fd_dev_mem);
		if (res <= 0 && errno == EINTR) {
			goto again;
		}
		state->fd_dev_mem = -1;
	}
}

static int _recv(struct prog_state_s *s)
{
	int64_t data[FIFO_TO_HPS_OUT_CSR_FIFO_DEPTH];
	int retval = 0;
	if (!FIFO_IS_EMPTY(s->fifo_to_hps_csr)) {
		int n  = FIFO_FILL_LEVEL(s->fifo_to_hps_csr);
		retval = 1;
		for (int i = 0; i < n; i++) {
			data[i] = *(s->fifo_to_hps_data);
		}
		s->recv_count += n;
		if (VERIFY_VALUE) {
			for (int i = 0; i < n; i++) {
				ES_NEW_ASRT(data[i] == s->expected,
				            "Assertion failed with data[%d]=0x%X",
				            i,
				            data[i]);
			}
		}
	}
	return retval;
}

static int _send(struct prog_state_s *s)
{
	int retval = 0;
	if (!FIFO_IS_FULL(s->fifo_to_copro_csr)) {
		int n = FIFO_DEPTH - FIFO_FILL_LEVEL(s->fifo_to_copro_csr);
		for (int i = 0; i < n; i++) {
			*(s->fifo_to_copro_data) = s->value;
		}
		retval = 1;
		s->send_count += n;
	}
	return retval;
}

static int _pipeline(UNUSED int argc, UNUSED char **argv)
{
	puts("starting pipeline");
	CLEANUP(_state_cleanup) struct prog_state_s state = {};

	ES_NEW_INT_ERRNO(state.fd_dev_mem = open("/dev/mem", (O_RDWR | O_SYNC)));
	ES_NEW_ASRT_ERRNO((state.virtual_base = mmap(NULL,
	                                             HPS2FPGA_SPAN,
	                                             PROT_READ | PROT_WRITE,
	                                             MAP_SHARED,
	                                             state.fd_dev_mem,
	                                             HPS2FPGA_BASE)) != MAP_FAILED);

	state = (struct prog_state_s){
	    .fd_dev_mem         = state.fd_dev_mem,
	    .virtual_base       = state.virtual_base,
	    .value              = (0x3C00ll << 48) | (0x4248ll << 32) | (0x3C00ll << 16) | (0x4248ll),
	    .expected           = (0x4248ll << 32) | (0x4248ll),
	    .fifo_to_copro_data = (void *) (state.virtual_base + FIFO_TO_COPRO_IN_BASE),
	    .fifo_to_copro_csr  = (void *) (state.virtual_base + FIFO_TO_COPRO_IN_CSR_BASE),
	    .fifo_to_hps_data   = (void *) (state.virtual_base + FIFO_TO_HPS_OUT_BASE),
	    .fifo_to_hps_csr    = (void *) (state.virtual_base + FIFO_TO_HPS_OUT_CSR_BASE),
	    .pio_status         = (void *) (state.virtual_base + PIO_0_BASE),
	};
	printf("virtual_base=%X\n", (unsigned int) state.virtual_base);
	printf("fd_dev_mem=%d\n", state.fd_dev_mem);

	int a = *state.pio_status;
	while (state.send_count != N_TRANSFERS) {
		int res = _send(&state);
		int res_recv;
		ES_FWD_INT_NM(res_recv = _recv(&state));
		if (res > 0) {
			/*printf("send_count=%d, recv_count=%d, pio_status=%d, send_fill=%d, recv_fill=%d\n",
			       state.send_count,
			       state.recv_count,
			       *state.pio_status,
			       *state.fifo_to_copro_csr,
			       *state.fifo_to_hps_csr);*/
		}
	}
	// printf("Done sending\n");
	while (state.recv_count != N_TRANSFERS) {
		int res;
		ES_FWD_INT_NM(res = _recv(&state));
		if (res > 0) {
			// printf("recv_count=%d\n", state.recv_count);
		}
	}
	int b = *state.pio_status;
	printf("Done reading\n");
	printf("Pipeline finished\n");
	printf("b-a=%d\n", b - a);

	double bandwidth = (95.0) * (N_TRANSFERS * 8 * 2) / (b - a);
	printf("bandwidth = %f MBps\n", bandwidth);
	return 0;
}

int main(UNUSED int argc, UNUSED char **argv)
{
	int retval = -1;
	/* Execute program */
	retval = _pipeline(argc, argv);
	if (retval < 0) {
		ES_FWD("Pipeline failed");
		printf("Unrecoverable: [ ");
		ES_PRINT();
		printf("\n ]\n");
		return -1;
	}
	return 0;
}
