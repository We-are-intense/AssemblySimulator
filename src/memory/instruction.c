#include "instruction.h"
#include "cpu/register.h"
#include "memory/dram.h"
#include <assert.h>
#include <string.h>
handler_t handler_table[NUM_INSTRTYPE];

typedef struct Parse_inst_state
{
    // 0: 开始解析指令 
    // 1: 开始解析源操作 
    // 2: 开始解析目的操作
    // 3: 解析完成
    int inst_state;
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
// reflection
static uint64_t reflect_register(const char *reg_name, core_t *cr)
{
    // lookup table
    reg_t *reg = &(cr->reg);
    uint64_t reg_addr[72] = {
        (uint64_t)&(reg->rax),(uint64_t)&(reg->eax),(uint64_t)&(reg->ax),(uint64_t)&(reg->ah),(uint64_t)&(reg->al),
        (uint64_t)&(reg->rbx),(uint64_t)&(reg->ebx),(uint64_t)&(reg->bx),(uint64_t)&(reg->bh),(uint64_t)&(reg->bl),
        (uint64_t)&(reg->rcx),(uint64_t)&(reg->ecx),(uint64_t)&(reg->cx),(uint64_t)&(reg->ch),(uint64_t)&(reg->cl),
        (uint64_t)&(reg->rdx),(uint64_t)&(reg->edx),(uint64_t)&(reg->dx),(uint64_t)&(reg->dh),(uint64_t)&(reg->dl),
        (uint64_t)&(reg->rsi),(uint64_t)&(reg->esi),(uint64_t)&(reg->si),(uint64_t)&(reg->sih),(uint64_t)&(reg->sil),
        (uint64_t)&(reg->rdi),(uint64_t)&(reg->edi),(uint64_t)&(reg->di),(uint64_t)&(reg->dih),(uint64_t)&(reg->dil),
        (uint64_t)&(reg->rbp),(uint64_t)&(reg->ebp),(uint64_t)&(reg->bp),(uint64_t)&(reg->bph),(uint64_t)&(reg->bpl),
        (uint64_t)&(reg->rsp),(uint64_t)&(reg->esp),(uint64_t)&(reg->sp),(uint64_t)&(reg->sph),(uint64_t)&(reg->spl),
        (uint64_t)&(reg->r8),(uint64_t)&(reg->r8d),(uint64_t)&(reg->r8w),(uint64_t)&(reg->r8b),
        (uint64_t)&(reg->r9),(uint64_t)&(reg->r9d),(uint64_t)&(reg->r9w),(uint64_t)&(reg->r9b),
        (uint64_t)&(reg->r10),(uint64_t)&(reg->r10d),(uint64_t)&(reg->r10w),(uint64_t)&(reg->r10b),
        (uint64_t)&(reg->r11),(uint64_t)&(reg->r11d),(uint64_t)&(reg->r11w),(uint64_t)&(reg->r11b),
        (uint64_t)&(reg->r12),(uint64_t)&(reg->r12d),(uint64_t)&(reg->r12w),(uint64_t)&(reg->r12b),
        (uint64_t)&(reg->r13),(uint64_t)&(reg->r13d),(uint64_t)&(reg->r13w),(uint64_t)&(reg->r13b),
        (uint64_t)&(reg->r14),(uint64_t)&(reg->r14d),(uint64_t)&(reg->r14w),(uint64_t)&(reg->r14b),
        (uint64_t)&(reg->r15),(uint64_t)&(reg->r15d),(uint64_t)&(reg->r15w),(uint64_t)&(reg->r15b),
    };

    for (int i = 0; i < 72; ++ i) {
        if (strcmp(reg_name, reg_name_list[i]) == 0) {
            return reg_addr[i];
        }
    }
    assert(0);
}

static uint64_t string2uint_range(const char *str, int start, int end) {
    
    return 0;
}

static uint64_t string2uint(const char *str) {
    return string2uint_range(str, 0, -1);
}

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

static void parse_operate_inst(inst_t *inst, int state, char *cc_stack[], core_t *cr) {
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
    switch (type)
    {
        case EMPTY:
        {
            break;
        }
        case IMM:
        {
            uint64_t imm = string2uint_range(cc_stack[0], 1, -1);
            if (state == 1) {
                inst->src.imm = imm;
            } else {
                inst->dst.imm = imm;
            }
            break;
        }
        case REG:
        case MM_REG:
        {
            uint64_t reg = reflect_register(cc_stack[0], cr);
            if (state == 1) {
                inst->src.reg1 = reg;
            } else {
                inst->dst.reg1 = reg;
            }
            break;
        }
        case MM_IMM_REG:
        {
            uint64_t imm = string2uint(cc_stack[0]);
            uint64_t reg = reflect_register(cc_stack[1], cr);
            if (state == 1) {
                inst->src.imm  = imm;
                inst->src.reg1 = reg;
            } else {
                inst->dst.imm  = imm;
                inst->dst.reg1 = reg;
            }
            break;
        }
        case MM_REG1_REG2:
        {
            uint64_t reg1 = reflect_register(cc_stack[0], cr);
            uint64_t reg2 = reflect_register(cc_stack[1], cr);
            if (state == 1) {
                inst->src.reg1 = reg1;
                inst->src.reg2 = reg2;
            } else {
                inst->dst.reg1 = reg1;
                inst->dst.reg2 = reg2;
            }
            break;
        }
        case MM_IMM_REG1_REG2:
        {
            uint64_t imm  = string2uint(cc_stack[0]);
            uint64_t reg1 = reflect_register(cc_stack[1], cr);
            uint64_t reg2 = reflect_register(cc_stack[2], cr);
            if (state == 1) {
                inst->src.imm  = imm;
                inst->src.reg1 = reg1;
                inst->src.reg2 = reg2;
            } else {
                inst->dst.imm  = imm;
                inst->dst.reg1 = reg1;
                inst->dst.reg2 = reg2;
            }
            break;
        }
        case MM_REG2_S:
        {
            uint64_t reg2 = reflect_register(cc_stack[0], cr);
            uint64_t s    = string2uint(cc_stack[1]);
            if (state == 1) {
                inst->src.reg2 = reg2;
                inst->src.s    = s;
            } else {
                inst->dst.reg2 = reg2;
                inst->dst.s    = s;
            }
            break;
        }
        case MM_IMM_REG2_S:
        {
            uint64_t imm  = string2uint(cc_stack[0]);
            uint64_t reg2 = reflect_register(cc_stack[1], cr);
            uint64_t s    = string2uint(cc_stack[2]);
            if (state == 1) {
                inst->src.imm  = imm;
                inst->src.reg2 = reg2;
                inst->src.s    = s;
            } else {
                inst->dst.imm  = imm;
                inst->dst.reg2 = reg2;
                inst->dst.s    = s;
            }
            break;
        }
        case MM_REG1_REG2_S:
        {
            uint64_t reg1 = reflect_register(cc_stack[0], cr);
            uint64_t reg2 = reflect_register(cc_stack[1], cr);
            uint64_t s    = string2uint(cc_stack[2]);
            if (state == 1) {
                inst->src.reg1 = reg1;
                inst->src.reg2 = reg2;
                inst->src.s    = s;
            } else {
                inst->dst.reg1 = reg1;
                inst->dst.reg2 = reg2;
                inst->dst.s    = s;
            }
            break;
        }
        case MM_IMM_REG1_REG2_S:
        {
            uint64_t imm  = string2uint(cc_stack[0]);
            uint64_t reg1 = reflect_register(cc_stack[1], cr);
            uint64_t reg2 = reflect_register(cc_stack[2], cr);
            uint64_t s    = string2uint(cc_stack[3]);
            if (state == 1) {
                inst->src.imm  = imm;
                inst->src.reg1 = reg1;
                inst->src.reg2 = reg2;
                inst->src.s    = s;
            } else {
                inst->dst.imm  = imm;
                inst->dst.reg1 = reg1;
                inst->dst.reg2 = reg2;
                inst->dst.s    = s;
            }
            break;
        }
        default: break;
    }
}

static void parse_operate_token(inst_t *inst, parse_inst_t *pit, core_t *cr) {
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
                    od_type = MM_REG;
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
                    parse_operate_inst(inst, pit->inst_state, cc_stack, cr);
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
                    parse_operate_inst(inst, pit->inst_state, cc_stack, cr);
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
                    parse_operate_inst(inst, pit->inst_state, cc_stack, cr);
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
                    parse_operate_inst(inst, pit->inst_state, cc_stack, cr);
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

static void parse_instruction(inst_t *inst, const char *str, core_t *cr) {
    parse_inst_t pit = {
        .inst_state = 0,
        .offset = 0,
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
    // 将指令解析成 token
    parse_inst_token(inst, &pit);
    parse_operate_token(inst, &pit, cr);
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

}

void push_handler(uint64_t src, uint64_t dst) {

}

void pop_reg_handler(uint64_t src, uint64_t dst) {

}

void call_handler(uint64_t src, uint64_t dst) {

}

void ret_handler(uint64_t src, uint64_t dst) {

}

void add_handler(uint64_t src, uint64_t dst) {

}

void init_handler_table() {
    handler_table[INST_MOV]  = &mov_handler;
    handler_table[INST_PUSH] = &push_handler;
    handler_table[INST_CALL] = &call_handler;
    handler_table[INST_RET]  = &ret_handler;
    handler_table[INST_ADD]  = &add_handler;
}

void instruction_cycle(core_t *cr) {
    const char *inst_str = (const char *)cores[ACTIVE_CORE].rip;
    inst_t instr;
    parse_instruction(&instr, inst_str, cr);

    return;
    uint64_t src = decode_od(instr.src);
    uint64_t dst = decode_od(instr.dst);

    handler_t handler = handler_table[instr.op];
    handler(src, dst);
}

void test_parse_inst(uint64_t value, core_t *cr) {
    const char *inst_str = (const char *)value;
    inst_t instr;
    parse_instruction(&instr, inst_str, cr);
}