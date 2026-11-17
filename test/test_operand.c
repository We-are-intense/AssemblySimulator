// 操作数解析与有效地址计算的测试
// 直接 include 实现文件，以便测试其中的 static 函数；
// 因此 Makefile 链接测试时会排除 instruction.o，避免符号重复。
#include "memory/instruction.c"

#include "test_common.h"

// 解析一个内存操作数，检查各字段与算出的有效地址
static void check_mem(const char *text, int64_t disp, int base, int index,
                      uint64_t scale, uint64_t expect_addr, core_t *cr)
{
    od_t od = {0};
    const char *s = text;
    parse_operand(&s, &od);
    printf("%-26s", text);
    print_od("->", &od);

    CHECK(*s == '\0', "\"%s\" 没有被完整解析，剩余 \"%s\"", text, s);
    CHECK(od.type == OD_MEM, "\"%s\" type=%d，应为 OD_MEM", text, od.type);
    CHECK(od.imm == disp, "\"%s\" disp=%lld，应为 %lld", text, (long long)od.imm, (long long)disp);
    CHECK(od.base == base, "\"%s\" base=%s，应为 %s", text, reg_label(od.base), reg_label(base));
    CHECK(od.index == index, "\"%s\" index=%s，应为 %s", text, reg_label(od.index), reg_label(index));
    CHECK(od.scale == scale, "\"%s\" scale=%llu，应为 %llu", text,
          (unsigned long long)od.scale, (unsigned long long)scale);

    uint64_t addr = effective_address(&od, cr);
    CHECK(addr == expect_addr, "\"%s\" 有效地址=0x%llx，应为 0x%llx", text,
          (unsigned long long)addr, (unsigned long long)expect_addr);
}

static void test_memory_operands(core_t *cr)
{
    puts("== 内存操作数：disp + base + index * scale");
    const int RSI = reg_id("rsi");
    const int RDI = reg_id("rdi");
    const int N   = REG_NONE;
    cr->reg.rsi = 0x100;
    cr->reg.rdi = 0x10;

    //          写法                       disp   base index scale  有效地址
    check_mem("0x12",                      0x12,  N,   N,    1,     0x12,  cr);
    check_mem("-8",                        -8,    N,   N,    1,     (uint64_t)-8, cr);
    check_mem("(%rsi)",                    0,     RSI, N,    1,     0x100, cr);
    check_mem("0x12(%rsi)",                0x12,  RSI, N,    1,     0x112, cr);
    check_mem("-0x8(%rsi)",                -8,    RSI, N,    1,     0xf8,  cr);
    check_mem("(%rsi,%rdi)",               0,     RSI, RDI,  1,     0x110, cr);
    check_mem("0x12(%rsi, %rdi)",          0x12,  RSI, RDI,  1,     0x122, cr);
    check_mem("(,%rdi,4)",                 0,     N,   RDI,  4,     0x40,  cr);
    check_mem("0x12(,%rdi,4)",             0x12,  N,   RDI,  4,     0x52,  cr);
    check_mem("(%rsi,%rdi,8)",             0,     RSI, RDI,  8,     0x180, cr);
    check_mem("0x12( %rsi , %rdi , 8 )",   0x12,  RSI, RDI,  8,     0x192, cr);
}

static void test_instructions(void)
{
    puts("== 整条指令");
    inst_t in;

    parse_instruction(&in, "mov    $0x12,%rax");
    CHECK(in.op == INST_MOV, "op=%d，应为 INST_MOV", in.op);
    CHECK(in.src.type == OD_IMM && in.src.imm == 0x12, "src 应为 IMM $0x12");
    CHECK(in.dst.type == OD_REG && in.dst.reg == reg_id("rax"), "dst 应为 REG %%rax");

    parse_instruction(&in, "mov    %rdi,-0x18(%rbp)");
    CHECK(in.src.type == OD_REG && in.src.reg == reg_id("rdi"), "src 应为 REG %%rdi");
    CHECK(in.dst.type == OD_MEM && in.dst.imm == -0x18 && in.dst.base == reg_id("rbp"),
          "dst 应为 MEM -0x18(%%rbp)");

    parse_instruction(&in, "push   %rbp");
    CHECK(in.op == INST_PUSH, "op=%d，应为 INST_PUSH", in.op);
    CHECK(in.src.type == OD_REG && in.dst.type == OD_EMPTY, "push 只有一个寄存器操作数");

    parse_instruction(&in, "retq");
    CHECK(in.op == INST_RET, "op=%d，应为 INST_RET", in.op);
    CHECK(in.src.type == OD_EMPTY && in.dst.type == OD_EMPTY, "retq 没有操作数");

    puts("== 符号（标签）引用：只记名字，地址留给重定位");
    parse_instruction(&in, "callq  sub");
    print_od("src", &in.src);
    CHECK(in.src.type == OD_MEM && strcmp(in.src.sym, "sub") == 0 && in.src.imm == 0,
          "callq sub 的操作数应为 MEM，符号 sub");

    parse_instruction(&in, "jne .L2");
    CHECK(in.op == INST_JCC && in.cond == CC_NE && strcmp(in.src.sym, ".L2") == 0, "jne .L2");

    parse_instruction(&in, "mov $main,%rax");
    print_od("src", &in.src);
    CHECK(in.src.type == OD_IMM && strcmp(in.src.sym, "main") == 0, "$main 应为带符号的立即数");
}

int main(void)
{
    test_memory_operands(&cores[ACTIVE_CORE]);
    test_instructions();

    return finish();
}
