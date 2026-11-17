# AssemblySimulator
一个简单的汇编模拟器

进度: [#-------------------------------------------------] 1%

使用:

```shell
make
make run                          # 运行 examples/add.s
make run ASM=examples/sum.s       # 运行指定的汇编文件
./csapp examples/sub.s main       # 汇编文件 + 入口标签
make test                         # 运行 test/ 下的全部测试
```

支持 AT&T 语法：标签（`main:`、`.L2:`）、`#` 注释、`.text` 等伪指令（忽略），
指令 `mov / push / pop / call / ret / add / sub / cmp / and / or / xor / jmp / jcc`，
后缀 `b/w/l/q`，全部 9 种内存寻址写法。

还没做的:

- `leave` 指令：枚举里已有，handler 未实现（gcc `-O0` 输出常见）
- 带 `*` 的间接跳转写法（`call *%rax`、`jmp *(%rax)`）；目前 `call %rax` 直接按间接跳转处理
- 符号加偏移（`$sym+8`、`sym+8(%rip)`）及 `%rip` 相对寻址
- 更多指令：`lea`、`inc / dec`、`neg / not`、`imul / idiv`、移位、`test`、`setcc / cmovcc`
- 64 位指令的立即数在真实 x86 中只有 32 位（符号扩展），这里未做限制
- 内存模型：虚拟地址按 `vaddr % 1000` 映射到 1000 字节物理内存，没有页表，不同地址可能互相覆盖
- 代码段没有机器码，每条指令占固定 4 字节的槽位
- `newprj/` 下的 Objective-C 版本未同步这些改动

函数跳转: 使用 `bear -- make` 生成 `compile_commands.json` 


# 参考

1. [bcst_csapp](https://github.com/yangminz/bcst_csapp)
2. [南京大学 计算机科学与技术系 计算机系统基础 课程实验 2023](https://nju-projectn.github.io/ics-pa-gitbook/ics2023/)
3. [操作系统：设计与实现 (2023 春季学期)](https://jyywiki.cn/OS/2023/index.html)
4. [写给初学者的Makefile入门指南](https://zhuanlan.zhihu.com/p/618350718)
5. [跟我一起写Makefile](https://seisman.github.io/how-to-write-makefile/functions.html)