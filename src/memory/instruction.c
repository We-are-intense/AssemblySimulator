#include "instruction.h"
#include "cpu/register.h"
#include "memory/dram.h"
handler_t handler_table[NUM_INSTRTYPE];

// Private Methods
static uint64_t decode_od(od_t od) {
    if (od.type == IMM) {
        return *((uint64_t *)&od.imm);
    } else if (od.type == REG) {
        return (uint64_t)od.reg1;
    } else {
        // mm
        uint64_t vaddr = 0;
        switch (od.type) {
        case MM_IMM:            vaddr = od.imm; break;
        case MM_REG:            vaddr = *(od.reg1); break;
        case MM_IMM_REG:        vaddr = (od.imm + *(od.reg1)); break;
        case MM_REG1_REG2:      vaddr = (*(od.reg1) + *(od.reg2)); break;
        case MM_IMM_REG1_REG2:  vaddr = (od.imm + *(od.reg1) + *(od.reg2)); break;
        case MM_REG2_S:         vaddr = (*(od.reg1)) * od.scale; break;
        case MM_IMM_REG2_S:     vaddr = (od.imm + (*(od.reg1)) * od.scale); break;
        case MM_REG1_REG2_S:    vaddr = (*(od.reg1) + (*(od.reg2)) * od.scale); break;
        case MM_IMM_REG1_REG2_S:vaddr = (od.imm + *(od.reg1) + (*(od.reg2)) * od.scale); break;
        default:break;
        }
        return vaddr;
    }
}

// Public Methods

void mov_imm_reg_handler(uint64_t src, uint64_t dst) {
    // mov $0x123, %rax
    *(uint64_t *)dst = src;
    reg.rip = reg.rip + sizeof(inst_t);
}

void mov_reg_reg_handler(uint64_t src, uint64_t dst) {
    // mov %rdx, %rax
    *(uint64_t *)dst = *(uint64_t *)src;
    reg.rip = reg.rip + sizeof(inst_t);
}

void mov_mem_reg_handler(uint64_t src, uint64_t dst) {
    // mov (%rcx), %rax
    *(uint64_t *)dst = read64bits_dram_virtual(src);
    reg.rip = reg.rip + sizeof(inst_t);
}

void mov_imm_mem_handler(uint64_t src, uint64_t dst) {
    // mov $0x4050, (%rsp)
    write64bits_dram_virtual(*(uint64_t *)dst, src);
    reg.rip = reg.rip + sizeof(inst_t);
}

void mov_reg_mem_handler(uint64_t src, uint64_t dst) {
    // mov %rax, -12(%rbp)
    write64bits_dram_virtual(*(uint64_t *)dst, 
                             *(uint64_t *)src);
    reg.rip = reg.rip + sizeof(inst_t);
}

void push_reg_handler(uint64_t src, uint64_t dst) {
    reg.rsp = reg.rsp - 0x8;
    write64bits_dram_virtual(reg.rsp, 
                             *(uint64_t *)src);
    reg.rip = reg.rip + sizeof(inst_t);
}

void pop_reg_handler(uint64_t src, uint64_t dst) {
    *(uint64_t *)src = read64bits_dram_virtual(reg.rsp);
    reg.rsp = reg.rsp + 0x8;
    reg.rip = reg.rip + sizeof(inst_t);
}

void call_handler(uint64_t src, uint64_t dst) {
    reg.rsp = reg.rsp - 0x8;
    write64bits_dram_virtual(reg.rsp, 
                             reg.rip + sizeof(inst_t));
    reg.rip = src;
}

void add_reg_reg_handler(uint64_t src, uint64_t dst) {
    *(uint64_t *)dst = *(uint64_t *)src + *(uint64_t *)dst;
    reg.rip = reg.rip + sizeof(inst_t);
}

void ret_handler(uint64_t src, uint64_t dst) {
    reg.rip = read64bits_dram_virtual(reg.rsp);
    reg.rsp = reg.rsp + 0x8;
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