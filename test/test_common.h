// 各测试共用的工具：CHECK 宏、CPU 复位、执行指令、期望解析失败、结果汇总
// 使用方式：先 #include "memory/instruction.c"（为了访问 static 函数），再 #include 本文件
#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static int fails = 0;

// 失败信息写 stderr，不受被测代码的 stdout 输出干扰
#define CHECK(cond, ...)                                          \
    do {                                                          \
        if (!(cond)) {                                            \
            fprintf(stderr, "  FAIL %s:%d: ", __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__);                         \
            fprintf(stderr, "\n");                                \
            fails++;                                              \
        }                                                         \
    } while (0)

#define RBP 0x7ffffffee110ULL
#define RSP 0x7ffffffee0f0ULL

// 第 i 条指令的地址
#define ADDR(i) (TEXT_BASE + (uint64_t)(i) * INST_SIZE)

static program_t test_prog;  // 较大，放静态区

static core_t *reset(void)
{
    core_t *cr = &cores[ACTIVE_CORE];
    memset(cr, 0, sizeof(*cr));
    memset(mm, 0, sizeof(mm));
    cr->reg.rbp = RBP;
    cr->reg.rsp = RSP;
    return cr;
}

// 汇编 source，然后从第一条指令开始执行 steps 条（寄存器和内存保持调用前的状态）
static void exec_n(core_t *cr, const char *source, int steps)
{
    assemble(&test_prog, source);
    cr->rip = TEXT_BASE;
    for (int i = 0; i < steps; i++) instruction_cycle(cr, &test_prog);
}

// 执行单条指令
static void exec(core_t *cr, const char *text)
{
    exec_n(cr, text, 1);
}

// 在子进程里汇编 source，期望解析器报错退出（parse_fail 会 abort）
static void expect_parse_fail(const char *source)
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        assemble(&test_prog, source);
        _exit(0);  // 走到这里说明没有报错
    }
    int status = 0;
    waitpid(pid, &status, 0);
    int failed = WIFSIGNALED(status);

    // 多行源码压成一行显示：换行显示为 " | "，去掉每行开头的缩进
    char shown[128];
    size_t n = 0;
    for (const char *p = source; *p && n + 4 < sizeof(shown); p++) {
        if (*p == '\n') {
            if (p[1]) { memcpy(shown + n, " | ", 3); n += 3; }
            while (p[1] == ' ') p++;
        } else if (!(n == 0 && *p == ' ')) {
            shown[n++] = *p;
        }
    }
    shown[n] = '\0';
    printf("  %-34s -> %s\n", shown, failed ? "拒绝" : "接受");
    CHECK(failed, "\"%s\" 应被拒绝", source);
}

static int finish(void)
{
    fflush(stdout);
    if (fails) {
        printf("\n%d 项失败\n", fails);
        return 1;
    }
    printf("\n全部通过\n");
    return 0;
}
