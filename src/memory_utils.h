#pragma once

#include <stdbool.h>
#include <stdint.h>

// sync_mode options
#define MU_SYNC_MODE_CACHE_ENABLE2        (0)
#define MU_SYNC_MODE_CACHE_OPTION         (1)
#define MU_SYNC_MODE_WRITE_COMBINE_OPTION (2)
#define MU_SYNC_MODE_DMA_COHERENCY_OPTION (3)
#define MU_SYNC_MODE_CACHE_ENABLE         (4)
#define MU_SYNC_MODE_CACHE_DISABLE        (5)
#define MU_SYNC_MODE_WRITE_COMBINE        (6)
#define MU_SYNC_MODE_DMA_COHERENCY        (7)

// sync_direction options
#define MU_SYNC_DIRECTION_DMA_BIDIRECTIONAL (0)
#define MU_SYNC_DIRECTION_DMA_TO_DEVICE     (1)
#define MU_SYNC_DIRECTION_DMA_FROM_DEVICE   (2)

// dma_coherent options
#define MU_DMA_COHERENT_GUARANTEED (1)

// sync_owner options
#define MU_SYNC_OWNER_CPU    (0)
#define MU_SYNC_OWNER_DEVICE (1)

// sync_for_cpu options
#define MU_SYNC_FOR_CPU_ENABLE  (1)
#define MU_SYNC_FOR_CPU_DISABLE (0)

// sync_for_device options
#define MU_SYNC_FOR_DEVICE_ENABLE  (1)
#define MU_SYNC_FOR_DEVICE_DISABLE (0)

struct udmabuf_s
{
	int fd;
	int id;
	void *virtual_base;
	uint32_t phys_addr;
	uint32_t size;
	// uint32_t dma_coherent;
};
typedef struct udmabuf_s udmabuf_t;

int mu_virt_to_phys(uint64_t *phys_addr, uint64_t virtual_addr);
int mu_is_continuous(bool *is_continuous, uint64_t virtual_addr_a, uint64_t virtual_addr_b);
void *mu_alloc(int n_pages);

int mu_get_udmabuf(udmabuf_t *dst, int id);
int mu_udmabuf_sync_for_cpu(udmabuf_t *target, uint32_t sync_offset, uint32_t sync_size);
int mu_udmabuf_sync_for_device(udmabuf_t *target, uint32_t sync_offset, uint32_t sync_size);

void cleanup_udmabuf(udmabuf_t *to_clean);

/*SDRAM Controller Address Map*/
// Width:32 Access:RW Reset:0x0 Descr:Controller Configuration Register
#define MU_SDRAM_CSR_CTRLCFG (0x5000)
// Width:32 Access:RW Reset:0x0 Descr:DRAM Timings 1 Register
#define MU_SDRAM_CSR_DRAMTIMING1 (0x5004)
// Width:32 Access:RW Reset:0x0 Descr:DRAM Timings 2 Register
#define MU_SDRAM_CSR_DRAMTIMING2 (0x5008)
// Width:32 Access:RW Reset:0x0 Descr:DRAM Timings 3 Register
#define MU_SDRAM_CSR_DRAMTIMING3 (0x500C)
// Width:32 Access:RW Reset:0x0 Descr:DRAM Timings 4 Register
#define MU_SDRAM_CSR_DRAMTIMING4 (0x5010)
// Width:32 Access:RW Reset:0x0 Descr:Lower Power Timing Register
#define MU_SDRAM_CSR_LOWPWRTIMING (0x5014)
// Width:32 Access:RW Reset:0x0 Descr:ODT Control Register
#define MU_SDRAM_CSR_DRAMODT (0x5018)
// Width:32 Access:RW Reset:0x0 Descr:DRAM Address Widths Register
#define MU_SDRAM_CSR_DRAMADDRW (0x502C)
// Width:32 Access:RW Reset:0x0 Descr:DRAM Interface Data Width Register
#define MU_SDRAM_CSR_DRAMIFWIDTH (0x5030)
// Width:32 Access:RW Reset:0x0 Descr:DRAM Status Register
#define MU_SDRAM_CSR_DRAMSTS (0x5038)
// Width:32 Access:RW Reset:0x0 Descr:ECC Interrupt Register
#define MU_SDRAM_CSR_DRAMINTR (0x503C)
// Width:32 Access:RW Reset:0x0 Descr:ECC Single Bit Error Count Register
#define MU_SDRAM_CSR_SBECOUNT (0x5040)
// Width:32 Access:RW Reset:0x0 Descr:ECC Double Bit Error Count Register
#define MU_SDRAM_CSR_DBECOUNT (0x5044)
// Width:32 Access:RW Reset:0x0 Descr:ECC Error Address Register
#define MU_SDRAM_CSR_ERRADDR (0x5048)
// Width:32 Access:RW Reset:0x0 Descr:ECC Auto-correction Dropped Count Register
#define MU_SDRAM_CSR_DROPCOUNT (0x504C)
// Width:32 Access:RW Reset:0x0 Descr:ECC Auto-correction Dropped Address Register
#define MU_SDRAM_CSR_DROPADDR (0x5050)
// Width:32 Access:RW Reset:0x0 Descr:Low Power Control Register
#define MU_SDRAM_CSR_LOWPWREQ (0x5054)
// Width:32 Access:RW Reset:0x0 Descr:Low Power Acknowledge Register
#define MU_SDRAM_CSR_LOWPWRACK (0x5058)
// Width:32 Access:RW Reset:0x0 Descr:Static Configuration Register
#define MU_SDRAM_CSR_STATICCFG (0x505C)
// Width:32 Access:RW Reset:0x0 Descr:Memory Controller Width Register
#define MU_SDRAM_CSR_CTRLWIDTH (0x5060)
// Width:32 Access:RW Reset:0x0 Descr:Port Configuration Register
#define MU_SDRAM_CSR_PORTCFG (0x507C)
// Width:32 Access:RW Reset:0x0 Descr:FPGA Ports Reset Control Register
#define MU_SDRAM_CSR_FPGAPORTRST (0x5080)
// Width:32 Access:RW Reset:0x0 Descr:Memory Protection Port Default Register
#define MU_SDRAM_CSR_PROTPORTDEFAULT (0x508C)
// Width:32 Access:RW Reset:0x0 Descr:Memory Protection Address Register
#define MU_SDRAM_CSR_PROTRULEADDR (0x5090)
// Width:32 Access:RW Reset:0x0 Descr:Memory Protection ID Register
#define MU_SDRAM_CSR_PROTRULEID (0x5094)
// Width:32 Access:RW Reset:0x0 Descr:Memory Protection Rule Data Register
#define MU_SDRAM_CSR_PROTRULEDATA (0x5098)
// Width:32 Access:RW Reset:0x0 Descr:Memory Protection Rule Read-Write Register
#define MU_SDRAM_CSR_PROTRULERDWR (0x509C)
// Width:32 Access:RW Reset:0x0 Descr:Scheduler priority Register
#define MU_SDRAM_CSR_MPPRIORITY (0x50AC)
// Width:32 Access:RW Reset:0x0 Descr:Controller Command Pool Priority Remap Register
#define MU_SDRAM_CSR_REMAPPRIORITY (0x50E0)

void mu_sdram_csr_print(volatile void *src);
