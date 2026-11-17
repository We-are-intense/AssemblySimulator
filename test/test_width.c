// 操作宽度的测试：子寄存器读写、按宽度访问内存、按宽度算标志位、宽度推断与报错
// 直接 include 实现文件，以便测试其中的 static 函数（见 Makefile 的 test 目标）。
#include "memory/instruction.c"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

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

static char line[MAX_INSTRUCTION_CHAR];

static core_t *reset(void)
{
    core_t *cr = &cores[ACTIVE_CORE];
    memset(cr, 0, sizeof(*cr));
    memset(mm, 0, sizeof(mm));
    cr->reg.rbp = RBP;
    cr->reg.rsp = RSP;
    return cr;
}

static void exec(core_t *cr, const char *text)
{
    snprintf(line, sizeof(line), "%s", text);
    cr->rip = (uint64_t)line;
    instruction_cycle(cr);
}

static int parse_width(const char *text)
{
    inst_t in;
    parse_instruction(&in, text);
    return in.width;
}

// 在子进程里解析，期望解析器报错退出（parse_fail 会 abort）
static void expect_parse_fail(const char *text)
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        inst_t in;
        parse_instruction(&in, text);
        _exit(0);  // 走到这里说明没有报错
    }
    int status = 0;
    waitpid(pid, &status, 0);
    int failed = WIFSIGNALED(status);
    printf("%-30s -> %s\n", text, failed ? "拒绝" : "接受");
    CHECK(failed, "\"%s\" 应被解析器拒绝", text);
}

static void test_sub_registers(void)
{
    puts("== 子寄存器：只改自己那一段");
    core_t *cr = reset();
    cr->reg.rax = 0x1122334455667788ULL;
    cr->reg.rbx = 0xbbbbbbbbbbbbbbbbULL;

    exec(cr, "mov $0xff,%al");
    CHECK(cr->reg.rax == 0x11223344556677ffULL, "写 %%al 后 rax=0x%llx", cr->reg.rax);

    // 原实现从 &rax+1 开始写 8 字节，会把 rbx 的最低字节也改掉
    exec(cr, "mov $0xaa,%ah");
    CHECK(cr->reg.rax == 0x112233445566aaffULL, "写 %%ah 后 rax=0x%llx", cr->reg.rax);
    CHECK(cr->reg.rbx == 0xbbbbbbbbbbbbbbbbULL, "写 %%ah 不应影响 rbx=0x%llx", cr->reg.rbx);

    exec(cr, "mov $0x1234,%ax");
    CHECK(cr->reg.rax == 0x1122334455661234ULL, "写 %%ax 保留高 48 位: rax=0x%llx", cr->reg.rax);

    exec(cr, "mov $0x1,%eax");
    CHECK(cr->reg.rax == 0x1, "写 %%eax 高 32 位清零: rax=0x%llx", cr->reg.rax);

    cr->reg.rax = 0x1122334455667788ULL;
    exec(cr, "mov %ah,%bl");
    CHECK(cr->reg.rbx == 0xbbbbbbbbbbbbbb77ULL, "读 %%ah 写 %%bl: rbx=0x%llx", cr->reg.rbx);

    cr->reg.r9 = 0xffffffffffffffffULL;
    exec(cr, "movl %eax,%r9d");
    CHECK(cr->reg.r9 == 0x55667788ULL, "写 %%r9d 高位清零: r9=0x%llx", cr->reg.r9);
}

