#pragma once

#include <stdio.h>
#include <stdint.h>
#define NUM_INSTRTYPE 30

typedef enum OP 
{
    mov_imm_reg,        // 0 mov $0x4050, %eax
    mov_reg_reg,        // 1 mov %rbp, %rsp
    mov_mem_reg,        // 2 mov (%rcx), %rax
    mov_imm_mem,        // 3 mov $0x4050, (%rsp)
    mov_reg_mem,        // 4 mov %rax, -12(%rbp)
    push_reg,           // 5
    pop_reg,            // 6
    call,               // 7
    ret,                // 8
    add_reg_reg,        // 9
} op_t;

typedef enum OD_TYPE
{
    EMPTY,
    IMM, /* $0x123 立即数 */
    REG, /* rax 寄存器寻址 */
    MM_IMM, /*Imm 绝对寻址 M[Imm] */
    MM_REG, /* (rax) 间接寻址 */
    MM_IMM_REG, /* M[Imm + REG] (基址 + 偏移量) 寻址 */
    MM_REG1_REG2, /* M[REG1 + REG2] 变址寻址 */
    MM_IMM_REG1_REG2, /* M[Imm + REG1 + REG2] 变址寻址 */
    MM_REG2_S, /* M[REG2 * s] 比例变址寻址 */
    MM_IMM_REG2_S, /* M[REG2 * s] 比例变址寻址 */
    MM_REG1_REG2_S, /* M[REG1 + REG2 * s] 变址寻址 */
    MM_IMM_REG1_REG2_S /* M[Imm + REG1 + REG2 * s] 变址寻址 */
} od_type_t;

typedef struct OD
{
    od_type_t type;
    int64_t imm;
    int64_t scale;
    uint64_t *reg1;
    uint64_t *reg2;
} od_t;

typedef struct INSTRUCT_STRUCT
{
    op_t op;   // mov, push
    od_t src; 
    od_t dst;
    char code[100];
} inst_t;

// pointer pointing to the function
typedef void (*handler_t)(uint64_t, uint64_t);

extern handler_t handler_table[NUM_INSTRTYPE];


void init_handler_table();
void instruction_cycle();

void mov_imm_reg_handler(uint64_t src, uint64_t dst);
void mov_reg_reg_handler(uint64_t src, uint64_t dst);
void mov_mem_reg_handler(uint64_t src, uint64_t dst);
void mov_imm_mem_handler(uint64_t src, uint64_t dst);
void mov_reg_mem_handler(uint64_t src, uint64_t dst);

void push_reg_handler(uint64_t src, uint64_t dst);
void pop_reg_handler(uint64_t src, uint64_t dst);

void call_handler(uint64_t src, uint64_t dst);
void add_reg_reg_handler(uint64_t src, uint64_t dst);
void ret_handler(uint64_t src, uint64_t dst);
