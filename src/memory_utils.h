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
	//uint32_t dma_coherent;
};
typedef struct udmabuf_s udmabuf_t;

int mu_virt_to_phys(uint64_t *phys_addr, uint64_t virtual_addr);
int mu_is_continuous(bool *is_continuous, uint64_t virtual_addr_a, uint64_t virtual_addr_b);
void *mu_alloc(int n_pages);

int mu_get_udmabuf(udmabuf_t *dst, int id);
int mu_udmabuf_sync_for_cpu(udmabuf_t *target, uint32_t sync_offset, uint32_t sync_size);
int mu_udmabuf_sync_for_device(udmabuf_t *target, uint32_t sync_offset, uint32_t sync_size);

void cleanup_udmabuf(udmabuf_t *to_clean);