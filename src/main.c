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

#pragma pack(push, 1)
typedef struct matrix_intrinsic_s
{
	uint8_t data[16][16];
} matrix_t;
#pragma pack(pop)

struct prog_state_s
{
	int fd_dev_mem;
	void *virtual_base;

	udmabuf_t udmabuf;

	int32_t send_count;
	int32_t recv_count;

	volatile uint64_t *fifo_instr;
	volatile int32_t *fifo_instr_csr;
	volatile int32_t *pio_status;
	volatile uint32_t *systolic_csr;

	struct
	{
		volatile uint32_t *csr;
		volatile uint32_t *descriptor;
	} read_dma;

	struct
	{
		volatile uint32_t *csr;
		volatile uint64_t *descriptor;
	} write_dma;
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

void print_state(struct prog_state_s *s)
{
	printf("sys_state: 0x%08x\n", *(s->systolic_csr));
	printf("sys_n_col: 0x%08x\n", *(s->systolic_csr + 1));
	printf("sys_n_row: 0x%08x\n", *(s->systolic_csr + 2));
	printf("sys_cycle: 0x%08x\n", *(s->systolic_csr + 3));
	printf("sys_stream: 0x%08x\n", *(s->systolic_csr + 4));
	printf("sys_c0: 0x%08x\n", *(s->systolic_csr + 5));
	printf("sys_c1: 0x%08x\n", *(s->systolic_csr + 6));
	printf("sys_c2: 0x%08x\n", *(s->systolic_csr + 7));
	printf("sys_c3: 0x%08x\n", *(s->systolic_csr + 8));
	printf("sys_c4: 0x%08x\n", *(s->systolic_csr + 9));
	printf("sys_c5: 0x%08x\n", *(s->systolic_csr + 10));
	printf("sys_c6: 0x%08x\n", *(s->systolic_csr + 11));
	printf("sys_c7: 0x%08x\n", *(s->systolic_csr + 12));
	printf("wr_status: 0x%08x\n", *(s->write_dma.csr));
	printf("wr_fill: 0x%08x\n", *(s->write_dma.csr + 2));
	printf("rd_status: 0x%08x\n", *(s->read_dma.csr));
	printf("rd_fill: 0x%08x\n\n", *(s->read_dma.csr + 2));
}

static int _send_instr(struct prog_state_s *s, uint64_t data)
{
	int retval = 0;
	int n      = FIFO_INSTR_IN_CSR_FIFO_DEPTH - FIFO_FILL_LEVEL(s->fifo_instr_csr);
	if (n > 0) {
		*(s->fifo_instr) = data;
		retval           = 1;
		s->send_count += 1;
	}
	return retval;
}

#define _SEND_INSTR(state, n_rows, n_cols)                                                         \
	({                                                                                             \
		uint64_t _send_val = (n_cols & 0b111111) | ((n_rows & 0b111111) << 6);                     \
		while (_send_instr(state, _send_val) == 0)                                                 \
			sched_yield();                                                                         \
	})

static int _send_read(struct prog_state_s *s,
                      uint32_t phys_addr,
                      uint32_t n_bytes,
                      uint32_t channel)
{
	// ES_NEW_ASRT((phys_addr & 0x1F) == 0, "physical address must be 32 byte aligned");
	// ES_NEW_ASRT((n_bytes & 0x1F) == 0, "length must be in 32 byte increments");
	*s->read_dma.descriptor       = phys_addr;
	*(s->read_dma.descriptor + 2) = n_bytes;
	// Go bit and early done enable
	*(s->read_dma.descriptor + 3) = (1u << 31) | (1u << 24) | (channel & 0xFF);
	return 0;
}

static int _send_write(struct prog_state_s *s, uint32_t phys_addr)
{
	// ES_NEW_ASRT((phys_addr & 0x1F) == 0, "physical address must be 32 byte aligned");
	// ES_NEW_ASRT((n_bytes & 0x1F) == 0, "length must be in 32 byte increments");
	*(s->write_dma.descriptor) = phys_addr;
	return 0;
}

/**
 * @brief Perform a matrix multiplication with FPGA hardware
 *
 * @param s program state handle for hardware
 * @param dst column-major order
 * @param left_src column-major order
 * @param right_src row-major order
 */
void matrix_mult16(struct prog_state_s *s, matrix_t *dst, matrix_t *left_src, matrix_t *right_src)
{
	_send_read(s, (uint32_t) left_src, sizeof(matrix_t), 1);
	printf("send read1\n");
	_send_read(s, (uint32_t) right_src, sizeof(matrix_t), 0);
	printf("send read2\n");
	_send_write(s, (uint32_t) dst);
	printf("send write\n");
	_SEND_INSTR(s, 16, 16);
	printf("send instr\n");

	while (({
		uint32_t status = *(s->write_dma.csr);
		status;
	})) {
		sched_yield();
	}
	return;
}

void _print_mat(matrix_t *src, bool col_major)
{
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			int v = col_major ? src->data[j][i] : src->data[i][j];
			printf("%02x ", v);
		}
		printf("\n");
	}
}

