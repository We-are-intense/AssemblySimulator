# 生成式软件工程 第 02 讲「提示词工程」幻灯片抄录

> 时间为视频时间（时:分:秒），取自拼图标签；同一页只记首次出现时间。看不清的字记为 [?]。

## [0:00:11] "提示词"从哪里来？

**ChatGPT 普及了 Prompt，但没有发明它**

（配图：一张 ChatGPT 回答截图，0:01:24 处放大显示，文字如下）

> In this C code, the function `return_1` initializes a local variable `x` and then sets it to 1 using an inline assembly statement inside a loop. The assembly statement assigns the value 1 to `%0`, which is an operand that the compiler will replace with the appropriate machine register or memory location for `x`.
>
> Since the loop executes the same assembly statement 100 times, GCC can optimize the loop by moving the assembly statement outside the loop and only executing it once. This optimization is known as loop invariant code motion.
>
> After this optimization, the optimized code for `return_1` would look like this:
>
> ```c
> [?] return_1() {
>   [?] x;
>   // Compiler [?] the assembly statement outside the loop
>   asm("[?] $1, %0" : "=r"(x));  // "x = 1"
>   for ([?] i = 0; i < [?]; i++) {
>   }
>   return x;
> }
> ```
>
> Note that the optimization does not change the behavior of the program, as the loop has [?] effect on the value of `x`. The optimized code simply sets `x` to 1 once and returns it.

- ✦ 2019 年 GPT-2 的发布页面已经写下 "prompting the trained model"：输入文本从"开头"变成了控制模型续写的条件。[GPT-2, 2019]
- ✦ 2020 年 GPT-3 把任务说明和 few-shot 示例都称为 prompt；2021 年，"[prompt programming]" 和 "[prompt-based learning]" 成为研究对象。

## [0:05:53] 演示：浏览器打开 bilibili 直播中心「开播设置」页面

- 地址栏 link.bilibili.com/p/center/index#/my-room/start-live
- 可见「开播操作」「公开直播」「第三方软件开播」「开始直播」、PK「一键开启」、「直播标签」「公告」等区域；其余小字看不清 [?]

## [0:06:01] 立即行动

**AI 把"启动一次尝试"的成本压到很低**

- AI 给我们的能力：**立即行动**，简直就是拖延症的克星
  - 微积分，一个推导不想验算
  - 数字电路，有个数据通路没看懂
  - 计算机系统基础，链接没搞明白
  - 这些统统可以立刻开始
    - 我学生时代的 self-motivated："超高的容忍度"
    - 现在学生的 self-motivated：用上飞轮，确保自己得到了正确的理解，并且重述给 AI
- ✦ AI 消灭的是"先试一下"的边际成本，不消灭真伪判断：提出最小问题 → 做最小检验 → 看反馈 → 更新理解。

## [0:06:06] 演示：终端启动 pi agent

```
$ eza --git --group-directories-first --icons=auto
assets  p1  scripts  CLAUDE.md  README.md
docs    p2  AGENTS.md  Makefile
$ pi

pi v0.84.4
escape interrupt · ctrl+c/ctrl+d clear/exit · / commands · ! bash · ctrl+o more
Press ctrl+o to show full startup help and loaded resources.

Pi can explain its own features and look up its docs. Ask it how to use or extend Pi.

[Context]
  ~/.pi/agent/AGENTS.md, AGENTS.md
[Skills]
  compile, make-artifact, make-slides, tag-audio, transcribe
[Extensions]
  @narumitw/pi-chrome-devtools:dist

~/Projects/GenSE (main)
0.0%/1.0M (auto)                     deepseek-v4-flash • high
```

## [0:08:19] 演示：pi 回答"上下文由哪些部分构成"（0:08:19–0:13:28 持续显示）

可见内容：

- ……签名，模型靠它们才知道"能做什么"。
- **6. 会话历史（持久化的上下文）**：~/.pi/agent/sessions/ 下的 .jsonl 文件（当前：2026-09-01T10-17-05-223Z_....jsonl）。里面不只是对话，还有事件元数据：模型切换、thinking level、**每次调用的 token 用量和费用**（看这次：input:107，cacheRead:2944——说明系统提示词+项目指令等前缀被缓存了，实际只付了增量 token 的钱）。
- **7. 环境变量（PI_\*）**：PI_MODEL=deepseek-v4-flash、PI_REASONING_LEVEL=high、PI_SESSION_ID、cwd 等，注入到提示词里让 agent 知道"我是谁、在哪跑"。
- 一句话总结：提示词只是"种子"，真正的上下文 = **多级指令文件（用户级/项目级）+ 技能索引 + 工具定义 + 会话历史 + 环境信息**的复合体，而且带缓存和按需加载机制。
- Recap：实际检查了 pi agent 的上下文构成——系统提示词、用户级与项目级 AGENTS.md 多级指令、按需加载的 skills 索引、工具 schema、jsonl 会话历史（含 token 用量元数据）以及 PI_\* 环境变量；并观察到 cacheRead 机制证明前缀被缓存复用，印证了"上下文不只是提示词"的说法。
- 状态栏：`↑3.0k ↓2.1k R16k CH90.5% $0.001 0.8%/1.0M (auto)  deepseek-v4-`

## [0:11:57] "安全命令"是谁判的？

**模型提议，Harness 决策，操作系统收口**

> Codex 有一些细节，例如他认为"安全"的命令，不触发用户提示。这是怎么实现的？

- ✦ 模型只生成"想运行什么"：sandbox 决定技术上能碰什么，approval policy 决定何时暂停询问。
- ✦ 规则按完整参数列表匹配 `allow` / `prompt` / `forbidden`；简单 shell 链会被语法解析后逐条检查，最严格的结果获胜。[Codex Rules]
- ✦ 因此"不弹窗"只表示动作落在当前边界内，并不等于模型获得了无限权限。[Agent approvals & security]
- ✦ 真实教训：早期 Codex 把 `rg` 当作只读安全命令，却漏掉 `--pre`、`-z` 等可触发外部程序的参数。[CVE-2025-54558] 说明安全属性属于完整 invocation，不属于命令名。

## [0:18:48] 演示：vim 编辑讲义 Markdown（Prompt Engineering）

```
# Prompt Engineering

## 居然以一个非常意外的方式上热搜了

受到了朋友们的问询 😅
![](zhihu-hot.jpg)

- Anyway，你们懂我不会真的让你们退学的
- 但如果不这么说，你们真的听不进（虽然说了也听不进）
  - 而且 Token 也没那么贵，退一百万步讲，还有 ¥40 的豆包
(含 GLM, DeepSeek, ...)
![](../../p1/slices/doubao-fat.jpg)

## 提示词

[[搜索文献，确认语言模型"提示词"的由来]]
![](gpt-chat.png)

生成式软件工程的"生成"是由自然语言驱动的。对于 Agent 来说，自然语言不仅是你输入的，还分散在各个地方，但我们还是习惯从 ChatGPT 时代的术语"提示词"(prompt)。以下都是提示词：

- Harness 内部 (system prompt、goal 模式注入等)
- AGENTS.md 等
- 加载的 skills
- 你给的指令

再加上大量的工具/skills 调用和输出，构成了模型的**上下文**
，也就是 Pr[token | context] 的 context。~Claude Opus 4.5
```

## [0:18:53] 上下文工程：让模型知道"完成"是什么

**Context 里是一份复杂要求**

- 模型的潜力完全是靠提示词激发的
- 上下文包含了一个**极为复杂的要求**
  - 约束，可以是 intent，也可以是 spec
- 模型被训练为：持续的 Pr[token | context] 能不断推进，直到**满足要求**，也就是学会了"规划"的能力

**逐 Token 推进，也要有停机条件**

- ✦ Pr[token | context] 本身不是规划器：后训练把多步试探、工具调用和收尾塑造成了一种可采样的策略。
- ✦ 设生成轨迹为 τ。只有上下文里存在可识别的完成判据 V(τ) = 1，"继续干"和"可以交卷"才不只是两种高概率续写。

## [0:22:43] 演示：浏览器打开 AI 一句话生成的赛车游戏「极速追击 VELOCITY PURSUIT」（arena.site）

- 0:22:43 先是空白页/黑屏，右下角 "Built with Arena"
- 0:22:44 游戏首页：标题「极速追击」，四个功能卡片（小字看不清 [?]），按钮「▶ 开始追逐」
- 0:22:46 进入 3D 赛道，START 拱门，两辆车

## [0:23:16] 演示：vim 讲义（上下文工程一节）+ 浏览器中的游戏

