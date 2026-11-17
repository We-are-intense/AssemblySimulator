#include "instruction.h"
#include "cpu/register.h"
#include "memory/dram.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef void (*handler_t)(const inst_t *, core_t *);
handler_t handler_table[NUM_INSTRTYPE];

/* ================================================================
 * 寄存器：每个名字都是某个 64 位寄存器的一段位
 *
 *   %rax = rax 的 [0,64)   %eax = rax 的 [0,32)   %ax = rax 的 [0,16)
 *   %al  = rax 的 [0,8)    %ah  = rax 的 [8,16)
 *
 * 所以一个名字只需要三个数：{哪个 gpr, 多宽, 从第几位开始}。
 * ================================================================ */
typedef struct
{
    const char *name;
    uint8_t     id;    // gpr 编号
    uint8_t     width; // 字节数
    uint8_t     shift; // 位偏移，只有 %ah/%bh/%ch/%dh 是 8
} reg_desc_t;

#define GPR(n64, n32, n16, n8, ID) \
    {n64, ID, 8, 0}, {n32, ID, 4, 0}, {n16, ID, 2, 0}, {n8, ID, 1, 0}

static const reg_desc_t REGS[] = {
    GPR("rax", "eax",  "ax",   "al",   RAX), {"ah", RAX, 1, 8},
    GPR("rbx", "ebx",  "bx",   "bl",   RBX), {"bh", RBX, 1, 8},
    GPR("rcx", "ecx",  "cx",   "cl",   RCX), {"ch", RCX, 1, 8},
    GPR("rdx", "edx",  "dx",   "dl",   RDX), {"dh", RDX, 1, 8},
    GPR("rsi", "esi",  "si",   "sil",  RSI),
    GPR("rdi", "edi",  "di",   "dil",  RDI),
    GPR("rbp", "ebp",  "bp",   "bpl",  RBP),
    GPR("rsp", "esp",  "sp",   "spl",  RSP),
    GPR("r8",  "r8d",  "r8w",  "r8b",  R8),
    GPR("r9",  "r9d",  "r9w",  "r9b",  R9),
    GPR("r10", "r10d", "r10w", "r10b", R10),
    GPR("r11", "r11d", "r11w", "r11b", R11),
    GPR("r12", "r12d", "r12w", "r12b", R12),
    GPR("r13", "r13d", "r13w", "r13b", R13),
    GPR("r14", "r14d", "r14w", "r14b", R14),
    GPR("r15", "r15d", "r15w", "r15b", R15),
};
#define NUM_REGS ((int)(sizeof(REGS) / sizeof(REGS[0])))

// 名字不带 '%'
static int reg_id(const char *name)
{
    for (int i = 0; i < NUM_REGS; ++i) {
        if (strcmp(name, REGS[i].name) == 0) return i;
    }
    return REG_NONE;
}

static uint64_t width_mask(int width)
{
    return width >= 8 ? ~0ULL : (1ULL << (width * 8)) - 1;
}

static uint64_t reg_read(const core_t *cr, int r)
{
    const reg_desc_t *d = &REGS[r];
    return (cr->reg.gpr[d->id] >> d->shift) & width_mask(d->width);
}

static void reg_write(core_t *cr, int r, uint64_t value)
{
    const reg_desc_t *d = &REGS[r];
    uint64_t *g = &cr->reg.gpr[d->id];
    if (d->width == 4) {
        // x86-64 规则：写 32 位寄存器时高 32 位清零
        *g = value & 0xffffffffULL;
    } else {
        // 写 8/16/64 位：只替换自己那一段，其余位保持不变
        uint64_t m = width_mask(d->width) << d->shift;
        *g = (*g & ~m) | ((value << d->shift) & m);
    }
}

/* ================================================================
 * 解析：一个指针在字符串上往前走，按语法一段段"吃掉"
 *
 *   instruction = mnemonic[suffix] [ operand [ ',' operand ] ]
 *   suffix      = 'b' | 'w' | 'l' | 'q'                    宽度 1/2/4/8
 *   operand     = '$' num                                  立即数
 *               | reg                                      寄存器
 *               | [num] [ '(' [reg] [ ',' reg [ ',' num ] ] ')' ]   内存
 *
 * 内存那一行的每一段都是"可选"，可选段不出现就保持默认值，
 * 9 种写法因此不需要分别识别。
 * ================================================================ */
static void parse_fail(const char *at, const char *what)
{
    fprintf(stderr, "parse error: %s, at \"%s\"\n", what, at);
    abort();
}

