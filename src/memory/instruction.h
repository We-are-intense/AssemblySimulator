#pragma once

#include <stdio.h>
#include <stdint.h>
#include "cpu/register.h"

#define NUM_INSTRTYPE 30

typedef enum OP 
{
    INST_MOV,
    INST_PUSH,
    INST_POP,
    INST_LEAVE,
    INST_CALL,
    INST_RET,
    // 算术逻辑类：都交给同一个 ALU，只是运算种类和是否写回不同
    INST_ADD,
    INST_SUB,
    INST_CMP,           // = sub，但不写回
    INST_AND,
    INST_OR,
    INST_XOR,
    // 跳转类：jmp / je / jne / jl ... 都是"条件成立就跳"，条件放在 inst_t.cond
    INST_JCC,
} op_t;

// 跳转条件：每一种都是标志位的一个布尔表达式，见 instruction.c 的 cond_holds
typedef enum COND
{
    CC_ALWAYS,  // jmp
    CC_E,       // je  / jz   相等
    CC_NE,      // jne / jnz  不等
    CC_L,       // jl   有符号 <
    CC_GE,      // jge  有符号 >=
    CC_LE,      // jle  有符号 <=
    CC_G,       // jg   有符号 >
    CC_B,       // jb   无符号 <
    CC_AE,      // jae  无符号 >=
    CC_BE,      // jbe  无符号 <=
    CC_A,       // ja   无符号 >
    CC_S,       // js   结果为负
    CC_NS,      // jns  结果非负
} cond_t;



// 操作数只有三种：立即数、寄存器、内存。
// 内存操作数的 9 种 AT&T 写法都是同一个公式的特例：
//     有效地址 = disp + base + index * scale
// 写法里缺哪一项，就让那一项取"零元"：disp 取 0，base/index 取 REG_NONE，scale 取 1。
//     0x12              → disp=0x12
//     (%rsi)            → base=rsi
//     0x12(%rsi)        → disp=0x12, base=rsi
//     (%rsi,%rdi)       → base=rsi,  index=rdi
//     0x12(,%rdi,4)     → disp=0x12, index=rdi, scale=4
//     0x12(%rsi,%rdi,4) → 四项都有
typedef enum OD_TYPE
{
    OD_EMPTY,
    OD_IMM, /* $0x123 */
    OD_REG, /* %rax   */
    OD_MEM, /* disp(base, index, scale) */
} od_type_t;

#define REG_NONE (-1)

typedef struct OD
{
    od_type_t type;
    int64_t   imm;   // OD_IMM: 立即数；OD_MEM: 位移 disp
    // 以下寄存器字段都是寄存器描述表的下标（见 instruction.c），不是 gpr 编号：
    // 同一个 gpr 有 %rax/%eax/%ax/%al/%ah 多个名字，描述表的每一项记录了它的位段
    int       reg;   // OD_REG: 寄存器
    int       base;  // OD_MEM: 基址寄存器，REG_NONE 表示没有，必须是 64 位寄存器
    int       index; // OD_MEM: 变址寄存器，REG_NONE 表示没有，必须是 64 位寄存器
    uint64_t  scale; // OD_MEM: 比例因子 1/2/4/8，没写时为 1
} od_t;

typedef struct INSTRUCT_STRUCT
{
    op_t op;    // mov, push
    int  width; // 操作宽度（字节）1/2/4/8：来自后缀 b/w/l/q，或由寄存器操作数推断
    cond_t cond; // 仅 INST_JCC 使用
    od_t src;
    od_t dst;
    char code[100];
} inst_t;

// commonly shared variables
#define MAX_INSTRUCTION_CHAR 64

void test_parse_inst(uint64_t value, core_t *cr);
void init_handler_table();
void instruction_cycle(core_t *cr);
