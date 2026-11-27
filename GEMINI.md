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

cp项目（当前项目）路径为：`/data/dev/cp/`;
下游capsurf项目路径为：`/data/dev/capsurf/`；
下游bun项目路径为：`/data/dev/bun/`；

在执行下游生态迁移与编译器协同任务时，你（`cp_agy`）拥有 **多套紧密联动的核心工作区**（包括 `bun_agy` 与 `capsurf_agy`），并且拥有对下游源代码的完全读取、拷贝权限，复用下游智能体迁移到 CP 的成果，不用重新造轮子。

### 授权直通声明（编译器自愈权限）

- **权限开放**：下游项目目录对你完全开放**读取权限**，不开放**修改、删除权限**！

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

<!--- **容错内存安全（Crash-Proof）**：跨越 `await` 的资源（显存、句柄）底层封装为 **NIF Resource Objects**。当 Agent 异常退出时，由 BEAM 回收机制自动触发 Zig 析构函数兜底，配合编译期主动注入的 `defer` 实现双重保障。-->

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

1. **SESSION.md（零遗漏、零截断铁律 — 最高优先级）**:
   - 排除执行过程中输出的冗长工具日志，将**① 用户发送的每一条完整原始指令（严禁仅写笼统摘要或跳过）**、**② 收到的每一条下游 AGY 工单（含 `task_id`、来源智能体、报错/需求摘要与结单结果）**、**③ 主智能体针对该指令的完整全部回复与结单汇报（严禁截断 `...(截断)` 或仅写摘要，且必须与对应指令严格 1-to-1 精确配对，不得被并行子智能体汇报串台）**，按日期及执行顺序完整追加到文件底部。
   - **脚本操作零遗漏约束**：凡使用脚本进行历史迁移、热冷归档或新增记录（如 `scripts/archive_hot_logs.py`、`scripts/register_stdlib.py`、`scripts/sync_session_audit.py`、`scripts/log_session.py`），脚本逻辑中必须完整包含上述三类内容（用户完整原始指令 + 下游 AGY 工单 + 主智能体完整回复与结单汇报），**绝不允许遗漏、截断、覆盖、删除或丢失任何一条记录！**
   - 定期及每次里程碑提交前，必须运行 `python3 scripts/sync_session_audit.py --check` 校验遗漏数为 0；若存在遗漏须立即补录后再提交。
2. **CHANGELOG.md**: 对每个文件的修改、删除、新增，按日期追加记录，让我知道你新增、删除、修改了什么文件。**之前的记录绝不能丢失！**
3. **DEVLOG.md**: 开发日志文件。按日期往后追加你每次执行任务解决问题的思路、原理及规划的执行方案与步骤，让我知道你为何这样执行。**绝不能删除之前的记录！**
4. **三轨解耦规范手册体系（低 Token 高效架构）**：
   - **`cp_agent_lib_ref.md`（智能体日常唯一标准库/生态极简速查手册，<45KB 单次可读）**：由 `python3 scripts/gen_agent_lib_ref.py` 从 `cp_lib_ref.md` 自动压缩生成（-88%+ 体积），覆盖全部 `cap:*`、`@web/*` 与 `@cap/*` 分类索引；**智能体日常查库只读 `cp_agent_lib_ref.md`，严禁日常全量读取 370KB 的 `cp_lib_ref.md`**；
   - **`cp_lib_ref.md`（标准库完整契约全量索引）**：独立承载全部内置标准库（`lib/*` / `cap:*` / `import cap from "cap"`）。**新增或升级标准库/生态包时，追加 `cp_lib_ref.md` + `doc/lib/<mod>.md`，并运行 `python3 scripts/gen_agent_lib_ref.py` 同步刷新 `cp_agent_lib_ref.md`，严禁为此读写 `cp_ref.md` 或 `cp_agent_ref.md`**；
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
   - **紧急热修不强制每个小修复都写全套 `CHANGELOG.md`/`DEVLOG.md` 或单独做 Git 提交**，但**必须**在 `task_queue.py resolve` 结单后，通过 `python3 scripts/sync_session_audit.py` 或 `python3 scripts/log_session.py` 将收到的下游工单与结单汇报完整追加至 `SESSION.md`，确保零工单遗漏。
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

热修工作路径为 `cpc/`。
自研工作路径为 `cpc_dev/`。
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

4. **合并代码并构建正式 Release 版 `cpc**`：

- 将 `cpc_dev/src` 的代码同步更新至 `cpc/src`，然后构建正式 Release 版本并更新到 `cap/bin/cpc`：

```bash
cargo build --release --manifest-path /data/dev/cp/cpc/Cargo.toml

```

```bash
cp /data/dev/cp/cpc/target/release/cpc /data/dev/cp/cap/bin/cpc

```

5. **切换回正式版 `CPC_BIN` 进行最终验证**：

- 通过 `export` 指令将当前终端的 `CPC_BIN` 指回正式版本路径（`/data/dev/cp/cap/bin/cpc` 已在系统 `PATH` 中，切换后直接执行测试命令即可，切勿在测试命令前拼接 `env CPC_BIN=...`）：

```bash
export CPC_BIN=/data/dev/cp/cap/bin/cpc

```

