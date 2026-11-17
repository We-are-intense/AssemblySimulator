#pragma once

#include <stdlib.h>
#include <stdint.h>

// 16 个通用寄存器，顺序与 x86 指令编码一致。
// 子寄存器（%eax、%ax、%al、%ah、%r8d ...）不单独存储，它们只是某个 64 位寄存器的
// 一段位，由 instruction.c 的寄存器描述表 {id, width, shift} 描述并负责读写。
typedef enum GPR_ID
{
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    R8,  R9,  R10, R11, R12, R13, R14, R15,
    NUM_GPR
} gpr_id_t;

// 同一块存储两种访问方式：cr->reg.rax 按名字，cr->reg.gpr[RAX] 按编号
typedef union REGISTER_STRUCT
{
    uint64_t gpr[NUM_GPR];
    struct
    {
        uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
        uint64_t r8,  r9,  r10, r11, r12, r13, r14, r15;
    };
} reg_t;

// the 4 flags be a uint64_t in total
typedef struct CPU_FLAGS_STRUCT
{
    union
    {
        uint64_t __cpu_flag_value;
        struct
        {    
            // carry flag: 进位标识，最近的操作产生了进位
            uint16_t CF;
            // zero flag: 零标识，最近的操作结果为零
            uint16_t ZF;
            // sign flag: 负号标识，最近的操作结果为负数
            uint16_t SF;
            // overflow flag: 溢出标识，最近的操作导致一个补码溢出: 正溢出或者负溢出
            uint16_t OF;
        };
    };
} cpu_flag_t;

typedef struct CORE_STRUCT
{
    // program counter or instruction pointer
    union 
    {
        uint64_t rip;
        uint32_t eip;
    };
    
    // cpu flags
    cpu_flag_t flags;

    // register files
    reg_t       reg;
} core_t;

// define cpu core array to support core level parallelism
#define NUM_CORES 1
extern core_t cores[NUM_CORES];
// active core for current task
extern uint64_t ACTIVE_CORE;
