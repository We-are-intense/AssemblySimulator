#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include <stdio.h>
#include <stdint.h>
#define NUM_INSTRTYPE 30

typedef enum OP 
{
    mov_reg_reg,        // 0 
    mov_reg_mem,        // 1
    mov_mem_reg,        // 2
    push_reg,           // 3
    pop_reg,            // 4
    call,               // 5
    ret,                // 6
    add_reg_reg,        // 7
} op_t;



void init_handler_table();
void instruction_cycle();

#endif // !_INSTRUCTION_H_