- 运行一次 `cap` 测试，确认正式版二进制工作正常。

6. **完成交付并恢复并行**：

- 汇报正式版 `cpc` 更新完毕，供用户恢复 Bun 迁移项目继续执行任务。

### 3. 多子智能体并行工作流（核心）

#### 3.1 后期 cpc 编译器基本稳定，工作中心转向 cpc 热修与标准库开发（2 路正交并发节能规范）

- **2 路正交标准库并发与分级模型选型（节约配额）**：
  - **常规 2 路并行子智能体**：日常严格保持 **2 个目录正交的子智能体**并显式指定 `Model: "inherit"`（继承主控模型）并行自研/开发/吸收/审计，兼顾高吞吐与节约配额；自研cpc工作路径为cpc_dev；
  - **`cpc` 编译器热修子智能体**：收到下游 `bun_agy` / `capsurf_agy` cpc编译器热修工单时，临时单独开启专属热修子智能体并指定 **`Model: "inherit"`**（继承主控模型）确保疑难 Bug 一击精准定位，修完即刻回收恢复日常 2 路；热修cpc工作路径为 `cpc`正式路径。
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
4. **吸收强制“大纲索引 + 局部切片读取”**：子智能体读取 `/data/dev/cp/src/` 大文件时，先用 `rg -n "^(pub )?(fn|struct|enum|const)"` 提取符号行号再分段精读，彻底剔除 JSValue/Bun 内部耦合，禁止整文件吞入上下文。
5. **主控空隙巡检与日志驱动持续调优（不闲着铁律）**：在等待 2 路并发子智能体推进的间隙（或收到用户巡检指令时），主控主动执行全库 `lib/` 合规扫描（`std.posix` / `@import("bun")` / `// Source:` / `doc/lib/*.md` / `cp_lib_ref.md` 完整性）、盘点 `/data/dev/cp/src/` 剩余待吸收清单，并统计分析近期子智能体 `transcript.jsonl`（步数、工具分布、报错重试点），持续把新发现的卡点反哺固化到本规范与派发模板中。

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

---

## 十一、标准库“做深不做薄”与内置/外部生态分层铁律 (CRITICAL)

为避免标准库沦为“文件数量多、单个功能浅、难以直接用于生产”的浅层合集，所有新增与深化核心库必须严格遵守以下工程底线：

### 1. “做深不做薄”生产级实现铁律

1. **解除小容量玩具上限（外部切片 / Arena 驱动）**：
   - 除协议规范本身规定小上限的场景（如 W3C `tracestate` 最多 32 项）外，**检索、优化、统计、调度、通信、多媒体等重型核心引擎严禁硬编码小容量定长数组（如 `MAX_DOCS = 256`、`MAX_TABLES = 8`）作为唯一上限**。
   - 必须优先采用**调用方传入任意长度外部切片/工作缓冲**（`[]f32`/`[]f64`/`[]u8`/`[]Node`）或 Arena 驱动，既保持 100% 纯 CP 零动态堆分配与内存安全，又能无缝承接十万级/高并发真实生产数据规模。
2. **功能完备优先于模块数量**：
   - 复杂领域引擎不再为了“每轮凑 1 个 300 行小文件”而阉割关键能力；单核心模块按业务真实复杂度写透（通常 **800–1,500 行**，或按清晰子职责分多轮打磨成完整套件），补齐**核心算法 + 边界/数值稳定性 + 二进制持久化/流式编解码 + 权威基准值对照验证**（如 R/SciPy、RFC 测试向量、Parseval 定理、召回率实测）。
   - 单文件体量仍受总规范 `<= 3000` 行约束；超出时按职责拆分为同主包下的清晰子模块（并在 `index.cp` 统一导出）。
3. **存量旧库回溯审查与深化补全（杜绝阉割版留存）**：
   - “做深不做薄、做全不做残”**同样适用于此前已编写与已吸收的存量标准库**；必须定期系统回溯审查早期实现（尤其是检索/优化/统计/调度/通信/多媒体/文档处理中的小容量或浅层模块），凡存在功能不完备或小容量硬上限的模块，必须就地重构补全完整生产级功能与文档，不留“演示级阉割版”。
4. **文档与可复用性闭环**：
   - 每个核心引擎必须同步补全 `doc/lib/<pkg>.md` 的架构说明、参数调优要点与端到端可运行示例，以及 `cp_lib_ref.md` 索引条目。

### 2. 内置标准库（`cap:*`）vs 外部 `cap` 生态包分层边界与两阶段路线图

1. **流媒体 / 实时语音（ASR/TTS）与音视频四层性能超越原则**：
   - **零 GC + 外部切片零拷贝**：音视频帧（PCM/RTP/fMP4/YUV420）全程原地切片视图传递，杜绝 GC 尾延迟尖刺；
   - **原生 SIMD 算子 + Zig `@Vector` 向量化**：利用 `@dot` / `@fma` / `@l2sq` / `@popCount` 加速 FFT、Biquad/FIR 滤波、Sinc 重采样、Mel 滤波器组、VAD 与图像卷积；
   - **`comptime !{}` 编译期查表烘焙**：窗函数、G.711 μ-law/A-law、YUV↔RGB 矩阵、CRC/FEC 伽罗瓦域表在编译期预生成进只读段；
   - **BEAM Actor + CPS + `Refc Binary`/`SharedTensor` 无锁流水线**：`RTP -> JitterBuffer -> 解码 -> 重采样/VAD -> ASR/LLM -> TTS -> 打包 -> SRTP` 按 Actor 故障隔离，零拷贝指针接力。
