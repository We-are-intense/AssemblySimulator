#include <stdio.h>
#include <stdlib.h>
#include "cpu/register.h"
#include "memory/instruction.h"

// 用法：csapp [汇编文件] [入口标签]
//   默认运行 examples/add.s 的 main
#define STACK_TOP 0x7ffffffee110ULL

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)size + 1);
    size_t n = fread(buf, 1, (size_t)size, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static program_t prog;

int main(int argc, char const *argv[])
{
    const char *path  = argc > 1 ? argv[1] : "examples/add.s";
    const char *entry = argc > 2 ? argv[2] : "main";

    char *source = read_file(path);
    assemble(&prog, source);
    free(source);
    printf("%s: %d instructions, %d labels\n\n", path, prog.count, prog.nlabels);

    init_handler_table();
    core_t *cr = &cores[ACTIVE_CORE];
    cr->reg.rsp = STACK_TOP;
    cr->reg.rbp = STACK_TOP;

    int steps = run_program(cr, &prog, entry, 1);

    printf("\n%d steps\n", steps);
    printf("rax = 0x%llx (%lld)\n", (unsigned long long)cr->reg.rax, (long long)cr->reg.rax);
    return 0;
}
