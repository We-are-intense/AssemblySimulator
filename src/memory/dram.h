#ifndef _DRAM_H_
#define _DRAM_H_

#include <stdint.h>

#define MM_LEN 1000

extern uint8_t mm[MM_LEN]; // physical memory

uint64_t read64bits_dram(uint64_t paddr);
uint64_t read64bits_dram_virtual(uint64_t vaddr);
void write64bits_dram(uint64_t paddr, uint64_t data);
void write64bits_dram_virtual(uint64_t vaddr, uint64_t data);

#endif // !_DRAM_H_

