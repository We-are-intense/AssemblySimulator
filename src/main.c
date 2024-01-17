#include <stdio.h>
#include "cpu/mmu.h"
#include "cpu/register.h"
#include "memory/dram.h"
#include "memory/instruction.h"
#include "disk/code.h"

int main(int argc, char const *argv[]) {
    // init state
    init_handler_table();

    reg.rax = 0xabcd;
    reg.rbx = 0x8000670;
    reg.rcx = 0x8000670;
    reg.rdx = 0x12340000;
    reg.rsi = 0x7ffffffee208;
    reg.rdi = 0x1;
    reg.rbp = 0x7ffffffee110;
    reg.rsp = 0x7ffffffee0f0;

    write64bits_dram_virtual(0x7ffffffee110, 0x0000000000000000);    // rbp
    write64bits_dram_virtual(0x7ffffffee108, 0x0000000000000000);
    write64bits_dram_virtual(0x7ffffffee100, 0x0000000012340000);
    write64bits_dram_virtual(0x7ffffffee0f8, 0x000000000000abcd);
    write64bits_dram_virtual(0x7ffffffee0f0, 0x0000000000000000);    // rsp
#define Inst_Num 18
    char assembly[Inst_Num][64] = {
        "mov    %rax,(%rsi, %rdi, 2)",  // 2
        "mov    %rax,-0x18(, %rdi, 2)",  // 2
        "mov    %rax,-0x18(%rsi, %rdi, 2)",  // 2
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
    test_parse_inst((uint64_t)&assembly[3]);
    reg.rip = (uint64_t)&assembly[11];
    sprintf(assembly[13], "callq  $%p", &assembly[0]);
    for (int i = 0; i < Inst_Num; i++)
    {
        test_parse_inst((uint64_t)&assembly[i]);
    }
    
    return 0;
}