2. **Phase 1（优先完成）：内置标准库（`lib/` / `cap:*`）三层内置核心**：
   - **WebRTC 与实时流媒体协议层**（`cap:net` / `cap:media`）：RTP/RTCP/SRTP/SDP/ICE/STUN/TURN/SCTP/WHEP、HLS/DASH 播放列表与分片调度、fMP4/WebM/Ogg/WAV 容器读写；
   - **轻量实时语音与音频 DSP 层**（`cap:media`）：PCM、ITU-T G.711（PCMU/PCMA）、自适应 Jitter Buffer & PLC、Radix-2 FFT/STFT、RBJ Biquad/SOS 滤波器、加窗 Sinc 重采样、ITU-R BS.1770 响度、VAD 与 ASR Mel 声学特征前端；
   - **纯 CP 基础图像处理与文档层**（`cap:media` / `cap:doc`）：RGBA/YUV420/NV12 色彩互转、双线性/Bicubic/Lanczos 缩放、高斯/Sobel 卷积、无损 BMP/PPM/PNG 基础读写，以及 DOCX/PDF/XLSX 读写强化；
   - **高频通用计算与 AI/Agent 核心原语**（`cap:vector` / `cap:math` / `cap:sys` / `cap:mcp` / `cap:a2a` / `cap:agentmem` / `cap:attention`）。
3. **Phase 2（内置库完备后推进）：外部官方可安装 `cap` 生态包（`cap install` 按需安装）**：
   - 凡涉及百万行级 C/C++ 代码、硬件加速栈（NVENC/VAAPI/VideoToolbox）或专利编解码的知名重型库，**绝不塞进内置标准库**，而是作为官方外部包通过 `cap` 预编译二进制矩阵（`.so`/`.dylib`/`.dll` + 全局缓存软链）按需安装：
     - 音频/视频编解码：`@cap/opus`（libopus）、`@cap/av`（FFmpeg 封装）、`@cap/codecs`（x264/x265/dav1d/SVT-AV1）；
     - 图像与计算机视觉：`@cap/vips`（libvips）、`@cap/opencv`（OpenCV）、`@cap/magick`（ImageMagick）；
     - 推理运行时：`@cap/onnx`（ONNX Runtime）、`@cap/llama`（llama.cpp）。

### 3. 热路径强制 SIMD 加速与确定性 Fuzz 模糊测试门禁铁律 (CRITICAL)

为真正发挥 CP 无 GC + 原生硬件加速的性能优势，并确保未经大规模生产流量洗礼的标准库具备零崩溃健壮性，所有新增核心引擎与存量深化模块必须严格执行以下双门禁：

1. **热路径强制评估并接入 CP 原生 SIMD 加速**：
   - **数值与多媒体计算核**（矩阵 GEMM/GEMV、向量检索、卷积/FIR、Sinc 重采样、DCT/FFT、图像缩放/高斯模糊/PSNR、时间序列自相关 ACF、声学 Mel/MFCC）：必须优先使用 CP 原生 SIMD 算子（`@dot` / `@fma` / `@l2sq` / `@popCount`），并附带与朴素标量参考实现的精度对拍断言（通常 `< 1e-4`）；
   - **字节流、文档与协议扫描核**（CSV/PDF/OOXML/JSON/源码预扫描、Annex-B 起始码、EBSP 脱壳、分词大小写折叠）：必须优先调用 `cap:simd`（`Simd.indexOfByte` / `Simd.indexOfAny2` / `Simd.indexOfAny4` / `skipWhitespace` / `firstNonAscii` / `toLowerAscii` / `countByte`）替代逐字节标量循环，最大化吞吐。
2. **确定性 Fuzz 模糊测试与外部真值交叉验真门禁**：
   - 凡涉及解析外部不可信输入（文件容器、压缩流、网络报文、查询表达式、二进制快照）的模块，必须配备**确定性 PRNG（如 xorshift64）畸形输入 Fuzz 测试**（覆盖随机字节、截断、超大宽高/乘法溢出、`i32::MIN` 边界、深层嵌套递归、单比特校验篡改），强制要求 **0 panic、0 越界、0 整数溢出、0 死循环、0 内存泄漏**；
   - 编解码器与标准协议模块必须优先与真实外部工业工具（`libjpeg-turbo` / `libwebp` / `zstd` / `node zlib` / `ffmpeg` / RFC 官方测试向量）做交叉验真，绝不“自写自测闭门造车”。

### 4. MIT/Apache-2.0 Rust 标杆仓 `_ref/` 对照迁移与测试集继承铁律 (CRITICAL)

为确保 CP 内置核心库具备工业级稳定性、边缘场景完备度与商用合规性，新增与深化重型纯逻辑核心库优先采用“巨人肩膀”策略：