static void skip_space(const char **s)
{
    while (**s == ' ' || **s == '\t') (*s)++;
}

// 吃掉一个指定字符，前后空白一并跳过
static int accept(const char **s, char c)
{
    skip_space(s);
    if (**s != c) return 0;
    (*s)++;
    skip_space(s);
    return 1;
}

static void expect(const char **s, char c)
{
    if (!accept(s, c)) {
        char msg[32];
        snprintf(msg, sizeof(msg), "expect '%c'", c);
        parse_fail(*s, msg);
    }
}

// 十进制 / 0x 十六进制，可带负号；负数按补码存进 64 位
static int64_t parse_num(const char **s)
{
    char *end = NULL;
    uint64_t v = strtoull(*s, &end, 0);
    if (end == *s) parse_fail(*s, "expect number");
    *s = end;
    return (int64_t)v;
}

static int parse_reg(const char **s)
{
    const char *start = *s;
    if (**s != '%') parse_fail(*s, "expect register");
    (*s)++;
    char name[8] = {0};
    int len = 0;
    while (isalnum((unsigned char)**s) && len < (int)sizeof(name) - 1) {
        name[len++] = *(*s)++;
    }
    int id = reg_id(name);
    if (id == REG_NONE) parse_fail(start, "unknown register");
    return id;
}

// 地址计算只用 64 位寄存器（不支持 32 位地址前缀）
static int parse_addr_reg(const char **s)
{
    const char *start = *s;
    int id = parse_reg(s);
    if (REGS[id].width != 8) parse_fail(start, "address register must be 64-bit");
    return id;
}

static void parse_operand(const char **s, od_t *od)
{
    skip_space(s);

    if (accept(s, '$')) {
        od->type = OD_IMM;
        od->imm  = parse_num(s);
        return;
    }
    if (**s == '%') {
        od->type = OD_REG;
        od->reg  = parse_reg(s);
        return;
    }

    // 内存：先放好每一项的默认值，再按顺序尝试吃掉每个可选段
    od->type  = OD_MEM;
    od->imm   = 0;
    od->base  = REG_NONE;
    od->index = REG_NONE;
    od->scale = 1;

    if (**s != '(') od->imm = parse_num(s);           // [disp]
    if (!accept(s, '(')) return;                      // 只有 disp：绝对寻址
    if (**s == '%') od->base = parse_addr_reg(s);     // [base]
    if (accept(s, ',')) {
        od->index = parse_addr_reg(s);                // index
        if (accept(s, ',')) {
            const char *at = *s;
            od->scale = (uint64_t)parse_num(s);       // [scale]
            if (od->scale != 1 && od->scale != 2 && od->scale != 4 && od->scale != 8) {
                parse_fail(at, "scale must be 1/2/4/8");
            }
        }
    }
    expect(s, ')');
}

// 助记符表只放词根；fixed_width 非 0 表示宽度固定（栈和跳转类指令在 64 位模式下总是 8 字节）
// 所有跳转共用 INST_JCC，差别只在 cond
static const struct { const char *name; op_t op; int fixed_width; cond_t cond; } MNEMONICS[] = {
    {"mov",  INST_MOV,  0, 0},
    {"add",  INST_ADD,  0, 0},
    {"sub",  INST_SUB,  0, 0},
    {"cmp",  INST_CMP,  0, 0},
    {"and",  INST_AND,  0, 0},
    {"or",   INST_OR,   0, 0},
    {"xor",  INST_XOR,  0, 0},
    {"push", INST_PUSH, 8, 0},
    {"pop",  INST_POP,  8, 0},
    {"call", INST_CALL, 8, 0},
    {"ret",  INST_RET,  8, 0},
    {"jmp",  INST_JCC,  8, CC_ALWAYS},
    {"je",   INST_JCC,  8, CC_E},   {"jz",  INST_JCC, 8, CC_E},
    {"jne",  INST_JCC,  8, CC_NE},  {"jnz", INST_JCC, 8, CC_NE},
    {"jl",   INST_JCC,  8, CC_L},   {"jge", INST_JCC, 8, CC_GE},
    {"jle",  INST_JCC,  8, CC_LE},  {"jg",  INST_JCC, 8, CC_G},
    {"jb",   INST_JCC,  8, CC_B},   {"jae", INST_JCC, 8, CC_AE},
    {"jbe",  INST_JCC,  8, CC_BE},  {"ja",  INST_JCC, 8, CC_A},
    {"js",   INST_JCC,  8, CC_S},   {"jns", INST_JCC, 8, CC_NS},
};

