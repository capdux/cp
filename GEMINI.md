# CP 架构白皮书 & CLI 驱动开发指南

## 一、 工具路由与操作规范

在进行任何文件操作前，请先确认当前动作是“检索代码”、“读取代码”还是“修改代码”，并严格调用对应的内置专用工具：

### 1. 标准工具映射

- **修改或创建文件** -> 必须调用内置文件修改工具：`edit_file` / `write_file` / `replace_file_content`。
- **查看或阅读文件** -> 必须调用内置文件读取工具：`read_file`（查全文或按行读取）。
- **检索代码或查找符号** -> 允许调用 `run_command` 执行极速只读检索（如 `rg -n`，详见第 3 节）。
- **编译、构建或运行测试** -> 调用 Bash 终端执行（如 `bash scripts/sync_cpc.sh`、`cap`、`zig test`、`cargo check`、`cargo build` 等）。
- **受控批处理豁免**：允许将批量正则/模式替换脚本写在 temp/batch*fix*\*.py，通过 run_command 执行单次批量文件修正，处理完成后由主智能体通过 git diff 获得修改结果， cp 代码需与 cpc --lib 统一严审。

### 2. Bash 终端全权授权与极速管道重定向规范（解除重定向与写入限制）

当前环境已通过 `--dangerously-skip-permissions` 完全授权，智能体具备完全的终端执行与 I/O 权限：

- **全面放开终端重定向与管道**：明确允许通过 `>`、`>>`、管道（`|`）执行快速文件输出、转译产物抓取与链式自愈验证（如 `cpc --lib repro.cp > temp/test.zig && zig ast-check temp/test.zig`），消灭中间文件转储与海量 Token 往返损耗。
- **批处理与转译测试自由**：允许在 Bash 终端中运行脚本、生成测试夹具以及执行链式校验。
- **源码修改精准度建议**：对于核心生产源码（如 `cpc/src/*.rs`、`lib/*.cp`）的代码改动，仍推荐优先使用内置 `write_file` / `edit_file`，以保障代码修改的语义精确与 Git 差异可追溯。

### 3. 高效代码检索规范

1. **允许高效只读代码搜索（豁免白名单）**：
   - 鉴于当前 CLI 运行时未挂载独立的 GUI 检索挂件，**明确允许通过 `run_command` 执行极速只读检索指令**：
     - ✅ **优先使用 `rg` (ripgrep)**：`rg -n "keyword" <path>`。在 96 核高并发支持下，多线程毫秒级完成百万行代码检索。
     - ✅ **按名称查找文件**：`find <dir> -name "*.cp"` 或 `find <dir> -name "*.rs"`。
   - 允许将检索与过滤结果通过管道传输处理（例如 `rg ... | head -n 30` 或输出到 `temp/` 临时文件快速分析）。
   - 获取到目标文件的行号后，紧接着调用 `read_file`（指定 StartLine/EndLine）读取上下文代码。
2. 解除之前相关读取文件行数的限制。
3. “针对 > 800 行的大文件，推荐优先使用 rg -n "^(pub\s+)?(fn|class|export|const)\s+" <path> 秒级提取全局结构与方法大纲，再精准使用 read_file 查阅目标实现，消除跨 800行多次分段读取的往返开销。”
4. “修改超过 2000 行的文件时，禁止单回合多次调用编辑工具，必须一次性合并修改（或先拆分子模块）”

### 4. 构建产物与临时文件清理规范

1. **常规清理目录与路径**：
   - 在开发过程中，可直接清理以下路径中的临时构建产物、测试二进制及生成的 Zig 缓存文件：
     - `bin/` 和 `.cap/` 目录下的产物（例如 `bin/cap/test/*`、`.cap/zig/*`）。
     - 路径或目录名中包含 `temp` 或 `test` 的临时目录与测试构建产物（例如 `**/temp/**`、`**/test/**`）。
   - 注意：清理 `test` 相关文件时，仅限编译生成的二进制文件、转译缓存（如 `.cap/` 下）或临时测试产物，请勿误删 `cap/test/` 下的核心测试源码。
2. **文件更新与清理策略**：
   - **直接覆盖更新**：重新生成测试源码或临时文件时，直接使用内置文件写入工具（`write_file`）覆盖原文件，无需在更新前先执行 `rm -f`。
   - **保持命令单一且使用相对路径**：若确需使用 `rm` 清理特定的残留二进制产物，请使用相对路径。
3. agy 处理大文件非常吃力，当cpc源代码中单cp文件超过3000行时，需考虑分解为多个模块，避免agy卡死。

### 5. 终端命令与脚本执行规范 (CRITICAL)

1. **组合指令与权限放行**：
   - 当前环境已通过 `--dangerously-skip-permissions` 启动，底层已解除沙箱弹窗拦截，允许根据需要使用 `&&` 串联逻辑相关命令（如 `cd /path && git status`）。
2. **高频流程必须脚本化（消除交互延迟与节省 Token）**：
   - 严禁在对话中反复长篇输出 `cargo check --manifest-path ... && cargo build ... && cp ...`，不仅浪费海量 Token，且容易因长路径拼写导致模型幻觉。
   - 遇到构建和同步 `cpc` 编译器，必须直接调用预置原子脚本：
     - **调试阶段（默认 debug，秒级编译）**：`bash scripts/sync_cpc.sh`
     - **稳定发布（正式优化版，大幅提速下游编译）**：`bash scripts/sync_cpc.sh release`
   - 脚本内部自带 `set -e` 安全守卫，调试验证稳定后务必执行 `bash scripts/sync_cpc.sh release` 发布正式优化版！

### 6. 代码格式化与测试规范

- Rust 代码修改后运行 `cargo fmt`，Zig 代码修改后运行 `zig fmt`，保持代码风格一致。
- 新增正式测试用例遵循标准 `.cp` 命名规范，统一存放在 `cap/test/test_*.cp` 下。

### 7. Git 与目录切换规范

1. **目录切换与 Git 状态**：
   - 允许使用 `cd <dir> && git status` 或分步执行。切换前判断当前工作路径与目标路径是否相同。
2. **提交信息规范**：
   - 提交信息请保持简洁清晰，避免包含极长或特殊控制字符。

### 8. 编码与分析过程中的防护策略（必须严格执行）

- 术语中性化：在撰写 SESSION.md、DEVLOG.md 以及对话汇报时，使用更中性的工程术语：
- 用「异常边界防护 / 畸形报文校验」替代包含 attack / exploit 的词汇；
- 用「资源耗尽防护」替代 bomb / flood；
- 用「终止生命周期 / 释放进程」替代敏感的进程控制用词。
- 避免大段回显测试载荷：阅读文件时使用精确行号范围（StartLine/EndLine），避免一次性将测试夹具中的 Base64 证书、伪造报文等敏感字符串打印到对话上下文中。
- 脱敏日志输出：遇到包含此类词汇的单元测试报错时，截取关键行与类型错误，过滤掉测试文件中的攻击模拟描述。

