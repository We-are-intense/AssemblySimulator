# long sum(long n) { long s = 0; for (long i = 1; i <= n; i++) s += i; return s; }
# long main() { return sum(10); }                       结果 rax = 55
# 演示条件跳转与向前、向后引用的标签
    .text
    .globl main
sum:
    xorl   %eax, %eax          # s = 0（写 32 位寄存器，高 32 位自动清零）
    movl   $1, %ecx            # i = 1
    jmp    .L2
.L3:
    addq   %rcx, %rax          # s += i
    addq   $1, %rcx            # i++
.L2:
    cmpq   %rdi, %rcx
    jle    .L3                 # i <= n 继续
    retq
main:
    movl   $10, %edi
    callq  sum
    retq