1. **严格许可证白名单（商用合规底线）**：
   - 仅允许对照迁移 **MIT / Apache-2.0 / BSD-2-Clause / BSD-3-Clause / Zlib / ISC** 宽松许可的 Rust 标杆开源库；**严禁**引入任何 GPL / AGPL / LGPL 传染性代码；
   - 迁移模块顶部必须保留上游来源与许可证溯源注释，并统一登记至项目根目录 `THIRD_PARTY_NOTICES.md`。
2. **`_ref/<crate>/` 参考仓隔离与测试集继承**：
   - 沿用 `_ref/swc/` 隔离模式，将目标 Rust 标杆仓浅克隆至 `_ref/<crate>/`（由 `.gitignore` 忽略，不提交上游源码）；
   - **代码 + 实战测试集同步迁移**：不仅对照 Rust 标杆仓补齐边界逻辑、状态机与容错分支，更必须将其 `tests/` 中的权威测试向量、真实野样本与历史回归用例一并迁移至 `cap/test/`，再叠加 CP 原生 `@dot` / `@fma` / `@l2sq` / `cap:simd` 硬件向量加速与零堆外部切片改造。
3. **存量自研核心库分批对标加固**：
   - 对已自研的重点领域核心库（WebRTC/流媒体、文档/表格、图像/压缩、全文检索/向量、数值线性代数/空间索引），按优先级拉取对应 MIT/Apache-2.0 Rust 标杆仓逐项对标合并与测试集补齐，消除边缘缺陷。
4. **重量级 C/C++ 工业库统一 Phase 2 外部包路线**：
   - 凡 FFmpeg、libopus、libvips、OpenCV、ImageMagick、ONNX Runtime、llama.cpp 等百万行级 C/C++ 重量级库，统一采用 `@c` FFI 轻量封装 + `cap add` 安装 + 预编译跨平台二进制矩阵（`.so`/`.dylib`/`.dll` + 全局缓存软链），绝不塞入内置 `lib/`。

### 5. 高吞吐与极致省 Token 自我优化铁律（实测固化，2026-11-24）

1. **子智能体极简回执协议（实测单轮省 65%–70% 跨上下文 Token）**：
   - 主控派发任务时默认附带 `<= 12` 行极简回执要求（仅列改动文件/行数、核心新增符号、4 门禁结果、测试通过项与 Fuzz 规模），严禁回传大段源码或长测试日志；主控对用户汇报保持 `3–6` 行极简进度摘要。
2. **子智能体 8 轮强制上下文轮换**：
   - 单路并行子智能体连续完成 ~8 轮后主动 `kill` 并启动全新轻量实例，消除长对话历史重传导致的指数级 Token 膨胀（实测可稳定保持单轮 4–6 分钟高吞吐）。
3. **主控“元数据更新 + 下轮派发 + 查队列”同回合并行批处理**：
   - 验收通过后，在同一回合内并行执行 `cp_lib_ref.md` / `THIRD_PARTY_NOTICES.md` 更新、下轮子智能体派发与下游工单队列检查，紧接着单条命令完成 `git add && git commit`，将主控每轮往返压缩至 2 回合以内；修改已有表格行前先精准 `rg -n` 确认原文，杜绝盲猜失败浪费整轮重传。
4. **高负载下 `cpc` 热修构建防超时策略**：
   - 若系统处于高负载（如下游 Bun 链接期间），先直跑 `cargo build --release` 完成增量编译（可见进度且无 120s/300s 静默强杀），再调用 `bash scripts/sync_cpc.sh release` 3 秒完成原子部署。
5. **静默编译输出节流（实测节省 98%+ 编译日志 Token，2026-11-24 Iteration 2）**：
   - 调用 `cargo check` / `cargo build` 时必须加 `RUSTFLAGS="-Awarnings"` 与 `-q`（实测仅 `-q` 仍输出 21,567B warning，加 `RUSTFLAGS="-Awarnings"` 后压至 418B，降幅 98.1%），仅保留真实报错与完成状态。
6. **多套件回归 4 路并行 + 独立 `--cache-dir` 隔离（实测 4x 提速、降 90% 回归日志 Token）**：
   - 批量跑多份 `cap/test/*.cp` 时统一使用 4 路并发 + 独立 `--cache-dir=.cap/ws_<tag>/<suite>` 避免缓存锁竞争与串台，PASS 项仅输出汇总计数、仅对 FAIL 项展开错误上下文。
7. **`cap` 工具链原子构建脚本与报错降噪（实测降幅 79.4% 报错日志 Token，2026-11-24 Iteration 3）**：
   - 构建部署 `cap` 统一调用 `bash scripts/sync_cap.sh [debug|release|fmt]`（一键完成可选 `zig fmt` + `zig build` + 原子部署至 `/root/.cap/bin/cap`，成功仅输出 1 行 52B；失败自动折叠冗长的 `failed command:` 完整参数行与重复 `Build Summary` 树，实测将典型报错从 1,346B 压至 277B，降幅 79.4%），禁止再手写长串 `cd ... && zig build && cp ... && mv ...`。
8. **大批量文件 Git 提交静默输出节流（实测降幅 99.4% 提交日志 Token，2026-11-25 Iteration 4）**：
   - 凡单次提交涉及 > 10 个新增/修改文件（如引入 C 生态包或批量重构），统一执行 `git commit -q -m "<msg>" && git show --stat --format="%h %s" HEAD | tail -n 3`，避免打印长达 13KB–26KB 的 `create mode 100644 ...` 文件刷屏（26,106B → ~120B，降幅 99.4%）。
