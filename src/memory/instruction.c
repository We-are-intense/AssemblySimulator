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
static const struct { const char *name; op_t op; int fixed_width; } MNEMONICS[] = {
    {"mov",  INST_MOV,  0},
    {"add",  INST_ADD,  0},
    {"sub",  INST_SUB,  0},
    {"cmp",  INST_CMP,  0},
    {"push", INST_PUSH, 8},
    {"pop",  INST_POP,  8},
    {"call", INST_CALL, 8},
    {"ret",  INST_RET,  8},
    {"jmp",  INST_JMP,  8},
    {"jne",  INST_JNE,  8},
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

// 解析助记符，返回指令；*width 置为后缀或固定宽度给出的宽度，都没有时为 0
static op_t parse_mnemonic(const char **s, int *width)
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
        *width = fixed ? fixed : suffix;
        return MNEMONICS[i].op;
    }
    parse_fail(start, "unknown instruction");
    return INST_MOV; // unreachable
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
    inst->op = parse_mnemonic(&s, &inst->width);
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

// width 字节宽的数的符号位
#define SIGN(x, width) (((x) >> ((width) * 8 - 1)) & 0x1)

/* ================================================================
 * 指令语义
 * 进入 handler 时 rip 已经指向下一条指令（见 instruction_cycle），
 * 顺序执行的指令不用管 rip；只有 call / ret / jmp 会改写它。
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

static void add_handler(const inst_t *in, core_t *cr)
{
    int w = in->width;
    uint64_t s = SRC(in);
    uint64_t d = DST(in);
    uint64_t r = (d + s) & width_mask(w);
    cr->flags.CF = (r < s);                                             // 无符号进位
    cr->flags.ZF = (r == 0);
    cr->flags.SF = SIGN(r, w);
    cr->flags.OF = (SIGN(d, w) == SIGN(s, w)) && (SIGN(r, w) != SIGN(d, w));  // 同号相加变号
    SET_DST(in, r);
}

static void sub_handler(const inst_t *in, core_t *cr)
{
    int w = in->width;
    uint64_t s = SRC(in);
    uint64_t d = DST(in);
    uint64_t r = (d - s) & width_mask(w);
    cr->flags.CF = (d < s);                                             // 无符号借位
    cr->flags.ZF = (r == 0);
    cr->flags.SF = SIGN(r, w);
    cr->flags.OF = (SIGN(d, w) != SIGN(s, w)) && (SIGN(r, w) != SIGN(d, w));  // 异号相减变号
    SET_DST(in, r);
}

void init_handler_table() {
    handler_table[INST_MOV]  = &mov_handler;
    handler_table[INST_PUSH] = &push_handler;
    handler_table[INST_POP]  = &pop_handler;
    handler_table[INST_CALL] = &call_handler;
    handler_table[INST_RET]  = &ret_handler;
    handler_table[INST_ADD]  = &add_handler;
    handler_table[INST_SUB]  = &sub_handler;
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
