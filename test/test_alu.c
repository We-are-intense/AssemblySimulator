// ALU 与条件跳转的测试：标志位、cmp 不写回、逻辑运算、全部跳转条件、循环
// 直接 include 实现文件，以便测试其中的 static 函数（见 Makefile 的 test 目标）。
#include "memory/instruction.c"

static int fails = 0;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            fprintf(stderr, "  FAIL %s:%d: ", __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__);                 \
            fprintf(stderr, "\n");                        \
            fails++;                                      \
        }                                                 \
    } while (0)

static char program[8][MAX_INSTRUCTION_CHAR];

static core_t *reset(void)
{
    core_t *cr = &cores[ACTIVE_CORE];
    memset(cr, 0, sizeof(*cr));
    memset(mm, 0, sizeof(mm));
    cr->reg.rbp = 0x7ffffffee110ULL;
    cr->reg.rsp = 0x7ffffffee0f0ULL;
    return cr;
}

static void exec(core_t *cr, const char *text)
{
    snprintf(program[0], MAX_INSTRUCTION_CHAR, "%s", text);
    cr->rip = (uint64_t)program[0];
    instruction_cycle(cr);
}

// 直接调用 alu，按 C 的有/无符号语义核对四个标志位
static void check_alu(alu_op_t op, uint64_t d, uint64_t s, int width)
{
    cpu_flag_t f = {0};
    uint64_t m = width_mask(width);
    uint64_t r = alu(op, d & m, s & m, width, &f);

    // 用"更宽的数"算出不会溢出的真实结果，再判断 CF / OF
    int bits = width * 8;
    __int128 ud = (__int128)(d & m), us = (__int128)(s & m);
    __int128 sd = (__int128)(int64_t)((d & m) << (64 - bits)) >> (64 - bits);
    __int128 ss = (__int128)(int64_t)((s & m) << (64 - bits)) >> (64 - bits);
    __int128 smin = -((__int128)1 << (bits - 1)), smax = ((__int128)1 << (bits - 1)) - 1;

    __int128 exact_u = op == ALU_ADD ? ud + us : ud - us;
    __int128 exact_s = op == ALU_ADD ? sd + ss : sd - ss;
    int cf = exact_u < 0 || exact_u > (__int128)m;
    int of = exact_s < smin || exact_s > smax;

    CHECK(r == ((uint64_t)exact_u & m), "w=%d d=0x%llx s=0x%llx 结果", width, d, s);
    CHECK(f.CF == cf, "w=%d d=0x%llx s=0x%llx CF=%d 应为 %d", width, d, s, f.CF, cf);
    CHECK(f.OF == of, "w=%d d=0x%llx s=0x%llx OF=%d 应为 %d", width, d, s, f.OF, of);
    CHECK(f.ZF == (r == 0), "w=%d d=0x%llx s=0x%llx ZF", width, d, s);
    CHECK(f.SF == SIGN(r, width), "w=%d d=0x%llx s=0x%llx SF", width, d, s);
}

static void test_flags(void)
{
    puts("== add / sub 标志位：与精确结果对照");
    const uint64_t vals[] = {
        0, 1, 2, 0x7f, 0x80, 0xff, 0x7fff, 0x8000, 0xffff,
        0x7fffffff, 0x80000000, 0xffffffff,
        0x7fffffffffffffffULL, 0x8000000000000000ULL, 0xffffffffffffffffULL,
    };
    const int widths[] = {1, 2, 4, 8};
    int n = sizeof(vals) / sizeof(vals[0]);
    int before = fails;
    for (int w = 0; w < 4; w++)
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                check_alu(ALU_ADD, vals[i], vals[j], widths[w]);
                check_alu(ALU_SUB, vals[i], vals[j], widths[w]);
            }
    printf("  %d 组 (d, s, 宽度, 运算) 已核对%s\n", 4 * n * n * 2, fails == before ? "" : "，有失败");
}

static void test_cmp_logic(void)
{
    fprintf(stderr, "%s\n", "== cmp 不写回；逻辑运算");
    core_t *cr = reset();

    cr->reg.rax = 5;
    exec(cr, "cmp $5,%rax");
    CHECK(cr->reg.rax == 5 && cr->flags.ZF == 1, "cmp 只改标志位，不写回");

    cr->reg.rax = 0xffffffffffffffffULL;
    cr->flags.CF = cr->flags.OF = 1;
    exec(cr, "xor %eax,%eax");
    CHECK(cr->reg.rax == 0, "xor %%eax,%%eax 清零整个 rax（32 位写入高位清零）");
    CHECK(cr->flags.ZF == 1 && cr->flags.CF == 0 && cr->flags.OF == 0, "逻辑运算 CF=OF=0");

    cr->reg.rbx = 0xf0f0;
    exec(cr, "and $0xff,%rbx");
    CHECK(cr->reg.rbx == 0xf0, "and: rbx=0x%llx", cr->reg.rbx);
    exec(cr, "orw $0x0f00,%bx");
    CHECK(cr->reg.rbx == 0x0ff0, "or: rbx=0x%llx", cr->reg.rbx);

    exec(cr, "movq $0x7,-0x8(%rbp)");
    exec(cr, "subq $0x7,-0x8(%rbp)");
    CHECK(read_dram_virtual(0x7ffffffee110ULL - 8, 8) == 0 && cr->flags.ZF == 1, "sub 写回内存");
}