9. **统一 `RUSTFLAGS="-Awarnings"` 消除 `sync_cpc.sh` 指纹失效重编（实测将热修部署从 >120s 超时压至 ~3s，提速 >40x，2026-11-25 Iteration 5）**：
   - `scripts/sync_cpc.sh` 已统一注入 `RUSTFLAGS="${RUSTFLAGS:--Awarnings}"`，与 Rule 5 保持完全一致的 Cargo 编译指纹，彻底消除 `cargo check`/`cargo build` 与 `sync_cpc.sh` 交替执行时触发全量依赖重编与 120s 超时重试（同时节省 2 轮重试上下文 Token）。
10. **词法器 `Lexer Error` 限流与批量自检静默（实测将 `cap from` 批量解析日志从 219 KB 压至 ~260 B，降幅 99.9%，2026-11-25 Iteration 6）**：
    - `cpc/src/lexer.rs` 对单文件 `Lexer Error` stderr 输出实施 5 条上限封顶并支持 `QUIET_LEXER` 原子静默；`cpc/src/migrate/mod.rs` 批量 `check_cp_parse` 自动开启静默模式，彻底杜绝大批量解析时百 KB 级无用刷屏污染上下文。
11. **`git push -q` 静默推送 + 实时真实榜单驱动 + 单行 Python 零转义铁律（2026-11-27 3 小时审计固化 Iteration 7）**：
    - **`git push -q` 降噪 99.9%**：所有推送统一使用 `git push -q origin main && echo "PUSH_OK"`，将 ~11KB 进度条刷屏压缩为 7 字节；单测验收统一加 `| tail -n 8` 过滤横幅；
    - **真实下载量实时榜单驱动**：使用 `scripts/fetch_real_top700.py` 拉取 `crates.io/api/v1/crates?sort=downloads` 与 `wooorm/npm-high-impact` + `api.npmjs.org` 实时榜单落盘 `temp/real_top700_*.json`，所有派发严格按差集真实下载量降序执行，杜绝凭记忆估算下载量或吸收低下载长尾包浪费算力；
    - **单行 `python3 -c '...'` 零反斜杠转义铁律**：禁止在单行 Python f-string 内写 `\"` 转义（Python 3.12 必报 `SyntaxError` 浪费整轮重试），统一改用 `"%s" % (d["k"],)` 格式化；
    - **CP 1-Pass Codegen 新增 3 条一遍过避坑**：① 结构体方法避免命名为 `.toString(...)`（会被 `cpc` sema 固定推断返回 `string`，写入缓冲返回长度统一命名 `.writeTo(out): usize`）；② 科学计数法字面量必须带小数点（写 `1.0e12` 而非 `1e12`）；③ 浮点无穷/NaN 严禁写编译期 `1.0 / 0.0`，统一使用 `std.math.inf(f64)` / `std.math.nan(f64)`。
12. **串行 `cap test` 防 Zig 缓存锁争用 + L0 基座零冲突命名 + `cap brief` 90% 省 Token（2026-11-27 3 小时审计固化 Iteration 8）**：
    - **禁止主智能体并发启动多个后台 `cap test`**：实测并发运行多路 `zig build-exe` 争用 `.cap/zig-cache` 锁会使单测从 **650ms 劣化至 160,000ms（慢 240 倍）**；主智能体统一采用单命令串行链式 `cap test a.cp | tail -n 8 && cap test b.cp | tail -n 8` 保持 650ms 级极速；
    - **L0 标准库跨通道导出命名零冲突铁律**：新增/修改 `cap:*` 内置通道（如 `cap:ascii`、`cap:bin`、`cap:fmt`、`cap:utf8`）严禁导出同名顶层符号（如 `cap:ascii.findByte` vs `cap:bin.findByteInBytes`），防止多通道混用时因模块展平触发 Zig `duplicate struct member name`；
    - **生态包文档极简与 `cap brief <pkg>` 查库铁律**：`cp_agent_lib_ref.md` 采用紧凑逗号列表替代逐项反引号，体积压至 **38.3KB**（-90.7%）；全生态 904 包均已配备仅含公开 API 的 `README_AGENT.md` 与 `src/index.d.cp`，AI 查包优先运行 `cap brief <pkg>`，杜绝全量读源码浪费 Token。

---

## 六、 CP 内置库（`lib/`）与 `@cap` 生态吸收指南（病毒式生态自我繁衍规范，2026-11-26）

为持续、安全、高质量地吸收 **npm（Node/Bun 生态）** 与 **crates.io（Rust Cargo 生态）** 的高频优质资产，避免生态碎片化爆炸与版权风险，所有智能体与 `cap from` 工具链必须严格遵守以下五项铁律：

### 1. 双层架构分层准则（杜绝重复与生态爆炸）