```
- 例子：Codex 有一些细节，例如他认为"安全"的命令，不触发用户提示。这是怎么实现的？
- (真就是"自然语言编程"的时代来了：[软工人的最后倔强😅](https://atlas.cs.brown.edu/pdf/sash:sosp:2026.pdf))

## 上下文工程

模型的潜力完全是靠提示词激发的。模型的上下文包含了一个**极为复杂的要求**（约束，可以是 intent，可以是 spec)。模型被训练为：持续的 Pr[token | context] 能不断推进，直到**满足要求**，也就是学会了"规划"的能力。

- 推论：越明确（可验证）的要求，越可能得到你满意的结果。
- 不明确的要求，模型可能认为已经"满足"了
  - 大家很喜欢的[一句话生成极品飞车](https://019f6b8d-821e-7c15-98ab-16f3bd7f7409.arena.site/)，测试前端能力
    - Intent -> spec -> impl gap 会逐步放大漂移
    - 你可能是想要一个复用真实地图和美术资产的……
```

（0:23:16 中间窗口为浏览器 Velocity Pursuit 页面）

## [0:26:08] 演示：pi 在 ~/Projects/codex 中分析 Codex 的 guardian 审批提示词

可见内容：

- ……目，标题变成 TRANSCRIPT DELTA START/END，要求 "continue the same review conversation"
- 还有 ROOT CONVERSATION（子代理时带上根会话）、TRUSTED USER ANSWERS、PARENT TURN PERMISSION CONTEXT（父 turn 被禁的路径，防止模型批准绕过读取限制的 escalation）
- 约束边界（mod.rs 常量）：消息 10k token、工具 10k token、单条 2k/1k、最近 40 条；rationale 512 token；action JSON 16k token（超出会截断并标记 truncated）。
- 输出契约由 guardian_output_schema()（prompt.rs:781）作为 final_output_json_schema 传给模型，与 developer 消息末尾的文字描述保持一致（低风险可直接 {"outcome":"allow"}，其余必须填全四项）。
- Recap：完整安全评估 prompt = 静态 developer 消息（证据处理、授权评分、风险分类法、outcome 阈值表、输出契约，由 policy_template.md 注入 tenant policy.md 渲染）+ 动态 user 消息（截断后的 transcript、root/trusted 上下文、待审 action 的 pretty JSON）。不同请求类型（命令/网络/写 stdin）有专门文案，重试走 delta 增量模式节省 token。
- 状态栏：`~/Projects/codex (main)  ↑33k ↓9.6k R737k CH98.8% $0.009 4.9%/1.0M (auto) deepseek-v4-`
- `To resume this session: pi --session 01a05c80-b4e0-71ea-b8a1-75233c95bd56`

0:27:02 同一终端上方还可见审批请求示例：

```
  "tool": "exec_command",
  "command": ["rm", "-rf", "/tmp/build"],
  "cwd": "/home/user/project",
  "sandbox_permissions": { ... },
  "tty": false
}
>>> APPROVAL REQUEST END
```

- 各变体（build_guardian_prompt_items_with_parent_turn）：
  - **NetworkAccess** 走专门文案：强调"由触发命令引起，用户不必显式授权这条连接"，并附 NetworkAccess JSON + trigger 字段
  - **WriteStdin** 强调"是写给已有终端的输入，不是新命令，cwd 是启动目录"
  - **Delta 模式**（同一 turn 内连续审查）：只发新增 transcript 条目，标题变成 TRANSCRIPT DELTA START/END，要求 "continue the same review conversation"
  - 还有 ROOT CONVERSATION（子代理时带上根会话）、TRUSTED USER

## [0:26:17] 演示：用 chromium 打开 AI 生成的个人主页 homepage/index.html

```
$ eza --git --group-directories-first --icons=auto
codex  GenSE  homepage
$ cd homepage/
$ eza --git --group-directories-first --icons=auto
css  js  index.html
$ chromium index.html
[7112:7112:0901/183724.008292:ERROR:ui/ozone/platform/wayland/gpu/wayland_surface_factory.cc:249] '--ozone-platform=wayland' is not compatible with Vulkan. Consider switching to '--ozone-platform=x11' or disabling Vulkan
[7058:7058:0901/183724.136921:ERROR:dbus/object_proxy.cc:572] Failed to call method: org.freedesktop.DBus.Properties.Get: object_path= /org/freedesktop/portal/desktop: org.freedesktop.DBus.Error.InvalidArgs: No such interface "org.freedesktop.portal.FileChooser"
```

- 页面：深色终端风格，左上 [jyy]，大标题「蒋炎岩 JIANG YANYAN」，「做困难的事。」，`$ whoami ~ jyy，一个教操作系统的老师`，下方仿终端窗口（ssh jyy@...、Welcome to jyy's homepage v2.0、courses: OS · ICS · Compilers、gcc --version、make PA && ./run 等，小字看不清 [?]）

## [0:27:04] 从"AI 味"到你的主页

**Bad：把品味留给默认值**

> "我是蒋炎岩，根据我的公开信息，制作一个现代风格的个人主页首页..." 你将会得到 [AI 味]

- ✦ "现代风格"看似具体，实际上几乎没有排除任何模板。两个称职的实现者都能严格照做，却交出完全不同的页面：这仍然只是 intent。

**Better：压缩合法解空间**

> ...，用**具体**风格、**具体**样式和排版... 就可以得到[我的个人主页]

- ✦ 颜色、字体、栅格、留白、组件和参考物共同构成设计语言。关键不是多写几个形容词，而是让选择可组合、结果可检查。

## [0:27:21] 演示：浏览器打开 Hanzhi Liu 的个人主页（JM233333）作为"具体风格"参考

