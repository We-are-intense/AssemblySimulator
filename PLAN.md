# 长线计划：实现一套 OCS 式的 OC 动态化方案

> 课外时间慢慢推进。主线仍是信号与系统 / 音视频，这里不设截止日期，只要求每一步都能独立跑起来。

## 目标

在 iOS 上实现「OC 源码离线编译成自定义字节码，设备端用 VM 解释执行，并能替换原生方法实现」的完整链路。

```
OC 源码 ──clang──▶ AST ──自己的编译器──▶ 字节码 ──下发──▶ 设备端 VM ──桥接──▶ objc runtime / C 函数
```

## 五个部件

| 部件 | 职责 | 现状 / 现成积累 |
|---|---|---|
| 1. 前端 | OC 源码 → AST | 不再手写 parser，复用 clang（libclang 或 `clang -Xclang -ast-dump=json`）。parseObjc 是手写版的练习 |
| 2. 编译器 | AST → 自定义字节码 | 《编译原理之美》第二部分（20–26 讲） |
| 3. 虚拟机 | 设备上解释执行字节码 | 本仓库的汇编模拟器就是原型：`inst_t` + handler 分发 + 寄存器 + 栈 |
| 4. 桥接 | VM ⇄ 原生方法 / C 函数，核心是调用约定（ABI） | 最难的一块。iOS 禁止 JIT，只能靠 libffi 这类预先写好的汇编跳板 |
| 5. 替换 | 把 OC 方法实现换成「进 VM 执行某段字节码」 | objc runtime：`class_replaceMethod`、消息转发 |

## 路线

每一步都要能独立跑通、有测试，再进入下一步。

### 第 1 步：模拟器补上调用约定（本仓库）

- 参数按 System V ABI 走 `rdi / rsi / rdx / rcx / r8 / r9`，返回值走 `rax`
- 实现 `leave`，补齐 gcc `-O0` 常见指令（`lea`、`imul`、`test`、`setcc` 等）
- **验收**：能直接运行 gcc `-O0` 编出来的、带参数和递归的函数（如 `fib(n)`），结果与真机一致
- **练的是**：栈帧和调用约定的直觉，第 4 步的基础
- 参考：CSAPP 第 3 章；《编译原理之美》21–23 讲

### 第 2 步：设计自己的字节码 VM（C）

- 先按 OCS 的做法：栈式 + 按类型区分的 1 字节操作码（`PUSH_LOCAL`、`ADD_INT`、`MSGSEND_N`…），见 [docs/ocs/NOTES.md](docs/ocs/NOTES.md)
- 再对照 Lua 5.4（`lopcodes.h`、`lvm.c`）的寄存器式，理解两者取舍
- 字节码文件格式参考 OCS：`head | const pool | method names | code`
- 先只支持整数运算、比较、跳转、函数调用
- 分发先用 `switch`，再试 computed goto（参考 `ljumptab.h`），对比性能
- **验收**：手写字节码跑通循环求和、递归 `fib`

### 第 3 步：最小前端（libclang → 字节码）

- 用 libclang 遍历 AST，只处理一个只含 `int` 局部变量、算术、`if` / `while`、`return` 的 OC 方法 / C 函数
- 输出第 2 步的字节码文件
- **验收**：`.m` 文件里的函数离线编译后，在 VM 里执行结果正确
- 参考：《编译原理之美》24–26 讲；libclang 的 `clang_visitChildren`

### 第 4 步：第一次桥接（objc_msgSend）

- VM 里能执行 `[obj foo]`、`[obj foo:arg]`
- 先只支持对象参数、整数参数和对象 / 整数返回值
- 在 macOS 命令行程序里验证，先不碰 iOS
- **验收**：VM 里创建 `NSMutableArray`、添加元素、读出 `count`

### 第 5 步：接入 libffi，支持任意签名

- 根据方法的 type encoding（`method_getTypeEncoding`）构造 `ffi_cif`
- 支持浮点、结构体（`CGRect`、`CGPoint`）参数与返回值
- 支持调用普通 C 函数（如 `CGRectMake`）
- 原生 → VM 的回调：用 libffi closure（iOS 上走它预置的静态跳板）
- **验收**：VM 里调用 `CGRectMake` 并把结果传给 `-[UIView setFrame:]`
- 参考：arm64 过程调用标准（AAPCS64）；libffi 源码 `src/aarch64/`

### 第 6 步：方法替换（热修复雏形）

- 用 `class_replaceMethod` 把某个方法的 IMP 换成「进 VM 执行对应字节码」的跳板
- 字节码从文件 / 网络加载
- **验收**：iOS Demo App 中，不重新编译 App，仅替换字节码文件即可修改某个按钮点击后的行为

## 课程对照（理论补足）

宫文学老师的两门课互补：

| 课程 | 讲法 | 作用 |
|---|---|---|
| 《编译原理之美》 | 自底向上，从原理出发亲手实现一门语言 | 补齐前端、语义分析、IR、代码生成的基础 |
| 《编译原理实战课》 | 自顶向下，拆解真实编译器（javac、CPython、V8、Go、Julia / LLVM、Graal 等） | 看工业界的实际取舍：手写 parser、解释器设计、JIT 与运行时 |

按路线步骤拉着看，不线性重刷。实战课的具体讲次待重看时补上。

| 步骤 | 《编译原理之美》 | 《编译原理实战课》 | 其他 |
|---|---|---|---|
| 第 1 步 调用约定 | 21 运行时机制、22–23 生成汇编代码 | — | CSAPP 第 3 章 |
| 第 2 步 字节码 VM | 34 即时编译（了解解释器与 JIT 的分工） | CPython（栈式 VM）、V8 Ignition（解释器） | OCS 是栈式（见笔记）；Lua 5.4 `lvm.c` 是寄存器式，对照理解取舍 |
| 第 3 步 前端 → 字节码 | 02–05 前端（手写子集 parser 时）、11–12 语义分析、24–26 IR | javac（AST → 字节码，最接近本步） | libclang |
| 第 4–6 步 桥接与替换 | — | — | objc4、libffi、AAPCS64 |
| 后续（block、内存） | 33 垃圾收集 | — | objc4 的 ARC 与 block 实现 |

学法：每看完一讲，挑一个点落到模拟器或 VM 里，只看不写很快会忘。

## 之后再考虑

- block 的创建与调用（OCS 的「双向互调矩阵」，见笔记）
- ARC：编译器按 ARC 语义自己插入 retain / release（OCS 的做法）
- 多线程：每个线程一个运算核心（OCS 的 OCSCC，与模拟器的 `cores[]` 同一思路）
- `@property`、`super` 调用、类的新增
- 异常与崩溃保护
- 字节码的签名校验

## 现实约束

- App Store 审核条款 2.5.2 不允许下载代码改变 App 功能，这类方案商业上很敏感；本项目定位为个人学习
- iOS 不允许第三方 App 申请可执行内存，所以全程不能依赖 JIT

## 参考资料

- **OCS 原文与笔记**：[docs/ocs/article.md](docs/ocs/article.md)、[docs/ocs/NOTES.md](docs/ocs/NOTES.md)（本计划的源头）
- CSAPP 第 3 章（机器级表示）、第 7 章（链接）
- 《编译原理之美》《编译原理实战课》（宫文学），讲次对照见上文「课程对照」
- Lua 5.4 源码：`llex.c`、`lparser.c`、`lcode.c`、`lvm.c`
- objc4 源码（runtime）、libffi 源码
- bcst_csapp（yangminz）
