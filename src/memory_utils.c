
#define _XOPEN_SOURCE 500
#include "memory_utils.h"

#include <errstack.h>
#include <fcntl.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <util.h>

#include "util.h"

#define _PRESENT_IN_RAM(entry) (!!((pagemap_entry >> 63) & 1))
#define _SWAPPED(entry)        (!!((pagemap_entry >> 62) & 1))

int _get_pagemap_entry(uint64_t *dst, uint64_t virtual_addr)
{
	int read_count;
	char proc_pid_pagemap[256];
	CLEAN_FD int pagemap_fd = -1;

	uint64_t virtual_page_n = virtual_addr / sysconf(_SC_PAGE_SIZE);

	ES_NEW_INT_NM(
	    snprintf(proc_pid_pagemap, sizeof(proc_pid_pagemap), "/proc/%d/pagemap", getpid()));
	ES_NEW_INT_ERRNO(pagemap_fd = open(proc_pid_pagemap, O_RDONLY));
	ES_NEW_INT_ERRNO(
	    read_count = pread(pagemap_fd, (void *) dst, sizeof(*dst), virtual_page_n * sizeof(*dst)));
	ES_NEW_ASRT(read_count == sizeof(*dst), "Failed to read in one go");
	return 0;
}

int mu_virt_to_phys(uint64_t *phys_addr, uint64_t virtual_addr)
{
	uint64_t pagemap_entry;
	uint64_t pageframe_n;
	ES_NEW_ASRT(phys_addr, "NULL check failed");
	ES_FWD_INT_NM(_get_pagemap_entry(&pagemap_entry, virtual_addr));
	ES_NEW_ASRT(_PRESENT_IN_RAM(entry), "Expected entry exists in RAM");
	ES_NEW_ASRT(!_SWAPPED(entry), "Expected entry is not swapped");

	pageframe_n = pagemap_entry & (((uint64_t) 1 << 55) - 1);
	*phys_addr  = pageframe_n * sysconf(_SC_PAGE_SIZE) + virtual_addr % sysconf(_SC_PAGE_SIZE);

	return 0;
}

int mu_is_continuous(bool *is_continuous, uint64_t virtual_addr_a, uint64_t virtual_addr_b)
{
	uint64_t virtual_page_na  = virtual_addr_a / sysconf(_SC_PAGE_SIZE);
	uint64_t virtual_page_nb  = virtual_addr_b / sysconf(_SC_PAGE_SIZE);
	uint64_t pageframe_n_last = 0;

	ES_NEW_ASRT(virtual_addr_a < virtual_addr_b, "Expected a < b");

	for (uint64_t i = virtual_page_na; i <= virtual_page_nb; i++) {
		uint64_t pagemap_entry;
		uint64_t pageframe_n;
		ES_FWD_INT_NM(_get_pagemap_entry(&pagemap_entry, i * sysconf(_SC_PAGE_SIZE)));
		if (!_PRESENT_IN_RAM(pagemap_entry) || _SWAPPED(pagemap_entry)) {
			*is_continuous = false;
			return 0;
		}
		pageframe_n = pagemap_entry & (((uint64_t) 1 << 55) - 1);
		if (i == virtual_page_na) {
			pageframe_n_last = pageframe_n;
			continue;
		}
		if (pageframe_n_last + 1 != pageframe_n) {
			*is_continuous = false;
			return 0;
		}
		pageframe_n_last = pageframe_n;
	}
	*is_continuous = true;
	return 0;
}

void *mu_alloc(int n_pages)
{
	return valloc(n_pages * sysconf(_SC_PAGE_SIZE));
}

enum attr_format
{
	ATTR_FORMAT_INT,
	ATTR_FORMAT_HEX,
};

int _read_attr(uint32_t *dst, int id, const char *attr_name, enum attr_format fmt)
{
	char file_name[64];
	char attr[1024] = {};
	CLEAN_FD int fd;
	ES_NEW_INT(
	    snprintf(file_name, sizeof(file_name), "/sys/class/u-dma-buf/udmabuf%d/%s", id, attr_name),
	    "failed to make file name");
	ES_NEW_INT_ERRNO(fd = open(file_name, O_RDONLY));
	ES_NEW_INT_ERRNO(read(fd, attr, sizeof(attr) - 1));
	switch (fmt) {
	case ATTR_FORMAT_INT:
		sscanf(attr, "%d", dst);
		break;
	case ATTR_FORMAT_HEX:
		sscanf(attr, "%x", dst);
		break;
	default:
		ES_NEW("Given bad attr_format");
		break;
	}
	return 0;
}

int _write_attr(int id, const char *attr_name, enum attr_format fmt, uint32_t value)
{
	char file_name[64];
	char attr[1024] = {};
	CLEAN_FD int fd;
	ES_NEW_INT(
	    snprintf(file_name, sizeof(file_name), "/sys/class/u-dma-buf/udmabuf%d/%s", id, attr_name),
	    "failed to make file name");
	ES_NEW_INT_ERRNO(fd = open(file_name, O_WRONLY));
	switch (fmt) {
	case ATTR_FORMAT_INT:
		snprintf(attr, sizeof(attr), "%d", value);
		break;
	case ATTR_FORMAT_HEX:
		snprintf(attr, sizeof(attr), "0x%x", value);
		break;
	default:
		ES_NEW("Given bad attr_format");
		break;
	}
	ES_NEW_INT_ERRNO(write(fd, attr, strlen(attr)));
	return 0;
}