static int suffix_width(char c)
{
    switch (c) {
        case 'b': return 1;
        case 'w': return 2;
        case 'l': return 4;
        case 'q': return 8;
        default:  return 0;
    }
}

// 解析助记符，填入 op / cond / width（后缀或固定宽度给出的宽度，都没有时为 0）
static void parse_mnemonic(const char **s, inst_t *inst)
{
    const char *start = *s;
    char word[16] = {0};
    int len = 0;
    while (isalpha((unsigned char)**s) && len < (int)sizeof(word) - 1) {
        word[len++] = *(*s)++;
    }

    for (size_t i = 0; i < sizeof(MNEMONICS) / sizeof(MNEMONICS[0]); i++) {
        const char *root = MNEMONICS[i].name;
        size_t root_len = strlen(root);
        int suffix = 0;

        if (strcmp(word, root) == 0) {
            suffix = 0;                                   // 没写后缀：mov
        } else if ((size_t)len == root_len + 1 && strncmp(word, root, root_len) == 0
                   && suffix_width(word[root_len])) {
            suffix = suffix_width(word[root_len]);        // 带后缀：movl
        } else {
            continue;
        }

        int fixed = MNEMONICS[i].fixed_width;
        if (fixed && suffix && suffix != fixed) parse_fail(start, "invalid suffix for this instruction");
        inst->op    = MNEMONICS[i].op;
        inst->cond  = MNEMONICS[i].cond;
        inst->width = fixed ? fixed : suffix;
        return;
    }
    parse_fail(start, "unknown instruction");
}

// 宽度三个来源必须一致：后缀 / 固定宽度、src 寄存器、dst 寄存器
static void infer_width(inst_t *inst, const char *str)
{
    const od_t *ods[2] = {&inst->src, &inst->dst};
    for (int i = 0; i < 2; i++) {
        if (ods[i]->type != OD_REG) continue;
        int w = REGS[ods[i]->reg].width;
        if (inst->width == 0) inst->width = w;
        else if (inst->width != w) parse_fail(str, "operand size mismatch");
    }
    if (inst->width == 0 && inst->src.type != OD_EMPTY) {
        parse_fail(str, "operand size is ambiguous, add suffix b/w/l/q");
    }
}

static const char *reg_label(int id)
{
    return id == REG_NONE ? "-" : REGS[id].name;
}

static void print_od(const char *tag, const od_t *od)
{
    switch (od->type) {
        case OD_IMM:
            printf("  %s: IMM $%lld\n", tag, (long long)od->imm);
            break;
        case OD_REG:
            printf("  %s: REG %%%s\n", tag, reg_label(od->reg));
            break;
        case OD_MEM:
            printf("  %s: MEM disp=%lld base=%s index=%s scale=%llu\n", tag,
                   (long long)od->imm, reg_label(od->base), reg_label(od->index),
                   (unsigned long long)od->scale);
            break;
        default:
            break;
    }
}

static void parse_instruction(inst_t *inst, const char *str)
{
    memset(inst, 0, sizeof(*inst));

    const char *s = str;
    skip_space(&s);
    parse_mnemonic(&s, inst);
    skip_space(&s);
    if (*s != '\0') {
        parse_operand(&s, &inst->src);
        if (accept(&s, ',')) parse_operand(&s, &inst->dst);
    }
    skip_space(&s);
    if (*s != '\0') parse_fail(s, "unexpected trailing characters");

    // x86 编码层面的约束：目的操作数不能是立即数，两个操作数不能同时是内存
    if (inst->dst.type == OD_IMM) parse_fail(str, "destination cannot be immediate");
    if (inst->src.type == OD_MEM && inst->dst.type == OD_MEM) {
        parse_fail(str, "both operands cannot be memory");
    }
    infer_width(inst, str);

    printf("parse: %s  (width=%d)\n", str, inst->width);
    print_od("src", &inst->src);
    print_od("dst", &inst->dst);
}

/* ================================================================
 * 操作数的读与写
 *
 * 操作数在指令里只扮演两种角色："从这里取一个值" 或 "把值存到这里"。
 * 三种操作数的差异关在 od_read / od_write 里；宽度作为参数一并传入，
 * 寄存器的位段规则在 reg_read / reg_write，内存按 width 字节访问。
 * ================================================================ */
static void exec_fail(const char *what)
{
    fprintf(stderr, "execute error: %s\n", what);
    abort();
}

