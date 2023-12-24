#include "instruction.h"
#include "cpu/register.h"

handler_t handler_table[NUM_INSTRTYPE];

// Private Methods
static uint64_t decode_od(od_t od) {
    if (od.type == IMM) {
        return *((uint64_t *)&od.imm);
    } else if (od.type == REG) {
        return (uint64_t)od.reg1;
    } else {
        // mm
        /*
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
        */
        uint64_t vaddr = 0;
        switch (od.type) {
        case MM_IMM:
            vaddr = od.imm;
            break;
        case MM_REG:// store reg
            vaddr = *(od.reg1);
            break;
        default:
            break;
        }
        return vaddr;
    }
}

// Public Methods

void mov_imm_reg_handler(uint64_t src, uint64_t dst) {

}

void mov_reg_reg_handler(uint64_t src, uint64_t dst) {
    // src: reg
    // dst: reg
    *(uint64_t *)dst = *(uint64_t *)src;
    reg.rip = reg.rip + sizeof(inst_t);
}

void mov_mem_reg_handler(uint64_t src, uint64_t dst) {

}

void mov_imm_mem_handler(uint64_t src, uint64_t dst) {

}

void mov_reg_mem_handler(uint64_t src, uint64_t dst) {

}

void push_reg_handler(uint64_t src, uint64_t dst) {

}

void pop_reg_handler(uint64_t src, uint64_t dst) {

}

void call_handler(uint64_t src, uint64_t dst) {

}

void add_reg_reg_handler(uint64_t src, uint64_t dst) {

}

void ret_handler(uint64_t src, uint64_t dst) {

}

void init_handler_table() {
    handler_table[mov_imm_reg] = &mov_imm_reg_handler;
    handler_table[mov_reg_reg] = &mov_reg_reg_handler;
    handler_table[mov_mem_reg] = &mov_mem_reg_handler;
    handler_table[mov_imm_mem] = &mov_imm_mem_handler;
    handler_table[mov_reg_mem] = &mov_reg_mem_handler;
    handler_table[push_reg]    = &push_reg_handler;
    handler_table[call]        = &call_handler;
    handler_table[ret]         = &ret_handler;
    handler_table[add_reg_reg] = &add_reg_reg_handler;
}

void instruction_cycle() {
    inst_t *instr = (inst_t *)reg.rip;

    uint64_t src = decode_od(instr->src);
    uint64_t dst = decode_od(instr->dst);

    handler_t handler = handler_table[instr->op];
    handler(src, dst);
}