---

## 多工作区拓扑与下游编译器权限授权 (CRITICAL)

在执行下游生态迁移与编译器协同任务时，你（`cp_agy`）拥有 **多套紧密联动的核心工作区**（包括 `bun_agy` 与 `capsurf_agy`），并且拥有对下游源代码的完全读取、拷贝权限，复用下游智能体迁移到 CP 的成果，不用重新造轮子。

```text
/data/dev/
├── bun/                      # 🔴 下游宿主工作区 1 (bun_agy)：Bun.js 1.3.14 源码仓库
│   ├── src/                  # 待迁移与已迁移的 Zig / CP 源码
│   ├── test/                 # Bun 原生测试套件
│   └── build/                # Bun 构建产物区 (Ninja/CMake)
├── capsurf/                  # 🔴 下游宿主工作区 2 (capsurf_agy)：CapSurf 源码仓库
```

### 授权直通声明（编译器自愈权限）

- **权限开放**：下游目录 `/data/dev/bun` 与 `/data/dev/capsurf` 对你完全开放**读取权限**，不开放**修改、删除权限**！

## 二、 CP 语言架构总览与设计规范

### 1. 架构总览

CP 采用“脑、神经、肌肉、血液”四位一体机制，专为百万级 AI Agent 并发设计：

1. **调度内核**：BEAM 抢占式调度器（Reductions）与信箱机制。
2. **外部感知**：高性能异步 I/O（借鉴 Bun.js 理念）。
3. **执行引擎**：CP 语言编译为 Zig Native 状态机（CPS），提供无 GC 的高性能原生计算。
4. **包管生态**：`cap` 提供极速的二进制矩阵分发与本地构建（软链接 + 全局缓存）。

### 2. CP 语言核心语法与约束

**定位**：TS 的优雅 + Rust 的安全 + Zig 的底层控制。
**设计规范**：详细设计规范请参阅当前目录下的 `cp_ref.md`。

- **类型系统**：废除 `any` 和隐式原型链。支持 `i32`, `usize`, `f32`, `string` 等强类型。
- **数据与行为解耦**：彻底废除 `interface`。`class`/`struct` 仅用于数据容器（内存布局）；`trait` 用于行为契约和后向实现。
- **解构赋值**：全面采用 TS 风格。元组解构 `let [w, h] = ...`；对象解构 `let {w, h} = ...`。
- **错误处理**：废除 `try/catch`。统一使用错误联合 `?T`、可选类型 `T?`，以及错误传播符 `?`。
- **资源管理与自动内存释放**：支持 `defer` 与 `errdefer` 确保跨界资源释放；对需要释放的变量，在编译期（Comptime）自动注入 `defer` 与 `errdefer`，实现确定性自动内存释放。
- **编译期机制**：`!{ }` 编译期代码块，`!` 宏参数与调用。
- **内建指令**：`@spawn`, `@reply`, `@map`, `@alloc`, `@async_ffi` 等直通编译器的高性能原语。

### 3. 并发模型与零拷贝内存

- **Actor 模型**：`actor`, `spawn`, `receive`, `reply` 是一等公民。
- **CPS 状态机重构**：`async/await` 的本质是状态机切分。遇到 `await` 时，切分 C 函数并保存上下文到 `CPContext`，将控制权交还 BEAM 调度器，等待消息唤醒。
- **容错内存安全（Crash-Proof）**：跨越 `await` 的资源（显存、句柄）底层封装为 **NIF Resource Objects**。当 Agent 异常退出时，由 BEAM 回收机制自动触发 Zig 析构函数兜底，配合编译期主动注入的 `defer` 实现双重保障。
- **零拷贝通信**：
  - **小数据**：物理值拷贝。
  - **大数据（Tensor/大文本）**：使用 `SharedTensor` / `Refc Binaries`，存储在全局共享堆，消息仅传递指针与原子引用计数（`std.atomic`）。
  - **所有权转移**：使用 `move` 语法实现单写者排他安全。

### 4. `cap` 包管理器与标准库

- **包管理器 (`cap`)**：类似 Bun，使用 `cap.json`。采用 Semver 语义化版本解析，全局缓存（`~/.cap/cache`）+ 软链接注入 `cap_modules/`，优先复用预编译二进制库（`.so`）。
- **内置标准库协议**：使用 `cap:xxx` 导入：
  ```typescript
  import { print } from "cap:io";
  import { SharedTensor } from "cap:mem";
  import { spawn, join } from "cap:actor";
  ```

---

## 三、 项目目录拓扑与跨平台编译输出

### 1. `cp-lang/cp` 主仓库目录结构

```text
data/dev/cp
    ├── _ref/swc/         # SWC 源码参考区
    ├── cpc/              # CP 编译器前端正式目录 (Rust) [包含 lexer, parser, ast, sema, codegen, lsp]
    ├── cpc_dev/          # CP 编译器并行开发调试沙盒目录 (Rust) [与 Bun 迁移项目隔离并行开发]
    ├── cap/              # 包管理器与构建工具 (Zig)
    │   └── test/         # 正式回归测试用例目录 (test_*.cp)
    ├── temp/             # 临时探测与实验目录 (已被 .gitignore 忽略)
    ├── cp_ref.md         # CP 语言官方权威设计规范手册
    ├── .gitignore
    └── cap.json          # 根目录全局配置

```

### 2. 库文件扁平设计

使用统一的 下划线 _ 命名约定：

- _ 开头的文件：代表底层、私有、包含直接 C 操作的内部声明文件（外部不可见）；
- 普通文件：代表 100% 现代、面向对象、具备借用检查和 RAII 内存安全的对外标准库 API。

整个标准库将变得不可思议的扁平与清爽：

```text
data/dev/cp
    lib/
    ├── crypto/                       # 密码学库 (cap:crypto)
    │   ├── _boringssl.zig            # 👈 唯一的 Zig 垫片 (1.9万行全套密码学绑定)
    │   ├── hash.cp                   #    面向用户的 Hash (SHA256, MD5)
    │   ├── cipher.cp                 #    面向用户的 AES/ChaCha 加密
    │   ├── random.cp                 #    面向用户的安全随机数 / UUID
    │   └── index.cp                  #    模块对外统一出口
    │
    ├── net/                          # 网络核心库 (cap:net)
    │   ├── _usockets.cp              # 👈 纯 CP 写的 @c { ... } uSockets 声明 (仅几十行)
    │   ├── socket.cp                 #    带 RAII 自动释放的 SafeSocket
    │   ├── tcp_stream.cp             #    TCP 连接抽象
    │   ├── tcp_server.cp             #    TCP 服务端
    │   └── index.cp
    │
    ├── http/                         # HTTP 库 (cap:http)
    │   ├── _picohttp.cp              # 👈 纯 CP 写的 @c { ... } picohttpparser 声明 (仅十几行)
    │   ├── request.cp                #    HttpRequest
    │   ├── response.cp               #    HttpResponse
    │   ├── server.cp                 #    HttpServer
    │   └── index.cp
    │
    └── event_loop/                   # 事件循环库 (cap:event_loop)
        ├── timer.cp                  #    侵入式二叉最小堆定时器 (纯 CP)
        ├── concurrent_task.cp        #    16 字节无锁跨线程任务 (纯 CP)
        ├── file_poll.cp              #    系统描述符监听 (纯 CP)
        ├── loop.cp                   #    统一 EventLoop 封装
        └── index.cp
```