static uint64_t effective_address(const od_t *od, const core_t *cr)
{
    uint64_t addr = (uint64_t)od->imm;
    if (od->base  != REG_NONE) addr += reg_read(cr, od->base);
    if (od->index != REG_NONE) addr += reg_read(cr, od->index) * od->scale;
    return addr;
}

// 寄存器操作数的宽度已在解析时保证与 width 一致
static uint64_t od_read(const od_t *od, const core_t *cr, int width)
{
    switch (od->type) {
        case OD_IMM: return (uint64_t)od->imm & width_mask(width);
        case OD_REG: return reg_read(cr, od->reg);
        case OD_MEM: return read_dram_virtual(effective_address(od, cr), width);
        default:     exec_fail("read from empty operand"); return 0;
    }
}

static void od_write(const od_t *od, core_t *cr, int width, uint64_t value)
{
    switch (od->type) {
        case OD_REG: reg_write(cr, od->reg, value); break;
        case OD_MEM: write_dram_virtual(effective_address(od, cr), value, width); break;
        default:     exec_fail("operand is not writable");
    }
}

// call / jmp 的操作数表示"跳到哪里"，不是"取什么值"：
//   callq $0x400540、callq 0x400540 → 跳到 0x400540
//   callq %rax                       → 跳到 rax 的值
static uint64_t jump_target(const od_t *od, const core_t *cr)
{
    if (od->type == OD_MEM) return effective_address(od, cr);
    return od_read(od, cr, 8);
}

static void push64(core_t *cr, uint64_t value)
{
    cr->reg.rsp -= 8;
    write_dram_virtual(cr->reg.rsp, value, 8);
}

static uint64_t pop64(core_t *cr)
{
    uint64_t value = read_dram_virtual(cr->reg.rsp, 8);
    cr->reg.rsp += 8;
    return value;
}

/* ================================================================
 * ALU：标志位的唯一生产者
 *
 * 硬件里只有一个加法器。减法 d - s 就是 d + (~s) + 1：把 s 取反、进位输入置 1。
 * 所以 add / sub / cmp 共用 adder，四个标志位也只在这里算一次：
 *   CF  加法器最高位有没有进位（减法时取反，表示"借位"）    → 无符号比较用
 *   ZF  结果是否为 0                                         → 相等比较用
 *   SF  结果的符号位                                         → 有符号比较用
 *   OF  两个同号数相加，结果却变号（有符号溢出）              → 有符号比较用
 * 逻辑运算 and / or / xor 不会进位也不会溢出，CF = OF = 0。
 * ================================================================ */
// width 字节宽的数的符号位
#define SIGN(x, width) (((x) >> ((width) * 8 - 1)) & 0x1)

typedef enum { ALU_ADD, ALU_SUB, ALU_AND, ALU_OR, ALU_XOR } alu_op_t;

// width 位宽的加法器：返回 (a + b + carry_in) 的低 width 字节，*carry_out 为最高位进位
static uint64_t adder(uint64_t a, uint64_t b, int carry_in, int width, int *carry_out)
{
    uint64_t r = (a + b + (uint64_t)carry_in) & width_mask(width);
    // 没进位时 r >= a（只有 b 和 carry_in 都为 0 才相等）；进位时结果绕回，r 比 a 小，
    // carry_in 为 1 时还可能恰好绕回到 a
    *carry_out = carry_in ? (r <= a) : (r < a);
    return r;
}

static uint64_t alu(alu_op_t op, uint64_t d, uint64_t s, int width, cpu_flag_t *f)
{
    uint64_t r;
    if (op == ALU_ADD || op == ALU_SUB) {
        int sub   = (op == ALU_SUB);
        uint64_t b = sub ? (~s & width_mask(width)) : s;  // 减法：加上 s 的反码，再进位 1
        int carry;
        r = adder(d, b, sub, width, &carry);
        f->CF = sub ? !carry : carry;                      // 减法没有进位 = 发生了借位
        f->OF = (SIGN(d, width) == SIGN(b, width)) && (SIGN(r, width) != SIGN(d, width));
    } else {
        r = op == ALU_AND ? (d & s) : op == ALU_OR ? (d | s) : (d ^ s);
        f->CF = 0;
        f->OF = 0;
    }
    f->ZF = (r == 0);
    f->SF = SIGN(r, width);
    return r;
}

