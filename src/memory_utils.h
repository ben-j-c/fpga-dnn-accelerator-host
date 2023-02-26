#pragma once

#include <stdbool.h>
#include <stdint.h>

int mu_virt_to_phys(uint64_t *phys_addr, uint64_t virtual_addr);
int mu_is_continuous(bool *is_continuous, uint64_t virtual_addr_a, uint64_t virtual_addr_b);