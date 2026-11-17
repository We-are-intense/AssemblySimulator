// 指令执行的测试：每次执行一条指令，检查寄存器 / 内存 / 标志位 / rip
// 直接 include 实现文件，以便测试其中的 static 函数（见 Makefile 的 test 目标）。
#include "memory/instruction.c"

static int fails = 0;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            printf("  FAIL %s:%d: ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                          \
            printf("\n");                                 \
            fails++;                                      \
        }                                                 \
    } while (0)

#define RBP 0x7ffffffee110ULL
#define RSP 0x7ffffffee0f0ULL

// 指令按定长字符串存放，rip 指向字符串地址
static char program[8][MAX_INSTRUCTION_CHAR];

static core_t *reset(void)
{
    core_t *cr = &cores[ACTIVE_CORE];
    memset(cr, 0, sizeof(*cr));
    memset(mm, 0, sizeof(mm));
    cr->reg.rbp = RBP;
    cr->reg.rsp = RSP;
    return cr;
}

// 把一条指令放到 program[0] 并执行
static void exec(core_t *cr, const char *text)
{
    snprintf(program[0], MAX_INSTRUCTION_CHAR, "%s", text);
    cr->rip = (uint64_t)program[0];
    instruction_cycle(cr);
}

static void test_mov(void)
{
    puts("== mov：任意可读操作数 → 任意可写操作数");
    core_t *cr = reset();

    exec(cr, "mov $0x12,%rax");
    CHECK(cr->reg.rax == 0x12, "imm → reg: rax=0x%llx", cr->reg.rax);
    CHECK(cr->rip == (uint64_t)program[1], "顺序指令执行后 rip 应指向下一条");

    exec(cr, "mov %rax,%rbx");
    CHECK(cr->reg.rbx == 0x12, "reg → reg: rbx=0x%llx", cr->reg.rbx);

    exec(cr, "mov %rbx,-0x8(%rbp)");
    CHECK(read64bits_dram_virtual(RBP - 8) == 0x12, "reg → mem");

    exec(cr, "mov -0x8(%rbp),%rcx");
    CHECK(cr->reg.rcx == 0x12, "mem → reg: rcx=0x%llx", cr->reg.rcx);

    // 原实现没有这个分支，会 assert(0)
    exec(cr, "movq $0x34,-0x10(%rbp)");
    CHECK(read64bits_dram_virtual(RBP - 0x10) == 0x34, "imm → mem");

    // mov 不影响标志位
    cr->flags.ZF = 1;
    exec(cr, "mov $0x1,%rax");
    CHECK(cr->flags.ZF == 1, "mov 不应清除标志位");
}

static void test_push_pop(void)
{
    puts("== push / pop");
    core_t *cr = reset();
    cr->reg.rax = 0xabc;

    exec(cr, "push %rax");
    CHECK(cr->reg.rsp == RSP - 8, "push 后 rsp 应减 8");
    CHECK(read64bits_dram_virtual(RSP - 8) == 0xabc, "push 应把值写到栈顶");

    // 原实现只接受寄存器操作数
    exec(cr, "push $0x5");
    CHECK(read64bits_dram_virtual(RSP - 16) == 0x5, "push imm");

    exec(cr, "pop %rbx");
    CHECK(cr->reg.rbx == 0x5 && cr->reg.rsp == RSP - 8, "pop 到寄存器");

    exec(cr, "pop -0x20(%rbp)");
    CHECK(read64bits_dram_virtual(RBP - 0x20) == 0xabc && cr->reg.rsp == RSP, "pop 到内存");
}

static void test_call_ret(void)
{
    puts("== call / ret");
    core_t *cr = reset();
    snprintf(program[1], MAX_INSTRUCTION_CHAR, "callq  $%p", (void *)program[5]);
    snprintf(program[5], MAX_INSTRUCTION_CHAR, "retq");

    cr->rip = (uint64_t)program[1];
    instruction_cycle(cr);
    CHECK(cr->rip == (uint64_t)program[5], "call 应跳到目标");
    CHECK(read64bits_dram_virtual(cr->reg.rsp) == (uint64_t)program[2], "返回地址应是 call 的下一条");

    instruction_cycle(cr);
    CHECK(cr->rip == (uint64_t)program[2] && cr->reg.rsp == RSP, "ret 应回到 call 的下一条");
}

static void test_add_sub(void)
{
    puts("== add / sub 与标志位");
    core_t *cr = reset();

    cr->reg.rax = 3; cr->reg.rdx = 5;
    exec(cr, "add %rdx,%rax");
    CHECK(cr->reg.rax == 8, "add reg,reg: rax=%llu", cr->reg.rax);

    exec(cr, "add $0x2,%rax");
    CHECK(cr->reg.rax == 10, "add imm,reg: rax=%llu", cr->reg.rax);

    exec(cr, "movq $0x7,-0x8(%rbp)");
    exec(cr, "add %rax,-0x8(%rbp)");
    CHECK(read64bits_dram_virtual(RBP - 8) == 17, "add reg,mem");

    // 原实现在这里会把立即数当指针解引用而崩溃
    exec(cr, "sub $0x3,%rax");
    CHECK(cr->reg.rax == 7, "sub imm,reg: rax=%llu", cr->reg.rax);
    CHECK(cr->flags.CF == 0 && cr->flags.ZF == 0, "7-3 不借位、非零");

    exec(cr, "sub $0x7,%rax");
    CHECK(cr->reg.rax == 0 && cr->flags.ZF == 1, "相减为 0 时 ZF=1");

    exec(cr, "sub $0x1,%rax");
    CHECK(cr->reg.rax == UINT64_MAX && cr->flags.CF == 1 && cr->flags.SF == 1,
          "0-1 应借位且为负");

    cr->reg.rax = 0x7fffffffffffffffULL;
    exec(cr, "add $0x1,%rax");
    CHECK(cr->flags.OF == 1 && cr->flags.CF == 0, "INT64_MAX+1 有符号溢出、无进位");

    cr->reg.rax = UINT64_MAX;
    exec(cr, "add $0x1,%rax");
    CHECK(cr->flags.CF == 1 && cr->flags.ZF == 1 && cr->flags.OF == 0, "-1+1 进位、为零、不溢出");
}

int main(void)
{
    init_handler_table();
    test_mov();
    test_push_pop();
    test_call_ret();
    test_add_sub();

    if (fails) {
        printf("\n%d 项失败\n", fails);
        return 1;
    }
    printf("\n全部通过\n");
    return 0;
}
