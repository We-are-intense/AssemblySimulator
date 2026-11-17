#include "dram.h"
#include "cpu/mmu.h"

uint8_t mm[MM_LEN]; // physical memory

// 小端序：低地址存低字节。n 为访问宽度（1/2/4/8 字节）。
// 每个字节单独做地址转换，跨过 MM_LEN 边界时也不会越界。
uint64_t read_dram_virtual(uint64_t vaddr, int n) {
    uint64_t val = 0;
    for (int i = 0; i < n; i++) {
        val |= (uint64_t)mm[va2pa(vaddr + i)] << (8 * i);
    }
    return val;
}

void write_dram_virtual(uint64_t vaddr, uint64_t data, int n) {
    for (int i = 0; i < n; i++) {
        mm[va2pa(vaddr + i)] = (data >> (8 * i)) & 0xff;
    }
}

uint64_t read64bits_dram(uint64_t paddr) {
    uint64_t val = 0;
    for (int i = 0; i < 8; i++) {
        val |= (uint64_t)mm[paddr + i] << (8 * i);
    }
    return val;
}

void write64bits_dram(uint64_t paddr, uint64_t data) {
    for (int i = 0; i < 8; i++) {
        mm[paddr + i] = (data >> (8 * i)) & 0xff;
    }
}

uint64_t read64bits_dram_virtual(uint64_t vaddr) {
    return read_dram_virtual(vaddr, 8);
}

void write64bits_dram_virtual(uint64_t vaddr, uint64_t data) {
    write_dram_virtual(vaddr, data, 8);
}
