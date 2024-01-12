#include "instruction.h"
#include "cpu/register.h"
#include "memory/dram.h"
#include <assert.h>
handler_t handler_table[NUM_INSTRTYPE];

// Private Methods
typedef enum Parse_State {
    parse_state_init,
    parse_state_inst
} parse_state;

typedef struct Parse_inst_state
{
    // 0: 开始解析指令 
    // 1: 开始解析源操作 
    // 2: 开始解析目的操作
    // 3: 解析完成
    int inst_state;
    // 开始索引
    int start;
    // 偏移
    int offset;
    // 当前在解析的状态
    parse_state state;    
} parse_inst_t;
static inline int is_num(char c) {
    return (c >= '0' && c <= '9');
}

static inline int is_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline void parse_next(const char *str, parse_inst_t *pit) {
    if (str[pit->offset] == '\0') {
        return;
    }
    pit->start  = pit->offset + 1;
    pit->offset = pit->offset + 1;
}

static inline void parse_remove_white_space(const char *str, parse_inst_t *pit) {
    int offset = pit->offset;
    while (str[offset] == ' ' && str[offset] != '\0')
    {
        offset++;
    }
    pit->start = offset;
    pit->offset = offset;
}
/*
*   解析数字
*   return type:
*  -1: 解析出错
*   0: 立即数 $123, $0x12, $-123, $-0x12
*   1: 绝对寻址 123, 0x12, -123
*   2: 变址寻址 mov 0x12(%rax), mov 0x12(%rsi, %rdi), %rax
* */
static int parse_number(const char *str, parse_inst_t *pit, int64_t *value) {
    parse_remove_white_space(str, pit);
    int start = pit->start, offset = pit->offset;
    int type = -1;
    char c = str[offset];
    if (c == '\0') return -1;
    if (c == '$') {
        // 跳过 '$'
        parse_next(str, pit);
        int is_hex = 0;
        int is_negative = 1; // 是否是负数
        // mov $0x123, %rax
        // 立即数只会在源操作上
        char cc[32] = {'\0'};
        char c1 = str[pit->offset];
        assert(!(c1 == '\0'));
        if (c1 == '-') {
            is_negative = -1;
            // 跳过负号
            parse_next(str, pit);
            c1 = str[pit->offset];
            assert(!(c1 == '\0'));
        }
        char c2 = str[pit->offset + 1];
        assert(!(c2 == '\0'));
        if (c1 == '0' && c2 == 'x') {
            is_hex = 1;
            // 跳过 '0'
            parse_next(str, pit);
            // 跳过 'x'
            parse_next(str, pit);
        } else {
            assert(is_num(c1));
            cc[0] = c1;
            if (is_num(c2)) {
                cc[1] = c2;
            }
        }
        start  = pit->start - (is_hex ? 2 : 0);
        offset = pit->offset;
        c1 = str[offset];
        while (is_num(c1) || is_char(c1)) {
            cc[offset - start] = c1;
            offset ++;
        }
        parse_remove_white_space(str, pit);
        c1 = str[pit->offset];
        assert(c1 == ',');
    }
    return type;
}

static void parse_string(const char *str, parse_inst_t *pit, char *result) {
    parse_remove_white_space(str, pit);
    int start = pit->start, offset = pit->offset;
    char c = str[offset];
    while (c != '\0') {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'B')) {
            result[offset - start] = c;
            offset++;
            c = str[offset];
        }  else  {
            break;
        }
    }
    pit->start  = offset;
    pit->offset = offset;
} 
/*
*   解析变址寻址
*   return type:
*  -1: 解析出错
*   0: mov (%rax),     %rdx         --> first = rax
*      mov 0x12(%rax), %rdx         --> first = rax
*   2: mov (%rsi, %rdi), %rax       --> first = rsi, second = rdi 
*      mov 0x12(%rsi, %rdi), %rax   --> first = rsi, second = rdi 
*   3: mov (, %rdi, 2), %rax        --> first =    , second = rdi, scale = 2
*      mov 0x12(, %rdi, 2), %rax    --> first =    , second = rdi, scale = 2
*   4: mov (%rsi, %rdi, 2), %rax    --> first = rsi, second = rdi, scale = 2
*      mov 0x12(%rsi, %rdi, 2), %rax
* */
static int parse_index_addressing(const char *str, 
                                   parse_inst_t *pit, 
                                   char *first, 
                                   char *second, 
                                   int  *scale) {

    return -1;
}