// 每个条件跳转：给定 a、b，执行 cmp %rbx,%rax（即比较 a 与 b），检查是否跳转
static void test_conditions(void)
{
    fprintf(stderr, "%s\n", "== 条件跳转：cmp b,a 后与 C 的比较结果对照");
    static const struct { const char *mnem; int signed_cmp; int rel; } jumps[] = {
        // rel: 0 ==, 1 !=, 2 <, 3 >=, 4 <=, 5 >, 6 结果为负, 7 结果非负, 8 总是
        {"je", 0, 0}, {"jne", 0, 1},
        {"jl", 1, 2}, {"jge", 1, 3}, {"jle", 1, 4}, {"jg", 1, 5},
        {"jb", 0, 2}, {"jae", 0, 3}, {"jbe", 0, 4}, {"ja", 0, 5},
        {"js", 0, 6}, {"jns", 0, 7}, {"jmp", 0, 8},
    };
    const int64_t vals[] = {
        0, 1, -1, 5, -5, INT64_MAX, INT64_MIN, INT64_MAX - 1, INT64_MIN + 1,
    };
    int nv = sizeof(vals) / sizeof(vals[0]);
    int before = fails;

    for (size_t k = 0; k < sizeof(jumps) / sizeof(jumps[0]); k++) {
        for (int i = 0; i < nv; i++) {
            for (int j = 0; j < nv; j++) {
                core_t *cr = reset();
                int64_t a = vals[i], b = vals[j];
                cr->reg.rax = (uint64_t)a;
                cr->reg.rbx = (uint64_t)b;
                exec(cr, "cmp %rbx,%rax");

                snprintf(program[1], MAX_INSTRUCTION_CHAR, "%s $%p", jumps[k].mnem, (void *)program[5]);
                cr->rip = (uint64_t)program[1];
                instruction_cycle(cr);
                int taken = (cr->rip == (uint64_t)program[5]);

                uint64_t ua = (uint64_t)a, ub = (uint64_t)b;
                int lt = jumps[k].signed_cmp ? a < b : ua < ub;
                int eq = a == b;
                int neg = (int64_t)(ua - ub) < 0;
                int expect = 0;
                switch (jumps[k].rel) {
                    case 0: expect = eq; break;
                    case 1: expect = !eq; break;
                    case 2: expect = lt; break;
                    case 3: expect = !lt; break;
                    case 4: expect = lt || eq; break;
                    case 5: expect = !lt && !eq; break;
                    case 6: expect = neg; break;
                    case 7: expect = !neg; break;
                    case 8: expect = 1; break;
                }
                CHECK(taken == expect, "a=%lld b=%lld %s 跳转=%d 应为 %d",
                      (long long)a, (long long)b, jumps[k].mnem, taken, expect);
            }
        }
    }
    fprintf(stderr, "  %d 组 (条件, a, b) 已核对%s\n",
           (int)(sizeof(jumps) / sizeof(jumps[0])) * nv * nv, fails == before ? "" : "，有失败");
}

static void test_loop(void)
{
    fprintf(stderr, "%s\n", "== 循环：1 + 2 + ... + 10");
    core_t *cr = reset();
    snprintf(program[0], MAX_INSTRUCTION_CHAR, "mov $0,%%rax");
    snprintf(program[1], MAX_INSTRUCTION_CHAR, "mov $1,%%rcx");
    snprintf(program[2], MAX_INSTRUCTION_CHAR, "add %%rcx,%%rax");     // loop:
    snprintf(program[3], MAX_INSTRUCTION_CHAR, "add $1,%%rcx");
    snprintf(program[4], MAX_INSTRUCTION_CHAR, "cmp $11,%%rcx");
    snprintf(program[5], MAX_INSTRUCTION_CHAR, "jne $%p", (void *)program[2]);

    cr->rip = (uint64_t)program[0];
    int steps = 0;
    while (cr->rip != (uint64_t)program[6] && steps < 1000) {
        instruction_cycle(cr);
        steps++;
    }
    CHECK(cr->reg.rax == 55, "rax=%llu，应为 55", cr->reg.rax);
    CHECK(steps == 2 + 10 * 4, "执行了 %d 条指令，应为 42", steps);
}

int main(void)
{
    init_handler_table();
    // 本测试会执行上千条指令，屏蔽 stdout 上的解析日志；失败信息和进度走 stderr
    FILE *saved = stdout;
    test_flags();
    stdout = fopen("/dev/null", "w");
    test_cmp_logic();
    test_conditions();
    test_loop();
    fclose(stdout);
    stdout = saved;

    if (fails) {
        printf("\n%d 项失败\n", fails);
        return 1;
    }
    printf("\n全部通过\n");
    return 0;
}
