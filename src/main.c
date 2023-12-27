#include <stdio.h>
#include "cpu/mmu.h"
#include "cpu/register.h"
#include "memory/dram.h"
#include "memory/instruction.h"
#include "disk/code.h"

int main(int argc, char const *argv[]) {
    printf("hello world !\n");
    init_handler_table();
    // instruction_cycle();
    uint64_t address = va2pa(0x1000);
    printf("address: 0x%llx\n", address);
    
    return 0;
}