static void parse_inst_str(inst_t *inst, const char *str, parse_inst_t *pit) {
    int state = pit->inst_state;
    if (state == 0) {
        char cc[8] = {'\0'};
        parse_string(str, pit, cc);
        parse_remove_white_space(str, pit);
        printf("parse: 指令: %s\n", cc);
        pit->inst_state = 1;
    } else if (state == 1 || state == 2) {
        char c = str[pit->offset];
        assert(!(c == '\0'));
        if (c == '%') {
            // mov %rax, %rdi
            char cc[8] = {'\0'};
            parse_next(str, pit);
            parse_string(str, pit, cc);
            parse_remove_white_space(str, pit);
            if (state == 1) {
                printf("src token: %%%s\n", cc);
                c = str[pit->offset];
                assert((c == ','));   
                parse_next(str, pit);
                parse_remove_white_space(str, pit);
                pit->inst_state = 2;
            } else if (state == 2) {
                printf("dst token: %%%s\n", cc);
                pit->inst_state = 3;
            }
        } else if (c == '-' || c == '$' || 
                  (c >= 'a' && c <= 'z') || 
                  (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9')) {
            // 解析数字
            int64_t value = 0;
            //-1: 解析出错
            // 0: 立即数 $123, $0x12, $-123, $-0x12
            // 1: 绝对寻址 123, 0x12, -123
            // 2: 变址寻址 mov 0x12(%rax), mov 0x12(%rsi, %rdi), %rax
            int num_type = parse_number(str, pit, &value);
            if (num_type == 0 || num_type == 1) {
                pit->inst_state = state + 1;
                return;
            }
            // 3: 变址寻址 mov 0x12(%rax), mov 0x12(%rsi, %rdi), %rax
            char first[8]  = {'\0'};
            char second[8] = {'\0'};
            int scale = 1;
            // 解析变址寻址
            // return type:
            //-1: 解析出错
            // 0: mov (%rax),     %rdx         --> first = rax
            //    mov 0x12(%rax), %rdx         --> first = rax
            // 2: mov (%rsi, %rdi), %rax       --> first = rsi, second = rdi 
            //    mov 0x12(%rsi, %rdi), %rax   --> first = rsi, second = rdi 
            // 3: mov (, %rdi, 2), %rax        --> first =    , second = rdi, scale = 2
            //    mov 0x12(, %rdi, 2), %rax    --> first =    , second = rdi, scale = 2
            // 4: mov (%rsi, %rdi, 2), %rax    --> first = rsi, second = rdi, scale = 2
            //    mov 0x12(%rsi, %rdi, 2), %rax
            num_type = parse_index_addressing(str, pit, 
                                              first, second, &scale);
            printf("index address: first:%s secode=%s scale=%d\n", 
                   first, second, scale);
            pit->inst_state = state + 1;
        }
    }
}

static void parse_instruction(inst_t *inst, const char *str) {
    parse_inst_t pit = {0};
    printf("parse: %s\n", str);
    // 解析指令
    parse_inst_str(inst, str, &pit);
    // 解析源操作
    parse_inst_str(inst, str, &pit);
    // 解析目的操作
    parse_inst_str(inst, str, &pit);
}

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
    const char *inst_str = (const char *)reg.rip;
    inst_t instr;
    parse_instruction(&instr, inst_str);

    return;
    uint64_t src = decode_od(instr.src);
    uint64_t dst = decode_od(instr.dst);

    handler_t handler = handler_table[instr.op];
    handler(src, dst);
}

void test_parse_inst(uint64_t value) {
    const char *inst_str = (const char *)value;
    inst_t instr;
    parse_instruction(&instr, inst_str);
}