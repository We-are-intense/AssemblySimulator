#include "instruction.h"
#include "cpu/register.h"
#include "memory/dram.h"
#include <assert.h>
#include <string.h>
handler_t handler_table[NUM_INSTRTYPE];

// Private Methods
typedef struct Parse_State {
    char condition;
    int current;
    int next;

} parse_state_t;
#define parse_state_num 19
static parse_state_t parse_state_list[parse_state_num] = {
    {'0',  0, 1},// initial($0x12) --> 1
    {'3',  0, 1},// inital(%rax) --> 1
    {'1',  0, 2},// inital(0x12) --> 2
    {'4',  0, 2},// inital( ( )  --> 2
    {'\0', 1,16},// 1(\0) --> 16 解析完成
    {'2',  1, 0},// 1(,) --> 0, 回到初始状态，解析出了 src
    {'2',  2, 0},// 2( , )  --> 0 回到初始状态，解析出了 src
    {'4',  2, 3},// 2( ( )  --> 3 
    {'3',  3, 4},// 3( %rax )  --> 4 
    {'2',  3, 7},// 3( , ) --> 7
    {'5',  4, 1},// 4( ) )  --> 1 
    {'2',  4, 5},// 4( , )  --> 5 
    {'3',  5, 6},// 5( %rax ) --> 6 
    {'5',  6, 1},// 6( ) ) --> 1 
    {'2',  6, 9},// 6( , ) --> 9 
    {'3',  7, 8},// 7( %rax ) --> 8 
    {'2',  8, 9},// 8( , ) --> 9 
    {'1',  9,10},// 9( 0x12 ) --> 10
    {'5', 10, 1},// 10( ) ) --> 1
};

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
    int token_num;
    char tokens[30][64];
    char token_type[25];   
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
static int parse_number(const char *str, parse_inst_t *pit, char *value) {
    parse_remove_white_space(str, pit);
    int start = pit->start, offset = pit->offset;
    int type = -1;
    char c = str[offset];
    if (c == '\0') return -1;
    if (c == '$') {
        type = 1;
    }

    if (c == '$') {
        // 跳过 '$'
        parse_next(str, pit);
        int is_hex = 0;
        int is_negative = 1; // 是否是负数
        char cc[64] = {'\0'};
        // mov $0x123, %rax
        // 立即数只会在源操作上
        // '0' 'x'
        // c1   c2
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
            assert(is_num(c1) || is_char(c1));
            cc[0] = c1;
            if (is_num(c2) || is_char(c2)) {
                cc[1] = c2;
            }
        }
        start  = pit->start - (is_hex ? 0 : 2);
        offset = pit->offset;
        c1 = str[offset];
        while (is_num(c1) || is_char(c1)) {
            cc[offset - start] = c1;
            offset ++;
            c1 = str[offset];
        }
        pit->offset = offset;
        parse_remove_white_space(str, pit);
        c1 = str[pit->offset];
        assert(c1 == ',');
        offset = 0;
        if (is_negative < 0) {
            value[offset++] = '-';
        }
        if (is_hex) {
            value[offset++] = '0';
            value[offset++] = 'x';
        }
        start = 0;
        while (cc[start] != '\0') {
            value[offset++] = cc[start++];
        }
        type = 1;
    }
    return type;
}

