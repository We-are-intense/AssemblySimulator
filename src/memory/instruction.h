#pragma once

#include <stdio.h>
#include <stdint.h>
#include "cpu/register.h"

#define NUM_INSTRTYPE 30

typedef enum OP 
{
    INST_MOV,           // 0
    INST_PUSH,          // 1
    INST_POP,           // 2
    INST_LEAVE,         // 3
    INST_CALL,          // 4
    INST_RET,           // 5
    INST_ADD,           // 6
    INST_SUB,           // 7
    INST_CMP,           // 8
    INST_JNE,           // 9
    INST_JMP,           // 10
} op_t;



typedef enum OD_TYPE
{
    EMPTY,
    IMM, /* $0x123 立即数 */
    REG, /* rax 寄存器寻址 */
    MM_IMM, /* mov 0x123, %rax 绝对寻址 0x123 对应地址内容放到 %rax */
    MM_REG, /* mov (%rsi), %rax 间接寻址 */
    MM_IMM_REG, /* mov 0x12(%rsi), %rax M[Imm + REG] (基址 + 偏移量) 寻址 */
    MM_REG1_REG2, /* mov (%rsi, %rdi), %rax M[REG1 + REG2] 变址寻址 */
    MM_IMM_REG1_REG2, /* mov 0x12(%rsi, %rdi), %rax M[Imm + REG1 + REG2] 变址寻址 */
    MM_REG2_S, /* mov (, %rsi, s), %rax M[REG2 * s] 比例变址寻址 */
    MM_IMM_REG2_S, /* mov 0x12(, %rsi, s), %rax M[Imm + REG2 * s] 比例变址寻址 */
    MM_REG1_REG2_S, /* mov (%rsi, %rdi, s), %rax M[REG1 + REG2 * s] 比例变址寻址 */
    MM_IMM_REG1_REG2_S /* mov 0x12(%rsi, %rdi, s), %rax M[Imm + REG1 + REG2 * s] 比例变址寻址 */
} od_type_t;

typedef struct OD
{
    od_type_t type; // IMM, REG, MEM
    uint64_t  imm;  // 立即数
    uint64_t  s;    // 比例变址寻址,比例因子必须 1，2，4，8
    uint64_t  reg1; // main src register
    uint64_t  reg2; // dst register
} od_t;

typedef struct INSTRUCT_STRUCT
{
    op_t op;   // mov, push
    od_t src; 
    od_t dst;
    char code[100];
} inst_t;

// commonly shared variables
#define MAX_INSTRUCTION_CHAR 64

void test_parse_inst(uint64_t value, core_t *cr);
void init_handler_table();
void instruction_cycle(core_t *cr);
