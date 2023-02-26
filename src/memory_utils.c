
#define _XOPEN_SOURCE 500
#include "memory_utils.h"

#include <errstack.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

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
	ES_NEW_ASRT(((pagemap_entry >> 63) & 1) == 0, "Expected entry exists in RAM");
	ES_NEW_ASRT(((pagemap_entry >> 62) & 1) == 0, "Expected entry is not swapped");

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
		if (((pagemap_entry >> 63) & 1) || ((pagemap_entry >> 62) & 1)) {
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