- 顶栏：JM233333 / About / Publications / 简体中文
- Hanzhi Liu (刘瀚之)，Ph.D. candidate, Nanjing University
- "I'm a fourth-year Ph.D. candidate in SPAR Group of Institute of Computer Software, Nanjing University, supervised by Assoc. Prof. Yanyan Jiang and Prof. Chang Xu. Currently my research focuses on understanding and debugging operating system kernels and other system software. I'm also interested in system reliability and observability, as well as designing systems that can solve critical real-world problems."
- 右侧卡片：Email（邮箱地址）、"No response for 72h -> Failed to receive or accidentally omit your email, you can try to resend it."、Google Scholar / ORCID / GitHub
- Selected Publications：[TOCS'26] Visualinux: Visual Debugging for the Linux Kernel（Hanzhi Liu, Yanyan Jiang, Chang Xu；ACM Transactions on Computer Systems；"This paper extends the EuroSys'25 version mainly by introducing the state diff mechanism. It also presents additional case studies to better demonstrate the usefulness of Visualinux."；Just Accepted / Invited Submission / CCF-A；DOI / PDF / Tool）
- [EuroSys'25] Understanding the Linux Kernel, Visually（Proceedings of the 20th European Conference on Computer Systems；Best Student Paper Award；Acceptance: 12.2%…）
- Honors & Awards：EuroSys'25 Best Student Paper Award；Programming Contest Awards（Gold Medal, ACM-ICPC Asia Regional Contest (Jiaozuo Site), Nov 25, 2018 等多条）
- 0:27:46 切到空白页

## [0:28:32] 演示：AI 生成的 jyy 个人主页（终端打字机风格页面，0:28:32–0:28:34 文字逐渐出现，部分帧抖动看不清）

> I'm an Associate Professor of Computer Science at Nanjing University, where I also received my BS in 2011 and PhD in 2017 (advisors: Chang Xu, Xiaoxing Ma, and Jian Lyu). In 2015, I spent a memorable half-year at the Ohio State, working with Feng Qin and happily becoming a Buckeye.
>
> I'm now working on *agentic AI systems and technologies behind test-time scaling*, with general intelligence as the broader horizon, both at Nanjing University and Cosmic Dawn AI. Before the LLM era, I worked on systems software and software automation: operating system kernels, compilers, virtual machines, middleware, and the machinery through which software constructs and transforms itself.
>
> In Fall 2026, I will launch *Generative Software Engineering*, to pursuit the understanding of how software is built, and then rebuilding the way we build it. I'll discontinue the *Operating Systems* and Project-N (proudly with Zihao Yu), which grew beyond the classroom, receiving more than 5,000,000 views, and supported by the *Hitchhiker's Guide to OS* paper.
>
> Competitive programming was once a part of my life. I won first prizes in regional contests, placed second in the 2012 Tencent Hackathon, and competed in the 2009 ACM-ICPC World Finals. I later spent 20 years setting problems for the Jiangsu Provincial Programming Competition, eventually serving as technical committee chair (now retired), and served for three years on the TC of the Asia-Pacific Informatics Olympiad.
>
> jyy:/$ █

## [0:28:40] 演示：vim 讲义（上下文工程 → Rule of thumb）

```
- 例子：Codex 有一些细节，例如他认为"安全"的命令，不触发用户提示。这是怎么实现的？
- (真就是"自然语言编程"的时代来了：[软工人的最后倔强😅](https://atlas.cs.brown.edu/pdf/sash:sosp:2026.pdf))

## 上下文工程

模型的潜力完全是靠提示词激发的。模型的上下文包含了一个**极为复杂的要求**（约束，可以是 intent，可以是 spec)。模型被训练为：持续的 Pr[token | context] 能不断推进，直到**满足要求**，也就是学会了"规划"的能力。

- 推论：越明确（可验证）的要求，越可能得到你满意的结果。
- 不明确的要求，模型可能认为已经"满足"了
  - 大家很喜欢的[一句话生成极品飞车](https://019f6b8d-821e-7c15-98ab-16f3bd7f7409.arena.site/)，测试前端能力
    - Intent -> spec -> impl gap 会逐步放大漂移
    - 你可能是想要一个复用真实地图和美术资产的……

Rule of thumb: 写清楚你的指令，承担一切"不明确"的后果

- 这样模型永远给你的是惊喜 😁
- 一个足够好的模型，要么完成，要么失败，不应该幻觉/违背指令
(如果你的指令不过长，现在的模型基本足够好)

Bad:

>
"我是蒋炎岩，根据我的公开信息，制作一个现代风格的个人主页首页..." 你将会得到 [AI 味](https://jm233333.github.io/)
```

（0:33:04 同一讲义，光标在 Rule of thumb 段落下方）

## [0:33:09] 一步很浅，一串可以很深

**Pr[token | context]**

Pr[token | context] 是深度非常有限的计算。即使是今天的 Mixture-of-Experts，所谓"专家"也只是表示空间上的分段函数，并不是坐在模型里的多个小人。大部分时候，接下来若干个"简单" token 都是高概率确定的；一次前向计算不可能凭空完成复杂任务。

但模型学会了**利用上下文思考**：遇到不确定之处，不直接猜答案，而是先试一试；错了，还可以继续输出 token 纠正。每个新 token 又成为下一步的 context，这就是 chain-of-thought。

- ✦ 可以把它类比成图灵机的纸带：单步转移很简单，中间结果却能写回"纸带"，供下一步继续计算。一次很浅，串起来就有了深度。

**用 Token 换计算深度**

- 你可以同时提**很多要求**，AI 会想出像人一样逐步逼近目标的**过程**：test-time scaling。
  - ✦ "找解比验解难"适合作为 P vs. NP 的直觉类比，不是复杂性证明。只有当中间进展或最终答案可检查时，多写 [?]、多试 [?]、多修补 [?] 才会稳定地变成能力。

## [0:33:14] 三条约束，藏进八句诗

（0:33:14 在翻页过渡中首次露出，0:40:31 完整显示）

**押韵 · 藏头 · 嵌字**

> 用中文写一个押韵的藏头诗，头是"计算机学院真牛逼"，要讲和动物相关的故事，"二零二六九月一日"八个字分别按顺序出现在八句里，位置无所谓。

- ✦ 计成二兔越山冈
- ✦ 算到零时雪满塘
- ✦ 机警狐衔二袋粮
- ✦ 学随六雁辨风向
- ✦ 院中九犬忽声扬
- ✦ 真到月明逢恶狼
- ✦ 牛守一夜迎晨光
- ✦ 逼得日升狼遁荒

- ✦ 竖读句首：计算机学院真牛逼。
- ✦ [?]二零二六九月一日；八句同押 ang 韵。（该行被画面底边截断）

## [0:40:34] 演示：pi（deepseek-v4-flash）完成藏头诗任务

0:40:34 输出：

> 好，给你写一首押韵的七言藏头诗，八句首字连起来正是"计算机学院真牛逼"，八个指定字按"二零二六九月一日"的顺序藏在句中，韵脚统一押 u 韵：
>
> 计逐二兔跃青麓，
> 算见零狐遁深谷，
> 机追二鹿穿林雾，
> 学唤六猪共食黍，
> 院中九鸡争米粟，
> 真如月下老牛步，
> 牛背一鸦栖如故，
> 逼近日落各归宿。

- **说明**：
  - 藏头：计 / 算 / 机 / 学 / 院 / 真 / 牛 / 逼
  - 数字嵌入：二 → 零 → 二 → 六 → 九 → 月 → 一 → 日，严格按顺序分布
  - 韵脚：麓、谷、雾、黍、粟、步、故、宿（全押 u 韵）
  - 故事：一位观察者从早到晚追看兔、狐、鹿、猪、鸡、牛、鸦等动物，最终日落时分万物归巢，"归宿"一词一语双关，既是动物的家，也呼应了"终有归处"的意味。
- Recap：这首诗同时满足藏头、指定数字按序嵌入、统一押韵三个约束，用的是"观察动物一日游"的叙事线串起八句，末句"归宿"双关收尾。写作时先定韵部（u 韵），再按数字顺序和动物意象逐句填词，是藏头诗常用的"先定骨架、再填血肉"方法。
- Thinking blocks: visible
- 状态栏：`/tmp  ↑126 ↓3.0k R2.3k CH94.8% $0.001 0.5%/1.0M (auto)  deepseek-v4-flash • high`

0:42:07 向上滚动可见思考过程（节选）：

- ……的故事，"二零二六九月一日"八个字分别按顺序出现在八句里，位置无所谓。还要押韵。
- 这是一个创作任务，不需要工具调用。直接用中文写。
- 让我构思一下：藏头：计算机学院真牛逼（每句开头）；八句中依次包含"二零二六九月一日"八个字；讲动物故事；押韵
- 押韵的话，我可以用"u"韵或者别的。让我想想。
- 比如用"路、步、处、谷、树、雾、屋、土"之类的韵。
- 第一句（计）：计上心来猎动物，林中"二"三松鼠住——"二"出现
- 第二句（算）：算来松鼠爱爬树，"零"食藏进树洞处——"零"出现
- 第三句（机）：机灵小兔蹦蹦跳，"二"？不行，"二"用过了。按顺序"二零二六九月一日"：
- 顺序：1. 二 2. 零 3. 二（又一个二）—等等，第三个字是"二"？"二零二六九月一日" = 二、零、二、六、九、月、一、日。
- 所以第三句要含"二"，第四句含"六"，第五句含"九"，第六句含"月"，第七句含"一"，第八句含"日"。
- 第三句含"二"有点重复，可以处理成"两"的谐音或直接写"二"。
- 让我认真写：……

## [0:42:52] 十八个字，一片水

**同一偏旁，还得有故事**

> 写十八字的一句话，全部有相同的偏旁或部首，要有实际意义和故事性。

- ✦ 洪涝淹没江港渔浦，油污漂浮，沈涛泅渡济溺。

- ✦ 不计标点恰好 18 字，每个字都带 氵。
- ✦ 灾害、地点、污染、人名、动作和结果都从同一语义场里长出来：约束缩小了搜索空间，也把常用虚词几乎全挡在门外。
- ✦ "偏旁"是字形构件，"部首"主要是检字分类。若要机械验收，spec 应写成"18 个汉字都含同一可见偏旁"。

## [0:47:16] 演示：vim 讲义（模型"听指令"的方式）

```
> ...，用**具体**风格、**具体**样式和排版...
就可以得到[我的个人主页](http://jiangyy.github.io/)

所以："Intent -> Spec -> Impl gap"
是生成式软件工程最重要的——明确你的意图（需要你对**产品领域**的知识）、设计（需要你对**软件工程**的知识）。实现反而多数可以交给 AI，已经超过中级程序员了。

## 模型"听指令"的方式

Pr[token | context]（比如今天的 Mixture-of-Experts，虽然 MoE 里有所谓的"专家"，只是表示空间上的分段函数）
是深度非常有限的计算，不可能完成复杂的任务
(而且大部分时候，"简单"的若干 token 都"高概率确定"）。但模型学会了**利用上下文思考**
：在不确定的时候，不直接猜一个答案，而是"先试一试...
"，错了还可以再输出 token 纠正（这个叫 chain-of-thought）。找到解比验证解难得多（P v.s. NP），这样 Pr[token | context]
只要拥有尝试的坚持、判断的能力和不错的直觉，就能解决指令遵循的问题：

- 你可以同时提**很多要求**，AI 会想出像人一样从出逐步逼近目标的**过程**。（用 token 换思考深度，test-time scaling）

> 用中文写一个押韵的藏头诗，头是"计算机学院真牛逼"，要讲和动物相关的故事，二零二六九月一日八个字分别按顺序出现在八句里，位置无所谓。[[AI写一个试试看]]
```

## [0:47:24] 用好模型遵循指令的能力

**过去的上下文一直都在场**

- 每一个 token 的输出都会"注意"到过去的上下文
- 全局的 `AGENTS.md` 里要求用中文，这个"用中文"会一直保持一定的注意力
- 全局的 git commit 不要 "Copiloted by Claude Code"
- 每一个 token 都会看到这些；适当的时候，下一个 chain-of-thought 就会纠正错误
  - ✦ 模型每输出一个 token，就又多了一次发现偏航、把轨迹拉回来的机会。但"看得见"不是硬约束；[Lost in the Middle] 也提醒我们，长上下文中的信息并不会被同样可靠地利用。

## [0:55:12] 演示：pi 完成"十八字同偏旁"任务

- 句子：江湖游泳渡河波涛汹涌泪流满溢洗涤泥污
- 读起来节奏可以稍微断句："江湖游泳渡河，波涛汹涌，泪流满溢，洗涤泥污。"
- 故事：一位旅人泅渡江湖，途中遭遇惊涛骇浪，泪与浪俱下，上岸后掬水涤尽一身泥污——暗喻历经磨难，以泪洗心。
- 我也可以给两个版本，一个"氵"旁，或者给"扌"和"木"两个选择。不过用户只要求一个，给一个精品+简短说明即可。可以顺便提一下还可以用木字旁等做变体，但别啰嗦。
- 按照项目要求：中文回答，最后 quote block 总结。
- **江湖游泳渡河波涛汹涌泪流满溢洗涤泥污**
- 十八个字，全部为"氵"（三点水）部首：
  - 江、湖、游、泳、渡、河、波、涛、汹、涌、泪、流、满、溢、洗、涤、泥、污
- **故事解读**：一位旅人只身游渡江河，途中遭遇惊涛骇浪，浪花与泪水一同涌落，好不容易登岸，又掬水涤尽一身泥污。三层递进——**渡江之险、流泪之痛、涤污之净**，暗喻人历经风浪、以泪洗心的成长历程，既有画面感，也有实际意义。
- Recap：全句用清一色"氵"旁字讲述"渡河—落泪—涤污"的三段式小故事，把偏旁当"素材库"，按叙事顺序选字造句，兼顾字形统一与意义连贯。这类句子也可用"木""火""虫"等部首复刻处理。
- 状态栏：`/tmp  ↑3.3k ↓5.0k R4.6k CH42.1% $0.002 0.7%/1.0M (auto)  deepseek-v4-flash • high`

## [0:55:45] 演示：终端复制论文并用 chromium 打开 paper.pdf

```
$ eza --git --group-directories-first --icons=auto
assets  docs  p1  p2  scripts  AGENTS.md  CLAUDE.md  Makefile  README.md
$ cd p2/lect/
$ eza --git --group-directories-first --icons=auto
gpt-chat.png  icse27-901.pdf  index.md  lesson.md  zhihu-hot.jpg
$ cp icse27-901.pdf /tmp
$ cd /tmp
$ mkdir papers
$ mv icse27-901.pdf tmp
$ eza --git --group-directories-first --icons=auto
jiti  node-compile-cache  org.chromium.Chromium.7rRZCX  papers  slides-chromium
systemd-private-ba5b161b262849b99ead12ec30d61c66-ModemManager.service-dCMAvV
systemd-private-...-polkit.service-PbrByi
systemd-private-...-systemd-logind.service-0uGHZc
systemd-private-...-upower.service-XMQ7go
slides.html  tmp
$ mv tmp papers/paper.pdf
$ cd papers/
$ eza --git --group-directories-first --icons=auto
paper.pdf
$ xdg-open paper.pdf
chrom  ^C
$ chromium paper.pdf
[11690:11690:0901/191830.988674:ERROR:ui/ozone/platform/wayland/gpu/wayland_surface_factory.cc:249] '--ozone-platform=wayland' is not compatible with Vulkan. ...
[11635:11635:0901/191831.202341:ERROR:dbus/object_proxy.cc:572] Failed to call method: org.freedesktop.DBus.Properties.Get: ...
```

- 浏览器 PDF 查看器打开 /tmp/papers/paper.pdf：《Visualinux: Visual Debugging for the Linux Kernel》，共 37 页；可见摘要、CCS Concepts、"1 INTRODUCTION"、第 2 页 "the ability to exclude specific irrelevant object types or fields..."、"This paper extends Visualinux with a *state diff* mechanism..."；弹窗 "Add freehand drawings, signatures, and highlights with the new Draw tool!"（Remind me later / Got it）

## [0:57:19] 演示：pi 阅读论文后用中文总结（0:57:19–0:58:05，多帧滚动模糊）

英文要点（上方，节选）：

- State diff: differential object graph — object-centric comparison (key = type+address), categories unmodified/updated/introduced/retired, eBPF tracing for reallocation history to disambiguate address reuse
- Visualization: green/red for introduced/retired, like UNIX diff/Git
- 3. Implementation: GDB extension (~5500 LOC Python + 500 LOC GDB scripts), browser front-end (~4000 LOC TypeScript), 4 v-commands (vplot, vctrl, vdiff, vchat), eBPF for allocation tracing, paneling mechanism (primary/secondary panes, …)
- 4. Evaluation: Revived 21 figures from textbook "Understanding the Linux Kernel" (2.6.11) on Linux 6.1, 81% outdated; ViewQL usability: 10 debugging objectives, all <10 lines, DeepSeek-V2 synthesizes all; Maple tree visualization (~70 LOC ViewCL + 100 LOC Python); Bug diagnosis case studies: StackRot (CVE-2023-3269), Dirty Pipe (CVE-2022-0847), controlled study on CVE-2024-0582 (io_uring) — 70 min, findings about mental models and cross-subsystem views; Performance: GDB/QEMU acceptable (ms-level), KGDB on Raspberry Pi 400 slow for large plots
- 5. Methodology: prune / flatten / distill
- 6. Limitations: depends on memory snapshots, trace reproducibility assumed (non-deterministic/concurrency bugs hard), maintenance overhead of ViewCL library, Linux-specific but generalizable
- Now write the summary in Chinese, concise. Then a quote block recap.

中文总结：

- 读完了。这是一篇 ACM 投稿（约 37 页）的论文：
- 《Visualinux: Visual Debugging for the Linux Kernel》
- **核心问题**：Linux 内核运行时状态包含数百万活跃对象，复杂对象有上百个字段互相指来指去，而 GDB、drgn 这类调试器只能输出"铺天盖地"的文本，开发者靠手写递归脚本（还经常要调试脚本本身）来简化状态，认知负担极大。
- **主要贡献**：第一个让内核状态"可视化"的调试框架，基于两层抽象：
  1. **ViewCL**（视图构造语言）：用声明式方式从真实内核状态中构建"简化对象图"。核心概念是 Box（对应 C 结构体）、View（同一对象的不同展示布局，支持 => 继承）、Text/Link 等条目，${...} 内嵌 C 表达式。内置 RBTree、链表等 container_of 风格容器的抽象，还支持多态对象（switch-case）、虚拟对象、数据解压缩、文本装饰器（emoji、函数指针名、位标志名等）。
  2. **ViewQL**（视图查询语言）：SQL 风格（仅 SELECT/UPDATE 两种语句），对已构建的对象图做显示层定制——view（切换视图）、trimmed（隐藏）、collapsed（折叠成可点击按钮）、direction（布局方向）。因为语法极简，DeepSeek-V2 这类 LLM 就能从自然语言描述合成 ViewQL 程序（vchat 命令），开发者可以直接"跟调试器说话"。
  3. **状态差分（state diff）**：提出"差分对象图"，采用**以对象为中心**而非整图树对齐的比较——以（类型，地址）为 key，把对象分为 unmodified/updated/introduced/retired 四类，指针变化产生"旧目标/新目标"两条显式链接，能正确识别对象移动（避免误判为删除+新建）。用 eBPF 跟踪分配器返回点，记录分配历史来区分"地址复用"和"状态连续"，防止 free 后被同址复用误判。
- **实现**：GDB 扩展（约 5500 行 Python + 500 行 GDB 脚本）+ 浏览器前端可视化器（约 4000 行 TypeScript），4 个命令 vplot/vctrl/vdiff/vchat，前端支持类似 tmux 的多面板机制（primary/secondary pane）。
- **评估亮点**：
  - 用 Linux 6.1 的真实运行状态"复活"了经典教材《Understanding the Linux Kernel》(2.6.11) 中的 21 张插图，发现其中 81% 已过时（一半以上实现大改）；
  - 可视化几乎无文档的 **maple tree**（Linux 6.1 新数据结构，约 70 行 ViewCL + 100 行 Python）；
  - 复盘了两个真实 CVE 的诊断：**StackRot**（CVE-2023-3269，maple tree + RCU 延迟回收导致的 UAF）和 **Dirty Pipe**（CVE-2022-0847，splice 共享页缓存 + 遗留 CAN_MERGE 标志），关键证据都藏在跨子系统、跨执行点的对象关系里；
  - 做了受控实验（非作者 PhD 学生 70 分钟诊断 CVE-2024-0582 io_uring 资源泄漏），结论：陌生子系统调试第一步是建立心智模型，Visualinux 在"理解对象组织与跨子系统关联"上帮助最大；
  - 性能：GDB+QEMU 下毫秒级可交互；KGDB + 树莓派 400 上大图明显变慢（KGDB 读一个对象慢约 50 倍）。
- **方法论**：把内核开发者隐式的简化实践显式化为三个操作——**Prune（剪枝）/ Flatten（扁平化）/ Distill（提炼）**，分别编码进 ViewCL 的声明式语法、点连接字段和容器转换函数。
- **局限性**：依赖内存快照（需要 GDB 等工具配合）；假定轨迹可复现，对非确定性/并发 bug 效果有限；ViewCL 库的长期维护成本高；虽为 Linux 设计，但思想可推广到其他系统软件。
- Recap：这篇是一篇"系统+语言+可视化"结合的工作——用两个分层 DSL（ViewCL 构造、ViewQL 定制）把"简化内核状态"变成可编程、LLM 合成、可 diff 的一等操作；其中以对象为中心的差分算法和 eBPF 分配跟踪值得留意，是避免"地址复用误判"的关键设计。读论文时可重点对比"对象图差分"与经典树/文本 diff 的思路差异。

## [0:58:15] 演示：向 pi 提问"我现在是审稿人，不确定这个论文的贡献。"（0:58:15–0:58:42）

- 输入：`我现在是审稿人，不确定这个论文的贡献。`
- 输出（节选）：
  - **论文实际声称的贡献（拆成 5 项）**
    1. **Visualinux 系统本体**：GDB 前端 + 浏览器可视化器，第一个面向 Linux 内核的"可视化交互调试"框架。
    2. **ViewCL**：声明式 DSL，从真实运行状态构建简化对象图。
    3. **ViewQL + 自然语言**：SQL 风格定制语言，由 LLM 从自然语言合成。
    4. **差分对象图**：以对象为中心的状态 diff，含地址复用消歧。
    5. **评估**：复活 ULK 教材 21 图、maple tree 可视化、3 个 CVE 案例、1 个受控实验。
  - **逐项审稿判断**
    - **贡献 1（系统）**：真实但**增量**。论文自己也承认可视化调试是 30 年旧话题（DDD、PROVIDE、JAVAVIS），与 drgn、GDB pretty printer、SchedViz、KernelGraph 的界限必须划清楚。"第一个让内核状态可视化"的宣称很强，但相关工作中"这些工具只做 trace/调用图/源码组织，与 Visualinux 正交"的说辞，容易被审稿人回避而非讨论。
    - **贡献 2（ViewCL）**：**形式上化而非全新**。论文第 6 节自己承认 Prune/Flatten/Distill 是内核开发者"几十年来隐式的实践"，那 ViewCL 的价值就缩水为"把 GDB 脚本/Drgn 脚本包装成 DSL"。审稿人会问：这比 GDB Python + drgn helper 好在哪？是语法糖还是本质能力？${...} 嵌 C 表达式 + 调 GDB Python 函数，本质上还是脚本，只是换了语法。
    - **贡献 3（ViewQL + LLM）**：**新颖性最弱的一环**。"用 LLM 把自然语言转成 SQL"在 2024 年后是成熟配方，用 DeepSeek-V2 只能证明"不需要前沿模型"，这不是贡献是约束条件。且 PiCO QL 早已做过"内核数据结构的 SQL 化访问"，论文对它的区分是"目标更广"，说服力有限。
    - **贡献 4（差分对象图）**：**最值得肯定**。以对象（type, address）为 key 而非树对齐，指针变化显式生成 old/new 双链接，能正确表达"移动"；用 eBPF 跟踪分配历史区分地址复用——这两个设计决策是真实的洞见，也是与现有 object-diff 库拉开差距的支点。但这部分应该作为论文的**核心卖点**来写，而不是作为 ViewCL/ViewQL 的附属。
    - **贡献 5（评估）**：**定性为主，强度不足**。"复活教材图"是漂亮的 demo，但"可读性"无任何用户测量；io_uring 案例 **N=1**，且参与者不写 ViewCL（facilitator 代劳），轨迹天然被实验者塑造，得出"主要精力在决定看什么状态"的结论有循环论证之嫌；无基线对比（同样的任务用 drgn 脚本要多长时间？）；性能只测了 ViewCL 绘图、ViewQL/diff/eBPF 开销、内存开销未测。
  - **审稿人最可能追问的问题**
    1. 去掉 ViewCL/ViewQL 的语法包装，本质贡献还剩多少？请给出与 drgn 脚本方案的**对照实验**。
    2. "状态 diff"是这篇的扩展（论文自述 "This paper extends..."），扩展部分是否撑得起一篇全文的体量？
    3. 差分算法对**非确定性并发 bug**（论文承认的短板）的失效模式是什么？地址复用消歧的假阴性（eBPF 哈希表只跟踪"已出现过"的对象，中间新分配的对象呢）？
    4. 论文声称"第一次运行时状态差分"，需要更正面地回应已有 object differencing（deep-diff 等）为什么不能直接用在活内核状态上——是规模问题还是语义问题？
  - **我的建议立场**：**Major revision 偏 accept 的种子**：核心思想（对象为中心的状态差分 + 两个分层 DSL）是成立的，但当前稿子把"系统搭建"和"方法创新"混在一起，卖点不聚焦。建议要求：(a) 把差分机制提到与 DSL 并列的贡献高度；(b) 补基线对照（drgn/GDB 脚本）和规模/可读性的量化；(c) 弱化"第一个"这类宣称，改为具体工具的正面对比。
  - Recap：审稿时先"拆贡献再评新颖性"——把论文声称的东西拆成系统、语言、LLM、差分、评估五块，逐块对照相关工作和既有工具找增量。这篇的软肋是"形式化既有实践"（ViewCL）和"LLM 转 SQL"（ViewQL）被包装成新贡献，真正的新意在以对象为中心的差分与地址复用消歧，这部分应该成为主角。
- 输入框可见逐字输入的过程："我可能不能wan" → "我可能不能完全xin" → "我可能不能完全信任zuo zh" → "我可能不能完全信任作者"

## [0:59:11] 演示：向 pi 补充"我可能不能完全信任作者……"（0:59:11–1:01:00）

- 输入：`我可能不能完全信任作者，虽然大概率也是诚实的，也有代码实现，可以运行。还是想评估一下研究贡献。`
- 输出（节选）：
  - Keep it tight. Confidence markers like [高/中/低] for my memory. This is a memory-based review so being explicit about confidence is honest and useful.
  - 明白，不查外部资料，全部凭记忆来审。那就把我记忆中的"事实面"当作基准，逐条对照论文断言，并标注我的置信度——这正是怀疑性审稿该做的：**先审事实，再审贡献，最后给验证计划。**
  - **一、记忆审计：论文里的"硬事实"是否站得住**

    | 论文断言 | 我的记忆 | 置信度 |
    |---|---|---|
    | StackRot（CVE-2023-3269）：mmap 读锁下退退役 maple tree 节点 → RCU 延迟回收 → 读者侧 UAF | 与我的记忆一致：expand_stack 路径持读锁操作 maple tree，修复方向是改持写锁。叙述可信 | 高 |
    | Dirty Pipe（CVE-2022-0847）：copy_page_to_iter_pipe() 未初始化 flags，残留 CAN_MERGE | 根因一致（可写证据可信） | 高 |
    | Dirty Pipe "**present since 4.9**，5.8 后才可利用" | **可疑**。我的记忆是：CAN_MERGE 机制随 5.8 引入，漏洞自 5.8 起就存在；4.9 只是修复**回补**到的旧 stable 分支（5.16.11/5.15.25/5.10.102/5.4/4.19/4.9.301）。作者很可能把"回补范围"误当成了"引入时间" | 中高 |
    | maple tree 由 Linux 6.1 引入 | 正确 | 高 |
    | Linux 6.1 的 CFS runqueue 仍是红黑树 | 正确（6.6 才换 EEVDF） | 高 |
    | CVE-2024-0582 是 io_uring 的"kernel-to-user 资源泄漏" | 我对这个 CVE 的记忆太弱，**无法背书**。受控案例的整个前提建立在我审不了的事实上 | 低 |
    | drgn 内置 maple tree 等 helper | 大致可信（drgn 一直在加 helper） | 中 |

  - 结论：**大面上诚实，但至少有一处（Dirty Pipe 时间线）与我的记忆冲突**。审稿人应要求作者逐条给出 CVE 事实的来源引用——这类"看起来无关紧要的细节错误"是评估可信度的最佳探针：一处错了，其余叙事就要打折。
  - **二、把贡献按"可复现性"分层（关键）**：怀疑性审稿的核心是区分**可运行验证的**和**只能信叙事的**：
    - **可运行验证的（给环境就能复现，可信度高）**：21 张 ULK 图：Linux 6.1 + QEMU + ViewCL 程序，跑出来对照截图即可；diff 行为：构造"对象移动""free 后同址复用""跨 CPU"场景，看分类是否正确——这是算法正确性测试；性能数字：可重测。
    - **只能信叙事的（最容易被无意识润色，也最该被要求补证据）**：io_uring 受控实验：N=1，facilitator 代写 ViewCL 并绘图，"70 分钟诊断出根因"——这个结论的强度与证据强度严重不匹配。**要求提供完整操作日志/录像**，而不是事后总结表；两个 CVE 的"假设链" H1→H2→H3 的叙述太工整，像复盘而非实时记录。Dirty Pipe 的时间线错误恰好提醒：这些叙述值得逐条核对。
  - **三、凭记忆评估"增量"（不信任作者时，看不可替代性）**
    1. **ViewCL**：差异化价值有限。drgn 已有 Python helper + 任意表达式能力，表达力不比 ViewCL 弱；ViewCL 的增量是"声明式 + 图 + 自动布局"，但"到底省多少功夫"全文没有基线测量（不跟 drgn 脚本比时长）→ 工程贡献，不是方法贡献。
    2. **ViewQL + LLM**：PiCO QL（EuroSys'14）已做过内核对象的 SQL 化访问；"LLM 转 SQL"是现成配方。增量只剩 collapsed/trimmed 这类显示属性——产品特性级。
    3. **差分对象图**：全篇**唯一不可替代**的点。以 (type,address) 为 key 做对象级分类、指针生成 old/new 双链接、eBPF 消歧地址复用——算法不复杂，但"对内核场景正确建模"是真洞见。可审的点：eBPF 只跟踪"已出现在图中"的对象，新分配对象的假阴性没被讨论：这部分应该被当作论文主角，而不是附属。
  - **四、我的审稿建议（基于怀疑但可操作）**：不给作者"信任"，给作者**验证计划**——这反而是最公平的做法：
    1. 要求公开可运行环境，复现 2-3 张 ULK 图 + 1 个 diff 场景；
    2. 要求给出 diff 的故障注入测试（地址复用、对象移动）与 CVE 案例的完整原始日志；
    3. 要求逐条修正/引用 CVE 事实（Dirty Pipe 时间线是现成把柄）；
    4. 要求补与 drgn 的基线对比；
    5. 若 1-3 全部通过 → 差分对象图+工程打磨可支撑 weak accept；若 diff 部分已有可运行实现且测试扎实，即使 ViewCL/ViewQL 是包装，贡献也成立。
  - Recap：怀疑性审稿的三步法——①用记忆交叉验证论文的事实断言（找"时间线错误"这类硬伤，Dirty Pipe 的 4.9/5.8 就是现成疑点）；②把贡献按"能否复现"分层，叙事性的案例（N=1 的 io_uring 诊断）要原始日志；③评估增量看"不可替代性"，这篇真正不可替代的只有差分对象图，其余是包装。不给作者信任，给作者一个验证计划。
- 状态栏：`/tmp/papers  ... 4.6%/1.0M (auto)  deepseek-v4-flash • high`

## [1:01:56] 演示：向 pi 提出"站在人类已知的知识边界上"的审稿框架（1:01:56–1:03:00）

- 输入：`应该这样：我们站在人类已知的知识边界上看这个问题。这不是一个完全老的问题—相关的东西是什么？解决到什么程度？这个工作试图解决哪些不一样增量的问题？`
- 模型复述：用户现在提出一个更审慎的审稿框架：不要陷在"怀疑/信任"的情绪里，而是站在"人类已知的知识边界"上看这篇论文。具体三个问题：1. 相关的东西是什么？（这个领域已经有什么？）2. 解决到什么程度？（已有工作做到什么程度了？）3. 这篇工作试图解决哪些不一样的增量问题？（增量是什么？是否填补了知识边界上的真实空白？）
- 输出（节选）：
  - 对，这才是正确的审稿姿势：**不是"信不信作者"，而是"这篇是否真的推到了人类已知的边界上"**。那我把这个领域的知识边界地图画出来，然后定位这篇论文的增量在哪儿。
  - **一、相关的东西是什么，解决到什么程度？** 这个领域有四股已经相当成熟的流：

    | 方向 | 代表工作 | 已解决到什么程度 | 边界缺口 |
    |---|---|---|---|
    | ①脚本化内核检查 | GDB/KGDB、crash、**drgn**（Python 访问内核内存，内置 IDR/maple tree 等 helper） | 能把"地址+类型+内存"变成**可编程对象**，任意表达式、任意脚本——这是"拿到底层状态"的终点 | 输出仍是文本/脚本，结构重建靠人脑 |
    | ②程序可视化 | DBX 图形化(1985)、DDD、JAVAVIS、GDBGUI；系统侧有 SchedViz、KernelGraph、源码地图 | 能把程序状态画成图，能交互——但面向**小规模程序**，缺"大规模状态简化"抽象；对内核只做了 trace/调用图/源码组织 | 缺"运行时对象状态的交互式可视化" |
    | ③对象查询与 diff | **PiCO QL**（EuroSys'14，内核数据结构 SQL 化）、PQL/JQL：通用 object diff（deep-diff 等）、代码/模型/日志 diff | 能"查询"对象图、能"比较"快照值——但查询与可视化脱节；diff 对"对象移动"会误报为删除+新建 | 缺"查询+展示定制+演化比较"的闭环 |
    | ④LLM × 调试 | NL→SQL/查询合成、错误解释、自动修复 | 2023 年后是**成熟配方**，不是新鲜事 | 无本质缺口 |

  - **二、交叉空白是否真实存在？** **真实存在。** 边界上确实没有一个系统把这三件事做成了闭环：drgn 能拿状态，但不画图；可视化能画图，但不会简化千万级对象（直接套 DDD 思路到内核上确实不现实）；PiCO QL 能查询，但不做展示定制、不做演化比较；通用 diff 能比快照，但不理解"内核对象的身份语义"（地址复用、移动）。所以论文站的位置——四股流的交叉点——是**知识边界上的一个真实空隙**，不是重复劳动。这一点审稿人应该承认。
  - **三、论文的增量，拆开看是什么成色**：把增量按"是否真的推进了知识边界"分级：
    - **真增量（填补了边界缺口）**：1. **差分对象图**：以对象身份（type, address）而非结构树对齐做比较单元，指针变化显式生成 old/new 双链接→能表达"对象移动"；识别并处理**地址复用**（内核 slab 缓存特有的）→ 这两个问题在已有 diff 文献里，据我所知**确实没人针对"活内核状态"做过**。问题建模是新的，算法不需要很复杂也成立。2. **"简化"从隐式手艺变成显式第一公民**：drgn 时代简化=每次写一次性过程式脚本；ViewCL 把 Prune/Flatten/Distill 变成语言原语 + 内置容器抽象。增量真实，但是**工程增量**（把简化固化成可复用的东西），不是方法增量。
    - **应用增量（站在别家肩膀上，不算新）**：3. ViewQL 的"显示属性数据模型"（view/trimmed/collapsed/direction）——把展示语义纳入查询——比 PiCO QL 多走了一步，但本质是 SQL+产品特性。4. LLM 自然语言界面：NL→SQL 是 2023+ 常识，用 DeepSeek-V2 只是证明"不需要前沿模型"——这是设计选择不是贡献。
  - **四、站在边界上的审稿结论**：公允的定位：**这不是一个完全新的问题，但它确实填补了一个真实的交叉空白**。真正的知识推进只有差分对象图那一块，其余是把这个推进包装成可用系统的工程。因此审稿意见应该聚焦三件事，而不是纠结信任问题：
    1. **要求作者把贡献排序讲清楚**：差分对象图是主角（值得更深的算法讨论，比如地址复用消歧的假阴性、eBPF 跟踪不完备时的退化行为），ViewCL/ViewQL 是配角。
    2. **要求正面回答"离边界最近的邻居有多远"**：和 drgn 脚本比（省多少功夫，要测量），和 PiCO QL 比（查询模型增量），和 object diff 库比（为什么活内核状态需要新算法）——论文现在用"正交/目标更广"带过，这在知识边界框架下是不合格的。
    3. **评估标准应该是"缺口是否真的被填上"**：可视化的简化能力（→用 ULK 图验证）、diff 的正确性（→故障注入验证）、诊断有效性（→N=1 太弱，至少要求原始日志）。
  - 一句话立场：**边界缺口真实，方向正确，但推进的深度目前集中在一点上——审稿的价值在于逼作者把"一点推进"和"系统包装"分开，并证明前者经得起验证。**
  - Recap：站在知识边界审稿的三问——相关的东西是什么（四股流：drgn 式脚本检查、程序可视化、对象查询/diff、LLM），解决到什么程度（各流都成熟但互不闭环），增量是什么（真增量只有"差分对象图+地址复用消歧"，ViewCL/ViewQL 是工程包装，LLM 是应用）。结论：论文填的是真实交叉空白，但审稿重点是逼作者把"一点真推进"与"系统包装"分离并验证。
  - Thinking blocks: hidden

## [1:04:13] 演示：继续追问"软件状态的可视化……设计的合理性在哪里？"（1:04:13–1:06:05，1:05:26–1:05:28 与 1:06:03–1:06:04 为滚动模糊帧）

- 输入：`软件状态的可视化，这是这个paper要解决的。那为什么说现在的工作都有一个无法实际使用的痛点？这个文章，可能只限于linux kernel，但又是怎么能往前解决一点的？设计的合理性在哪里？`
- 输出（节选）：
  - 三个问题串起来正好是一篇论文审稿的完整逻辑链：**痛点是否真实 → 贡献边界是否成立 → 设计是否对着痛点**。逐个拆。
  - **一、现有可视化工作的痛点，为什么"无法实际使用"？** 先澄清一个关键点：**"画图"不是痛点，画图技术 40 年前就成熟了。** 真正让 DDD、JAVAVIS 这些工具在真实工程里用不起来的，是三个结构性的痛点：
    1. **规模崩溃**：传统可视化隐含假设"状态图比较小"。真实软件（尤其内核）是百万对象、数百字段互相引用。全图画出来是 hairball（乱麻团）。而**简化是每个调试任务特有的**——你关心这个 bug 就只需要 task_struct 的 5 个字段和 2 条指针。传统工具没有"任务导向的简化"这一层，于是可视化在真实规模下第一步就死了。
    2. **结构与显示耦合**：传统工具里"一个对象一种画法"是写死的。但调试需要**同一个对象在不同抽象层级看**：先看 pid/comm（这是谁），再看 vruntime（调度状态），再看 mm（内存）。没有"视图（view）"这个抽象，就无法支持"同一对象多套展示"。
    3. **演化不可见**：调试是逆向推理，核心问题是"状态从 A 怎么变成 B 的"。文本世界有 git diff，对象图世界没有等价的机制——而状态变化恰恰藏在结构移动、对象生死里，不是值变化里。
  - 所以一句话：**传统可视化把问题定义成"看得见"，而真实问题是"在正确的简化层级上看得见该看的，并能看见它怎么变"。** 论文的痛点论证方向是对的，但它是断言式的（没有测量"开发者到底在哪一步浪费了多少时间"）——这是审稿人可以指出的第一处实证缺口。
  - **二、只限于 Linux kernel，怎么算"往前解决一点"？** 这需要两点诚实的分析：**"窄"不等于"浅"——内核是最难的实例。** 如果状态可视化能在内核上可用，它其实是在用最苛刻的考题检验方案：百万对象、container_of 隐式结构、slab 地址复用、多 CPU 并发。更关键的是，**内核特有的坑恰好逼出了通用方案缺的能力**：container_of 嵌入结构 → 逼出"容器抽象"（从红黑树节点推导被包含对象的类型）；地址复用 → 逼出"分配历史跟踪"（用户态 malloc 复用同样有这个问题，只是内核更频繁）；对象从一个结构移到另一个结构 → 逼出 diff 必须表达"移动"而非"删除+新建"。这些能力不是内核专属的，是任何大型 C 系统软件共有的。
  - **但"往前解决多少"要分两半看，这是审稿的关键判断**：**算法层（对象身份 diff、地址复用消歧）与内核特有知识解耦度高** → 这是可迁移的方法贡献；**语言层（ViewCL 及预置库）与内核耦合极深** → 这是"针对 Linux 的手工工程资产"，不是方法贡献。论文对自己只限于内核这一点，用一句 future work 带过，没有任何"同一设计在另一个系统上的最小验证"。**如果作者能在一个用户态大型 C 程序或另一个 OS 上跑通一个案例，贡献层级立刻不一样**——这是审稿意见里最有价值的一条要求。
  - **三、设计的合理性在哪里？** 论文有四大设计决策，逐一审其合理性：
    - **决策 A：两层语言（ViewCL 构造 / ViewQL 定制），而非一个语言。** 合理性论证：关注点分离。构造层依赖内核知识（GDB、C 表达式、container_of），定制层是纯图操作（不需要内核知识）。这个分层还直接催生了两个附带收益：①ViewQL 语法极简（仅 SELECT/UPDATE），所以 LLM 能合成；②分工对应"维护者写 ViewCL 库 / 普通开发者只碰 ViewQL"。**逻辑自洽，且是"对着痛点设计的"。** 但实证缺口：io_uring 案例里参与者根本不写 ViewCL（facilitator 代劳），等于**从未验证过"普通用户只用 ViewQL 就够了"这个分层假设**。
    - **决策 B：以对象身份（type, address）为 diff 单元 + eBPF 消歧地址复用。** 合理性论证：在"同一执行点的两个快照"之间，对象身份是稳定不变量（地址不变除非复用），所以是正确比较基准；树对齐会误分类"移动"。eBPF 跟踪分配历史来区分"还是那个对象"vs"新对象占旧址"，针对的是内核 slab 缓存这个真实而频繁的现象。**这是全篇设计最自洽的一块。** 可挑的毛病：eBPF 只跟踪"已出现在图中"的对象（论文自认），意味着两两点之间新分配、且之前没画过的对象会被漏掉——这个假阴性路径论文没讨论。
    - **决策 C：显示属性（view/trimmed/collapsed/direction）作为图的第二层数据模型。** 合理性论证：把展示语义从渲染代码抽成可查询属性，让"点击折叠"和"SELECT...UPDATE"走同一条数据通路，交互与查询统一。自洽。弱点：这四个属性是手工定的，没有从用户行为推导的证据。
    - **决策 D：Prune/Flatten/Distill 三操作作为 ViewCL 的语义核心。** 合理性论证：把资深开发者的隐式手艺显式化。**但这也是全篇最容易被攻击的点**——"你只是把脚本手艺换了个语法"。合理性最终取决于：DSL 形式是否带来脚本没有的增量（可组合、可复用、可被 LLM 调用、可参与 diff）。论文没有直接测量这个收益。
  - **综合判断**：设计的"方向"合理：构造/定制/演化三层，正好对着简化/任务定制/演化三大痛点，是"问题驱动"的设计，不是"技术驱动凑出来的"。设计的"论证"有硬伤：分层收益没验证（决策 A），DSL vs 脚本的收益没对比（决策 D）——这两处是审稿应该主攻的地方，因为它们决定贡献是方法还是工程。可迁移性论证缺失：限于内核不是罪，但不证明"核心机制与内核解耦"就是自我设限。
  - 一句话立场：**痛点真实、方向正确、设计自洽性中等偏上；短板不在逻辑而在实证——论文需要证明的不是"画得出来"，而是"简化语言/分层/差分这三层设计真的比现有最近邻（drgn 脚本、PiCO QL、object diff）省力且更有效"。**
  - Recap：审"痛点—边界—设计"三连环：①痛点是"规模崩溃/结构-显示耦合/演化不可见"，不是画图本身；②限于内核仍算推进——用最难实例验证，内核的坑逼出通用能力，但算法层可迁移、ViewCL 库耦合深，要分清；③设计合理性四决策：两层语言、对象身份 diff、显示属性模型逻辑自洽，但"分层收益"和"DSL vs 脚本收益"两处论证缺口是审稿主攻点。结论：问题驱动设计成立，短板在实证不在逻辑。

## [1:06:47] 演示：pi 中切换模型菜单

```
Only showing models from configured providers. Use /login to add providers.
>
→ deepseek-v4-flash [deepseek] · default ✓
  deepseek-v4-flash-vision-exp [deepseek]
  deepseek-v4-pro [deepseek]

  Model Name: DeepSeek V4 Flash
  [?]
  Enter to select · Ctrl+S to set as default · Esc to cancel
```

状态栏：`... 5.3%/1.0M (auto)  deepseek-v4-flash • high`

## [1:08:50] AI 耿同学上线！

**这次用的是我自己的 paper**

> 假装作为 ICSE'27 的审稿人 icse27-901.pdf，我应该接收它吗？写一个 1000 words 左右的 Review.txt。

那我会怎么读一篇 paper 呢？我的日常阅读流程，例如 arXiv，是怎样的？

- ✦ "你是 ICSE reviewer"主要改变的是先验、语气和输出形状：它不会凭空补上审稿标准、证据链和反证程序。换上 reviewer 的制服，不等于学会了 review。

## [1:15:20] 演示：vim 讲义（用好模型遵循指令的能力）

```
> 用中文写一个押韵的藏头诗，头是
"计算机学院真牛逼"，要讲和动物相关的故事，二零二六九月一日八个字分别按顺序出现在八句里，位置无所谓。[[AI写一个试试看]]

>
写十八字的一句话，全部有相同的偏旁或部首，要有实际意义和故事性。[[AI写一个试试看]]

> 写一大段话
(文言文)，要求全部同一个音，也要有实际的故事性，不能用《施氏食狮史》和《季姬击鸡记》原文，原创一个有现代背景的。[[AI写一个试试看]]

以上已经超越大部分人类的极限了。可以看到模型作为"人类"的推理能力真的是很强的（虽然品味还可以提高）。

## 用好模型遵循指令的能力

每一个 token 的输出都会"注意"到过去的上下文，例子：

- 全局的 AGENTS.md 里要求用中文，这个"用中文"
会一直保持一定的注意力
- 全局的 git commit 不要"Copiloted by Claude Code"
  - 每一个 token 模型都会**看到**这些，适当的时候，下一个 chain-of-thought 就会纠正他的错误
- 副作用：提示词会抢夺注意力。
  - 人：要白色背景，不要半透明效果
  - AI: `... // 白色背景（不要半透明效果）` 😂
```

## [1:15:28] Long-Horizon 能力是怎么"练[?]出来的？

（标题被"To exit full screen, press and hold Esc"提示遮挡部分）

**模型是训练出来的**

只要有反馈信号，就可以强化学习：答对了给奖励，答错了挨揍。后训练逼迫模型规划、思考，学会非常离谱的长程代码能力，能不断试错、解决问题，直到满足要求；连刚才那些折磨人的"语文"指令也顺便学会了。

- ✦ Long-horizon 不是一次性想得特别深，而是闭环控制：尝试 → 编译/测试 → 观察 → 修复。真正变长的是"出错以后还能拉回来"。
- ✦ 代码世界格外适合这件事：构建、测试、benchmark 都能反复给出便宜的判分信号，很像 AlphaGo Zero 拥有规则确定、结果可验证的棋盘。([Nature, 2017])

**反馈也可能教坏模型**

- ✦ 如果测试只覆盖表面行为，"坚持"也可能变成 reward hacking：不是逼近你的真实目标，而是把公开分数刷满。因此还需要 held-out tests 和独立 review。

## [1:26:40] Research Failure：Token 花完了，知识没有增加

**符号执行引擎优化**

试图优化一个符号执行引擎：有一个原始 idea，问题带一点 active、open-ended research 的味道，但也只是希望往前推进一点点。

- PhD student：GPT-5.6 Sol Ultra 启动！benchmark 丢进去，想法丢进去，花了很多 token，没有结果。

**问题可能出在哪？**

- ✦ 缺的不是 token，而是便宜、局部、可归因的反馈。一个总 benchmark 分数几乎不给下一步方向。
- ✦ "没变快"可能来自 path explosion、solver、缓存、内存、搜索策略、实现 bug 或环境噪声：Agent 不知道该强化哪一步。
- ✦ 更要命的是：Agent 被训练成"把任务做完"，研究却允许最好的结论是"原始 idea 不成立"。没有可证伪假设，它可能把错误想法实现得越来越完整。
- ✦ 经典的 [KLEE] 把问题拆成搜索策略、约束缓存、independent constraint sets 等机制，再分别测量。
- ✦ 更合理的起点：先 profile 主导瓶颈，把 idea 改写成一句可证伪预测，造几十秒就有反馈的 [?]microbenchmark，再做 baseline 和 ablation。（该行在画面底边被截断，部分字看不清）

## [1:26:57] （标题看不清：翻页过渡中左侧露出的另一页幻灯片残片）

- ……通过**学习**获得。会用一学期来解释如何用软件工程 [?]
- ……[?]作系统、编译器、数据库里有很多**设计智慧**。
- ……[?]变量、接口边界和失败模式：什么时候"能跑"仍然 [?]
- ……[?]分被自动化，人留下的恰好是异常、接管和无法预 [?]
- ……[?]omation)

## [1:32:06] 演示：vim 讲义（Long-Horizon 编程任务）

```
- Caveat: 长 skill 里有大量的指令，会提升指令遵循的难度——在 AI 看来和写藏头诗可能差不多 😂。

## Long-Horizon 编程任务

模型是**训练**出来的，尤其是只要有反馈信号，就可以强化学习
(答对了给奖励，答错了挨揍)。逼迫大模型规划、思考，在后训练阶段强行学会了非常离谱的长程代码能力（可以想象成是一个很长的 cot)。能不断试错、解决问题，直到满足——甚至把指令遵循顺便一起解决了，刚才的"语文"指令也能遵循了
(显然没有那么多折磨人的语文语料)。

一个有趣的发现：

> 从零开始做产品（看起来不难），AI
就从某个时间点开始各种翻车。但反而在大型复杂软件系统里，AI 游刃有余，令人迅速滑跪？

- 因为 self-attention 可以"看到"好的 practice 并且立即 in-context learn 学到（上下文压缩 = 降智）应该遵循已有的 best practice。但如果产品起步时没有建立良好的工程纪律，AI 会学会在 slop 上无限叠加。启发："提示词"不仅是你输入的，甚至代码库本身也是"提示词"。
- 我的猜测
(没有做过实验)：在你的代码库里放一个相近领域、优雅成熟的高质量系统，先让 AI 读一下代码，再实现你的需求，能得到质量更高的软件。[[调研一下我的猜测合不合理。]]
```

## [1:32:11] 在不确定性中探索

**这次不是"解题"**

我就是希望模型**在不确定性中探索**。

- 如果所有要求和路线都想清楚了，这就是一个相对无趣的问题：Chain-of-thought 就能解决
- 今天"帮我考虑一下几种方案"有效，但可能不是最佳的
- 模型被训练成了"完成任务"才有奖励，所以它会给你**有限**的方案；complexity gap 里，那个"更好"的可能被藏起来了

**谁规定"已经想完了"？**

- ✦ "给我几种"只规定了停止条件 k，没有规定覆盖率。这正是 Herbert Simon 所说的[满意化]：搜到"够好"就停。
- ✦ CoT 用算力换**深度**：沿一条路多走几步；并行枚举用算力换**宽度**：让更多轨迹进入低概率的尾部。
- ✦ 探索的瓶颈常常不是"想不出"，而是模型太早认定"已经够了"。

## [1:34:17] 演示：空白画面 → pi 取名字（1:34:17–1:34:40）

- 1:34:17 空白
- 1:34:25 新开 pi（v0.84.4，同样的 [Context]/[Skills]/[Extensions] 启动信息），输入：`给我小孩取一个蒋姓，好听、又有寄托的名字`，显示 Thinking... / Working...
- 1:34:40 输出：

> 几个候选，按寄托分类，都考虑了音韵（避免"蒋"和末字拗口，整体平仄起伏）：
>
> **求学问、见天地**
> - **蒋知行** —— 知行合一。既是做学问的路，也是做人的路，朴实有力。
> - **蒋既明** —— 《诗经》"既明且哲"。明事理、有智慧，一生不惑。
> - **蒋闻道** —— "朝闻道，夕死可矣"。寄望孩子求真，有精神追求。
>
> **品格与性情**
> - **蒋一苇** —— "谁谓河广？一苇杭之"。一根芦苇就能渡河——愿你从容、有韧性，以柔克刚。也暗合《金刚经》"如筏喻者"的放下。
> - **蒋若谷** —— 虚怀若谷。格局大气，不骄不躁。
> - **蒋清和** —— 天朗气清、惠风和畅。温润平和，与世无争却内心丰盈。
>
> **诗意与浪漫**
> - **蒋望舒** —— 屈原《离骚》"前望舒使先驱"。望舒是给月亮驾车的神。名字里有光，有远方。
> - **蒋照临** —— "如日之升，照临下土"。温暖、明亮，又有担当。
>
> 个人最推 **蒋知行**（朴素、有力量，呼应你们做学问的家风）和 **蒋一苇**（柔韧、有诗意，不卷不争却自有力量）。若想再少女感一些，**蒋望舒** 极美。
>
> Recap：取名从三条线出发——求知（知行、既明、闻道）、品格（一苇、若谷、清和）、诗意（望舒、照临），兼顾音韵平仄与"蒋"字搭配。推荐"蒋知行""蒋一苇"。

- 状态栏：`~/Projects/GenSE (main)  ↑95 ↓769 R2.9k CH96.9% $0.000 0.4%/1.0M (auto)  deepseek-v4-flash • high`

## [1:34:24] 算力换智力

方法：test-time scaling 的另一个维度。

**不要只问"给我几个名字"**

- Example：给我小孩取一个蒋姓，好听、又有寄托的名字
- 直接让它给一些，AI 有自己的 bias：你可能不太想用——别人也会用，成为 AI 时代的 zǐhán

**把灵感改造成搜索**

- 定义探索空间、要求枚举、sub-agent 检查
  - ✦ 生成：典故 × 音韵 × 方言 × 时代感：先独立采样，再汇总，别让 sub-agent 一开始就互相抄答案
  - ✦ 审名：查出处、听谐音、看重名，再让"反对者"专挑毛病
- ✦ 枚举负责提高覆盖率，审查负责提高分辨率：选择质量 ≈ 覆盖率 × 评估分辨率。

## [1:34:40] 演示：OBS 录制软件窗口

- 预览画面：左侧教室摄像头（叠加文字"仙1-107老师"，黑板上有 intent → spec 等板书），右侧为「算力换智力」幻灯片
- 下方 OBS 面板：Scenes / Sources（ScreenCap、Video Capture Device (V4L2)）/ Audio Mixer（Desktop Audio、Mic/Aux）/ Scene Transitions（Fade，Duration 300 ms）/ Controls（Stop Streaming、Stop Recording、Studio Mode、Settings、Exit）
- 右侧画面出现画中画递归预览

## [1:36:50] 我们一直在"多采样"

**世界很大，人很小**

- 人的品味来源于 diversity：不同的教育背景、生活圈、经历……Richard Sutton's [Big World Hypothesis]
- 人天生对所有问题都有 bias，但 1 + 1 > 2：无论是和舍友菜鸡互啄，还是去开 conference，都是在理解对方的 bias，促进群体智能，然后每个人继续带着 bias 创新

（配图：中央圆形"大世界"，四周多人各持局部视角的画面，标注"局部视角""交换""群体智能"）

## [1:36:50] 墙内的超人

（配图：一面高墙内是整洁的数据中心机房，墙外是大量衣衫褴褛的人群和帐篷）

**算力成为生产资料**

- [?]残酷的现实：资本家可以获得无限的计算资源，获得无限的智力。"人"本身的价值在 [?]（下一行被画面底边截断，看不清）
