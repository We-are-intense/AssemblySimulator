#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cpu/mmu.h"
#include "cpu/register.h"
#include "memory/dram.h"
#include "memory/instruction.h"
#include "disk/code.h"

int main(int argc, char const *argv[]) {
    // init state
    init_handler_table();
    core_t *cr = &cores[ACTIVE_CORE];
    cr->reg.rax = 0x3;
    cr->reg.rbx = 0x8000670;
    cr->reg.rcx = 0x8000670;
    cr->reg.rdx = 0x5;
    cr->reg.rsi = 0x7ffffffee208;
    cr->reg.rdi = 0x1;
    cr->reg.rbp = 0x7ffffffee110;
    cr->reg.rsp = 0x7ffffffee0f0;

    write64bits_dram_virtual(0x7ffffffee110, 0x0000000000000000);    // rbp
    write64bits_dram_virtual(0x7ffffffee108, 0x0000000000000000);
    write64bits_dram_virtual(0x7ffffffee100, 0x0000000000000001);
    write64bits_dram_virtual(0x7ffffffee0f8, 0x0000000000000002);
    write64bits_dram_virtual(0x7ffffffee0f0, 0x0000000000000000);    // rsp
#define Inst_Num 15
    char assembly[Inst_Num][MAX_INSTRUCTION_CHAR] = {
        "push   %rbp",              // 0
        "mov    %rsp,%rbp",         // 1
        "mov    %rdi,-0x18(%rbp)",  // 2
        "mov    %rsi,-0x20(%rbp)",  // 3
        "mov    -0x18(%rbp),%rdx",  // 4
        "mov    -0x20(%rbp),%rax",  // 5
        "add    %rdx,%rax",         // 6
        "mov    %rax,-0x8(%rbp)",   // 7
        "mov    -0x8(%rbp),%rax",   // 8
        "pop    %rbp",              // 9
        "retq",                     // 10
        "mov    %rdx,%rsi",         // 11
        "mov    %rax,%rdi",         // 12
        "callq  0",                 // 13
        "mov    %rax,-0x8(%rbp)",   // 14
    };
    cr->rip = (uint64_t)&assembly[11];
    sprintf(assembly[13], "callq  $%p", &assembly[0]);
    for (int i = 0; i < Inst_Num; i++) {
        instruction_cycle(cr);
    }
    assert(cr->reg.rax == 8);
    
    return 0;
}