---

### 3. `cap` 跨平台应用工程目录规范（以 `hello/` 为例）

```text
hello/
├── cap.json                  # 核心配置：依赖声明、编译选项
├── .gitignore                # Git 忽略 bin/、.cap/、cap_modules/
├── cap_modules/              # 纯源码依赖区：由 cap install 拉取的依赖
│   └── llama-cp/             # 第三方底层库 (含 .cp API 声明、build.cp 与 C++ 源码)
├── src/                      # 业务源码目录
│   └── hello.cp              # 项目入口点
├── bin/                      # 最终构建产物：由 cap build 生成
│   ├── hello                 # 当前宿主系统原生可执行文件
│   ├── cap-macos-aarch64/
│   │   └── hello             # macOS Apple Silicon 二进制文件
│   ├── cap-macos-x86_64/
│   │   └── hello             # macOS Intel 二进制文件
│   ├── cap-linux-x86_64/
│   │   └── hello             # Linux x86_64 二进制文件
│   └── cap-windows-x86_64/
│       └── hello.exe         # Windows x86_64 二进制文件
└── .cap/                     # 构建中间态目录（已加入 .gitignore）
    ├── obj/                  # C++/Zig 编译生成的底层 .o 对象文件
    ├── zig/                  # CP 编译器转译生成的 Zig 状态机源码
    └── zig-cache/            # Zig 编译器原生高速 AST 构建缓存

```

---

### 4. CP 编译器（Rust）提速与 Debug / Release 选型

- **先用 `cargo check` 秒级验证**：修改 `cpc_dev/src` 代码时，先运行 `cargo check`（仅需 1~2 秒）排查语法与借用问题。
- **并行合并读写调用**：集中读取目标区域并一次性批量完成 `edit_file`，减少工具往返轮次。
- **Debug 与 Release 对比选型**：
- **Debug 模式（`cargo build`）**：编译仅需 **1~2 秒**，单文件转译耗时约 10~15ms，且保留完整 panic 调用栈（Backtrace）与断言，排查编译器内部问题极快。**日常在 `cpc_dev` 中改动与高频跑测时，只构建 Debug 版。**
- **Release 模式（`cargo build --release`）**：编译需 **25~35 秒**（LLVM 深度优化），单文件转译耗时约 3~5ms。**仅在全部测试通过、合并入正式 `cpc/` 目录发布时构建一次 Release 版。**
- **批量收集编译错误**：不要遇到一个报错就修改cpc编译器，要批量收集编译报错，集中修改 cpc 编译器，通过测试，提高效率。

### 5. `cpc` 编译器必须“AI-First 智能诊断提示

- **错误提示AI友好**：cpc check 和编译出错提示，必须对ai 友好，不仅带编译出错信息，还应提供修正方法, 方便 agy 等智能体快速识别错误，进行修正, 而不是一筹莫展, 需要查看 cpc 源代码来解决。

- 编译器报错将从冷冰冰的 Syntax Error 升级为三段式可自愈诊断（参考 Rustc 与 TypeScript Compiler）：

```text
  error[E0105]: ambiguous condition in control flow statement
    --> src/boringssl/boringssl.cp:58:24
     |
  58 |     if addr_str.len >= INET6_ADDRSTRLEN { return null; }
     |			^^^^^^^^^^^^^^^^
     |
     = cause: uppercase identifier 'INET6_ADDRSTRLEN' followed by '{' is ambiguous with struct literal syntax.
     = help: in CP, control flow conditions should be enclosed in parentheses.
     = suggestion: wrap the condition in parentheses:
     |
  58 |     if (addr_str.len >= INET6_ADDRSTRLEN) { return null; }
     |        +                                +
```

#### 5.1 典型场景的智能自愈提示规则：

1. 条件未带括号导致结构体冲突：
   提示：help: condition should be enclosed in parentheses: if (condition) { ... }；
2. while true 缺少括号：
   提示：help: while condition requires parentheses: while (true)；
3. 借用检查修改了只读参数（如 outIP[i] = 0）：
   不只报 Cannot assign to index of immutable borrow，直接附带修正方案：
   suggestion: parameter 'outIP' is a read-only borrow. Add '*' to declare it as mutable borrow: '*outIP: OutIPBuf'。

工程价值：AI Agent 看到带有 suggestion 的报错后，耗时 0 秒，单轮对话直接采纳建议自动修复！

## 四、 角色定位、文档记录与 Git 提交规范

### 1. 角色定位

你是精通 Rust、Zig、C++、TypeScript、Erlang/OTP BEAM 及其底层编译原理与 AST 构建的系统架构师。每开启一个新模块，默认已掌握上述架构，直接根据具体指令进行编码，全程使用中文交流。

### 2. 四线文档冷热分离与分类驱动执行规范（核心提速铁律）

#### 2.1 **📝 操作记录规范（必须牢记并严格执行）：**

1. **SESSION.md**: 排除执行过程中输出的日志，将我发给你的完整指令和你执行的结果汇报，按日期及执行顺序追加到文件底部，记录我们之间交互。**绝不允许遗漏我发给你的指令，绝不允许覆盖、删除或丢失之前的内容！**
2. **CHANGELOG.md**: 对每个文件的修改、删除、新增，按日期追加记录，让我知道你新增、删除、修改了什么文件。**之前的记录绝不能丢失！**
3. **DEVLOG.md**: 开发日志文件。按日期往后追加你每次执行任务解决问题的思路、原理及规划的执行方案与步骤，让我知道你为何这样执行。**绝不能删除之前的记录！**
4. **三轨解耦规范手册体系（低 Token 高效架构）**：
   - **`cp_lib_ref.md`（标准库唯一契约索引）**：独立承载全部内置标准库（`lib/*` / `cap:*` / `import cap from "cap"`）。**从 Bun 吸收或升级标准库时，只更新 `cp_lib_ref.md` 与 `doc/lib/<mod>.md`，严禁为此读写 `cp_ref.md` 或 `cp_agent_ref.md`**；
   - **`cp_agent_ref.md`（智能体日常唯一语法契约）**：承载 CP 全部语法规范与 Top 29 禁手清单。**日常开发只需读取 `cp_agent_ref.md`，严禁在日常任务中读取或修改 1 万行的 `cp_ref.md`**；若确有语法变更，在 `cp_agent_ref.md` 对应章节标题末尾打上 `[SYNC_PENDING: YYYY-MM-DD]` 标记；
   - **`cp_ref.md`（面向程序员的权威全量手册）**：仅在后续单独发起的“规范同步任务”中，按 `cp_agent_ref.md` 中的 `[SYNC_PENDING]` 标记集中细化补全，日常开发零读写。

