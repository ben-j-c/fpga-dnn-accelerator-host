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

#define soc_cv_av
#include "errstack.h"
#include "hwlib.h"
#include "memory_utils.h"
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
#define FIFO_DEPTH           (256)

#define N_TRANSFERS (512 * 1024)
//#define N_BYTES (N_TRANSFERS * 8 * 2)
#define N_BYTES      (4096 * 2048)
#define VERIFY_VALUE 0

struct prog_state_s
{
	int fd_dev_mem;
	void *virtual_base;
	uint32_t phy_dma_addr[N_BYTES / 4096];
	void *virt_dma_addr;

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
	if (state->virt_dma_addr) {
		munlock(state->virt_dma_addr, N_BYTES);
		free(state->virt_dma_addr);
	}
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
/*
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
*/

static int _allocate_buffer(struct prog_state_s *state)
{
	int n_pages = N_BYTES / sysconf(_SC_PAGE_SIZE);
	ES_NEW_ASRT_NM(state->virt_dma_addr = mu_alloc(n_pages));
	memset(state->virt_dma_addr, 0, N_BYTES);
	ES_NEW_INT_ERRNO(mlock(state->virt_dma_addr, N_BYTES));
	for (int i = 0; i < n_pages; i++) {
		uint64_t phys_addr;
		ES_FWD_ASRT_NM(
		    mu_virt_to_phys(&phys_addr, state->virt_dma_addr + i * sysconf(_SC_PAGE_SIZE)));
		state->phy_dma_addr[i] = (uint32_t) phys_addr;
	}
	return 0;
}

static int _pipeline(UNUSED int argc, UNUSED char **argv)
{
	puts("starting pipeline");
	CLEANUP(_state_cleanup) struct prog_state_s state = {};
	ES_FWD_INT_NM(_allocate_buffer(&state));
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
	    .virt_dma_addr      = state.virt_dma_addr,
	    .phy_dma_addr       = state.phy_dma_addr,
	    .fifo_to_copro_data = (void *) (state.virtual_base + FIFO_TO_COPRO_IN_BASE),
	    .fifo_to_copro_csr  = (void *) (state.virtual_base + FIFO_TO_COPRO_IN_CSR_BASE),
	    .fifo_to_hps_data   = (void *) (state.virtual_base + FIFO_TO_HPS_OUT_BASE),
	    .fifo_to_hps_csr    = (void *) (state.virtual_base + FIFO_TO_HPS_OUT_CSR_BASE),
	    .pio_status         = (void *) (state.virtual_base + PIO_0_BASE),
	};
	printf("virtual_base=%X\n", (unsigned int) state.virtual_base);
	printf("fd_dev_mem=%d\n", state.fd_dev_mem);
	int a = *state.pio_status;
	{
		/*uint64_t msg;
		_send(&state, ((uint64_t) N_BYTES << 32) | state.phy_dma_addr);
		while (_recv(&msg, &state) == 0)
		    ;
		ES_NEW_ASRT(msg == N_BYTES * 3, "Unexpected value from FPGA");*/
	}
	int b = *state.pio_status;
	printf("Done reading\n");

	double bandwidth = (95.0) * (N_BYTES) / (b - a);
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
