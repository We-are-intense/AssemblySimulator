# long add(long a, long b) { long c = a + b; return c; }
# long main() { return add(3, 5); }                     结果 rax = 8
# 原 src/main.c 里的例子，改成用标签调用
add:
    push   %rbp
    mov    %rsp,%rbp
    mov    %rdi,-0x18(%rbp)
    mov    %rsi,-0x20(%rbp)
    mov    -0x18(%rbp),%rdx
    mov    -0x20(%rbp),%rax
    add    %rdx,%rax
    mov    %rax,-0x8(%rbp)
    mov    -0x8(%rbp),%rax
    pop    %rbp
    retq
main:
    mov    $0x3,%rdi
    mov    $0x5,%rsi
    callq  add
    retq
