// 程序级测试：标签、两遍汇编与重定位、整段程序运行（含 examples/ 下的示例）
// 直接 include 实现文件，以便测试其中的 static 函数（见 Makefile 的 test 目标）。
// 需在仓库根目录运行（make test 即是），以便找到 examples/。
#include "memory/instruction.c"
#include "test_common.h"

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        exit(1);
    }
    static char buf[64 * 1024];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static void test_labels(void)
{
    puts("== 标签：地址、向前 / 向后引用、同一行、注释与伪指令");
    assemble(&test_prog,
             "    .text                 # 伪指令，忽略\n"
             "    .globl main\n"
             "\n"
             "main:   call f            # 0  向后引用：f 在下面才定义\n"
             "        jmp  .L1          # 1\n"
             ".L0:    ret               # 2\n"
             ".L1:\n"
             "f:      mov $f,%rax       # 3  同一条指令上有两个标签\n"
             "        jmp .L0           # 4  向前引用\n");

    CHECK(test_prog.count == 5, "应有 5 条指令，实际 %d", test_prog.count);
    CHECK(test_prog.nlabels == 4, "应有 4 个标签，实际 %d", test_prog.nlabels);
    CHECK(label_address(&test_prog, "main") == ADDR(0), "main 地址");
    CHECK(label_address(&test_prog, ".L0") == ADDR(2), ".L0 地址");
    CHECK(label_address(&test_prog, ".L1") == ADDR(3), ".L1 与 f 指向同一条指令");
    CHECK(label_address(&test_prog, "f") == ADDR(3), "f 地址");

    const inst_t *call = &test_prog.insts[0];
    CHECK(call->src.type == OD_MEM && (uint64_t)call->src.imm == ADDR(3), "call f 重定位到 f 的地址");
    const inst_t *mov = &test_prog.insts[3];
    CHECK(mov->src.type == OD_IMM && (uint64_t)mov->src.imm == ADDR(3), "$f 重定位为 f 的地址");

    // 执行路径（共 8 步）：
    //   call f → mov → jmp .L0 → ret（回到 main 的第 1 条）
    //   → jmp .L1 → mov → jmp .L0 → ret（main 返回，程序结束）
    core_t *cr = reset();
    int steps = run_program(cr, &test_prog, "main", 0);
    CHECK(cr->reg.rax == ADDR(3), "rax 应为 f 的地址");
    CHECK(steps == 8, "执行了 %d 条，应为 8", steps);
    CHECK(cr->reg.rsp == RSP, "退出时栈应平衡");
}

static void test_errors(void)
{
    puts("== 标签相关的错误");
    expect_parse_fail("    jmp nowhere\n");            // 未定义的符号
    expect_parse_fail("a: ret\na: ret\n");             // 重复的标签
    expect_parse_fail("    call 1abc\n");              // 不是合法的数字或符号
}

static void test_example(const char *path, int64_t expect_rax)
{
    printf("== %s\n", path);
    assemble(&test_prog, read_file(path));
    core_t *cr = reset();
    int steps = run_program(cr, &test_prog, "main", 0);
    printf("  %d 条指令，执行 %d 步，rax=%lld\n", test_prog.count, steps, (long long)cr->reg.rax);
    CHECK((int64_t)cr->reg.rax == expect_rax, "%s: rax=%lld，应为 %lld",
          path, (long long)cr->reg.rax, (long long)expect_rax);
    CHECK(cr->reg.rsp == RSP, "%s: 退出时栈应平衡", path);
}

int main(void)
{
    init_handler_table();
    test_labels();
    test_errors();
    test_example("examples/add.s", 8);
    test_example("examples/sum.s", 55);
    test_example("examples/sub.s", 0);

    // sub.s 的 main 把 sub(5, 3) 的结果存在自己的 -16(%rbp)：
    // 进入 main 前 rsp = RSP - 8（返回地址），pushq %rbp 后 rbp = RSP - 16，所以在 RSP - 32
    CHECK(read_dram_virtual(RSP - 32, 4) == 2, "sub.s: sub(5, 3) 应为 2");
    return finish();
}