| 层级               | 目录与导入方式                                          | 准入标准                                                                                                                                                                        | 典型示例                                                                                                                                                                                                         |
| :----------------- | :------------------------------------------------------ | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **内置核心标准库** | `lib/<mod>/` (`import ... from "cap:<mod>"`)            | **零 C 依赖、基础通用、被引用极广的底座能力**（编解码、哈希/校验、时间/单位、MIME/HTTP 基础头、通用协议原语）。凡发现被高频依赖的通用集成包，应吸收沉淀入 `lib/` 丰富 CP 底座。 | `cap:json`, `cap:cbor`, `cap:codec`, `cap:crypto`, `cap:hash` (`crc32c`), `cap:time` (`human`), `cap:bytes`, `cap:mime`, `cap:etag`, `cap:sri`, `cap:slugify`, `cap:logfmt`, `cap:rate`, `cap:ulid`, `cap:jsonc` |
| **可选安装包**     | `packages/@cap/<pkg>/` (`import ... from "@cap/<pkg>"`) | **① 原生 C/C++ 绑定库**（`@c` FFI + 预编译矩阵）；**② 大型独立领域引擎/框架级纯 CP 包**（独立数据结构、算法引擎、复杂解析器/渲染器/中间件），不适合塞入核心 stdlib，按需安装。  | 原生绑定：`@cap/duckdb`, `@cap/llama`, `@cap/opencv`, `@cap/zstd`；纯 CP 引擎：`@cap/lru`, `@cap/qs`, `@cap/retry`, `@cap/diff`, `@cap/template`                                                                 |

- **去重与融合原则（防生态爆炸）**：
  - 吸收前**必须先检索** `packages/@cap/registry.json` 与 `lib/`、`packages/@cap/`、`packages/@web/`；
  - 若 CP 生态**已有相同/高度重叠功能的库**：**严禁再造重复独立包**！应将新包的差异化亮点与兼容 API 融合进已有库（保持 100% 向后兼容），并在 `registry.json` 的 `aliases` 中登记该上游包名，同时提示用户直接使用已有 CP 库，降低安全审计与长期维护成本。

### 2. 质量与性能铁律（“做深不做薄，做全不做残”）

1. **功能完备**：对标上游最新稳定版核心 API 与 RFC/行业标准测试向量（如 RFC 3720、RFC 4648、RFC 6265、RFC 8949、RFC 9110、W3C SRI §3.3.3 等），绝不做玩具级半成品。
2. **CP 专有性能优化（超越原包性能与内存安全）**：
   - 优先**零堆分配**（栈切片 + 固定池）与 **`= undefined` 零清零初始化**（消除大栈池启动清零开销）；
   - 合理使用 **Arena 批量分配**、**侵入式 O(1) 链表/哈希索引**、**纯整数/饱和算术（`+|`/`+%`）防溢出** 与 **SIMD（`@Vector`/`cap:simd`）向量加速**；
   - 严格遵循 CP 所有权借用安全与 4 门禁（`0 any`、`0 ?T`、`0 dec`、`0 unsafe`，边界/畸形报文零 panic）。
3. **四件套交付标准**：每个新吸收的 `@cap/<pkg>` 必须包含：`cap.json`、`src/index.cp`、`src/index.d.cp`（`cpc emit-types` 生成）、中文带完整可运行示例的 `README.md`，以及 `cap/test/testCap<Name>.cp` 深度回归测试；内置 `lib/<mod>` 必须同步更新 `doc/lib/<mod>.md`（带使用示例）与 `cp_lib_ref.md`。

### 3. 版权与开源协议合规铁律

1. **优先 Clean-Room 自主实现**：基于公开 RFC、W3C 规范、公开算法论文与公开 API 契约，以纯 CP 从零编写干净实现，统一声明 **`MIT`** 许可，并在 `README.md` 与 `THIRD_PARTY_NOTICES.md` 中清晰致谢所对标的生态规范。
2. **宽松协议白名单**：引用或参考上游源码时，仅限 **MIT / Apache-2.0 / BSD-2-Clause / BSD-3-Clause / ISC / Zlib / CC0 / Unlicense** 协议；**严格禁止**拷贝或衍生 **GPL / AGPL / LGPL / SSPL** 等传染性协议源码。
3. **溯源透明**：在 `packages/@cap/registry.json` 与 `cap.json` 中完整记录上游包名、仓库地址与协议类型。

### 4. 运行依赖独立化与全局索引（`packages/@cap/registry.json`）

1. **依赖隔离原则**：转换外部 npm / crates.io 包时，**仅递归处理运行依赖（`dependencies`）**，跳过开发依赖（`devDependencies`）；每个运行依赖**必须按其原始包名独立生成为独立 `@cap/<dep>` 包**（严禁将多个依赖粗暴合并进主包），并在主包 `cap.json` 的 `dep` 字段中声明引用。
2. **全局去重索引（`packages/@cap/registry.json`）**：
   - 集中登记所有已吸收/映射的 npm 与 crates.io 包，包含：`source_registry`（`npm` / `crates.io`）、`source_name`、`source_repo`（GitHub 地址）、`license`、`cp_target`（`cap:*` 或 `@cap/*`）、`kind`（`stdlib` / `package`）、`aliases`、`absorbed_at`；
   - 该索引文件托管于 GitHub 仓库（可直接通过 Raw URL 访问），`cap from -npm <pkg[@ver]>` 与 `cap from -crate <name[@ver]>` 转换前**必须先查询本地/远端索引**：已转换过的包直接复用不再重复转换；命中同功能别名则直接提示用户使用对应 CP 包。