static void test_memory_width(void)
{
    puts("== 内存按宽度读写");
    core_t *cr = reset();
    write64bits_dram_virtual(RBP, 0x0123456789abcdefULL);  // (%rbp) 处保存的旧 rbp

    // 编译器常见输出：两个 int 局部变量。原实现按 8 字节写，会互相覆盖并冲掉 (%rbp)
    cr->reg.rdi = 5;
    cr->reg.rsi = 3;
    exec(cr, "movl %edi,-4(%rbp)");
    exec(cr, "movl %esi,-8(%rbp)");
    CHECK(read_dram_virtual(RBP - 4, 4) == 5, "-4(%%rbp) 应为 5");
    CHECK(read_dram_virtual(RBP - 8, 4) == 3, "-8(%%rbp) 应为 3");
    CHECK(read64bits_dram_virtual(RBP) == 0x0123456789abcdefULL, "(%%rbp) 不应被覆盖");

    exec(cr, "movl -4(%rbp),%eax");
    exec(cr, "subl -8(%rbp),%eax");
    CHECK(cr->reg.rax == 2, "5 - 3 = %llu", cr->reg.rax);

    exec(cr, "movb $0x7f,-9(%rbp)");
    CHECK(read_dram_virtual(RBP - 9, 1) == 0x7f, "movb 写 1 字节");
    CHECK(read_dram_virtual(RBP - 8, 4) == 3, "movb 不应影响相邻的 -8(%%rbp)");
}

static void test_flags_width(void)
{
    puts("== 标志位按宽度计算");
    core_t *cr = reset();

    exec(cr, "movl $0x7fffffff,%eax");
    exec(cr, "addl $1,%eax");
    CHECK(cr->reg.rax == 0x80000000ULL, "rax=0x%llx", cr->reg.rax);
    CHECK(cr->flags.OF == 1 && cr->flags.SF == 1 && cr->flags.CF == 0,
          "INT32_MAX+1：32 位有符号溢出，结果为负");

    exec(cr, "movl $0xffffffff,%eax");
    exec(cr, "addl $1,%eax");
    CHECK(cr->reg.rax == 0 && cr->flags.CF == 1 && cr->flags.ZF == 1,
          "0xffffffff+1：32 位进位、结果为 0，rax=0x%llx", cr->reg.rax);

    exec(cr, "movb $0x80,%cl");
    exec(cr, "subb $1,%cl");
    CHECK(cr->reg.rcx == 0x7f && cr->flags.OF == 1 && cr->flags.SF == 0,
          "-128-1：8 位有符号溢出，rcx=0x%llx", cr->reg.rcx);
}

static void test_infer(void)
{
    puts("== 宽度推断");
    CHECK(parse_width("mov %eax,%ebx") == 4, "由寄存器推出 4");
    CHECK(parse_width("mov %al,-1(%rbp)") == 1, "由寄存器推出 1");
    CHECK(parse_width("movw $1,(%rax)") == 2, "由后缀推出 2");
    CHECK(parse_width("movl %edi,-4(%rbp)") == 4, "后缀与寄存器一致");
    CHECK(parse_width("pushq %rbp") == 8, "push 固定 8");
    CHECK(parse_width("push $1") == 8, "push 固定 8");
    CHECK(parse_width("retq") == 8, "ret 固定 8");

    puts("== 非法写法应被拒绝");
    expect_parse_fail("mov $1,-8(%rbp)");      // 宽度不明确
    expect_parse_fail("movl %rax,%rbx");       // 后缀与寄存器不一致
    expect_parse_fail("mov %eax,%rbx");        // 两个寄存器宽度不一致
    expect_parse_fail("push %eax");            // 64 位模式下 push 只能 8 字节
    expect_parse_fail("pushl %rax");           // push 不接受 l 后缀
    expect_parse_fail("mov (%eax),%rbx");      // 地址寄存器必须 64 位
    expect_parse_fail("movq $1,$2");           // 目的不能是立即数
    expect_parse_fail("movq (%rax),(%rbx)");   // 不能两个内存操作数
    expect_parse_fail("movx %rax,%rbx");       // 未知后缀
    expect_parse_fail("mov %sih,%rax");        // 原表里的假寄存器名
}

int main(void)
{
    init_handler_table();
    test_sub_registers();
    test_memory_width();
    test_flags_width();
    test_infer();

    if (fails) {
        printf("\n%d 项失败\n", fails);
        return 1;
    }
    printf("\n全部通过\n");
    return 0;
}