// 跳转条件：标志位的消费者。cmp s, d 之后，各条件对应 d 与 s 的比较关系
static int cond_holds(cond_t c, const cpu_flag_t *f)
{
    switch (c) {
        case CC_ALWAYS: return 1;
        case CC_E:      return f->ZF;
        case CC_NE:     return !f->ZF;
        case CC_L:      return f->SF != f->OF;                // 有符号：差为负且没溢出，或溢出后符号反了
        case CC_GE:     return f->SF == f->OF;
        case CC_LE:     return f->ZF || f->SF != f->OF;
        case CC_G:      return !f->ZF && f->SF == f->OF;
        case CC_B:      return f->CF;                         // 无符号：借位说明 d < s
        case CC_AE:     return !f->CF;
        case CC_BE:     return f->CF || f->ZF;
        case CC_A:      return !f->CF && !f->ZF;
        case CC_S:      return f->SF;
        case CC_NS:     return !f->SF;
    }
    return 0;
}

/* ================================================================
 * 指令语义
 * 进入 handler 时 rip 已经指向下一条指令（见 instruction_cycle），
 * 顺序执行的指令不用管 rip；只有 call / ret / 跳转会改写它。
 * mov / push / pop / call / ret 在 x86 上不影响标志位，所以这里也不碰 flags。
 * ================================================================ */
#define SRC(in)          od_read(&(in)->src, cr, (in)->width)
#define DST(in)          od_read(&(in)->dst, cr, (in)->width)
#define SET_DST(in, v)   od_write(&(in)->dst, cr, (in)->width, (v))

static void mov_handler(const inst_t *in, core_t *cr)
{
    SET_DST(in, SRC(in));
}

static void push_handler(const inst_t *in, core_t *cr)
{
    push64(cr, SRC(in));
}

static void pop_handler(const inst_t *in, core_t *cr)
{
    od_write(&in->src, cr, 8, pop64(cr));
}

static void call_handler(const inst_t *in, core_t *cr)
{
    push64(cr, cr->rip);              // 返回地址 = 下一条指令
    cr->rip = jump_target(&in->src, cr);
}

static void ret_handler(const inst_t *in, core_t *cr)
{
    cr->rip = pop64(cr);
}

// add / sub / cmp / and / or / xor：同一个 handler，只差运算种类和是否写回
static void alu_handler(const inst_t *in, core_t *cr)
{
    alu_op_t op;
    int writeback = 1;
    switch (in->op) {
        case INST_ADD: op = ALU_ADD; break;
        case INST_SUB: op = ALU_SUB; break;
        case INST_CMP: op = ALU_SUB; writeback = 0; break;
        case INST_AND: op = ALU_AND; break;
        case INST_OR:  op = ALU_OR;  break;
        case INST_XOR: op = ALU_XOR; break;
        default:       exec_fail("not an ALU instruction"); return;
    }
    uint64_t r = alu(op, DST(in), SRC(in), in->width, &cr->flags);
    if (writeback) SET_DST(in, r);
}

static void jcc_handler(const inst_t *in, core_t *cr)
{
    if (cond_holds(in->cond, &cr->flags)) cr->rip = jump_target(&in->src, cr);
}

void init_handler_table() {
    handler_table[INST_MOV]  = &mov_handler;
    handler_table[INST_PUSH] = &push_handler;
    handler_table[INST_POP]  = &pop_handler;
    handler_table[INST_CALL] = &call_handler;
    handler_table[INST_RET]  = &ret_handler;
    handler_table[INST_ADD]  = &alu_handler;
    handler_table[INST_SUB]  = &alu_handler;
    handler_table[INST_CMP]  = &alu_handler;
    handler_table[INST_AND]  = &alu_handler;
    handler_table[INST_OR]   = &alu_handler;
    handler_table[INST_XOR]  = &alu_handler;
    handler_table[INST_JCC]  = &jcc_handler;
}

void instruction_cycle(core_t *cr) {
    const char *inst_str = (const char *)cr->rip;
    inst_t instr;
    parse_instruction(&instr, inst_str);

    // 和真实 CPU 一样，先让 rip 指向下一条指令再执行。
    // 指令是定长字符串（MAX_INSTRUCTION_CHAR），真实 x86 指令则是变长编码。
    cr->rip += sizeof(char) * MAX_INSTRUCTION_CHAR;

    handler_t handler = handler_table[instr.op];
    if (handler == NULL) exec_fail("instruction not implemented");
    handler(&instr, cr);
}

void test_parse_inst(uint64_t value, core_t *cr) {
    (void)cr;
    const char *inst_str = (const char *)value;
    inst_t instr;
    parse_instruction(&instr, inst_str);
}