static int _pipeline(int argc, char **argv)
{
	puts("starting pipeline");
	CLEANUP(_state_cleanup) struct prog_state_s state = {};
	volatile void *sdramcsr_base;

	ES_NEW_INT_ERRNO(state.fd_dev_mem = open("/dev/mem", (O_RDWR | O_SYNC)));
	ES_NEW_ASRT_ERRNO((state.virtual_base = mmap(NULL,
	                                             HPS2FPGA_SPAN,
	                                             PROT_READ | PROT_WRITE,
	                                             MAP_SHARED,
	                                             state.fd_dev_mem,
	                                             HPS2FPGA_BASE)) != MAP_FAILED);
	ES_NEW_ASRT_ERRNO((sdramcsr_base = mmap(NULL,
	                                        SDRAMCSR_SPAN,
	                                        PROT_READ | PROT_WRITE,
	                                        MAP_SHARED,
	                                        state.fd_dev_mem,
	                                        SDRAMCSR_BASE)) != MAP_FAILED);
	ES_FWD_INT(mu_get_udmabuf(&state.udmabuf, 0), "Failed to get udmabuf0");

	state = (struct prog_state_s){
	    .fd_dev_mem     = state.fd_dev_mem,
	    .virtual_base   = state.virtual_base,
	    .udmabuf        = state.udmabuf,
	    .fifo_instr     = (void *) (state.virtual_base + FIFO_INSTR_IN_BASE),
	    .fifo_instr_csr = (void *) (state.virtual_base + FIFO_INSTR_IN_CSR_BASE),
	    .pio_status     = (void *) (state.virtual_base + PIO_0_BASE),
	    .systolic_csr   = (void *) (state.virtual_base + SYSTOLIC_CORE_BASE),
	    .read_dma =
	        {
	            .csr        = (void *) (state.virtual_base + MSGDMA_READ_CSR_BASE),
	            .descriptor = (void *) (state.virtual_base + MSGDMA_READ_DESCRIPTOR_SLAVE_BASE),
	        },
	    .write_dma =
	        {
	            .csr        = (void *) (state.virtual_base + DMA_WRITE_BASE),
	            .descriptor = (void *) (state.virtual_base + DMA_WRITE_FIFO_IN_BASE),
	        },
	};

	if (argc > 1) {
		const char arg = argv[1][0];
		if (arg == '1') {
			print_state(&state);
		} else if (arg == '2') {
			matrix_t *mat_v = state.udmabuf.virtual_base;
			printf("dst\n");
			_print_mat(&mat_v[0], true);
			printf("src1\n");
			_print_mat(&mat_v[1], true);
			printf("src2\n");
			_print_mat(&mat_v[2], true);
		} else if (arg == '3') {
			printf("Write DMA CSR: \n\t0x%08x\n\t0x%08x\n\t0x%08x\n\t0x%08x\n",
			       *(state.write_dma.csr),
			       *(state.write_dma.csr + 1),
			       *(state.write_dma.csr + 2),
			       *(state.write_dma.csr + 3));

		} else if (arg == '4') {
			printf("Sending null write to DMA write\n");
			*(state.write_dma.descriptor) = 0ull;
		} else if (arg == '5') {
			printf("Sending default write to DMA write\n");
			*(state.write_dma.descriptor) = state.udmabuf.phys_addr;
		} else if (arg == '6') {
			memset(state.udmabuf.virtual_base, 0, sizeof(matrix_t));
			ES_FWD_INT_NM(mu_udmabuf_sync_for_device(&state.udmabuf, 0, state.udmabuf.size));
		} else if (arg == '7') {
			mu_sdram_csr_print(sdramcsr_base);
		} else if (arg == '8') {
			ES_FWD_INT_NM(mu_udmabuf_sync_for_cpu(&state.udmabuf, 0, state.udmabuf.size));
			matrix_t *mat_v = state.udmabuf.virtual_base;
			_print_mat(&mat_v[0], false);
		}
		return 0;
	}
	// memset(state.udmabuf.virtual_base, 0, sizeof(matrix_t) * 3);
	// Write two 16x16 matricies
	{
		matrix_t *mat = state.udmabuf.virtual_base;
		memset(mat, 0, sizeof(matrix_t) * 3);
		for (int i = 0; i < 16; i++) {
			mat[2].data[i][i]            = i;  // rhs, rowmajor
			mat[2].data[(i + 1) % 16][i] = 1;
			mat[2].data[(i + 2) % 16][i] = 2;
			for (int j = 0; j < 16; j++) {
				mat[1].data[i][j] = 1;  // lhs, colmajor
				// mat[2].data[i][j] = 1;
			}
		}
		printf("Done patterning\n");
	}
	ES_FWD_INT_NM(mu_udmabuf_sync_for_device(&state.udmabuf, 0, state.udmabuf.size));
	printf("Done sync1\n");
	{
		matrix_t *mat_v = state.udmabuf.virtual_base;
		printf("dst\n");
		_print_mat(&mat_v[0], true);
		printf("src1\n");
		_print_mat(&mat_v[1], true);
		printf("src2\n");
		_print_mat(&mat_v[2], true);

		matrix_t *mat = (void *) state.udmabuf.phys_addr;
		printf("%08x, %08x, %08x\n", (uint32_t) &mat[0], (uint32_t) &mat[1], (uint32_t) &mat[2]);
		matrix_mult16(&state, &mat[0], &mat[1], &mat[2]);
		ES_FWD_INT_NM(mu_udmabuf_sync_for_cpu(&state.udmabuf, 0, state.udmabuf.size));
		printf("Done sync2\n");

		printf("dst\n");
		_print_mat(&mat_v[0], true);
		printf("src1\n");
		_print_mat(&mat_v[1], true);
		printf("src2\n");
		_print_mat(&mat_v[2], true);
	}
	printf("instr_n=%u\n", *state.fifo_instr_csr);
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