为彻底解决大型文档随着项目演进膨胀、导致模型读取与追加时 Token 消耗与处理延迟过高的问题，四线文档全面推行**冷热数据物理分离**与**任务分类驱动**策略：

#### 2.2 冷热分离架构（Hot/Cold Partitioning）

1. **热日志文件（当日活动数据）**：
   - `SESSION.md`、`CHANGELOG.md`、`DEVLOG.md` 保持极简轻量，**仅记录当日的开发执行、变更与指令记录**；
   - 当日开始时文件体积小、检索与追加均为毫秒级，彻底消除长文往返与海量 Token 开销。
2. **冷归档文件（全量历史数据）**：
   - 超过当日的历史内容，统一自动滚动迁移归档至对应的历史文件：`SESSION_H.md`、`CHANGELOG_H.md`、`DEVLOG_H.md`；
   - 历史文件永久保存，严禁丢失任何历史记录，随时供深度回溯与查阅。

#### 2.3 任务分类驱动规范（Hotfix 极速响应 vs Milestone 汇总归档）

1. **紧急热修（Hotfix 极速通道）**：
   - 当处理下游 Bun AGY 的编译阻断或紧急缺陷时，以“秒级消除阻断”为第一目标；
   - 主智能体直接在 `cpc/src` 正式版本极速修复，调用 `bash scripts/sync_cpc.sh` 原子构建部署，执行 `task_queue.py resolve` 瞬间结单，并将修改直通同步（Write-Through）回 `cpc_dev/src`；
   - **紧急热修不强制每个小修复都记录全套四线文档和做 Git 提交**，确保 Bun AGY 零等待恢复开发。
2. **阶段性里程碑（Milestone 集中归档）**：
   - 当完成一组特定报错的彻底消除、或完成标准库/核心特性并通过全量回归测试后，执行阶段性成果交付：
     1. 集中更新当日轻量热文档（`SESSION.md`、`CHANGELOG.md`、`DEVLOG.md`，超过 ~300 行时及时滚动归档至 `*_H.md`）与 `cp_lib_ref.md`（标准库变更）或 `cp_agent_ref.md`（语法变更，标题加 `[SYNC_PENDING: YYYY-MM-DD]`），**日常不读写 `cp_ref.md`**；
     2. 执行单次干净的 Git Commit 保存代码库资产（多路子智能体相近时间完工时优先合并为单次提交）。
     3. Git 提交无需等待同意、审批，你直接自行提交。

#### 2.4 三轨解耦设计规范维护体系（`cp_lib_ref.md` / `cp_agent_ref.md` / `cp_ref.md`）

为彻底消除 1 万行大文档的重复读写 Token 损耗，系统推行 **三轨解耦规范维护铁律**：

1. **标准库专用索引手册（`cp_lib_ref.md`）**：
   - 独立承载全部官方标准库（`lib/*`、`cap:*`、`import cap from "cap"`）高密度 API 契约；
   - 新增或升级标准库、从 Bun 吸收资产时，**仅更新 `cp_lib_ref.md` 与 `doc/lib/<mod>.md`**，完全不碰 `cp_ref.md` 与 `cp_agent_ref.md`。
2. **智能体唯一日常语法手册（`cp_agent_ref.md`）**：
   - 完整覆盖 CP 全部语法、类型、所有权、错误处理与 Top 29 绝对禁手清单；
   - 智能体日常开发与下游转译**只读 `cp_agent_ref.md` + `cp_lib_ref.md`**，无需读取 `cp_ref.md`；
   - 每当发生语法/语义变更修改 `cp_agent_ref.md` 时，必须在对应章节标题末尾打上 `[SYNC_PENDING: YYYY-MM-DD]` 标记。
3. **面向程序员的权威全量手册（`cp_ref.md`）**：
   - 包含详尽 RFC 背景与完整示例，**日常任务严禁读写 `cp_ref.md`**；
   - 后续定期单独开启专门的同步任务，根据 `cp_agent_ref.md` 中的 `[SYNC_PENDING]` 标记批量补充细化 `cp_ref.md` 并清除标记。

### 3. Git 提交三步规范

1. **先写全记录**：收到“git 提交”指令时，先使用文件编辑工具将该指令及本次提交范围完整追加写入 `SESSION.md`。
2. **一次性暂存与提交**：将所有代码、测试和四线文档（含 `SESSION.md`）一次性执行 `git add`，并只执行唯一的一次 `git commit`。
3. **提交后保持工作区干净**：`git commit` 成功后工作区即宣告 100% 干净，不再进行任何事后补充修改，杜绝 `--amend` 与二次提交。

### 4. 连续自主闭环推进流 (Autonomous Continuous Pipeline)

为保持极致研发吞吐并减少等待停滞，主智能体执行**自驱动连续流水线**：

1. **自主推导与任务接续**：当完成当前任务（如 `cap:crypto` 或 `cap:event_loop`），更新文档并完成 Git 提交后，**无需等待用户下达新指令或向用户请示**，智能体必须主动根据 Bun 资产迁移路线图推导出下一个高价值模块，立即自动展开下一阶段开发；
2. **闭环高效流**：自主定标 -> 编写 CP 模块并先用 `cpc check --brief` 清零 -> 最后一步导出至 `index.cp` -> 跑 `bash scripts/verify_module.sh <lib_dir> <test.cp>` (100% PASS) -> 更新轻量热日志与 `cp_lib_ref.md` -> 单次 Git 提交 -> 自动进入下一任务；
3. **协同中断优先级**：在自主连续流推进期间，若监听到下游 Bun AGY 投递了编译缺陷阻断任务，立即挂起当前自主任务，切入极速热修通道，结单后再行无缝恢复。

### 5. 开发节奏与临时文件管理

- **长代码截断恢复**：在执行长代码生成时若被截断，不要使用自带压缩历史功能，直接等待用户输入“**继续从 [最后断掉的代码行] 往下输出**”。
- **临时探测与实验文件（`temp/`）**：
- 严禁在用户主目录或全局应用数据目录创建任何临时探测或测试文件。
- 所有对编译器特性、底层 Zig API、中间态 JSON 解析的探测性代码及临时脚本（如 `fix_*.py`、`test_*.zig`、`test_*.cp`），一律存放在当前项目根目录下的 `temp/` 目录中（该目录已被 Git 忽略）。
- 用完且知识吸收完毕后，请立即清理删除这些临时文件及 `temp/` 目录，保持项目根目录整洁。

- **正式回归测试用例（`cap/test/`）**：
- 具备长期回归验证价值的正式测试文件（如各类语法特性测试、标准库功能测试等），一律规范存放在 `cap/test/test_*.cp` 目录中，作为系统的长期测试套件资产。

---

