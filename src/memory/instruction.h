#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

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
    IMM, 
    REG, 
    MM_IMM, 
    MM_REG, 
    MM_IMM_REG, 
    MM_REG1_REG2, 
    MM_IMM_REG1_REG2, 
    MM_REG2_S, 
    MM_IMM_REG2_S, 
    MM_REG1_REG2_S, 
    MM_IMM_REG1_REG2_S
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

#endif // !_INSTRUCTION_H_