static void parse_string(const char *str, parse_inst_t *pit, char *result) {
    parse_remove_white_space(str, pit);
    int start = pit->start, offset = pit->offset;
    char c = str[offset];
    while (c != '\0') {
        if (is_char(c)) {
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
// lookup table
static const char *reg_name_list[72] = {
    "%rax","%eax","%ax","%ah","%al",
    "%rbx","%ebx","%bx","%bh","%bl",
    "%rcx","%ecx","%cx","%ch","%cl",
    "%rdx","%edx","%dx","%dh","%dl",
    "%rsi","%esi","%si","%sih","%sil",
    "%rdi","%edi","%di","%dih","%dil",
    "%rbp","%ebp","%bp","%bph","%bpl",
    "%rsp","%esp","%sp","%sph","%spl",
    "%r8","%r8d","%r8w","%r8b",
    "%r9","%r9d","%r9w","%r9b",
    "%r10","%r10d","%r10w","%r10b",
    "%r11","%r11d","%r11w","%r11b",
    "%r12","%r12d","%r12w","%r12b",
    "%r13","%r13d","%r13w","%r13b",
    "%r14","%r14d","%r14w","%r14b",
    "%r15","%r15d","%r15w","%r15b",
};

static char parse_token_type(char *cc) {
    /*
        0:  imm $0x12
        1:  imm  0x12
        2:  ,
        3:  %rax
        4:  (
        5:  )
    */
    if (cc[0] == '\0') return '\0';
    switch (cc[0]) {
        case '$': return '0';
        case ',': return '2';
        case '%': return '3';
        case '(': return '4';
        case ')': return '5';
        default : return '1';
    }
}

static void parse_operate_inst(inst_t *inst, int state, char *cc_stack[]) {
    printf("%s= ", state == 1 ? "src" : "dst");
    for (int i = 0; i < 4; i++) {
        if (!cc_stack[i]) continue;
        printf("%d: %s ",i , cc_stack[i]);
    }
    od_type_t type = state == 1 ? inst->src.type : inst->dst.type;
#define OD_TYPE_CASE(x)  \
    case x:printf(" %s", #x);break
    switch (type)
    {
        OD_TYPE_CASE(EMPTY);
        OD_TYPE_CASE(IMM);
        OD_TYPE_CASE(REG);
        OD_TYPE_CASE(MM_IMM);
        OD_TYPE_CASE(MM_REG);
        OD_TYPE_CASE(MM_IMM_REG);
        OD_TYPE_CASE(MM_REG1_REG2);
        OD_TYPE_CASE(MM_IMM_REG1_REG2);
        OD_TYPE_CASE(MM_REG2_S);
        OD_TYPE_CASE(MM_IMM_REG2_S);
        OD_TYPE_CASE(MM_REG1_REG2_S);
        OD_TYPE_CASE(MM_IMM_REG1_REG2_S);
        default:break;
    }
    printf("\n");
}

static void parse_operate_token(inst_t *inst, parse_inst_t *pit) {
    if (pit->token_type[0] == '\0') return;
    int current = 0;
    pit->inst_state = 1;
    // 存储解析出来的指令，按解析顺序存放
    // 如: 0x12(%rsi, %rdi, s)
    char *cc_stack[4] = {NULL};
    int  cc_stack_index = 0;
    od_type_t od_type = EMPTY;
    for (int i = 0; i <= pit->token_num; i++) {
        char type = pit->token_type[i];
        switch (current)
        {
            case 0:
            {
                if (type == '0') {
                    // $0x12
                    current = 1;
                    assert(cc_stack_index <= 3);
                    cc_stack[cc_stack_index++] = pit->tokens[i+1];
                    od_type = IMM;
                } else if (type == '3') {
                    // %rax
                    current = 1;
                    assert(cc_stack_index <= 3);
                    cc_stack[cc_stack_index++] = pit->tokens[i+1];
                    od_type = REG;
                } else if (type == '1') {
                    assert(cc_stack_index <= 3);
                    cc_stack[cc_stack_index++] = pit->tokens[i+1];
                    current = 2;
                    od_type = MM_IMM;
                } else if (type == '4') {
                    // (
                    current = 3;
                    od_type == MM_REG;
                } else {
                    assert(0);
                }
                break;
            }
            case 1:
            {
                if (type == '2') {
                    current = 0;
                    inst->src.type = od_type;
                    parse_operate_inst(inst, pit->inst_state, cc_stack);
                    od_type = EMPTY;
                    cc_stack_index = 0;
                    pit->inst_state = 2;
                } else if (type == '\0') {
                    current = 16;
                    if (pit->inst_state == 1) {
                        inst->src.type = od_type;
                    } else {
                        inst->dst.type = od_type;
                    }
                    parse_operate_inst(inst, pit->inst_state, cc_stack);
                } else {
                    assert(0);
                }
                break;
            }
            case 2:
            {
                if (type == '2') {
                    // ,
                    current = 0;
                    assert(pit->inst_state == 1);
                    inst->src.type = MM_IMM;
                    parse_operate_inst(inst, pit->inst_state, cc_stack);
                    od_type = EMPTY;
                    pit->inst_state = 2;
                } else if (type == '4') {
                    // (
                    current = 3;
                    od_type = MM_IMM_REG;
                } else if (type == '\0') {
                    if (pit->inst_state == 1) {
                        inst->src.type = od_type;
                    } else {
                        inst->dst.type = od_type;
                    }
                    inst->dst.type = MM_IMM;
                    parse_operate_inst(inst, pit->inst_state, cc_stack);
                    current = 16;
                } else {
                    assert(0);
                }
                break;
            }
            case 3:
            {
                if (type == '3') {
                    // %rax
                    current = 4;
                    assert(cc_stack_index <= 3);
                    cc_stack[cc_stack_index++] = pit->tokens[i+1];
                } else if (type == '2') {
                    // ,
                    current = 7;
                    if (od_type == MM_IMM_REG) {
                        od_type = MM_IMM_REG2_S;
                    } else if (MM_REG) {
                        od_type = MM_REG2_S;
                    } else {
                        assert(0);
                    }
                } else {
                    assert(0);
                }
                break;
            }
            case 4:
            {
                if (type == '5') {
                    // )
                    current = 1;
                } else if (type == '2') {
                    // ,
                    current = 5;
                    if (od_type == MM_IMM_REG) {
                        od_type = MM_IMM_REG1_REG2;
                    } else if (MM_REG) {
                        od_type = MM_REG1_REG2;
                    } else {
                        assert(0);
                    }
                } else {
                    assert(0);
                }
                break;
            }            
            case 5:
            {
                if (type == '3') {
                    // %rax
                    assert(cc_stack_index <= 3);
                    cc_stack[cc_stack_index++] = pit->tokens[i+1];
                    current = 6;
                } else {
                    assert(0);
                }
                break;
            }
            case 6:
            {
                if (type == '5') {
                    // )
                    current = 1;
                } else if (type == '2') {
                    // ,
                    current = 9;
                    if (od_type == MM_IMM_REG1_REG2) {
                        od_type = MM_IMM_REG1_REG2_S;
                    } else if (MM_REG1_REG2) {
                        od_type = MM_REG1_REG2_S;
                    } else {
                        assert(0);
                    }
                } else {
                    assert(0);
                }
                break;
            }
            case 7:
            {
                if (type == '3') {
                    // %rax
                    assert(cc_stack_index <= 3);
                    cc_stack[cc_stack_index++] = pit->tokens[i+1];
                    current = 8;
                } else {
                    assert(0);
                }
                break;
            }
            case 8:
            {
                if (type == '2') {
                    // ,
                    current = 9;
                } else {
                    assert(0);
                }
                break;
            }
            case 9:
            {
                if (type == '1') {
                    // %rax
                    assert(cc_stack_index <= 3);
                    cc_stack[cc_stack_index++] = pit->tokens[i+1];
                    current = 10;
                } else {
                    assert(0);
                }
                break;
            } 
            case 10:
            {
                if (type == '5') {
                    // )
                    current = 1;
                } else {
                    assert(0);
                }
                break;
            }        
        default:
            break;
        }
    }
}

static void parse_inst_token(inst_t *inst, parse_inst_t *pit) {
    char *op = pit->tokens[0];
    if (strcmp(op, "mov") == 0 || strcmp(op, "movq") == 0) {
            inst->op = INST_MOV;
    } else if (strcmp(op, "push") == 0) {
            inst->op = INST_PUSH;
    } else if (strcmp(op, "pop") == 0) {
            inst->op = INST_POP;
    } else if (strcmp(op, "call") == 0 || strcmp(op, "callq") == 0) {
            inst->op = INST_CALL;
    } else if (strcmp(op, "ret") == 0 || strcmp(op, "retq") == 0) {
            inst->op = INST_RET;
    } else if (strcmp(op, "add") == 0) {
            inst->op = INST_ADD;
    } else if (strcmp(op, "sub") == 0) {
            inst->op = INST_SUB;
    } else if (strcmp(op, "cmpq") == 0) {
        inst->op = INST_CMP;
    } else if (strcmp(op, "jne") == 0) {
        inst->op = INST_JNE;
    } else if (strcmp(op, "jmp") == 0) {
        inst->op = INST_JMP;
    } else {
        return;
    }

    parse_operate_token(inst, pit);

}

static void parse_to_token(const char *str, parse_inst_t *pit) {
    if (str[pit->offset] == '\0') {
        return;
    }
    int index = 0, offset = 0;
    char c = str[offset];
    int token_num = pit->token_num;
    while (c != '\0') {
        switch (c)
        {
            case ' ':
            {
                if (pit->tokens[token_num][0] != '\0') {
                    token_num++;
                    index = 0;
                }
                break;
            }
            case '(':
            case ')':
            case ',':
            {
                while (pit->tokens[token_num][0] != '\0') {
                    token_num++;index = 0;
                }
                pit->tokens[token_num][index] = c;
                token_num++;index = 0;
                break;
            }
            default:
            {
                pit->tokens[token_num][index++] = c;
                break;
            }
        }
        offset++;
        c = str[offset];
    }
    if (pit->tokens[token_num][0] == '\0') {
        token_num -= 1;
    }
    for (int i = 1; i <= token_num; i++) {
        pit->token_type[i-1] = parse_token_type(pit->tokens[i]);
    }
    pit->token_num = token_num;
}

static void parse_instruction(inst_t *inst, const char *str) {
    parse_inst_t pit = {
        .inst_state = 0,
        .offset = 0,
        .start = 0,
        .token_num = 0,
        .tokens = {{'\0'}},
        .token_type = {'\0'}
    };
    printf("parse: %s\n", str);
    parse_to_token(str, &pit);
    printf("tokens: ");
    for (int i = 0; i <= pit.token_num; i++)
    {
        printf("<%s>  ",pit.tokens[i]);
    }
    printf("\n");
    // 解析指令
    parse_inst_token(inst, &pit);
}

static uint64_t decode_od(od_t od) {
    if (od.type == IMM) {
        return *((uint64_t *)&od.imm);
    } else if (od.type == REG) {
        return (uint64_t)od.reg1;
    } else {
        // mm
        uint64_t vaddr = 0;
        return vaddr;
    }
}

// Public Methods

void mov_handler(uint64_t src, uint64_t dst) {
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

void push_handler(uint64_t src, uint64_t dst) {
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

void ret_handler(uint64_t src, uint64_t dst) {
    reg.rip = read64bits_dram_virtual(reg.rsp);
    reg.rsp = reg.rsp + 0x8;
}

void add_handler(uint64_t src, uint64_t dst) {
    *(uint64_t *)dst = *(uint64_t *)src + *(uint64_t *)dst;
    reg.rip = reg.rip + sizeof(inst_t);
}

void init_handler_table() {
    handler_table[INST_MOV]  = &mov_handler;
    handler_table[INST_PUSH] = &push_handler;
    handler_table[INST_CALL] = &call_handler;
    handler_table[INST_RET]  = &ret_handler;
    handler_table[INST_ADD]  = &add_handler;
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