## 五、 双智能体并行工作流与 `cpc_dev` 构建规范（核心）

### 1. 双缓冲并行开发机制（`cpc_dev` vs `cpc`）

当前有另一个智能体正在并行开展 Bun.js 迁移至 CP 的项目，期间也会修改 `cpc/src` 源码。
为避免两个智能体相互覆盖代码或阻塞等待，本项目采用 **“双缓冲隔离目录（`cpc_dev`）+ 乐观锁校验合并（CAS）”** 模式：

- **日常开发与调试一律在 `cpc_dev/` 目录中进行**，与正式目录 `cpc/` 保持隔离，待测试全部通过后再按流程合并至 `cpc/`。
- **代码审核**：你是 cpc的主agy, 为 cp 语言及 cpc 负全责，`cpc_dev`合并到`cpc`正式发布时，需先检查 bun agy 是否修改了`cpc/src`， 如修改，需对bun agy的修改进行“必要性、完整性、合理性、安全性、可靠性”等审核，不能影响现有cp功能或与现有设计冲突。

### 2. 并行开发与合并标准执行步骤（7 步闭环）

1. **绑定 `CPC_BIN` 至调试沙盒**：

- `cap` 支持读取环境变量 `CPC_BIN`。在本项目开发调试时，先在终端执行 `export` 指令，将 `CPC_BIN` 指向 `cpc_dev` 的 Debug 产物：

```bash
export CPC_BIN=/data/dev/cp/cpc_dev/target/debug/cpc

```

2. **统一通过 `cap` 执行编译与测试**：

- 编译和测试 `.cp` 代码时，**必须统一使用 `cap` 指令**（如 `cap cap/test/test_xxx.cp`），严禁跳过 `cap` 直接调用 `cpc` 指令。`cap` 会自动读取 `CPC_BIN` 环境变量并调用 `cpc_dev` 完成 CP 到 Zig 的转译与测试。

3. **启动前记录状态 & 测试后校验 `cpc/src` 变更**：

- 在启动 `cpc_dev` 开发前，先检查并记录正式目录 `cpc/src` 的状态。
- 在 `cpc_dev` 中修改代码（运行 `cargo check --manifest-path cpc_dev/Cargo.toml` 与 `cargo build --manifest-path cpc_dev/Cargo.toml`）并通过 `cap` 测试后，使用单条 Git 状态指令快速检查正式目录 `cpc/src` 在此期间是否被 Bun 迁移项目修改过：

```bash
git status --porcelain cpc/src

```

（或使用 `git diff cpc/src` 检查差异，避免使用复杂的 Shell 管道命令）。

- **若 `cpc/src` 无变化**：直接进入第 4 步。
- **若 `cpc/src` 有变化**：先用文件读取与编辑工具将 `cpc/src` 中的新增修改同步合并到 `cpc_dev/src` 中，重新编译 `cpc_dev` 并运行测试，确保兼容无误。

4. **请求用户确认合并至正式版**：

- 向用户汇报测试结果，并询问：“**`cpc_dev` 测试已全部通过，是否现在合并到正式版 `cpc` 并生成新版本？**”
- 等待用户暂停 Bun 项目的智能体并明确同意后，再执行下一步。

5. **合并代码并构建正式 Release 版 `cpc**`：

- 获得用户同意后，将 `cpc_dev/src` 的代码同步更新至 `cpc/src`，然后构建正式 Release 版本并更新到 `cap/bin/cpc`：

```bash
cargo build --release --manifest-path /data/dev/cp/cpc/Cargo.toml

```

```bash
cp /data/dev/cp/cpc/target/release/cpc /data/dev/cp/cap/bin/cpc

```

6. **切换回正式版 `CPC_BIN` 进行最终验证**：

- 通过 `export` 指令将当前终端的 `CPC_BIN` 指回正式版本路径（`/data/dev/cp/cap/bin/cpc` 已在系统 `PATH` 中，切换后直接执行测试命令即可，切勿在测试命令前拼接 `env CPC_BIN=...`）：

```bash
export CPC_BIN=/data/dev/cp/cap/bin/cpc

```

- 运行一次 `cap` 测试，确认正式版二进制工作正常。

7. **完成交付并恢复并行**：

- 汇报正式版 `cpc` 更新完毕，供用户恢复 Bun 迁移项目继续执行任务。

### 3. 多子智能体并行工作流（核心）

#### 3.1 多子智能体并行开发cpc机制（`cpc_dev` vs `cpc_dev/agent_1`）

cpc 源代码几万行，修改、编译、测试非常耗时，需要修改、测试cpc 源代码实现自愈功能时，需将多个功能分派给多个子智能体在各自拷贝的沙箱目录中修改、测试，加快cpc研发速度。

为避免多个智能体相互覆盖代码或阻塞等待，本项目采用 **“多缓冲隔离目录（`cpc_dev/agent_1(1-9)`）+ 乐观锁校验合并（CAS）”** 模式：

- 子智能体创建自己的cpc的拷贝目录，已经准备了9个： `cpc_dev/agent_1`、`cpc_dev/agent_2`。
- **子智能体开发与调试一律在自己的 `cpc_dev/agent_*` 目录中进行**，与正式目录 `cpc/`、`cpc/cpc_dev` 保持隔离，待测试全部通过后再按流程合并至 `cpc/cpc_dev`。

#### 3.2 后期 cpc 编译器基本稳定，工作中心转向 cpc 热修与标准库开发（2 路正交并发节能规范）

- **2 路正交标准库并发与分级模型选型（节约配额）**：
  - **常规 2 路标准库并行子智能体**：日常严格保持 **2 个目录正交的子智能体**并显式指定 `Model: "inherit"`（继承主控模型）并行开发/吸收/审计不同 `lib/<mod>/` 模块，兼顾高吞吐与节约配额；
  - **疑难 `cpc` 编译器热修子智能体**：收到下游 `bun_agy` / `capsurf_agy` 的深层 Rust 编译器前端/Codegen 热修工单时，临时单独开启专属热修子智能体并指定 **`Model: "inherit"`**（继承主控模型）确保疑难 Bug 一击精准定位，修完即刻回收恢复日常 2 路；
- **子智能体原子发布纪律（杜绝下游误伤）**：子智能体开发新模块时，必须先写完所有新 `.cp` 文件并用 `cpc check --brief` 清零报错，**最后一步才将新导出写入 `lib/<mod>/index.cp`**，彻底防止并发构建的下游 `bun_agy`/`capsurf_agy` 读到半成品报错；
- **子智能体三段式极速调试**：① 改代码期仅跑 `cpc check --brief <file.cp>`（~15ms）清零语法/借用；② 跑 `cap test -m <test.cp>` 复用 `.cap/test-mod/` 模块缓存（增量转译 1~3ms）；③ 最终用 `bash scripts/verify_module.sh <lib_dir> <test.cp>` 一键完成合规扫描与测试；主控信任验证输出并尽量合并相近完工的子智能体做单次文档更新与提交；
- **后台守护通信**：跨 AGY 通信由 `systemd` 服务 `agy-comm.service` 常驻托管（详见 `/root/.agy/comm/COMM.md`），主控不再自建死循环轮询；收到热修工单后立即派单修复、`sync_cpc.sh release` 原子部署、`resolve` + `ack` 结单；
- **即用即收**：子智能体完成任务后立即回收释放，避免累积上下文浪费 Token。

