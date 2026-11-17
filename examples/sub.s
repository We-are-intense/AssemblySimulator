# int sub(int a, int b) { return a - b; }
# int main() { int r = sub(5, 3); return 0; }    结果 r 存在 main 的 -16(%rbp)
# 来自 newprj/csapp/csapp/parser/inst.txt（clang -O0 输出）
sub:
        pushq   %rbp
        movq    %rsp, %rbp
        movl    %edi, -4(%rbp)
        movl    %esi, -8(%rbp)
        movl    -4(%rbp), %eax
        subl    -8(%rbp), %eax
        popq    %rbp
        retq
main:
        pushq   %rbp
        movq    %rsp, %rbp
        subq    $16, %rsp
        movl    $0, -4(%rbp)
        movl    $5, -8(%rbp)
        movl    $3, -12(%rbp)
        movl    -8(%rbp), %edi
        movl    -12(%rbp), %esi
        callq   sub
        movl    %eax, -16(%rbp)
        xorl    %eax, %eax
        addq    $16, %rsp
        popq    %rbp
        retq