### 5. `cap from -npm` / `cap from -crate` 生态自繁衍工作流

- **命令形态**：
  - `cap from -npm <pkg[@ver]> [--verify] [--out <dir>]`（查询 npm registry `https://registry.npmjs.org/<pkg>` + GitHub 源码）
  - `cap from -crate <name[@ver]> [--verify] [--out <dir>]`（查询 crates.io `https://crates.io/api/v1/crates/<name>` + GitHub 源码）
- **标准流水线**：
  1. **查索引去重**：查 `packages/@cap/registry.json`（本地 + 可选远端 GitHub 索引），已存在则直接返回对应 `cap:*` / `@cap/*` 并提示用户使用；
  2. **合规与元数据检查**：拉取 registry 元数据，校验开源协议白名单（拒绝 GPL/AGPL），解析运行依赖列表 `dependencies`（忽略 `devDependencies`）；
  3. **递归独立转化依赖**：对尚未在索引中的运行依赖，逐个独立转化为 `@cap/<dep>` 并回写索引；
  4. **生成与自愈校验**：按 CP 规范生成目标包骨架/转译代码、`cap.json`、`index.d.cp`、`README.md` 与测试文件，执行 `--verify` 编译校验闭环，并登记至 `packages/@cap/registry.json`。

## 库吸收补充

1. 拿到npm 和 crates平台包的真实下载量, 把npm 和 crate 前 700名的包及其依赖包全部吸收尽量.
2. 这些包的引用包, 要作为独立包, 一并导入, 比如一个包引用了3个其他包, 我们要把这3个包, 单独吸收到@cap,等于一共 4个包.
3. 之前已经吸收的, 如果没有这样做, 需重新分离为该模式, 作为独立包, 因为独立包同时会被其他包引用,避免代码重复.
4. 你自己思考判断, 做好从 npm 和 crate 吸收优良资产任务, 不要混乱, 不要重复, 确保cp 中生态完整, 用户无论从 ts 或 rust 迁移到 cp, 常用的包, cp 都有, 5.
5. 确保吸收后功能完整没有残缺遗漏, 确保包的质量, "做多不做薄, 性能优化".
6. 你是库专家, 充分发挥你的专业能力, 做好这件至关重要的事情, 让前期你辛苦完成的cp, 发扬光大!
7. 确保开启并三个子智能体,与主智能体一起工作, 主智能体不可空闲等待,也不可陷入长时间任务. 子智能体收回后立刻新发, 确保三个子智能体在线.
8. 每波吸收后, 更新文档, 及时git提交并push, 固化成果， 然后继续下一波, 不要停, 完成两个平台排名各前700包及其依赖包.
9. **子智能体轻量定时轮换规则（防上下文膨胀与 Token 激增）**：
   - 每个子智能体在连续完成 **6~8 个包**（或运行上下文显著增长）后，主智能体必须在交付当前包后立即调用 `manage_subagents(Action="kill")` 回收旧子智能体，并同步通过 `invoke_subagent` 派生全新的零历史子智能体顶替上岗；
   - 轮换期间始终保持 **3 路子智能体 + 1 路主智能体（共 4 路并行）** 满载运行，严禁长期让单一子智能体累积数百轮工具日志导致每轮重传海量 Token；
   - 若某子智能体长时间卡死无响应（如超过预期时间无进展），主智能体可主动检查其状态并果断 `kill` 重建新路，避免算力空转。
10. **每波闭环交付与 LIVE 发布铁律**：
    - 每波吸收完工后按序执行：① 更新 `packages/@cap/registry.json`（保持 npm / crates.io 1:1 均衡，补齐所有独立依赖映射）、`cp_lib_ref.md`、`CHANGELOG.md`、`DEVLOG.md`（日常严禁读写万行 `cp_ref.md`）；② 精确 `git add` + `git commit` + `git push origin main` 固化成果（严禁提交或泄露任何 API Token/凭据）；③ 后台异步触发 `https://registry.capdux.org` LIVE 发布并持续推进下一波，永不中断。