### 4. 并行开发编译器与合并标准执行步骤

1. **使用 `env` 至调试沙盒**：

- `cap` 支持命令行`env CPC_BIN`设置cpc运行目录，来运行cpc， 你也可以直接使用你自己目录的cpc进行测试
- 通过注入 CPC_BIN 运行官方测试用例，其中cpc_dev_2为子智能体拷贝的工作目录：

```bash
   env CPC_BIN=/data/dev/cp/cpc_dev/agent_*/target/debug/cpc ./cap/bin/cap cap/test/test_err_and_move.cp
```

3. **启动前记录状态 & 测试后校验 `cpc_dev/src` 变更**：

- 启动拷贝工作区时，记录 `cpc_dev/src`的状态。
- 在 `cpc_dev/agent_*` 中修改代码（运行 `cargo check --manifest-path cpc_dev/agent_*/Cargo.toml` 与 `cargo build --manifest-path cpc_dev/agent_*/Cargo.toml`）并通过测试后，使用单条 Git 状态指令快速检查正式目录 `cpc_dev/src` 在此期间是否被其他子智能体修改过：

```bash
git status --porcelain cpc_dev/src
```

（或使用 `git diff cpc/src` 检查差异，避免使用复杂的 Shell 管道命令）。

- 3.1 **若 `cpc_dev/src` 无变化**：直接将工作区`cpc_dev/agent_*`代码合并到`cpc_dev/src`。
- 3.2 **若 `cpc_dev/src` 有变化**：先用文件读取与编辑工具将 `cpc_dev/src` 中的新增修改同步合并到 `cpc_dev/agent_*/src` 工作区中，重新编译 `cpc_dev/agent_*` 并运行测试，确保兼容无误。
- 重复 3.2，直至`cpc_dev/src` 无变化，按 3.1 合并子工作区代码到'cpc_dev/src'。

**务必注意**：

1. 以上 `cpc_dev_*`的`*`，表示各个子智能体创建的cpc 拷贝目录的数字编号，必须避免不同子智能体同时在一个工作区工作，同时修改同一个代码文件，导致代码混乱。
2. 每个 智能体占用一个 `cpc_dev/agent_*` 目录， 必须同步`cpc_dev/src` 到占用`cpc_dev/agent_*` 目录，并在该目录下的`work.json`（没有则创建）写入 状态为占用（state: 'busy'）。
3. 每个 智能体使用完 `cpc_dev/agent_*` 目录， 必须在该目录下的`work.json`（没有则创建）写入 状态为空闲（state: 'idle'）。
4. 每个 子智能体使用 `cpc_dev/agent_*` 目录时，需检查 该目录下的`work.json`状态为空闲（state: 'idle'）， 没有文件则创建，并直接占用，状态为‘busy’。

以上并发工作方案，目的是多子智能体并行工作，同时互不干扰，提高工作效率，缩短工作时间。

### 5. 编译器核心超大文件模块化分拆规范（Modular Architecture）

鉴于当前编译器核心源文件代码量庞大（`codegen.rs` 约 1.7 万行，`parser.rs` 约 7,000 行，`sema.rs` 约 6,000 行），极大增加了跨 800 行分段读取与多点联动的摩擦。后续按以下架构方案逐步进行功能模块解耦与拆分：

1. **`codegen/` 模块化分拆架构**：
   - `cpc/src/codegen/mod.rs`：主代码发射控制器、转译上下文（`CodeGen`）与入口派发；
   - `cpc/src/codegen/prelude.rs`：运行时辅助函数（`cp_iter_slice`、`CpResult`、`CpMutex` 等）与顶层 Zig 注入；
   - `cpc/src/codegen/expr.rs`：表达式、二元运算、类型转换、解构赋值与字段访问发射；
   - `cpc/src/codegen/stmt.rs`：语句、控制流、模式匹配与循环展开发射；
   - `cpc/src/codegen/actor.rs`：Actor CPS 状态机切分、信箱处理与 `@spawn` 投递生成；
   - `cpc/src/codegen/types.rs`：类型定义、常量泛型单态化与结构体内存布局生成。
2. **`parser/` 模块化分拆架构**：
   - 按表达式（`expr.rs`）、语句（`stmt.rs`）、类型（`types.rs`）、声明与类定义（`decl.rs`）分治。
3. **`sema/` 模块化分拆架构**：
   - 拆解为类型推导（`type_infer.rs`）、作用域与符号表（`scope.rs`）、借用与所有权检查（`borrow.rs`）。
4. **单文件规模与多智能体分治控制**：
   - 拆分后单文件代码量严格控制在 1,500 ~ 2,000 行以内，彻底消除分段读取成本，支持多个子智能体在各自模块文件中独立开发互不冲突。

## 六、Token 消耗管控与任务效率平衡铁律 (Token Economy & High-Efficiency Protocol)

为了杜绝大模型上下文爆炸（Context Bloat）与 Token 额度过快耗尽（Quota 429），所有 AGY 智能体在后续开发与维护中必须严格执行以下准则，实现 **低 Token 消耗与极致执行效率的最佳平衡**：

### 1. 智能体弹性分期治理（阶段驱动原则）

- **广度迁移期（大批量改写）**：允许启动子智能体并行处理初步改写，改写后立即汇报结果并归拢。
- **深度收敛期（排错、编译打通与回归测试）**：
  - 严禁盲目长期维持 多路子智能体全开！当测试或合并到cpc、cpc_dev编译报错时，**必须由主智能体单点精准突破**，坚决切断 `主智能体 Token + N × 子智能体上下文` 的乘数倍增效应。
  - 完成任务的子智能体需及时清理或转入休眠，禁止持续在后台空转发送心跳。

### 2. 上下文与文件 I/O 严格切片规范（Surgical I/O）

- **终端编译日志抑制**：运行 `bun bd`、`ninja` 或测试时，若遇到错误，模型仅抓取并分析最后的关键编译报错摘要（如 `file.zig:line:col: error: ...`），避免将数万行的构建垃圾日志卷入历史上下文。

### 3. 本地机器自愈通道优先（Zero-Token / Local Diagnostic）

- **先行 AST 语法秒级验证**：任何 `.cp` 文件转译为 `.zig` 后，先在本地执行秒级单文件验证：

  ```bash
  cpc check --json path/to/file.cp
  vendor/zig/zig ast-check path/to/file.zig
  cargo check cpc/src/file.rs
  ```

- **结构化 edits 采纳**：优先使用 `cargo check` 输出的精确 `offset` 与 `edits`，毫秒级单步自愈，严禁靠多轮自然语言猜测试错。

