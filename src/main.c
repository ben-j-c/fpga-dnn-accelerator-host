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
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define soc_cv_av
#include "errstack.h"
#include "hwlib.h"
#include "memory_utils.h"
#include "socal/hps.h"
#include "socal/socal.h"
#include "util.h"

// Cyclone V Hard Processor System Technical Reference Manual, Table 2-2
#define HPS2FPGA_BASE (0xC0000000)
// Size of 2 MiB, can be as much as 960 MiB
#define HPS2FPGA_SPAN (0x00200000)
// Cyclone V Hard Processor System Technical Reference Manual, Table 2-3
#define SDRAMCSR_BASE (0xFFC20000)
// Cyclone V Hard Processor System Technical Reference Manual, Table 2-3
#define SDRAMCSR_SPAN (0x000E0000)

#define FIFO_IS_FULL(csr)    !!(*(csr + 1) & 0b000001)
#define FIFO_IS_EMPTY(csr)   !!(*(csr + 1) & 0b000010)
#define FIFO_FILL_LEVEL(csr) (*csr)
#define FIFO_DEPTH           (256)

// 64 MiB
#define N_BYTES      (32 * (1 << 20))
#define VERIFY_VALUE 0

struct prog_state_s
{
	int fd_dev_mem;
	void *virtual_base;

	int fd_sdramcsr;
	void *sdramcsr_base;

	udmabuf_t udmabuf;

	int32_t send_count;
	int32_t recv_count;

	volatile int64_t *fifo_to_copro_data;
	volatile int32_t *fifo_to_copro_csr;
	volatile int64_t *fifo_to_hps_data;
	volatile int32_t *fifo_to_hps_csr;
	volatile int32_t *pio_status;
};

static void _state_cleanup(struct prog_state_s *state)
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
	cleanup_udmabuf(&state->udmabuf);
}

static int _recv(uint64_t *dst, struct prog_state_s *s)
{
	int retval = 0;
	int n      = FIFO_FILL_LEVEL(s->fifo_to_hps_csr);
	if (n > 0) {
		*dst   = *(s->fifo_to_hps_data);
		retval = 1;
		s->recv_count += 1;
	}
	return retval;
}

static int _send(struct prog_state_s *s, uint64_t data)
{
	int retval = 0;
	int n      = FIFO_DEPTH - FIFO_FILL_LEVEL(s->fifo_to_copro_csr);
	if (n > 0) {
		*(s->fifo_to_copro_data) = data;
		retval                   = 1;
		s->send_count += 1;
	}
	return retval;
}

#define _RECV(state)                                                                               \
	({                                                                                             \
		uint64_t _recv_val;                                                                        \
		while (_recv(&_recv_val, &state) == 0)                                                     \
			sched_yield();                                                                         \
		_recv_val;                                                                                 \
	})

#define _SEND(state, value)                                                                        \
	({                                                                                             \
		uint64_t _send_val = value;                                                                \
		while (_send(&state, _send_val) == 0)                                                      \
			sched_yield();                                                                         \
	})

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
	ES_NEW_INT_ERRNO(state.fd_sdramcsr = open("/dev/mem", (O_RDWR | O_SYNC)));
	ES_NEW_ASRT_ERRNO((state.sdramcsr_base = mmap(NULL,
	                                              SDRAMCSR_SPAN,
	                                              PROT_READ | PROT_WRITE,
	                                              MAP_SHARED,
	                                              state.fd_dev_mem,
	                                              SDRAMCSR_BASE)) != MAP_FAILED);
	ES_FWD_INT(mu_get_udmabuf(&state.udmabuf, 0), "Failed to get udmabuf0");

	{
		*(volatile uint32_t *) (state.sdramcsr_base + MU_SDRAM_CSR_FPGAPORTRST) = 0x3FFF;
	}

	state = (struct prog_state_s){
	    .fd_dev_mem         = state.fd_dev_mem,
	    .virtual_base       = state.virtual_base,
	    .udmabuf            = state.udmabuf,
	    .fd_sdramcsr        = state.fd_sdramcsr,
	    .sdramcsr_base      = state.sdramcsr_base,
	    .fifo_to_copro_data = (void *) (state.virtual_base + FIFO_TO_COPRO_IN_BASE),
	    .fifo_to_copro_csr  = (void *) (state.virtual_base + FIFO_TO_COPRO_IN_CSR_BASE),
	    .fifo_to_hps_data   = (void *) (state.virtual_base + FIFO_TO_HPS_OUT_BASE),
	    .fifo_to_hps_csr    = (void *) (state.virtual_base + FIFO_TO_HPS_OUT_CSR_BASE),
	    .pio_status         = (void *) (state.virtual_base + PIO_0_BASE),
	};
	// Test pattern
	printf("Writing test pattern\n");
	uint32_t sum = 0;
	for (uint32_t i = 0; i < N_BYTES; i++) {
		uint8_t *dst = (uint8_t *) (state.udmabuf.virtual_base + i);
		*dst         = ((i % 16) << 4) | (i % 16);
	}
	for (int i = 0; i < N_BYTES / 4; i++) {
		sum += *(uint32_t *) (state.udmabuf.virtual_base + i * 4);
	}
	printf("sum=%08x\n", sum);
	printf("Done writing test pattern\n");
	printf("Calling sync\n");
	ES_FWD_INT_NM(mu_udmabuf_sync_for_device(&state.udmabuf, 0, state.udmabuf.size));
	printf("Done calling sync\n");
	printf("send_n=%u\n", *state.fifo_to_copro_csr);
	printf("recv_n=%u\n", *state.fifo_to_hps_csr);
	uint64_t a, b;
	{
		_SEND(state, 0);
		_SEND(state, ((uint64_t) N_BYTES << 32 | state.udmabuf.phys_addr));
		printf("Sent addr and len\n");
		a = _RECV(state);
		printf("Recv'd %016llx\n", a);
		b = _RECV(state);
		printf("Recv'd %016llx\n", b);
		printf("checksum=0x%08x (%s)\n",
		       (uint32_t) (b >> 32) + sum,
		       (uint32_t) (b >> 32) + sum == 0 ? "good" : "bad");
	}
	printf("Done reading\n");
	printf("send_n=%u\n", *state.fifo_to_copro_csr);
	printf("recv_n=%u\n", *state.fifo_to_hps_csr);

	double bandwidth = (95.0) * N_BYTES / ((b & 0xFFFFFFFF) - (a & 0xFFFFFFFF));
	printf("bandwidth = %f MBps\n", bandwidth);
	return 0;
}

int main(int argc, char **argv)
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
