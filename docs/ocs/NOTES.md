# OCS 学习笔记

原文：[article.md](article.md)（腾讯手 Q 团队 2016 年发表，原文已删除；转载自 [wyanassert 的博客](https://blog.wyan.vip/2022/05/dynamic_load_ocs.html)，图片在 `images/`）

本笔记只记录从原文文字和配图里能确认的事实，以及它们与本仓库模拟器、[PLAN.md](../../PLAN.md) 的对应关系。原文没讲的，单独列在「原文没回答的问题」里。

## 一句话

OC 源码经 **clang 前端 + 自研后端** 离线编译成自定义字节码 OCScript，设备上由 OCSVM 解释执行，通过汇编实现的 OCSABI 与 OC runtime、C 函数、block 直接互调，再用 runtime 把原生方法替换成「进 VM 执行」。

## 为什么不用 JSPatch / WaxPatch（原文的五个痛点）

| 痛点 | 本质 | OCS 的对策 |
|---|---|---|
| 学习成本高 | 要学另一门脚本语言和一堆约定 | 直接写 OC，用宏包起来即可 |
| 语义不一致 | JS / Lua 的语义和 OC 不同，机械转换会引入未定义行为 | 指令集与 OC 语义一一对应 |
| 内存管理不一致 | 脚本引擎用 GC，原生用 ARC | 编译器按 ARC 语义生成 retain / release |
| 线程不一致 | 脚本引擎单线程，原生抢占式多线程 | 每个线程一个运算核心 |
| 性能低 | 消息发送要做字符串解析和类型转换 | 消息发送是一条指令，参数不做转换；ABI 用汇编实现 |

对应原文的「四大一致性原则」：开发体验、线程管理、内存管理、语义定义。

## 架构（四个部件）

| 部件 | 原文给出的细节 | PLAN 中的对应 |
|---|---|---|
| **OCScript**（指令集） | 五类指令：基本运算（`ADD_INT`、`SUB_LONG`）、内存操作（`PUSH_CONST_INT`、`PUSH_EXT_CHAR`）、跳转（`JMP_TRUE_CHAR`、`JMP_FALSE_SHORT`）、类型转换（`X_TO_CHAR`、`X_TO_USHORT`）、OC 调用（`MSGSEND_0`、`MSGSEND_1`、`MSGSEND_N`）。**操作码占 1 字节** | 第 2 步 |
| **OCSCompiler** | 前端是 **clang**，后端是自研的 `OCSCodeGenerator`：遍历 AST 生成字节码。能识别 ARC 语义并插入 retain / release；不支持的语法会准确报错 | 第 3 步 |
| **OCSVM** | 三个子系统：内存管理、解释执行、线程管理。解释执行由「指令译码器、程序计数器、操作数栈、程序正文储存区、逻辑 / 算术单元、状态寄存器」组成 | 第 2 步 |
| **OCSBridge** | 分为 OC Bridge、C Bridge、Block Bridge，三者都建在**用汇编实现的 OCSABI** 之上，最内层是 runtime | 第 4、5 步 |

外层的 **OCSEngine** 由三个子系统组成：配置管理（解析配置文件，用 runtime 动态添加 / 替换方法）、运算执行（OCSVM）、通信交互（OCSBridge）。启动流程是：初始化 → 解析配置文件 → 动态添加 / 替换 OC 方法 → OCSVM 运行（对应 PLAN 第 6 步）。

## 关键细节

### 1. 它是栈式 VM，指令带类型

原文用 `int d = a + b / c;` 举例，编译结果是：

```
 0  PUSH_INT_1BYTE  12
 2  POP_LOCAL        0      // a = 12
 4  PUSH_INT_1BYTE  13
 6  POP_LOCAL        1      // b = 13
 8  PUSH_INT_1BYTE  14
10  POP_LOCAL        2      // c = 14
12  PUSH_LOCAL       0      // 压入 a
14  PUSH_LOCAL       1      // 压入 b
16  PUSH_LOCAL       2      // 压入 c
18  DIV_INT                 // b / c
19  ADD_INT                 // a + (b / c)
```

- **栈式**：操作数通过 `PUSH` / `POP` 进出操作数栈，指令本身不带寄存器号。和 JVM 字节码（`iconst` / `istore` / `iload` / `idiv` / `iadd`）几乎一样，不是 Lua 那种寄存器式。
- **按类型区分操作码**：`ADD_INT` 和 `ADD_LONG` 是两条指令，类型在编译期由 clang 的语义分析确定，运行时不用判断类型。这就是「语义一致、无需转换」的来源。
- **按立即数大小区分**：`PUSH_INT_1BYTE` 说明小常数直接编码在指令里，大常数可能走常量池，用来压缩体积。
- 编码紧凑：上例 11 条指令共 20 字节。
- 生成方式就是**后序遍历 AST**：先生成子节点（a、b、c），再生成运算符（`/`、`+`）。

### 2. 字节码文件格式

`head | const pool | method names | code`。head 中记录各段位置。和 Java 的 `.class` 文件、Lua 的二进制 chunk 是同一个思路。

### 3. 多线程：每个线程一个运算核心

OCSCC（OCS 运算核心）有 1 到 n 个，主线程一个，每个子线程各一个，原文说核心结构紧凑，上百个线程也没有压力。

**这和本仓库模拟器的 `core_t cores[]` 是同一个设计**：每个核心有自己的 PC、操作数栈、状态寄存器；字节码（程序正文）共享。

### 4. Block Bridge 最复杂：双向互调矩阵

block 有四种情况都必须能调用：

| | Native 调用 | OCS 调用 |
|---|---|---|
| **Native 生成的 block** | 直接调用原生 invoke | OCS 拿到后要能调用原生 invoke |
| **OCS 生成的 block** | Native 拿到后调用 invoke，要经过「分发器 invoke」进入 VM 执行对应字节码 | 同上 |

难点在第二行：OCS 生成的 block 交给原生代码时，它的 invoke 函数指针必须是真实的机器码。iOS 不能 JIT，所以只能事先用汇编写好通用的分发器入口。

### 5. 使用方式

用 `OCS_PLUGIN_METHODS_START` / `OCS_PLUGIN_METHODS_END` 宏把方法包起来，编译器自动转换。

### 6. 数据

- 引擎体积不到 200k
- 2016 年 9 月在手 Q 上线，近 20 个业务、数万行代码，减包近 1M
- iPhone 5s 上测试，内存、CPU、流畅度、加载耗时都接近原生
- 日 Crash 率十万分之二

## 与本仓库模拟器的对照

OCSVM 解释执行系统的六个组成部分，模拟器里都已经有了：

| OCSVM | 本仓库 |
|---|---|
| 指令译码器 | `assemble()` 生成 `inst_t`；`MNEMONICS[]`、`parse_instruction` |
| 程序计数器 | `core_t.rip`，先 `rip += INST_SIZE` 再执行 |
| 操作数栈 | x86 用寄存器 + 内存栈（`push64` / `pop64`）；OCS 是纯栈式，不同点在这里 |
| 程序正文储存区 | `program_t.insts[]`，`TEXT_BASE` 起的代码段 |
| 逻辑 / 算术单元 | `adder()`、`alu()` |
| 状态寄存器 | `cpu_flag_t`（CF / ZF / SF / OF）、`cond_holds()` |

## 对 PLAN 的影响

1. **第 2 步的 VM 形态**：OCS 选的是栈式 + 带类型操作码。栈式的好处是编译器简单（后序遍历直接出指令，不用分配寄存器）、编码紧凑；寄存器式（Lua）执行时指令条数更少。建议第 2 步先按 OCS 的栈式实现，再对照 Lua 理解两者取舍。
2. **第 3 步确认可行**：OCS 也是 clang 前端 + 自研后端，路线一致。
3. **ARC**：编译器需要按 ARC 语义自己插入 retain / release。PLAN 原本没写，补进「之后再考虑」。
4. **block** 的双向互调矩阵是第 5 步之后最难的部分。

## 原文没回答的问题

这些需要以后自己摸索，或者从 libffi、objc4 源码里找答案：

- OCSABI 的汇编具体怎么写：VM 怎样把栈上的参数按 arm64 调用约定放进寄存器，调用原生函数？
- 原生代码调用一个被替换的方法时，怎样进入 VM？IMP 指向的是什么：通用汇编跳板，还是 `_objc_msgForward` 加 `forwardInvocation:`？跳板如何知道该执行哪段字节码？
- 结构体参数和返回值（如 `CGRect`）怎么处理？
- 字节码中的类名、selector 是在加载时解析成指针，还是每次执行时查找？
- 异常与崩溃保护怎么做？