11. **首轮一遍过 CP 编码清单（每 3 小时审计固化 — 大幅降低试错 Token 消耗）**：
    - 严禁使用 `let mut`（CP `let` 默认可变）、`var`、`self.`、`this.`、`let _ =`（改用 `* = expr;` 丢弃返回值）、Zig 风格 `if (x) |v|`（改用 `if (v := x)`）、内联 `if (...) { a } else { b }` 表达式（先声明 `let n = a; if (...) { n = b; }`）以及 CP/Zig 保留标识符（`dec`、`from`、`align`）；
    - 结构体无 receiver 工厂方法一律声明 `pub static fn`，只读实例方法省略 `_`，变更方法声明 `*_`；严禁将 `.startsWith`/`.endsWith` 作为结构体方法名（会被 `cpc` 拦截为 `std.mem.startsWith`，改用 `hasPrefix`/`hasSuffix`）；结构体方法名/字段名不得与导入顶层函数重名；
    - 链式调用 `Type.staticFn(...).method(...)` 易导致局部类型推断退化，先绑定 `let x = Type.staticFn(...);` 再调用实例方法；返回固定数组 `[N]T` 先绑定局部类型变量再 `return`；不可变接收者切分 `[N]u8` 字段前先拷贝到本地 `let local: [N]u8 = _.field;`；避免 `*usize` 出参指针赋值（改用小结果结构体返回）；
    - 高位 `u64` 字面量（`>= 0x8000_0000_0000_0000`）用 `((hi as u64) << 32) | (lo as u64)` 构造，非负有符号除法先转 `u64`，无符号防下溢严禁 `0 - x`（改用 XOR 掩码）；
    - 单测统一使用 `fn main() { test("...", () => { expect_eq(...); }); }`，私有辅助函数一律加 3~4 字符包前缀防模块展平冲突。
    - **Token 极简工作流铁律**：① 批量合并独立 `write_file`/`edit_file`/`run_command` 调用，单轮完成 4 件套落盘 + 1 次命令完成 `cpc emit-types && cap test`；② 派发新任务前先用单条 shell `for p in ...; do test -d packages/@cap/$p ...` 批量验空防重复；③ 严禁在日常任务中读取万行 `cp_ref.md`；④ 定期跑 `python3 scripts/sync_session_audit.py --check` 保证 `SESSION.md` 零遗漏。
12. **生态溯源、检索对齐与防重复铁律（用户 2026-11-27 固化）**：
    - **表格化汇报**：阶段汇报一律使用结构化 Markdown 表格，清晰列出包名、原平台与原库名、分类决策（`lib/` 内部库 vs `@cap/*` 安装包 vs 合并）、独立依赖复用关系与单测结果。
    - **`cap.json` 必须记录参考来源**：每个吸收包必须在 `cap.json` 中包含标准 `"source": { "registry": "npm|crates.io", "name": "<orig>", "repo": "<url>", "license": "<spdx>" }` 字段。
    - **`capdux.org` 包介绍与 `README.md` 必须注明原库来源**：`README.md` 顶部与 `capdux.org` 详情页必须显式展示参考/吸收来源（平台、原库名、原仓库链接），特别是已改名的包。
    - **注册检索必须可按原库名检索并展示原库名**：`capdux.org` 的 `/v1/search` 与首页搜索框必须关联 `upstream_mappings` 表，支持直接搜索原库名/别名，并在检索结果卡片与 API 返回中标明 `source_registry` + `source_name`，方便人类与 AI 精准替换依赖项。
    - **吸收与依赖分离前的强制生态去重决策矩阵（严禁重复堆积）**：
      - 无论是目标包还是分离出来的独立依赖包，编码前**必须先检索 `lib/` 与 `packages/@cap/` 及 `registry.json`**；
      - ① **高频底层通用能力**（全生态大量引用）→ 优先纳入已有分类的内部标准库 `lib/<cat>/`（无需单独安装），在 `registry.json` 中指向 `cap:<cat>`；
      - ② **CP 生态已有相同/高度类似包** → **严禁另起 `-crate`/`-core`/`-http` 重复造轮子！** 优先直接复用现有包，或将缺失的高价值特性合并增强到现有包中；
      - ③ **CP 生态尚无对应能力且属独立生态包** → 新增 `@cap/<name>`，并在内部优先 `import` 复用 CP 生态已有的 `lib/*` 或 `@cap/*` 同功能库，避免选择困难。


### 6.9 第 9 轮 3 小时效率与 Token 审计固化铁律（2026-11-27 14:30）

1. **管道测试必须开启 `set -eo pipefail`**：严禁将 `cap/bin/cap test <file> | tail -n 5` 直接拼接执行而不设 `pipefail`；否则 `tail` 返回 0 会掩盖测试失败退出码，导致误提交后再返工 1~2 轮。
2. **全仓体检先定位编译器根因，再决定改源码**：面对大批量（>10 处）相似静态报错（如本轮 904 包 `cpc check` 发现 46 例同类报错），严禁逐包打绕过补丁；先用 10 秒 Python 脚本归类错误码，定位为 `cpc/src/sema/` 6 类假阳性后在 `cpc_dev/` 集中修复 5 个文件，一次性实现 **904/904 (100%)** 生态包与 **2,626/2,626 (100%)** 纯 CP 标准库全绿，节省 ~15 万 Token。
3. **16-lane SIMD / `comptime` LUT / L0 标准库一遍过避坑**：
   - `@Vector(16, u8)` 不支持运行时动态下标，归约前必须先 `const a: [16]u8 = acc;` 转数组；
   - `comptime` 256-entry 建表函数开头须加 `@setEvalBranchQuota(20000);`；
   - `cap:fmt.writeStr(s, out, pos)` 参数顺序 `s` 在首位；`cap:bin.readU32LE/readU64LE` 返回 `?u32`/`?u64`；
   - `cap brief <pkg|cap:stdlib>` 已统一覆盖 904 生态包 + 208 内置标准库通道，查库直接调用 `cap brief` 替代整文件读取。
4. **批量代码改写工具必须限定语法上下文**：`cap fix` 重写旧 `@cap/<pkg>` 导入时必须限定匹配 `from "@cap/` 而非裸 `"@cap/`，彻底杜绝误伤测试字符串字面量与返工。
