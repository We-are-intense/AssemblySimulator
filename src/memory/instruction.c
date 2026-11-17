#include "instruction.h"
#include "cpu/register.h"
#include "memory/dram.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// pointer pointing to the function
typedef void (*handler_t)(od_t *, od_t *, core_t *);
handler_t handler_table[NUM_INSTRTYPE];

/* ================================================================
 * 寄存器：名字 <-> 编号 <-> 地址
 * 解析阶段只把名字翻译成编号，不碰 CPU；执行阶段再用编号取地址。
 * ================================================================ */
#define NUM_REG_NAME 72
static const char *reg_name_list[NUM_REG_NAME] = {
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

static int reg_id(const char *name)
{
    for (int i = 0; i < NUM_REG_NAME; ++i) {
        if (strcmp(name, reg_name_list[i]) == 0) return i;
    }
    return REG_NONE;
}

static uint64_t *reg_ptr(core_t *cr, int id)
{
    reg_t *r = &(cr->reg);
    void *addr[NUM_REG_NAME] = {
        &r->rax,&r->eax,&r->ax,&r->ah,&r->al,
        &r->rbx,&r->ebx,&r->bx,&r->bh,&r->bl,
        &r->rcx,&r->ecx,&r->cx,&r->ch,&r->cl,
        &r->rdx,&r->edx,&r->dx,&r->dh,&r->dl,
        &r->rsi,&r->esi,&r->si,&r->sih,&r->sil,
        &r->rdi,&r->edi,&r->di,&r->dih,&r->dil,
        &r->rbp,&r->ebp,&r->bp,&r->bph,&r->bpl,
        &r->rsp,&r->esp,&r->sp,&r->sph,&r->spl,
        &r->r8,&r->r8d,&r->r8w,&r->r8b,
        &r->r9,&r->r9d,&r->r9w,&r->r9b,
        &r->r10,&r->r10d,&r->r10w,&r->r10b,
        &r->r11,&r->r11d,&r->r11w,&r->r11b,
        &r->r12,&r->r12d,&r->r12w,&r->r12b,
        &r->r13,&r->r13d,&r->r13w,&r->r13b,
        &r->r14,&r->r14d,&r->r14w,&r->r14b,
        &r->r15,&r->r15d,&r->r15w,&r->r15b,
    };
    assert(id >= 0 && id < NUM_REG_NAME);
    return (uint64_t *)addr[id];
}

/* ================================================================
 * 解析：一个指针在字符串上往前走，按语法一段段"吃掉"
 *
 *   instruction = mnemonic [ operand [ ',' operand ] ]
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
    char name[8] = {'%'};
    int len = 1;
    (*s)++;
    while (isalnum((unsigned char)**s) && len < (int)sizeof(name) - 1) {
        name[len++] = *(*s)++;
    }
    int id = reg_id(name);
    if (id == REG_NONE) parse_fail(start, "unknown register");
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
    if (**s == '%') od->base = parse_reg(s);          // [base]
    if (accept(s, ',')) {
        od->index = parse_reg(s);                     // index
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

static const struct { const char *name; op_t op; } mnemonic_table[] = {
    {"mov",  INST_MOV},  {"movq",  INST_MOV},
    {"push", INST_PUSH}, {"pop",   INST_POP},
    {"call", INST_CALL}, {"callq", INST_CALL},
    {"ret",  INST_RET},  {"retq",  INST_RET},
    {"add",  INST_ADD},  {"sub",   INST_SUB},
    {"cmpq", INST_CMP},  {"jne",   INST_JNE},
    {"jmp",  INST_JMP},
};

static op_t parse_mnemonic(const char **s)
{
    const char *start = *s;
    char name[16] = {0};
    int len = 0;
    while (isalpha((unsigned char)**s) && len < (int)sizeof(name) - 1) {
        name[len++] = *(*s)++;
    }
    for (size_t i = 0; i < sizeof(mnemonic_table) / sizeof(mnemonic_table[0]); i++) {
        if (strcmp(name, mnemonic_table[i].name) == 0) return mnemonic_table[i].op;
    }
    parse_fail(start, "unknown instruction");
    return INST_MOV; // unreachable
}

static const char *reg_label(int id)
{
    return id == REG_NONE ? "-" : reg_name_list[id];
}

static void print_od(const char *tag, const od_t *od)
{
    switch (od->type) {
        case OD_IMM:
            printf("  %s: IMM $%lld\n", tag, (long long)od->imm);
            break;
        case OD_REG:
            printf("  %s: REG %s\n", tag, reg_label(od->reg));
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
    printf("parse: %s\n", str);

    const char *s = str;
    skip_space(&s);
    inst->op = parse_mnemonic(&s);
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

    print_od("src", &inst->src);
    print_od("dst", &inst->dst);
}

/* ================================================================
 * 操作数的读与写
 *
 * 操作数在指令里只扮演两种角色："从这里取一个值" 或 "把值存到这里"。
 * 把这两个动作做成 od_read / od_write，三种操作数的差异就被关在这两个
 * 函数里面，handler 只写指令自己的语义，不再按操作数类型分支。
 * ================================================================ */
static void exec_fail(const char *what)
{
    fprintf(stderr, "execute error: %s\n", what);
    abort();
}

static uint64_t effective_address(const od_t *od, core_t *cr)
{
    uint64_t addr = (uint64_t)od->imm;
    if (od->base  != REG_NONE) addr += *reg_ptr(cr, od->base);
    if (od->index != REG_NONE) addr += *reg_ptr(cr, od->index) * od->scale;
    return addr;
}

static uint64_t od_read(const od_t *od, core_t *cr)
{
    switch (od->type) {
        case OD_IMM: return (uint64_t)od->imm;
        case OD_REG: return *reg_ptr(cr, od->reg);
        case OD_MEM: return read64bits_dram_virtual(effective_address(od, cr));
        default:     exec_fail("read from empty operand"); return 0;
    }
}

static void od_write(const od_t *od, core_t *cr, uint64_t value)
{
    switch (od->type) {
        case OD_REG: *reg_ptr(cr, od->reg) = value; break;
        case OD_MEM: write64bits_dram_virtual(effective_address(od, cr), value); break;
        default:     exec_fail("operand is not writable");
    }
}

// call / jmp 的操作数表示"跳到哪里"，不是"取什么值"：
//   callq $0x400540、callq 0x400540 → 跳到 0x400540
//   callq %rax                       → 跳到 rax 的值
static uint64_t jump_target(const od_t *od, core_t *cr)
{
    if (od->type == OD_MEM) return effective_address(od, cr);
    return od_read(od, cr);
}

static void push64(core_t *cr, uint64_t value)
{
    cr->reg.rsp -= 8;
    write64bits_dram_virtual(cr->reg.rsp, value);
}

static uint64_t pop64(core_t *cr)
{
    uint64_t value = read64bits_dram_virtual(cr->reg.rsp);
    cr->reg.rsp += 8;
    return value;
}

#define SIGN(x) (((x) >> 63) & 0x1)

/* ================================================================
 * 指令语义
 * 进入 handler 时 rip 已经指向下一条指令（见 instruction_cycle），
 * 顺序执行的指令不用管 rip；只有 call / ret / jmp 会改写它。
 * mov / push / pop / call / ret 在 x86 上不影响标志位，所以这里也不碰 flags。
 * ================================================================ */
static void mov_handler(od_t *src, od_t *dst, core_t *cr)
{
    od_write(dst, cr, od_read(src, cr));
}

static void push_handler(od_t *src, od_t *dst, core_t *cr)
{
    push64(cr, od_read(src, cr));
}

static void pop_handler(od_t *src, od_t *dst, core_t *cr)
{
    od_write(src, cr, pop64(cr));
}

static void call_handler(od_t *src, od_t *dst, core_t *cr)
{
    push64(cr, cr->rip);              // 返回地址 = 下一条指令
    cr->rip = jump_target(src, cr);
}

static void ret_handler(od_t *src, od_t *dst, core_t *cr)
{
    cr->rip = pop64(cr);
}

static void add_handler(od_t *src, od_t *dst, core_t *cr)
{
    uint64_t s = od_read(src, cr);
    uint64_t d = od_read(dst, cr);
    uint64_t r = d + s;
    cr->flags.CF = (r < s);                                       // 无符号进位
    cr->flags.ZF = (r == 0);
    cr->flags.SF = SIGN(r);
    cr->flags.OF = (SIGN(d) == SIGN(s)) && (SIGN(r) != SIGN(d));  // 同号相加变号
    od_write(dst, cr, r);
}

static void sub_handler(od_t *src, od_t *dst, core_t *cr)
{
    uint64_t s = od_read(src, cr);
    uint64_t d = od_read(dst, cr);
    uint64_t r = d - s;
    cr->flags.CF = (d < s);                                       // 无符号借位
    cr->flags.ZF = (r == 0);
    cr->flags.SF = SIGN(r);
    cr->flags.OF = (SIGN(d) != SIGN(s)) && (SIGN(r) != SIGN(d));  // 异号相减变号
    od_write(dst, cr, r);
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
    handler(&(instr.src), &(instr.dst), cr);
}

void test_parse_inst(uint64_t value, core_t *cr) {
    (void)cr;
    const char *inst_str = (const char *)value;
    inst_t instr;
    parse_instruction(&instr, inst_str);
}