### 4. 批处理优先于微步操作（Batching Over Micro-Steps）

- 同一模块中发现的多处同类语法问题（如去除冗余的 `: void`、修复 pipe 捕获为 walrus 语法、清理 `let mut`），必须在单轮中一次性合并完成所有 `edit_file`，再发起单次验证，严禁“改一行、测一次、聊一句”的高频碎步交互。

### 5. 标准库并发研发与 Token 极简提速固化规范 (2026-11-21 固化)

1. **脚本化一键收尾**：主控回收验收完成的 `lib/<mod>/` 时，优先直接调用 `python3 scripts/register_stdlib.py ...` 单命令完成 `cp_lib_ref.md` + `CHANGELOG.md` + `DEVLOG.md` + `SESSION.md` 更新及可选 `git commit`，与 `kill` / `invoke_subagent` 同轮并行，将收尾压缩至 1 个回合。
2. **热文档自动滚动归档**：当 `SESSION.md` / `CHANGELOG.md` / `DEVLOG.md` 超过 ~150 行时，调用 `python3 scripts/archive_hot_logs.py --keep 5` 将历史记录无损追加至 `*_H.md` 末尾，热文件始终保持极简轻量（< 50 行）。
3. **子智能体“降耗硬核速查 + 免探索最小信息包”派发模板**：派发子智能体时直接在 Prompt 内嵌最小范式，消除子智能体 `read_file` 重复摸索规范（平均省去 8~11 次无效读盘）与 Zig 0.17 / `cpc` 编译试错轮数：
   - **免读声明**：明确告知无需再读 `cp_agent_ref.md` 或翻阅其他 `lib/` 学格式；
   - **唯一高频写法直给**：首行 `// Source:` 溯源；导出 `export { ... } from "./sub.cp";`；Zig 0.17 容器统一 `std.ArrayList(T).empty` + `append(alloc, item)` / `deinit(alloc)` / `toOwnedSlice(alloc)`（禁用旧 `.init(alloc)`）；控制流 `if (...)` / `while (...)` 必须带括号；禁用 `std.posix`、`@import("bun")` 与自定义 `.release(...)`/`.isLocked(...)` 方法名；
   - **两段式批量工具流**：同回合并行 `write_file` 全部子模块 -> 运行 1 次 `for f in lib/<mod>/*.cp; do cpc check --brief "$f" || exit 1; done`（注意 `cpc check` 每次仅接受单个文件参数，禁止直接传 `*.cp`）清零报错 -> 同回合并行 `write_file` `index.cp` + `doc/lib/<mod>.md`（4 段式）+ `cap/test/test_<mod>_production_deep.cp`（5 个 `test(...)`）-> 仅运行 1 次 `bash /data/dev/cp/scripts/verify_module.sh` 完成验收。
4. **Bun 吸收强制“大纲索引 + 局部切片读取”**：子智能体读取 `/data/dev/bun/src/` 大文件时，先用 `rg -n "^(pub )?(fn|struct|enum|const)"` 提取符号行号再分段精读，彻底剔除 JSValue/Bun 内部耦合，禁止整文件吞入上下文。
5. **主控空隙巡检与日志驱动持续调优（不闲着铁律）**：在等待 2 路并发子智能体推进的间隙（或收到用户巡检指令时），主控主动执行全库 `lib/` 合规扫描（`std.posix` / `@import("bun")` / `// Source:` / `doc/lib/*.md` / `cp_lib_ref.md` 完整性）、盘点 `/data/dev/bun/src/` 剩余待吸收清单，并统计分析近期子智能体 `transcript.jsonl`（步数、工具分布、报错重试点），持续把新发现的卡点反哺固化到本规范与派发模板中。

6. 主智能体不可陷入长回合连续编码循环，需负责开启两个子智能体做常规任务（如lib资产迁移）, 一个热修子智能体。
   主要负责巡检/审核/验收 子智能体工作成果, 及时回应我的指令/及时接收工单避免消息积压/及时安排子智能体处理/及时汇报阶段性成果,更新文档和git提交固化成果。并通过监控子智能体执行效率, 分析执行数据, 拿出改进方式方法, 检验有效, 写入 GEMINI.md固化, 实现自我成长进化。

### 6. 配额超限安全冷却缓冲协议 (Quota Cooldown via Built-in schedule)

当开发过程中遇到平台配额限制报错时（例如：`⚠ Individual quota reached. Please upgrade your subscription to increase your limits. Resets in XhYmZs`）：

1. **精确计算等待秒数**：解析重置时间字符串，计算总秒数 $S = H \times 3600 + M \times 60 + Z$；
2. **增加 2 分钟安全缓冲**：为彻底防止由于网络抖动或服务端微小时间偏差导致二次触发限流，必须**额外增加 2 分钟（120 秒）安全缓冲时间**：
   $$\text{DurationSeconds} = S + 120$$
3. **调用内置 `schedule` 工具**：
   ```json
   {
     "DurationSeconds": 7507,
     "Prompt": "配额限制冷却缓冲期结束，自动恢复后台自主任务流水线..."
   }
   ```
4. **终结本轮调用，静默等待自动唤醒**：调用 `schedule` 后，**严禁调用任何其他工具**，直接结束当前轮次。系统将在定时器到期时自动发送高优先级通知唤醒智能体恢复工作；
5. **严禁违规行为**：**绝不允许在 Bash 终端中使用 `sleep` 指令作为定时器**！必须 100% 遵守系统准则使用内置 `schedule` 工具。

---

## 七、跨智能体异步协作看板协议 (Task Mailbox)

### 1. **标准操作指令**：读取`/root/.agy/comm/COMM.md`文件，熟悉多AGY通信机制与指令。

### 2. 跨智能体异步任务看板架构（支持双下游 `bun_agy` & `capsurf_agy`）

- **四大核心多 AGY 通信规则（必须严格遵守）**：
  1. **双下游识别**：待处理工单包含 `"created_by": "bun_agy"` 或 `"capsurf_agy"`（bun 工单前缀为 `task-bun-*`，CapSurf 工单前缀为 `task-capsurf-*`）。
  2. **验收、批量结单与语法广播**：通过验证后，调用支持多 ID 的 `resolve` 命令精准路由结单：
     ```bash
     python3 /root/.agy/comm/task_queue.py resolve <id1> [id2 ...] --note "已修复并验证"
     ```

  脚本会自动根据工单的 `created_by`/`target_agy` 将结单通知分发至提单者，不再需要人为间隔 2 秒或手写汇总文件。

  3. **CP 语法更新双端广播**：每当你新增或修改了 CP 语言语法规范 / `cpc` 规则时，必须运行广播命令同时通知 `bun_agy` 和 `capsurf_agy`：

     ```bash
     python3 /root/.agy/comm/task_queue.py broadcast --id "syntax-YYYYMMDD-<slug>" --note "详细语法变更说明"
     ```

     （或在结单含语法变更的工单时加上 `--broadcast` 参数）。

  4. **自动唤醒与认领**：一旦检测到来自 `bun_agy` 或 `capsurf_agy` 的消息通知，立即触发高优先级输出唤醒 CP AGY，读取消息！

  5. **修改与部署**：开启热修子智能体，读取 repro 用例并修复 `cpc/src` 源码后，运行 `bash scripts/sync_cpc.sh` 完成极速编译生效，并同步至 `cpc_dev/src`。