int _write_sync_for(int id, const char *attr_name, uint32_t upper, uint32_t lower)
{
	char file_name[64];
	char attr[1024] = {};
	CLEAN_FD int fd;
	ES_NEW_INT(
	    snprintf(file_name, sizeof(file_name), "/sys/class/u-dma-buf/udmabuf%d/%s", id, attr_name),
	    "failed to make file name");
	ES_NEW_INT_ERRNO(fd = open(file_name, O_WRONLY));
	snprintf(attr, sizeof(attr), "0x%08X%08X", upper, lower);
	ES_NEW_INT_ERRNO(write(fd, attr, strlen(attr)));
	return 0;
}

int _mmap_udmabuf(udmabuf_t *src_dst, int id)
{
	// uint32_t size = src_dst->size;
	char file_name[64];
	CLEAN_FD int fd = -1;
	ES_NEW_INT_NM(snprintf(file_name, sizeof(file_name), "/dev/udmabuf%d", id));
	ES_NEW_INT_ERRNO(fd = open(file_name, O_RDWR));
	src_dst->fd = fd;
	// src_dst->virtual_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	// ES_NEW_ASRT_ERRNO((int) src_dst->virtual_base > 0);
	fd = -1;
	return 0;
}

int mu_get_udmabuf(udmabuf_t *dst, int id)
{
	CLEANUP(cleanup_udmabuf) udmabuf_t tmp = {.id = id, .fd = -1};
	ES_NEW_ASRT(id >= 0 && id < 8, "id must be in [0,8), it was %d", id);
	ES_FWD_INT_NM(_read_attr(&tmp.size, id, "size", ATTR_FORMAT_INT));
	ES_FWD_INT_NM(_read_attr(&tmp.phys_addr, id, "phys_addr", ATTR_FORMAT_HEX));
	ES_FWD_INT_NM(_write_attr(id, "sync_mode", ATTR_FORMAT_INT, MU_SYNC_MODE_CACHE_ENABLE2));
	ES_FWD_INT_NM(_mmap_udmabuf(&tmp, id));
	memcpy(dst, &tmp, sizeof(*dst));
	tmp = (udmabuf_t){.fd = -1};
	return 0;
}

int mu_udmabuf_sync_for_cpu(udmabuf_t *target, uint32_t sync_offset, uint32_t sync_size)
{
	const uint32_t sync_direction = MU_SYNC_DIRECTION_DMA_BIDIRECTIONAL;
	const uint32_t sync_for_cpu   = 1;
	ES_FWD_INT_NM(_write_sync_for(target->id,
	                              "sync_for_cpu",
	                              (sync_offset & 0xFFFFFFFF),
	                              (sync_size & 0xFFFFFFF0) | (sync_direction << 2) | sync_for_cpu));
	return 0;
}
int mu_udmabuf_sync_for_device(udmabuf_t *target, uint32_t sync_offset, uint32_t sync_size)
{
	const uint32_t sync_direction  = MU_SYNC_DIRECTION_DMA_BIDIRECTIONAL;
	const uint32_t sync_for_device = 1;
	ES_FWD_INT_NM(
	    _write_sync_for(target->id,
	                    "sync_for_device",
	                    (sync_offset & 0xFFFFFFFF),
	                    (sync_size & 0xFFFFFFF0) | (sync_direction << 2) | sync_for_device));
	return 0;
}

void cleanup_udmabuf(udmabuf_t *to_clean)
{
	if (!to_clean) {
		return;
	}
	if (to_clean->virtual_base) {
		munmap(to_clean->virtual_base, to_clean->size);
	}
	if (to_clean->fd >= 0) {
		close(to_clean->fd);
	}
}

#define _SDRAM_CSR_PRINTF(sig) printf("%30s: 0x%08X\n", #sig, *(volatile uint32_t *) (src + sig))

void mu_sdram_csr_print(volatile void *src)
{
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_CTRLCFG);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMTIMING1);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMTIMING2);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMTIMING3);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMTIMING4);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_LOWPWRTIMING);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMODT);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMADDRW);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMIFWIDTH);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMSTS);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DRAMINTR);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_SBECOUNT);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DBECOUNT);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_ERRADDR);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DROPCOUNT);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_DROPADDR);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_LOWPWREQ);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_LOWPWRACK);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_STATICCFG);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_CTRLWIDTH);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_PORTCFG);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_FPGAPORTRST);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_PROTPORTDEFAULT);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_PROTRULEADDR);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_PROTRULEID);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_PROTRULEDATA);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_PROTRULERDWR);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_MPPRIORITY);
	_SDRAM_CSR_PRINTF(MU_SDRAM_CSR_REMAPPRIORITY);
}