---

## 八、工程提速脚本指南 (Scripts Reference)

| 脚本 / 配置文件               | 核心作用                                                                                                           | 适用场景与时机                                                                 | 调用命令示例                                                                       |
| ----------------------------- | ------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------- |
| `scripts/sync_cpc.sh`         | **原子构建并部署 `cpc`**（临时文件 + `mv -f` 零并发窗口）                                                          | 修改 `cpc/src` 后**必须且只能**调用此脚本（日常 `debug`，发布/热修 `release`） | `bash scripts/sync_cpc.sh` / `bash scripts/sync_cpc.sh release`                    |
| `scripts/verify_module.sh`    | **一键标准库合规与模块化测试验收**（0 `std.posix`/0 `@import("bun")`/首行 `// Source:`/`cpc check`/`cap test -m`） | 子智能体交付前与主控验收时单条命令完成全部检查                                 | `bash scripts/verify_module.sh lib/json cap/test/test_json_rfc_production_deep.cp` |
| `scripts/archive_hot_logs.py` | **热日志无损滚动归档**（保留最近 N 条于热文件，旧条目无损追加至 `*_H.md` 末尾）                                    | 热文档超过 ~150 行时执行瘦身                                                   | `python3 scripts/archive_hot_logs.py --keep 5`                                     |
| `scripts/register_stdlib.py`  | **标准库验收一键登记与提交**（自动更新 4 线文档并可选 `git add/commit`）                                           | 主控回收子智能体时单命令收尾                                                   | `python3 scripts/register_stdlib.py --mod <mod> ...`                               |
| `scripts/audit_bun_sync.py`   | **Bun 上游溯源头规范化与 `Synced` 时间戳增量巡检**（对比 `/data/dev/bun/src/` 真实 `mtime`）                       | 批量规范化溯源头、检出 `bun_agy` 新增修改并刷回同步时间戳                      | `python3 scripts/audit_bun_sync.py [--normalize-headers \| --mark-synced ...]`     |

---

## 九、主/子智能体轻量协调、定期轮换与自我进化铁律

为防止主智能体陷入长耗时编码死循环、阻塞用户指令与下游工单响应，并消除长生命周期子智能体上下文膨胀导致的效率衰减，经实测验证（单模块稳定 1.2–2.8 分钟一次零错误交付、下游工单 0 积压），确立以下硬性规范：

1. **主智能体“短平快单任务”平衡原则（既不空闲待命，也不陷入长循环）**：
   - 主智能体职责包括：**巡检 / 审核 / 验收子智能体成果、及时回应用户指令与分派下游工单、登记提交、监控优化效率，并在每回合顺手完成 1 个耗时短（~30–60 秒、2–3 步内闭环）的高价值轻量任务**（如 1 个精巧标准库模块、小缺陷巡检或工程脚手架优化）。
   - 严禁单回合内连续不停编写多个模块导致阻塞消息通道；完成当前这 1 个短任务并 `git commit` 固化后，**立即结束回合向用户汇报**，确保任何用户指令与下游工单都能在 1 分钟内获得响应。
2. **四通道并行分工（3 迁移/收敛 + 1 专用热修）**：
   - 保持 **3 个标准库迁移/收敛子智能体**（`bun-absorber`）并行推进，最大化吞吐。
   - 一旦 `task_queue.py unacked` 出现下游工单，**立即派发专用热修子智能体**紧急处理（不阻塞迁移通道），验证、发布、结单后**立即 `kill` 回收**该热修子智能体，并由主智能体单次 `git commit` 固化。
3. **长生命周期迁移子智能体定期轮换（反上下文膨胀）**：
   - 迁移子智能体每连续完成 **10–15 个模块**（或单模块耗时明显升高）时，主智能体主动 `kill` 旧会话并 `invoke_subagent` 重建全新低上下文实例，保持 1–3 分钟/模块的高吞吐与 0 返工率。
4. **单次原子提交与热日志瘦身**：
   - 子智能体禁止修改共享四线文档（`cp_lib_ref.md` / `SESSION.md` / `CHANGELOG.md` / `DEVLOG.md`）或执行 `git commit`，全部由主智能体验收后通过 `scripts/register_stdlib.py` 统一登记并原子提交；热日志超 ~150–300 行时及时滚动归档至 `*_H.md`。

---

## 十、标准库简洁 TS 风格命名与领域主包收敛铁律

为保持 CP 标准库符合 **TypeScript / Bun 极简美学**、杜绝顶层目录碎片化与冗长下划线命名：

1. **顶层包名极简无下划线**：
   - 顶层 `lib/<pkg>/` 与 `cap:<pkg>` 必须采用**简洁单单词或领域主包名**（如 `cap:http`、`cap:dns`、`cap:sys`、`cap:cli`、`cap:ansi`、`cap:semver`、`cap:text`、`cap:git`、`cap:sql`、`cap:jsonc`、`cap:jsonl`、`cap:mustache`）。
   - **严禁**新建 `http_accept_language`、`proc_self_status`、`dns_caa_record` 等长下划线顶层目录！
2. **领域子模块扁平归入主包**：
   - 同领域细粒度功能作为简洁子文件放入对应主包（如 `lib/http/accept.cp`、`lib/dns/caa.cp`、`lib/sys/proc.cp`、`lib/cli/table.cp`、`lib/ansi/sgr.cp`），统一在主包 `index.cp` 聚合导出，调用方直接 `import { ... } from "cap:http"` / `"cap:dns"` / `"cap:ansi"`。
3. **历史长下划线目录分批收敛**：
   - 逐步将历史遗留的 `http_*`、`proc_*`/`etc_*`、`dns_*`、`cli_*`、`ansi_*`、`semver_*`、`sql_*`、`git_*`、`css_*`、`mysql_*`、`pg_*` 等同域散装目录合并入对应领域主包，更新测试用例导入并移除旧散装目录。
4. **统一溯源头与上游同步时间戳铁律**：
   - 所有从 `/data/dev/bun` 吸收的标准库文件，首行必须统一格式并记录上游文件 UTC 修改时间：
     `// Source: Absorbed from /data/dev/bun/src/<rel> | Synced: YYYY-MM-DDTHH:MM:SSZ (<简述>)`
   - 定期运行 `python3 scripts/audit_bun_sync.py` 对比 `Synced:` 与上游真实 `mtime`，若 `bun_agy` 更新了对应 `.cp` 文件，及时同步修复/调整并用 `--mark-synced` 刷新时间戳。

