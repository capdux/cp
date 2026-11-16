# CP 变更日志 (CHANGELOG) - 2026-11-16 (热数据)

> 历史变更日志已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本项目采用冷热分离架构，本文件仅保留当日活跃记录。

## [2026-11-16] - 核心里程碑：Spec 5 零堆分配流式直写与 Spec 8 跨协程静态隔离检查 E0305 实装

### 编译器新特性与优化 (Compiler Features & Optimizations)
- **零分配模板字符串流式直写 (Spec 5)**:
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：实现 `lower_template_stream`，为 `print`、`println`、`@print`、`writer.write`、`writer.print` 等提供流式直写 Lowering，彻底跳过 `allocPrint` 堆分配，常量内插编译期静态折叠，动态参数包覆 `cp_fmt` 直写底层流。
- **跨协程/异步边界静态隔离检查 E0305 (Spec 8)**:
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：引入 `local_borrow_vars` 追踪与 `check_async_boundary_arg` 检查；拦截 `spawn`、`@spawn`、`spawn.cpu`、`send` 边界传递局部借用指针 `*T`、`&T` 或裸切片，报出三段式 AI-First 智能诊断 `error[E0305]`。

### 双轨规范与文档 (Documentation & Specs)
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：在第 20.11 节增加第 10 小节（流式直写优化）与第 11 小节（跨协程隔离检查 E0305）。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：升级为 Top 29 绝对禁手清单，登记 #29 条（跨协程传递局部借用拦截）。

### 测试验证 (Tests)
- [`cap/test/test_stream_template_and_isolation.cp`](file:///data/dev/cp/cap/test/test_stream_template_and_isolation.cp)：Spec 5 流式直写与 Spec 8 安全并发 100% PASS。
- [`cap/test/test_parallel_fail_borrow.cp`](file:///data/dev/cp/cap/test/test_parallel_fail_borrow.cp)：精准触发 E0305 拦截。
- `test_all.cp` 回归验证：100% PASS。

---

## [2026-11-16] - 核心里程碑：LLVM libFuzzer 驱动接入、AddressSanitizer 内存检查与 cap 模糊测试指令矩阵 (Spec 12 第二部分)

### 编译器与构建工具链 (Compiler & Toolchain)
- **libFuzzer C-ABI 标准符号导出与构建模式**:
  - [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs)：新增 `--fuzz` 与 `--fuzz-target <name>` 编译命令行选项，支持环境标识 `CP_FUZZ`；
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：在 `--fuzz` 模式下抑制普通 `pub fn main` 生成，提取所有 `fuzz` 目标块，导出标准 C-ABI 入口函数 `export fn LLVMFuzzerTestOneInput(data: [*c]const u8, size: usize) callconv(.c) c_int`；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：登记所有 `fuzz` 目标块上下文，支持单独隔离为函数并安全分发。
- **构建工具端 `cap` 命令行指令矩阵与两阶段插桩链接**:
  - [`cap/src/main.zig`](file:///data/dev/cp/cap/src/main.zig)：
    - 扩展 `cap test` 支持 `--fuzz`（及一等子命令 `cap fuzz [target]`）；
    - 采用 `zig build-obj` + `clang -fsanitize=fuzzer,address` 两阶段构建，无缝引入 Coverage-guided 变异与 ASan 内存安全检查；
    - 支持可选参数 `--timeout <sec>` 与 `--jobs <n>` 控制执行时长与并发 Worker；
    - 支持崩溃复现指令 `cap test --repro <crash_file.bin>`，单次喂入输入文件，秒级触发崩溃并回溯堆栈现场；
    - `usage()` 帮助文档同步增补 Fuzz Options 指引。

### 双轨规范与文档 (Documentation & Specs)
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：在第 20.11 节补充 libFuzzer C-ABI 导出与 `cap test --fuzz`、`--repro` 官方规范。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：更新原生代码块矩阵中的 `fuzz` 块契约描述。

### 测试验证 (Tests)
- [`cap/test/test_fuzz_driver.cp`](file:///data/dev/cp/cap/test/test_fuzz_driver.cp)：新增专用模糊测试与崩溃复现器验证用例；
- 实测验证：
  - 常规测试模式 `cap test`: 100% PASS；
  - libFuzzer 模式 `cap test --fuzz`: 100% PASS (1000 轮动态变异)；
  - 超高吞吐测试 `cap fuzz --timeout 1`: 100% PASS (1 秒 220 万次执行，110 万 exec/s)；
  - 崩溃复现 `cap test --repro temp/crash_sample.bin`: 精准复现 panic 与堆栈定位。

---

## [2026-11-16] - 核心里程碑：Fuzz 模糊测试块、函数级 @arena 内存区域修饰器与字段级 @json 注解定制

### 编译器新特性与语法扩展 (Compiler Features)
- **一级 `fuzz` 模糊测试块 (Spec 12.2)**:
  - [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs)：新增保留字 `Token::Fuzz`；
  - [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) & [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs)：新增 `FuzzDecl` 声明与 AST 节点；
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：支持顶层与模块项 `fuzz <name> (<param>) { ... }` 语法解析；
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：语义分析自动将变异样本形参绑定为 `[]const u8`；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)、[`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs)、[`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：发射集成 16+ 高危边界样本语料库（Corpus）与 100 轮动态切片变异驱动循环的 `test "fuzz: <name>"`。
- **函数级 `@arena` 作用域内存区域修饰器 (Spec 7)**:
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：在 `parse_meta_ident` 映射 `Token::Arena` 为 `"arena"` 注解；
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：元注解校验增加 `@arena` 函数修饰器豁免；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：在函数序言处初始化栈 Arena 并压入活跃 Arena 作用域栈，退出时自动注入 `defer __fn_arena.deinit()`，使函数内局部临时分配离开时 $O(1)$ 瞬时整块释放，实现零堆碎片与强异常安全。
- **字段级 `@json("alias")` 与 `@json_ignore` 序列化定制 (Spec 6)**:
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：重构 `derive_json_method`，跳过标注 `@json_ignore` 的字段，提取 `@json("alias")` 别名作为 JSON 键，无有效字段时输出 `"{}"`。

### 双轨规范与文档同步 (Documentation & Specs)
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：在第 20.2 节补充函数级 `@arena`，并在第 20.11 节增补字段级 JSON 定制、函数级 `@arena` 与 `fuzz` 模糊测试块权威规范。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：扩充原生代码块矩阵至五大代码块（引入 `fuzz` 块），并同步更新字段级 `@json`、函数级 `@arena` 的精炼可运行契约示例。

### 测试验证 (Tests)
- [`cap/test/test_fuzz_arena_json.cp`](file:///data/dev/cp/cap/test/test_fuzz_arena_json.cp)：字段别名重命名、字段忽略过滤、函数级 `@arena` 作用域计算与 `fuzz` 驱动验证 100% PASS。
- `test_all.cp` 回归验证：100% PASS。

---

## [2026-11-16] - 核心里程碑：Phase 2 人体工程学语法糖实装、契约式设计、SIMD原语与双轨规范同步

### 编译器人体工程学与黑科技语法 (Compiler Ergonomics & Contracts)
- **空值合并赋值 (`??=`)**:
  - [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs) & [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：支持 `Token::QuestionQuestionAssign` 复合赋值解析；
  - [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs)：Lowering 为 `if (lhs == null) { lhs = rhs; }`，实现零堆开销天然短路求值。
- **契约式设计注解 (`@pre` 与 `@ret`)**:
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：`parse_meta_arg` 扩展二元断言表达式解析；支持在函数签名后尾随注解 `@pre(cond)` 和 `@ret(cond)`；
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：元注解校验对 `@pre` 和 `@ret` 豁免静态常量检查；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：在函数入口自动发射前置条件断言 `std.debug.assert(cond)`；
  - [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs)：在函数体各 `return` 返回处捕获返回值 `ret` 并发射后置条件断言 `std.debug.assert(cond)`。
- **万能降级解包 `?` 多行块与复合类型 `(?T)!` 双重解包**:
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：`?` 降级操作符全面支持后接代码块 `{ ... }`（`Expr::LabeledBlock`）；
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs) & [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：对可空且可能报错的复合类型 `(?T)!` 自动发射双重链式解包（先 `catch` 再 `orelse`），若成功直接解包出底层基型 `T`。
- **内建定长 SIMD 向量类型别名**:
  - [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs) & [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：内置 `u8x16`、`u8x32`、`u8x64`、`i16x8`、`i16x16`、`i32x4`、`i32x8`、`u32x4`、`u32x8`、`f32x4`、`f32x8`、`f32x16`、`f64x2`、`f64x4` 映射至底层 `@Vector(N, T)`，原生支持标量广播 `@splat` 与向量算术运算（`+`, `-`, `*`, `/`）。
- **派生宏全家桶扩展 (`@[Default]`)**:
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：登记并实装 `derive_default_method`，为结构体/类自动派生全零值初始化静态工厂 `pub static fn default(): Self`。
- **切片属性语法糖消歧保护 (`.slice`)**:
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：将 `.slice` 改为白名单判定（仅对字符串/数组/切片生效），保留用户自定义类型的同名 `.slice()` 成员方法。

### 双轨规范与文档同步 (Documentation & Specs)
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：新增第 20.11 小节，全面收录 Phase 2 人体工程学语法糖、契约式设计、SIMD 原语与派生宏规范。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：同步精简更新标量向量类型、`??=` 复合赋值、`@pre`/`@ret` 契约注解、`? { ... }` 万能降级与双重解包、`@[Default]` 派生宏。

### 测试验证 (Tests)
- [`cap/test/test_phase2_ergonomics.cp`](file:///data/dev/cp/cap/test/test_phase2_ergonomics.cp)：6 大特性全量端到端验证通过（100% PASS）。
- 回归测试：`test_all.cp` 验证 100% 通过。

---

## [2026-11-16] - 核心里程碑：Bun 高价值资产全面吸收、图像子系统（PNG截图/BMP/量化）实装与标准库全景扩展

### 图像子系统实装与截图引擎 (Added `cap:image` & `cap:png`)
- **纯 CP PNG 编解码器 (`png.cp`)**:
  - [`lib/image/png.cp`](file:///data/dev/cp/lib/image/png.cp)：为 `capsurf_agy` 网页截图提供专属引擎；
  - `Png.encodeRgba(rgba, width, height)`：支持标准 8 字节魔数签名、IHDR Chunk、扫描线 None 滤波（0x00 前缀）、Zlib 原生 Deflate 流式压缩生成 IDAT、IEND 闭合与 CRC-32 校验和；
  - `Png.decode(bytes)`：支持 IHDR 元数据解析、IDAT 聚合解压、5 种滤波逆运算（None, Sub, Up, Average, Paeth）像素还原；
  - `PngImage` 与 `RgbaColor` 强类型封装，支持坐标级读取与变异；
  - `Png.save(path, rgba, w, h)` 与 `Png.load(path)` 同步文件 I/O 管道。
- **图像格式嗅探与 MIME 推导 (`codecs.cp`)**:
  - [`lib/image/codecs.cp`](file:///data/dev/cp/lib/image/codecs.cp)：8 大图片格式（PNG/JPEG/WebP/BMP/GIF/TIFF/AVIF/HEIC）二进制魔数探测、MIME 字符串映射与文件名后缀推导。
- **Windows BMP DIB 编解码 (`bmp.cp`)**:
  - [`lib/image/bmp.cp`](file:///data/dev/cp/lib/image/bmp.cp)：支持 24/32 位 BMP 文件解码与 32 位 BMP 文件编码，包含 4 字节跨距行对齐转换。
- **纯 CP GIF89a/87a LZW 解码器 (`gif.cp`)**:
  - [`lib/image/gif.cp`](file:///data/dev/cp/lib/image/gif.cp)：4096 节点 LZW 字典流式解压、Logical Screen Descriptor 探测、全局/局部调色板映射、4-pass 交织扫描还原与透明色索引解析。
- **Evan Wallace ThumbHash 模糊占位图算法 (`thumbhash.cp`)**:
  - [`lib/image/thumbhash.cp`](file:///data/dev/cp/lib/image/thumbhash.cp)：将 RGBA8 像素压缩为 ≤25 字节紧凑哈希，并支持逆向解码为 32x32 模糊平滑占位图。
- **EXIF 方向解析与变换矩阵 (`exif.cp`)**:
  - [`lib/image/exif.cp`](file:///data/dev/cp/lib/image/exif.cp)：快速解析 JPEG APP1 / TIFF IFD0 0x0112 旋转标签，映射生成 `ExifTransform` 旋转与镜像动作。
- **高阶图像处理管线 (`pipeline.cp`)**:
  - [`lib/image/pipeline.cp`](file:///data/dev/cp/lib/image/pipeline.cp)：轻量结构体 `Image`，支持 `.flop()`、`.flip()`、`.rotate(90/180/270)`、`.crop(x, y, w, h)`、`.resize(w, h)` 双线性缩放及 `.thumbhash()`。
- **JPEG 与 WebP 容器探测 (`jpeg.cp`, `webp.cp`)**:
  - [`lib/image/jpeg.cp`](file:///data/dev/cp/lib/image/jpeg.cp)：SOF0/SOF2 帧头基准尺寸、分量与渐进式检测；
  - [`lib/image/webp.cp`](file:///data/dev/cp/lib/image/webp.cp)：RIFF WEBP VP8/VP8L/VP8X 容器头嗅探、透明度与动画标志提取。
- **全量标准库 Bun 溯源注释注入 (Provenance Headers)**:
  - 在 `lib/` 下 52 个从 Bun 吸收的文件头部，统一规范注入 `// Absorbed from Bun: /data/dev/bun/src/...` 与 `// Upstream Sync:` 溯源注释。
- **调色板颜色量化器 (`quantize.cp`)**:
  - [`lib/image/quantize.cp`](file:///data/dev/cp/lib/image/quantize.cp)：基于 Median-Cut 算法实现 RGBA 像素向 256 色紧凑调色板与索引图转换。
- **统一聚合出口 (`index.cp`)**:
  - [`lib/image/index.cp`](file:///data/dev/cp/lib/image/index.cp)：门面统一导出 `cap:image`、`cap:png`、`cap:img` 协议全部 10 大子模块。

### Bun 高价值资产吸收与标准库扩展 (Added Stdlib)
- **容器集合库 (`cap:col`)**:
  - [`lib/col/hash_map.cp`](file:///data/dev/cp/lib/col/hash_map.cp)：开放寻址双重散列 Robin Hood 哈希表，配合 `bit_set.cp`, `bounded_array.cp`, `hive_array.cp`, `object_pool.cp` 形成完整工业级集合体系。
- **高性能字符串库 (`cap:string`)**:
  - [`lib/string/smol.cp`](file:///data/dev/cp/lib/string/smol.cp)：24 字节 SSO 小字符串（小于 23 字节纯栈上分配，超长自动堆回退）；
  - [`lib/string/hashed.cp`](file:///data/dev/cp/lib/string/hashed.cp)：内联 Wyhash 预计算的快速比较字符串对象；
  - [`lib/string/joiner.cp`](file:///data/dev/cp/lib/string/joiner.cp) 与 [`lib/string/path.cp`](file:///data/dev/cp/lib/string/path.cp)。
- **线程并发同步库 (`cap:sync`)**:
  - [`lib/sync/pool.cp`](file:///data/dev/cp/lib/sync/pool.cp)：`Guarded<T>` 互斥锁作用域保护包装类与 `WorkPool` 多线程工作者队列；
  - 深度重构 `arc_slice.cp`, `concurrent_map.cp`, `condition.cp`, `futex.cp`, `mutex.cp`, `ring_buffer.cp`, `unbounded_queue.cp`, `wait_group.cp`。
- **动态事件与异步任务库 (`cap:event`)**:
  - [`lib/event/task.cp`](file:///data/dev/cp/lib/event/task.cp)：`AnyTask` 与 `ManagedTask` 动态多态任务调度与状态机切分。
- **轻量嵌入式 SQL 数据库 (`cap:sql`)**:
  - [`lib/sql/sqlite.cp`](file:///data/dev/cp/lib/sql/sqlite.cp)：内存数据库与持久化 SQLite 引擎封装。
- **内嵌微型脚本引擎 (`cap:shell`)**:
  - [`lib/shell/lexer.cp`](file:///data/dev/cp/lib/shell/lexer.cp), [`lib/shell/parser.cp`](file:///data/dev/cp/lib/shell/parser.cp), [`lib/shell/eval.cp`](file:///data/dev/cp/lib/shell/eval.cp), [`lib/shell/index.cp`](file:///data/dev/cp/lib/shell/index.cp)。

### 编译器与构建增强 (Compiler & Build)
- [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：注册 `"image" | "img" | "png" => "image"`、`"sync" | "threading" => "sync"`、`"shell" | "sh" => "shell"` 模块路径协议映射。
- [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs)：结构体字段默认值表达式扫描、位移检测、异步内建指令预加载标志修复。
- [`cap.json`](file:///data/dev/cp/cap.json)：声明链接动态库 `"sqlite3"` 与 `"z"`。

### 测试套件验证 (Tests)
- [`cap/test/test_image_png.cp`](file:///data/dev/cp/cap/test/test_image_png.cp)：6/6 PASS (100%)。
- [`cap/test/test_bun_absorbed_assets.cp`](file:///data/dev/cp/cap/test/test_bun_absorbed_assets.cp)：6/6 PASS (100%)。
- [`cap/test/test_collections_bun.cp`](file:///data/dev/cp/cap/test/test_collections_bun.cp)：6/6 PASS (100%)。
- [`cap/test/test_sqlite.cp`](file:///data/dev/cp/cap/test/test_sqlite.cp)：PASS (100%)。

---

## [2026-11-13] - 核心里程碑：安全防护与 CSRF Token 体系（cap:security）实装及基于 5 步法的 cp_agent_ref.md 全量重构

### 安全防护体系实装 (Added)
- **CSRF 防护核心库 (`csrf.cp`)**:
  - [`lib/security/csrf.cp`](file:///data/dev/cp/lib/security/csrf.cp)：基于 CSPRNG 随机 Nonce、HMAC-SHA256 签名校验、时钟漂移防重放，以及 `constantTimeEql` 恒定时间缓冲区对比的 CSRF Token 生成与验证；
  - 支持 `Base64Url`、`Base64` 与 `Hex` 三种工业编码格式。
- **模块重导出与单一出口 (`index.cp`)**:
  - [`lib/security/index.cp`](file:///data/dev/cp/lib/security/index.cp)：对外统一导出 `CSRF`、`TokenFormat`、`DEFAULT_EXPIRATION_MS`、`constantTimeEql`。

### 智能体高密度语法契约重构 (Agent Specification)
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md)：
  - 严格采纳用户 5 步法建议，从权威 `cp_ref.md` 提取单一真理来源，剔除全部历史原理、哲学论据与多语言评测（减重 80%）；
  - 保留纯正 CP 语法与精炼可运行示例（Canonical Examples）；
  - 彻底清除 Zig 语法渗漏（`try`、`for()|i|`、`and/or`、`!T`/`?T`、`Point{ .x = 1 }`、`&.{}` 等）；
  - 与编译器 AST/Parser/Sema 源码核对并经微型综合测试脚本编译验证通过（100% PASS）；
  - 结尾增设《Top 15 绝对禁手清单》。

### 编译器增强与热修自愈 (Compiler & Hotfix)
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 增强 `Type::Ref` 具名结构体解构赋值支持，自动解析结构体定义字段。
- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs) & [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs):
  - 修复未初始化栈缓冲区（`undefined`）切片（`raw[0..]`）与索引赋值（`raw[i] = ...`）误触 `uninitialized_vars` 读取拦截的缺陷。
- [`cpc/src/codegen/pattern.rs`](file:///data/dev/cp/cpc/src/codegen/pattern.rs) & [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs):
  - 修复常量模式解构 Rest 剩余绑定（`const [head, ...tail] = ...`）中不可变切片类型推断与闭包状态机上下文中的类型对齐（`[]const T`）。

### 标准库强化 (Library Updates)
- [`lib/crypto/hmac.cp`](file:///data/dev/cp/lib/crypto/hmac.cp)：新增零堆分配的高性能原生切片签名函数 `hmacSha256Raw(out, key, data)`。
- [`lib/crypto/random.cp`](file:///data/dev/cp/lib/crypto/random.cp)：新增原生直接就地填充随机字节函数 `fillRandom(buf)`。
- [`lib/encoding/base64.cp`](file:///data/dev/cp/lib/encoding/base64.cp)：全面升级为纯正面向对象 `export class Base64`，规范纯正 CP 返回类型与切片位移运算。

### 测试验证 (Tests)
- [`cap/test/test_security_csrf.cp`](file:///data/dev/cp/cap/test/test_security_csrf.cp)：4/4 端到端安全测试 100% PASS。
- 全量回归测试：`test_event_loop.cp`, `test_crypto_features.cp`, `test_encoding.cp`, `test_sync_primitives.cp` 全部 100% PASS。


## [2026-11-13] - 核心里程碑：工业级现代密码学体系（cap:crypto）建设、编译器枚举方法与解构自愈

### 工业级密码学体系实装 (Added)
- **真随机数与 CSPRNG (`random.cp`)**:
  - [`lib/crypto/random.cp`](file:///data/dev/cp/lib/crypto/random.cp)：直通 Linux 内核 `getrandom` 熵池的 CSPRNG，实现 RFC 4122 v4 `randomUUID()` 与无偏高精度 `randomInt(min, max)`。
- **散列哈希与流式增量散列 (`hash.cp`)**:
  - [`lib/crypto/hash.cp`](file:///data/dev/cp/lib/crypto/hash.cp)：`sha256`, `sha512`, `sha1`, `md5` 单次快速哈希计算；面向对象的流式 `Hash` 类，支持 `.update(data)` 增量注入与 `.digest()` 最终十六进制输出。
- **HMAC 消息认证码 (`hmac.cp`)**:
  - [`lib/crypto/hmac.cp`](file:///data/dev/cp/lib/crypto/hmac.cp)：RFC 2104 `hmacSha256(key, data)` 与流式 `Hmac` 类。
- **PBKDF2 口令派生密钥 (`pbkdf2.cp`)**:
  - [`lib/crypto/pbkdf2.cp`](file:///data/dev/cp/lib/crypto/pbkdf2.cp)：`pbkdf2Sync`（返回十六进制字符串）与 `pbkdf2Raw`（直接安全填充目标字节数组）。
- **现代 AEAD 认证对称加密 (`cipher.cp`)**:
  - [`lib/crypto/cipher.cp`](file:///data/dev/cp/lib/crypto/cipher.cp)：现代工业标准 `Aes256Gcm`（NIST SP 800-38D）与 `ChaCha20Poly1305`（RFC 8439），兼具保密性加密与带 16 字节认证标签的防篡改解密校验。
- **模块重导出与单一出口 (`index.cp`)**:
  - [`lib/crypto/index.cp`](file:///data/dev/cp/lib/crypto/index.cp)：对外统一规范重导出全部密码学符号。
- **废弃平铺单文件清理 (Removed)**:
  - 彻底删除旧版平铺单文件 `lib/crypto.cp`，消除同名文件与复合目录歧义。

### 编译器增强与热修自愈 (Compiler & Hotfix)
- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs) / [`cpc_dev/src/codegen/expr.rs`](file:///data/dev/cp/cpc_dev/src/codegen/expr.rs):
  - 修复枚举静态方法调用被误降级为结构体/联合体初始化的关键缺陷：在 `Expr::Call` 访问枚举成员时，严格校验 `is_method || !is_variant`，若为方法或非变体则生成标准函数调用 `Enum.method(...)`，杜绝非法 Zig 语法 `Enum{ .method = arg }`。
- [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs):
  - 修复类方法接收者 `self: *T` 在生成参数列表时过滤掉 `first.ident.sym == "self"` 并保留其可变指针类型（`*T` 而非 `*const T`）。
- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs):
  - 适配 Zig 0.17 结构体解包，将 `.@"struct".fields` 升级为 `.field_names` 和 `.field_types`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / `task-sema-object-tuple-destructure`:
  - 修复匿名结构体位置字段在推断为 `Type::Object` 时被丢弃的缺陷，打通 `const [a, b] = pair` 对匿名对象的解构模式分析，结单归档下游任务。

### 测试验证 (Tests)
- [`cap/test/test_crypto_features.cp`](file:///data/dev/cp/cap/test/test_crypto_features.cp)：涵盖 CSPRNG/UUID、SHA256/512/1/MD5/Streaming、HMAC/PBKDF2、AES-GCM/ChaCha20-Poly1305 4 大端到端套件，100% PASS。
- [`cap/test/test_object_destructure.cp`](file:///data/dev/cp/cap/test/test_object_destructure.cp)：匿名结构体元组解构测试，100% PASS。
- 全量回归测试：`test_http_full.cp`, `test_sync_primitives.cp`, `test_zero_copy_json.cp`, `test_io_streams.cp`, `test_path_url.cp` 等全部 100% PASS。

---

## [2026-11-13] - 核心里程碑：全面收编 Bun 工业级 HTTP 全协议栈引擎（Step 1/2/3 闭环）与底层编译器自愈

### 工业级 HTTP 核心体系完整迁入 (Added)
- **Step 1: 协议类型与基础设施助手 (Types & Infrastructure)**:
  - [`lib/http/encoding.cp`](file:///data/dev/cp/lib/http/encoding.cp)：`Encoding` 枚举（`Identity`, `Gzip`, `Deflate`, `Brotli`, `Zstd`, `Chunked`）与 `canUseLibDeflate()`、`isCompressed()`；
  - [`lib/http/fetch_modes.cp`](file:///data/dev/cp/lib/http/fetch_modes.cp)：`FetchCacheMode`, `FetchRedirect`, `FetchRequestMode` 及其字符串双向映射；
  - [`lib/http/cert.cp`](file:///data/dev/cp/lib/http/cert.cp)：`HTTPCertError`, `CertificateInfo`, `InitError`；
  - [`lib/http/url_path.cp`](file:///data/dev/cp/lib/http/url_path.cp)：`URLPath` 极速路径与参数切片分析器（`extname`, `path`, `pathname`, `first_segment`, `query_string`, `isRoot()`）；
  - [`lib/http/signals.cp`](file:///data/dev/cp/lib/http/signals.cp)：`Signals` 与 `SignalStore` 原子生命周期状态存储器；
  - [`lib/http/header_iterator.cp`](file:///data/dev/cp/lib/http/header_iterator.cp)：`HeaderValueIterator` 逗号分隔报头零拷贝切片迭代器；
  - [`lib/http/request_body.cp`](file:///data/dev/cp/lib/http/request_body.cp)：`HTTPRequestBody` 与 `HTTPRequestBodyTag` 请求体封装；
  - [`lib/http/internal_state.cp`](file:///data/dev/cp/lib/http/internal_state.cp)：`HTTPVerboseLevel`, `HTTPUpgradeState`, `Protocol`, `ShouldContinue`, `Stage`, `InternalStateFlags`, `InternalState` 客户端状态机。
- **Step 2: HTTP/2 二进制帧与 HTTP/3 客户端子系统 (H2 & H3 Subsystems)**:
  - [`lib/http/h2/frame.cp`](file:///data/dev/cp/lib/http/h2/frame.cp)：RFC 7540 HTTP/2 二进制帧协议（`CLIENT_PREFACE`, `FrameType`, `ErrorCode`, `SettingsType`, `FrameHeader` 9 字节帧头编解码, `SettingsPayloadUnit`）；
  - [`lib/http/h2/client.cp`](file:///data/dev/cp/lib/http/h2/client.cp)：`H2Stream`, `H2StreamState`, `H2ClientSession` 多路复用流会话与窗口控制器；
  - [`lib/http/h2/index.cp`](file:///data/dev/cp/lib/http/h2/index.cp)：HTTP/2 子系统统一聚合导出；
  - [`lib/http/h3/alt_svc.cp`](file:///data/dev/cp/lib/http/h3/alt_svc.cp)：RFC 7838 Alt-Svc 协商报头解析器（`AltSvc`, `AltSvcEntry`）；
  - [`lib/http/h3/client.cp`](file:///data/dev/cp/lib/http/h3/client.cp)：`H3Stream`, `H3StreamType`, `H3ClientSession` QUIC 双向请求流会话；
  - [`lib/http/h3/index.cp`](file:///data/dev/cp/lib/http/h3/index.cp)：HTTP/3 子系统统一聚合导出。
- **Step 3: 工业级核心客户端引擎与生命周期 (Client Engine & Lifecycle)**:
  - [`lib/http/decompressor.cp`](file:///data/dev/cp/lib/http/decompressor.cp)：`DecompressorType`, `Decompressor` 流式解压缩适配器；
  - [`lib/http/proxy.cp`](file:///data/dev/cp/lib/http/proxy.cp)：RFC 7231 `ProxyTunnel` HTTP CONNECT 代理隧道握手与校验器；
  - [`lib/http/sendfile.cp`](file:///data/dev/cp/lib/http/sendfile.cp)：基于 Linux 内核 `sendfile` 的零用户态拷贝文件上传器；
  - [`lib/http/context.cp`](file:///data/dev/cp/lib/http/context.cp)：`HTTPContext`, `PooledSocket` Keep-Alive 跨域套接字连接池与死链接驱逐；
  - [`lib/http/client.cp`](file:///data/dev/cp/lib/http/client.cp)：吸收自 Bun 2843 行工业核心的 `HTTPClient`, `HTTPClientResult`，完整实现 RFC 7230 请求格式化、分块传输解码（`decodeChunkedBody`）与状态解析；
  - [`lib/http/index.cp`](file:///data/dev/cp/lib/http/index.cp)：更新聚合导出全套 HTTP 工业能力。

### 编译器增强与热修自愈 (Compiler & Hotfix)
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 增强类型系统，打通 `Type::String` 与 `Type::Array(Box::new(Type::U8))` / `Type::SentinelSlice`（`[]const u8`）的自动安全双向转换，解决字符串字面量赋值给字节切片字段的类型不匹配阻断。
- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs) / [`cpc_dev/src/codegen/mod.rs`](file:///data/dev/cp/cpc_dev/src/codegen/mod.rs):
  - 增强 `lower_struct_body`：原生支持将 CP 原始指针类型 `*!?` 转换为 `?*anyopaque`、`*!` 转换为 `*anyopaque`，将函数类型 `?fn(...)` 转换为 `?*const fn(...)`、`fn(...)` 转换为 `*const fn(...)`。
- [`scripts/sync_cpc.sh`](file:///data/dev/cp/scripts/sync_cpc.sh):
  - 同步部署更新 `/data/dev/cp/cap/bin/cpc` 与 `/data/dev/cp/cpc/target/release/cpc`，保证 `cap` 工具链始终调用最新构建的编译器。
- 下游 Bun AGY 阻断结单：
  - 极速排查并解决任务 `task-struct-body-lower-rawptr-fn`，测试验证通过，结单归档。

### 测试验证 (Tests)
- [`cap/test/test_http_full.cp`](file:///data/dev/cp/cap/test/test_http_full.cp)：全新端到端全量综合测试套件，覆盖 Step 1, Step 2, Step 3 所有原语与流程，100% PASS。
- [`cap/test/test_http_features.cp`](file:///data/dev/cp/cap/test/test_http_features.cp)：现有基础套件 100% PASS。
- [`cap/test/test_sync_primitives.cp`](file:///data/dev/cp/cap/test/test_sync_primitives.cp)：现代化去除废弃 `try` 语法，8 大并发套件 100% PASS。
- 全量标准库回归：14 个标准库核心测试套件 100% PASS。

---

## [2026-11-12] - 架构清理：全面删除根目录冗余门面文件、规范单一真理来源

### 标准库清理与重构 (Refactored & Removed)
- **彻底删除 14 个根目录同名空壳门面文件**：
  - 删除 `lib/io.cp`, `lib/sync.cp`, `lib/router.cp`, `lib/http.cp`, `lib/path.cp`, `lib/url.cp`, `lib/encoding.cp`, `lib/glob.cp`, `lib/semver.cp`, `lib/dotenv.cp`, `lib/ini.cp`, `lib/which.cp`, `lib/ws.cp`, `lib/json.cp`；
  - 杜绝“既有目录又有同名门面”的双重文件形态，所有多文件复合库一律以 `lib/<module>/index.cp` 作为唯一入口，根目录保持纯正扁平清爽。
- [`lib/http/`](file:///data/dev/cp/lib/http/):
  - 新增 `request.cp`：`Request` 类定义；
  - 新增 `response.cp`：`Response` 类定义；
  - 新增 `server.cp`：`Server`、`fetch`、`json`、`serve` 服务与客户端函数；
  - 更新 `index.cp` 统一对外公开导出全部 HTTP 契约与能力。
- 清理空目录：移除 `lib/io_ext` 与 `lib/json_ext`。

### 编译器修复 (Fixed)
- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs) / [`cpc_dev/src/codegen/expr.rs`](file:///data/dev/cp/cpc_dev/src/codegen/expr.rs):
  - 修复 `r#"..."#` 原生字符串中误将 `@import("std")` 写为 `\"std\"` 导致生成的 Zig 语法中带有非法反斜杠的问题。

### 规范手册更新 (Docs)
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：
  - 第 1.1 与 1.2 节全面更新标准库组织规范：明确复合模块由 `lib/<name>/index.cp` 独立负责，杜绝根目录堆积多余门面。

## [2026-11-12] - 阶段审查：原生 Re-export 落地、lib/io 目录化重构与全标准库规范化审计

### 新特性与编译器增强 (Added & Compiler)
- [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs) / [`cpc_dev/src/ast.rs`](file:///data/dev/cp/cpc_dev/src/ast.rs):
  - 在 `ModuleItem` 枚举中新增 `Export(ExportStmt)`，支持记录导出符号列表与可选来源路径 `source`。
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 增加 `parse_export_stmt` 与 `parse_export_star_stmt`，完整支持 `export { foo, bar } from "./path";`、`export { foo, bar };` 与 `export * from "./path";`。
- [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) / [`cpc_dev/src/resolver.rs`](file:///data/dev/cp/cpc_dev/src/resolver.rs):
  - 提取公共 `resolve_source_path` 模块路径解析函数；在 `resolve_dependencies` 与 `resolve_imports` 中对 `ModuleItem::Export` 的来源文件递归展开解析与依赖收集。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 语义分析符号表中对 `ModuleItem::Export` 导出标识符进行符号声明登记与合法性校验。
- [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs):
  - 在生成运行时预加载时扫描 `ModuleItem::Export`，确保重导出的内部能力得到正确支持。

### 标准库重构与规范化 (Refactored)
- [`lib/io/`](file:///data/dev/cp/lib/io/) & [`lib/io.cp`](file:///data/dev/cp/lib/io.cp):
  - 严格遵循 `xxx_0.cp` 演进规则，将原单文件暂存为 `lib/io_0.cp`，创建模块化目录 `lib/io/`；
  - 拆分为 `traits.cp`、`buffer_stream.cp`、`stdio.cp`、`max_buf.cp`、`pipe.cp` 以及高内聚出口 `index.cp`；
  - 100% 覆盖原功能后安全移除 `lib/io_0.cp`，重写根目录 `lib/io.cp` 门面为规范的 `export * from "./io/index";`。
- 全量标准库重导出语法统一改造：
  - 更新 `lib/sync/index.cp` & `lib/sync.cp`
  - 更新 `lib/router/index.cp` & `lib/router.cp`
  - 更新 `lib/http/index.cp` & `lib/http.cp`
  - 更新 `lib/path/index.cp` & `lib/path.cp`
  - 更新 `lib/url/index.cp` & `lib/url.cp`
  - 更新 `lib/encoding/index.cp` & `lib/encoding.cp`
  - 更新 `lib/glob/index.cp` & `lib/glob.cp`
  - 更新 `lib/semver/index.cp` & `lib/semver.cp`
  - 更新 `lib/dotenv/index.cp` & `lib/dotenv.cp`
  - 更新 `lib/ini/index.cp` & `lib/ini.cp`
  - 更新 `lib/which/index.cp` & `lib/which.cp`
  - 更新 `lib/ws/index.cp` & `lib/ws.cp`
  - 更新 `lib/json/index.cp` & `lib/json.cp` & `lib/serde.cp`
  - 更新 `lib/stream.cp`
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：
  - 增加第 1.1 节重导出规范与门面模式使用指南。

## [2026-11-12] - 标准库第五阶段：可执行程序定位器、WebSocket 帧协议与文件系统原子增强

### 新特性 (Added)
- [`lib/which/`](file:///data/dev/cp/lib/which/) & [`lib/which.cp`](file:///data/dev/cp/lib/which.cp):
  - `which.cp`：吸收自 Bun 的跨平台可执行程序快速定位器，支持绝对路径、相对路径以及 `PATH` 环境变量遍历；在 Windows 下自动推导 `.exe`、`.cmd`、`.bat`；基于 `std.c.access(path, 1)`（`X_OK`）检测执行权限；
  - `index.cp` & `lib/which.cp`：官方标准库导出通道 `cap:which`。
- [`lib/ws/`](file:///data/dev/cp/lib/ws/) & [`lib/ws.cp`](file:///data/dev/cp/lib/ws.cp):
  - `opcode.cp`：标准 RFC 6455 `Opcode`（`Text`, `Binary`, `Close`, `Ping`, `Pong`, `Continue`）与 `CloseCode` 状态码；
  - `mask.cp`：RFC 6455 规范 4 字节 XOR 掩码编解码；
  - `frame.cp`：`FrameHeader` 支持 7 位基础、16 位中型与 64 位超大型载荷扩展与掩码解析，`FrameBuilder` 提供便捷帧构造方法；
  - `index.cp` & `lib/ws.cp`：官方标准库导出通道 `cap:ws`。
- [`lib/fs.cp`](file:///data/dev/cp/lib/fs.cp):
  - 扩充 `tmpDir()` 跨平台系统临时目录探测；
  - 扩充 `tmpPath()` 原子序列号与单调时钟纳秒时间戳保证唯一临时文件路径；
  - 扩充 `writeAtomic()` 崩溃安全的原子覆盖写入。
- 测试用例套件：
  - [`cap/test/test_which.cp`](file:///data/dev/cp/cap/test/test_which.cp)：可执行二进制定位与 PATH 解析全部 100% PASS；
  - [`cap/test/test_ws.cp`](file:///data/dev/cp/cap/test/test_ws.cp)：4 大 WebSocket 帧协议与掩码往返套件全部 100% PASS；
  - [`cap/test/test_fs_extensions.cp`](file:///data/dev/cp/cap/test/test_fs_extensions.cp)：临时文件与原子写入套件全部 100% PASS。

### 文档更新 (Docs)
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：
  - 第 12.1 节总表中更新 `cap:which` 与 `cap:ws` 状态为已全面实装；
  - 新增第 12.15 节 `cap:which`、第 12.16 节 `cap:ws` 与第 12.17 节 `cap:fs` 增强能力的详细技术契约与完整代码示例。

## [2026-11-12] - 标准库第四阶段：Unix Glob 模式匹配、Semver 2.0 版本解析与范围匹配、DotEnv 与 INI 配置解析

### 新特性 (Added)
- [`lib/glob/`](file:///data/dev/cp/lib/glob/) & [`lib/glob.cp`](file:///data/dev/cp/lib/glob.cp):
  - `matcher.cp`：基于路径分段（Segment-based）递归回溯匹配算法的工业级 Glob 引擎，支持 `*` 单段任意字符匹配、`**` 多级目录递归匹配、`?` 单字符通配、`[a-z]` / `[0-9]` 字符范围集合匹配、`!` 取反以及 `hasMagic()` 极速通配符探测；
  - `index.cp` & `lib/glob.cp`：官方标准库导出通道 `cap:glob`。
- [`lib/semver/`](file:///data/dev/cp/lib/semver/) & [`lib/semver.cp`](file:///data/dev/cp/lib/semver.cp):
  - `version.cp`：Semver 2.0 规范语义版本解析器与格式化器（支持 `v1.2.3`, `=2.10.5-beta.1+sha.abcdef` 等格式），严格遵循 Semver 2.0 优先级铁律（正式发布版 > 预发布版，点分数字按数值比较，字符按 ASCII 比较）；
  - `range.cp`：支持 Caret 锁版本（`^`）、Tilde 补丁更新（`~`）、不等式（`>=`, `<=`, `>`, `<`）、连字符范围（`1.0.0 - 2.0.0`）、逻辑或（`||`）与通配符（`*` / `x` / `X`）；
  - `index.cp` & `lib/semver.cp`：官方标准库导出通道 `cap:semver`。
- [`lib/dotenv/`](file:///data/dev/cp/lib/dotenv/) & [`lib/dotenv.cp`](file:///data/dev/cp/lib/dotenv.cp):
  - `loader.cp`：100% 栈分配零堆垃圾 `.env` 解析器，支持 `KEY=VALUE`、`KEY: VALUE` 与 `export KEY=VALUE`；支持单引号原样保护、双引号转义解析（`\n`, `\r`, `\t` 等）与内联注释安全截断；提供 `get()`, `getOr()`, `has()`；
  - `index.cp` & `lib/dotenv.cp`：官方标准库导出通道 `cap:dotenv`。
- [`lib/ini/`](file:///data/dev/cp/lib/ini/) & [`lib/ini.cp`](file:///data/dev/cp/lib/ini.cp):
  - `parser.cp`：工业级 INI 解析器，支持全局配置区与 `[section_name]` 独立段空间；支持 `;` 与 `#` 双注释符及行尾注释；提供 `get(section, key)`, `getGlobal(key)`, `hasSection(section)`, `getOr()`；
  - `index.cp` & `lib/ini.cp`：官方标准库导出通道 `cap:ini`。
- 测试用例套件：
  - [`cap/test/test_glob.cp`](file:///data/dev/cp/cap/test/test_glob.cp)：4 大 Glob 通配与模式匹配套件全部 100% PASS；
  - [`cap/test/test_semver.cp`](file:///data/dev/cp/cap/test/test_semver.cp)：4 大 Semver 解析、比较、格式化与范围匹配套件全部 100% PASS；
  - [`cap/test/test_config.cp`](file:///data/dev/cp/cap/test/test_config.cp)：2 大 DotEnv 与 INI 配置解析套件全部 100% PASS。

### 文档更新 (Docs)
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md)：
  - 第 12.1 节已实装标准库总览表中新增 `cap:glob`, `cap:semver`, `cap:dotenv`, `cap:ini`；
  - 第 12.4 ~ 12.14 节全量深度扩充详细技术契约与全套完整可运行代码示例（涵盖 `cap:sync`, `cap:stream`, `cap:json`/`cap:serde`, `cap:router`, `cap:http`, `cap:path`, `cap:url`, `cap:encoding`, `cap:glob`, `cap:semver`, `cap:dotenv`, `cap:ini`）。

## [2026-11-12] - 标准库第三阶段：Radix-Tree 路由引擎、HTTP 协议栈、跨平台路径与 WHATWG URL、Base64 编码

### 新特性 (Added)
- [`lib/router/`](file:///data/dev/cp/lib/router/) & [`lib/router.cp`](file:///data/dev/cp/lib/router.cp):
  - `param_map.cp`：纯栈 16 项定长零堆分配参数映射字典 `ParamMap`，提供 `get()`, `at()`, `has()`, `append()`, `count()`, `isEmpty()`；
  - `route.cp`：`RouteMatch<T>` 泛型路由匹配结构体；
  - `radix.cp`：8 树 HTTP 动词前缀树 `Router<T>` 与 `RadixNode<T>`，支持静态路径、动态参数（`:param` 与 `[param]`）、通配符（`*path` 与 `[...path]`）与 `ALL` 方法回退；
  - 新增 `lib/router.cp` 作为 `cap:router` 官方通道出口。
- [`lib/http/`](file:///data/dev/cp/lib/http/) & [`lib/http.cp`](file:///data/dev/cp/lib/http.cp):
  - `method.cp`：标准 HTTP `Method` 枚举与 `isIdempotent()`, `hasRequestBody()`, `toString()`, `fromString()`；
  - `headers.cp`：大小写不敏感的高性能 HTTP `Headers` 集合，支持 `get()`, `set()`, `append()`, `delete()`, `count()`, `at()`；
  - `etag.cp`：`ETag` 生成与强弱 `ifNoneMatch()` 条件匹配；
  - `mime.cp`：`MimeType` 常见文件后缀与文件路径极速映射；
  - `lib/http.cp`：导出 `Method`, `Headers`, `HeaderEntry`, `ETag`, `ParsedETag`, `MimeType`。
- [`lib/path/`](file:///data/dev/cp/lib/path/) & [`lib/path.cp`](file:///data/dev/cp/lib/path.cp):
  - `posix.cp`：POSIX 标准路径计算（`basename`, `dirname`, `extname`, `isAbsolute`, `normalize`, `join`）；
  - `windows.cp`：Windows 驱动器盘符、UNC 路径与反斜杠处理；
  - `index.cp`：跨平台统一 `Path`（以及 `path.posix`、`path.win32`）；
  - 新增 `lib/path.cp` 作为 `cap:path` 官方通道出口。
- [`lib/url/`](file:///data/dev/cp/lib/url/) & [`lib/url.cp`](file:///data/dev/cp/lib/url.cp):
  - `query.cp`：`URLSearchParams` 查询字符串解析、修改与序列化；
  - `url.cp`：WHATWG 规范 `URL` 对象（解析 `protocol`, `username`, `password`, `host`, `hostname`, `port`, `pathname`, `search`, `hash`, `origin`, `searchParams`）；
  - 新增 `lib/url.cp` 作为 `cap:url` 官方通道出口。
- [`lib/encoding/`](file:///data/dev/cp/lib/encoding/) & [`lib/encoding.cp`](file:///data/dev/cp/lib/encoding.cp):
  - `base64.cp`：纯 CP 编写的高性能 Base64 编解码器（标准 Standard 模式、URL-Safe 模式、无填充模式、自适应统一解码 `+`/`-` 与 `/`/`_`、无堆分配与内存分配两种模式）；
  - 新增 `lib/encoding.cp` 作为 `cap:encoding` 官方通道出口。
- 测试用例套件：
  - [`cap/test/test_router.cp`](file:///data/dev/cp/cap/test/test_router.cp)：3 大路由套件（静态、动态、通配符与 ALL 回退全部 100% PASS）；
  - [`cap/test/test_http_features.cp`](file:///data/dev/cp/cap/test/test_http_features.cp)：4 大 HTTP 协议特性全部 100% PASS；
  - [`cap/test/test_path_url.cp`](file:///data/dev/cp/cap/test/test_path_url.cp)：4 大 POSIX/Windows 路径与 WHATWG URL 套件全部 100% PASS；
  - [`cap/test/test_encoding.cp`](file:///data/dev/cp/cap/test/test_encoding.cp)：3 大 Base64 编解码套件全部 100% PASS。

## [2026-11-12] - 标准库第二阶段：并发同步基础设施、无锁原语、管道流与零拷贝 JSON

### 新特性 (Added)
- [`lib/sync/`](file:///data/dev/cp/lib/sync/):
  - `futex.cp`：跨平台系统级用户态同步原语，封装 Linux `SYS_futex`、macOS `__ulock`、Windows `WaitOnAddress`，提供纳秒单调时钟 `Deadline` 与超时等待；
  - `mutex.cp`：吸收 Bun 成熟的自旋与 Futex 混合互斥锁（`tryLock`、`lock`、`unlock`、`withLock`）；
  - `condition.cp`：跨平台条件变量（`wait`、`timedWait`、`signal`、`broadcast`）；
  - `wait_group.cp`：并发信号量计数器（`add`、`done`、`wait`、`finish`）；
  - `channel.cp`：线程安全有界 MPMC 数据通道，支持非阻塞与超时阻塞收发；
  - `ring_buffer.cp`：基于 Dmitry Vyukov 经典算法的定长多生产者多消费者（MPMC）无锁环形队列 `MpmcQueue<T>`；
  - `unbounded_queue.cp`：侵入式无界并发无锁链表队列 `UnboundedQueue<T>`；
  - `wyhash.cp`：从 Bun 移植的 64 位极速无状态非加密哈希算法 `Wyhash`；
  - `concurrent_map.cp`：基于 Wyhash 16 桶分片的高并发分段哈希映射表 `ConcurrentMap<V>`；
  - `arc_slice.cp`：原子引用计数切片 `ArcSlice<T>`，支持跨线程零拷贝共享与自动析构；
  - `index.cp` & `lib/sync.cp`：模块导出与对外统一标准库入口。
- [`lib/io.cp`](file:///data/dev/cp/lib/io.cp) & [`lib/stream.cp`](file:///data/dev/cp/lib/stream.cp):
  - 扩充 `MaxBuf`（动态零拷贝切片缓冲守卫）；
  - 扩充 `PipeReader`（实现 `Reader, Closer`）与 `PipeWriter`（实现 `Writer, Closer`）；
  - 扩充 `Pipe.create()` 工厂函数，支持快速创建双端数据管道；
  - 新增 `lib/stream.cp` 作为 `cap:stream` 官方通道出口。
- [`lib/json/`](file:///data/dev/cp/lib/json/), [`lib/json.cp`](file:///data/dev/cp/lib/json.cp), [`lib/serde.cp`](file:///data/dev/cp/lib/serde.cp):
  - `pool.cp`：移植 Bun 内存池机制 `JsonBufferPool`，实现多轮解析内存复用；
  - `value.cp`：零拷贝 AST `JsonValue`，直接借用源输入切片；
  - `parser.cp`：极速流式递归下降 JSON 解析器 `JsonParser`；
  - 新增 `lib/serde.cp` 作为 `cap:serde` 序列化统一出口通道。
- [`cap/test/test_sync_primitives.cp`](file:///data/dev/cp/cap/test/test_sync_primitives.cp):
  - 覆盖 Wyhash、Futex、Mutex、Condition、WaitGroup、MpmcQueue、Channel、ConcurrentMap、ArcSlice 8 大用例套件。
- [`cap/test/test_zero_copy_json.cp`](file:///data/dev/cp/cap/test/test_zero_copy_json.cp):
  - 验证基础标量、源缓冲区指针直通零拷贝借用及嵌套对象/数组检索。

### 优化与修复 (Fixed)
- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs) / [`cpc_dev/src/codegen/mod.rs`](file:///data/dev/cp/cpc_dev/src/codegen/mod.rs):
  - 在生成 Trait 虚表结构体时，若方法返回类型以 `!` 开头（错误联合类型），将其转译为 `anyerror{ret}`，彻底消除 Zig 函数指针类型无法使用裸 `!T` 的语法错误。
- [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs) / [`cpc_dev/src/codegen/stmt.rs`](file:///data/dev/cp/cpc_dev/src/codegen/stmt.rs):
  - 在生成 `__{var}_moved = true;` 标志位赋值时，增加 `self.drop_flag_vars.contains(&id.sym)` 守卫，避免对未注册 drop 标志的变量生成非法标识符引用。
- [`cap/test/test_json.cp`](file:///data/dev/cp/cap/test/test_json.cp):
  - 修复测试用例中将 `const conf` 进行成员字段修改的语义错误，改为 `let conf`。

## [2026-11-12] - 编译器核心 Codegen 模块化分拆与 Trait 作用域强化

### 架构演进 (Changed)
- [`cpc/src/codegen/`](file:///data/dev/cp/cpc/src/codegen/):
  - 彻底废除 1.7 万行巨石单体文件 `codegen.rs`，重构分拆为 8 个高内聚子模块：
    - `mod.rs`（3,398 行）：上下文派发中心、`Codegen` 结构体、代码缩进派发与模块级顶层项（Class/Trait/Enum/Impl/Interface/Reflection）；
    - `prelude.rs`（614 行）：`ModuleFeatures` AST 特性扫描与运行时环境感知；
    - `types.rs`（2,601 行）：类型推导、Zig 原生类型映射与常量泛型单态化；
    - `borrow.rs`（960 行）：NLL 借用检查与跨作用域冲突拦截；
    - `pattern.rs`（1,587 行）：模式匹配、穷尽性分析与 `switch` 展开；
    - `actor.rs`（1,577 行）：CPS 状态机切分、异步闭包变量捕获与 Actor 协程逻辑；
    - `stmt.rs`（2,114 行）：控制流语句生成（`let`、`if`、`while`、`for`、`return`、`defer`、`errdefer`）；
    - `expr.rs`（4,332 行）：表达式求值、运算符重载、方法调用与内联闭包。
  - 单文件规模降至 1,000 ~ 4,000 行，消除跨 800 行分段读取开销，全面赋能多子智能体并行开发。

### 优化与修复 (Fixed)
- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs):
  - **Trait 虚表局部作用域强化**：在 Trait 与 Interface 声明生成中，将虚表指针类型升级为 `vtable: *const @This().VTable`，彻底杜绝与业务代码中的顶层 `class VTable` 发生 Zig 符号歧义。

## [2026-11-12] - 方案 A 工业级统一 I/O 流体系与 C 变量外部绑定支持

### 新特性 (Added)
- [`lib/io.cp`](file:///data/dev/cp/lib/io.cp):
  - 新增统一流错误类型 `IOError`（`EndOfStream`, `UnexpectedEof`, `Closed`, `BufferFull`, `ShortWrite`, `InvalidSeek`）；
  - 新增寻址基准枚举 `SeekOrigin`（`Start`, `Current`, `End`）；
  - 新增统一流 Trait 契约：`Reader`、`Writer`、`Closer`、`Seeker`；
  - 新增零拷贝内存切片只读流 `BufferReader`，支持无堆分配读取与 Seek；
  - 新增内存定长安全写入器 `BufferWriter`（4096 字节），支持字节提取与重置；
  - 新增高性能泛型缓冲读取器 `BufferedReader<R: Reader>`（256 字节内部缓存，支持 `readByte()` 单字节速读）；
  - 新增高性能泛型缓冲写入器 `BufferedWriter<W: Writer>`（256 字节内部缓存，支持自动/手动 `flush()`）；
  - 新增流式全量传输管道 `copyStream<R: Reader, W: Writer>` 与定额流搬运 `copyN`；
  - 100% 保持已有 `Console`、`console`、`Stdio`、`stdio`、`print` 向后兼容。
- [`cap/test/test_io_streams.cp`](file:///data/dev/cp/cap/test/test_io_streams.cp):
  - 包含 6 大端到端用例套件：`BufferReader` 切片读取与 Seek 验证、`BufferWriter` 写入与字节校验、`BufferedReader` 缓冲与单字节速读、`BufferedWriter` 批量写入与 flush、`copyStream` 管道全量传输、`copyN` 定额截断传输。
- [`cap/test/test_extern_var.cp`](file:///data/dev/cp/cap/test/test_extern_var.cp):
  - 验证 `@c extern const` 与顶层 `extern var` 的 C 外部符号绑定。

### 优化与修复 (Fixed)
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 扩展 `parse_decl` 支持在 `extern` 之后解析 `const`/`var`/`let`，正确标记 `is_extern` 与 `is_c`。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) / [`cpc_dev/src/sema.rs`](file:///data/dev/cp/cpc_dev/src/sema.rs):
  - 增加对定长数组切片引用的元素类型匹配检查；
  - 在 `Expr::New` 类型检查时剔除泛型参数 `<...>`，精准定位基类定义与方法签名。
- [`cpc/src/codegen.rs`](file:///data/dev/cp/cpc/src/codegen.rs) / [`cpc_dev/src/codegen.rs`](file:///data/dev/cp/cpc_dev/src/codegen.rs):
  - 对 `Pattern::Ident("_")` 及通配符模式生成 `_ = expr;` 丢弃语句，消除 Zig 下非法 `var _` 错误；
  - 在异步状态机闭包上下文提取中跳过 `_` 变量，防止生成非法字段名；
  - 泛型类的 `_create` 构造函数统一采用 `@This()` 申请内存，解决单态化类型缺失 `_create` 的问题；
  - `lookup_method` 剥离类型中的泛型参数后缀，正确解析泛型实例的方法返回值类型。

## [未发布] - 2026-11-13

### 新增 (Added)
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 建立专为 AI Agent 设计的高密度（~220行）语法契约手册，涵盖模块导入导出、变量绑定与可变性、类型系统与裸指针、结构体与类、枚举与模式匹配、解构赋值、现代错误处理及 Top 10 禁手清单。
- [`GEMINI.md`](file:///data/dev/cp/GEMINI.md):
  - 增补第 2.4 节双轨语言规范维护机制；
  - 增补第 4 节自主推导持续工作流闭环规范；
  - 增补第 6.5 节配额耗尽倒计时与 `schedule` 挂起协议；
  - 增补第 7.1.3 节双智能体通信透明化与后台守护进程自动启动检查。
- [`lib/event_loop/`](file:///data/dev/cp/lib/event_loop/):
  - 建立工业级统一事件循环标准库（`cap:event_loop`）：
    - `timer.cp`：基于侵入式二叉最小堆的定时器系统（`Timer`, `TimerHeap`），支持精确到毫秒的超时管理与 $O(\log N)$ 节点注销；
    - `concurrent_task.cp`：16 字节跨线程无锁并发任务（`ConcurrentTask`, `ConcurrentQueue`），由 `Mutex` 保护；
    - `deferred_task.cp`：微任务队列（`DeferredTaskQueue`），实现上下文指针自动去重；
    - `file_poll.cp`：基于 `std.posix.poll` 的系统描述符监听引擎（`PollEngine`, `PollEvents`）；
    - `loop.cp`：统一 `EventLoop` 生命周期驱动（`tick` 单步步进与 `run` 常驻循环）；
    - `index.cp`：模块统一出口。
- [`cap/test/test_event_loop.cp`](file:///data/dev/cp/cap/test/test_event_loop.cp):
  - 5 大端到端回归测试套件：二叉最小堆定时器调度、16 字节并发任务队列、微任务队列与去重、I/O 描述符轮询引擎、统一 EventLoop 综合生命周期。

### 优化与修复 (Fixed)
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) / [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 解析 `[]const T` 时，保留 `has_const` 标志并将其构建为不可变借用切片 `Type::BorrowType { inner: Box::new(Type::Array(...)), is_mut: false }`；未带 `const` 的 `[]T` 保持为可变数组切片 `Type::Array(...)`。
- [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs) / [`cpc_dev/src/codegen/types.rs`](file:///data/dev/cp/cpc_dev/src/codegen/types.rs):
  - `Type::Array(inner)` 映射生成 Zig 可变切片 `[]T`，解除动态分配切片元素只读限制；
  - `Type::BorrowType { is_mut: false, inner: Array(elem) }` 映射生成 Zig 只读切片 `[]const T`，保障字符串切片等参数调用的常量性契约。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 增补第 12.19 节工业级统一事件循环体系（`cap:event_loop`）设计规范与完整使用代码示例；调整路线图章节序号为 12.20。

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 全面按照用户 5 步法重构重写，提炼出 605 行完备且纯正的 CP 语法契约速查手册；
  - 彻底清除 `var`、`let mut` 与 `threadlocal`，确立 `let` / `const` 纯净绑定与 `move` / `own` 所有权转移；
  - 完善参数借用与裸指针分类（`&T` 只读借用、`*T` 可变借用、`*T!` 裸指针、`move T`/`own T` 所有权转移）；
  - 规范四大代码块为 `zig { ... }`、`unsafe { ... }`、`arena { ... }` 与 `@c { ... }`；
  - 增补泛型与约束、常量泛型、类型别名及原生运算符矩阵；
  - 升级 Top 15 绝对禁手清单。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 全面审核并清洗第 12 章中遗留的历史代码示例，将旧版 `var` 统一修正为 `let`，将 Zig 前缀 `try` 替换为 CP 后缀 `?`，将 `for()|i|` 修正为 `for (i of ...)`，消除与 `cp_agent_ref.md` 的任何潜在不一致。
- [`lib/security/csrf.cp`](file:///data/dev/cp/lib/security/csrf.cp):
  - 将所有 `__zig__` 替换为标准 `zig` 逃逸块，将所有 `var` 替换为现代 `let` 变量声明。

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 全面扩充 17 大核心系统特性与 Bun Zig 迁移必备语法（657 行）：
    - 引入直接透明导入 `.zig` 源码文件语法及前柔后刚两级类型模型；
    - 补充 `@c struct`、`@root struct`、`@value struct`、`@align(N)`、`@refCount(cleanup)` 系统级装饰器；
    - 详述多字段分离借用（Disjoint Field Borrowing）、`ref struct` 栈绑定视图与显式生命周期；
    - 丰富 C 指针矩阵：`[*c]T`、`[*:0]T!`、`[*]([*:0]u8!)?!`、`*!`、`*!?`、`cstr` vs `cstring`；
    - 补全编译期函数与 Comptime 内省原语、AST 模板插值宏与元编程体系；
    - 全景覆盖 `switch` 12 种模式解构（含结构体整包捕获、元组解构、Walrus 绑定、动态类型匹配、三态错误 downcast）；
    - 补充多线程并发容器 `Shared<T>` 与内部可变性容器 `Shared(Mutex(T))` / `Shared(RwLock(T))`；
    - 扩充 Top 18 绝对禁手清单。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 更新第 7 章系统级内存布局列表，将 `@extern struct` 统一更正为现代规范 `@c struct`，并补充 `@root struct` 与 `@align(N)`。

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 逐章逐节对照 `cp_ref.md` 展开穷尽式查漏补缺，手册全面扩充至 820 行：
    - 补入命名空间全量导入 `import * as math` 与 Zig 风格原生物理导入 `const bun = @import("bun")`；
    - 新增第四章专门阐述 6.7.2 变量常量性与指针只读性的正交解耦（`const p: *T!`、`let p: *const T!`、`const p: *const T!`）；
    - 补全 6.7.3 获取变量裸指针的标准方式（字面量语法 `*v!` / `**v!` 与安全借用转换 `&v as *T!` / `*v as *T!`）；
    - 补全 6.7.4 完整裸指针类型转换矩阵 in `unsafe`（指针互转、物理内存整型地址、通用裸指针与未定长多项指针）；
    - 明确 `zig {}` 内嵌逃逸舱完全脱离 CP 借用检查的系统级定义；
    - 系统重构第九章错误体系：严密区分 `T!`（全局 `AnyErr`，禁止动态装箱）与 `T ! Err`（业务动态结构体，显式签名），补齐具名错误集、多错误联合声明 `T ! E1 || E2` 与 `from` 桥接机制。

- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 彻底解除行数限制，全面对齐 `cp_ref.md` 语法体系，扩充至 930+ 行权威智能体契约手册：
    - 补全第五章 `static class` 与 `static struct` 纯静态命名空间容器规则（全员自动静态、严禁实例化、无 `init`/`release`、禁止 `extends`）；
    - 补全第六章成员默认可见性差异：`struct` 成员默认 `pub`（公开透明 POD），`class` 成员默认 `priv`（强封装私有）；
    - 补全第一章模块级 `pub` 等价于 `export` 并作为官方首选推荐的规范；
    - 补全第六章 `class` 单继承生命周期体系：`class Child extends Parent`，`super.init(...)` 构造器调用，`pub fn release(*_)` 确定性析构钩子，`super.method(...)` 父类方法调用；
    - 确立 `@c` 全面替代 `@callconv(.c)`（同步更新第 VI.4 节与禁手清单 #9）；
    - 补全第七章字符串模板插值 `${expr}`、原生正则表达式字面量 `/pattern/`、管道操作符 `|>` 与通配占位符 `*`；
    - 补全第八章海象迭代解包循环 `while (binding := expr)` 与自动分号推导（ASI）规范；
    - 补全第九章胖指针动态错误与 `downcast` 双轨解构模式匹配语法（`err(e: T)` 与 `err(e := T)`）；
    - 补全第十章编译期计算函数 `comptime fn` 与 `@func(...)`。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 彻底清除 Zig 繁琐底层强转 Builtins（`@ptrCast`, `@alignCast`, `@intCast`, `@truncate`），确立 CP 现代统一的 `as` 运算符与 `Type(val)` 构造：
    - 明确 `ptr as *TargetType!` 在代码生成时由编译器底层自动发射 `@ptrCast(@alignCast(...))`，无需开发者手工管理对齐断言；
    - 纠正结构体指针/借用字段访问为自动解引用（`alloc.is_active = true;`），彻底杜绝 Zig 风格冗余的 `alloc.*.field`；
    - 清洗内建指令表，仅保留属于 CP 编译期的内省指令（`@sizeOf`, `@alignOf`, `@typeOf`, `@typeName`, `@fieldsOf`, `@field`, `@compileError`, `@as`）；
    - 禁手清单升级为 Top 23：追加 #22 严禁 `ptr.*.field` 冗余解引用、#23 严禁手动书写 `@ptrCast(@alignCast(ptr))`。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 清洗第 12 章遗留示例中的 `@ptrCast(@alignCast)` 与 `__zig__`，同步更新为规范 CP `as` 与 `zig {}`。


- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) & [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - `parse_params` 与 `parse_arrow_params` 支持 `Token::Let`（`fn foo(let p: T)`）作为可变局部值拷贝形参官方规范，`E0104` 错误提示同步引导使用 `let param: Type` / `*param: Type`。
- [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs) & [`cpc_dev/src/codegen/actor.rs`](file:///data/dev/cp/cpc_dev/src/codegen/actor.rs):
  - 为 `mutable_copy_params`（`let p: T` / `mut p: T`）生成 `_cp_<name>` 底层形参并在函数入口注入 `var <name> = _cp_<name>; _ = &<name>;`，消除 Zig 常量形参不可修改与同名前置遮蔽错误。
- [`cap/test/test_let_param.cp`](file:///data/dev/cp/cap/test/test_let_param.cp):
  - 新增 `fn foo(let p: T)` 标量、结构体值拷贝与循环计数器回归测试。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 修复第 5.2、9.4、11.9.4、14.12 节内部表述冲突，修正第 485/503-504、2660、5894 行的默认导出、`handler.downcast<NetworkHandler>()`、`cap:parallel` 与 `cap:actor` 导入示例错误。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 全面重构全 13 章并经独立审计核对修正，消除全部 Zig 管道残留（`inline for`、`receive {}`）、错误语法（`downcast(T)`、`/regex/`、`pub default class`、`import { @spawn }`）及命名冲突（`toString()`），对齐 **32 大官方 `cap:*` 模块矩阵**，补齐 `cstr?`、`->`、`?.`、`.?`、`orelse`、匿名块 `({ ... break val; })`、开放枚举 `_`、Zig 常量区间、`extern const`/`let`、`inline while`、`@getPageSize(4)`、块级 `@if/@else`、`@This()`/`@is_beam`/`@panic`、`_ = expr;` 忽略返回值、原生类型实参（`std.mem.trim(u8, ...)`）、`macro AutoReset(target)` 完整模板宏示例、表达式级 `arena { @alloc(...) }` 与完整 `cap`/`cpc` CLI（定稿 990 行）。

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs), [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs), [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs), [`cpc/src/ast_visitor.rs`](file:///data/dev/cp/cpc/src/ast_visitor.rs), [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs), [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs), [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs), [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs), [`cpc/src/codegen/borrow.rs`](file:///data/dev/cp/cpc/src/codegen/borrow.rs), [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs), [`cpc/src/dcp.rs`](file:///data/dev/cp/cpc/src/dcp.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 实现 TypeScript 风格 `/pattern/flags` 正则表达式字面量，降级为内置 `RegExp` 类构造（支持 `i`/`m` 标志位，无需 `import`）；
  - 新增 `Expr::TypeVal` 支持 `@c fn` 函数指针类型实参、`[N]T` 定长数组类型实参（如 `std.mem.zeroes([16]u8)`）及 `*i32` / `*Struct` 指针类型实参；
  - 修复 `[*]const T?!` 多项裸指针可空修饰符剥离顺序，生成 `?[*]const T`；
  - 支持 `@Vector(N, T)` 及 `const V = @Vector(N, T);` 类型别名、定长数组/字面量到向量的类型兼容与原生逐元素向量比较；
  - 支持 `*T!` -> `*const T!` 裸指针常量协变与 `Type::ErrorUnion` 在 `?` / `? fallback` 下的解包类型推导；
  - 完结 Bun AGY 7 项编译热修任务。
- [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md) & [`cp_ref.md`](file:///data/dev/cp/cp_ref.md):
  - 完成两轮 Gemini 4 审计项及 Bun AGY Wave 1/2 建议的双向同步：补齐 `.Variant` 载荷简写构造、`move u |> fn(*)` 与管道尾部 `?`/`??`、`*(*T?!)` 二级可空出参裸指针、`(&T)?`/`&(T!E)` 与 `: *x T`/`: *(x | y) T`/`: *_ T` 可变借用返回、`@inline` 与 `unsafe fn`、`let * = expr;`、`Err.init(...).with_cause(move b)`、内置 `/pattern/flags` 与 `RegExp`、`@bitCast` vs `as`、SIMD 与位计数原语（`@Vector`, `@splat`, `@clz`, `@ctz`, `@popCount`, `@reduce`, `@select`, `@byteSwap`, `@bitSizeOf`）、`@typeInfo(T)` 反射、多级点路径值类型直通（`Dotted Path Types`）、`T ! FileErr || err{...}` 匿名即席错误联合、单行 `@c fn`、枚举 `.get`/`.has`、前缀 `try` 迁移兼容说明，并修正 `cp_ref.md` 中 `export` 与 `Trait` 拼写笔误。
- [`lib/net/`](file:///data/dev/cp/lib/net/)（移除单文件 `lib/net.cp`，新建扁平化网络标准库目录）：
  - 新增 [`lib/net/_usockets.cp`](file:///data/dev/cp/lib/net/_usockets.cp)：底层 `@c { ... }` uSockets C-FFI 声明、不透明结构体句柄与监听常量/状态枚举；
  - 新增 [`lib/net/address.cp`](file:///data/dev/cp/lib/net/address.cp)：`AddressFamily`、`Ipv4Addr`、`Ipv6Addr` 与 `SocketAddress` 解析及检测；
  - 新增 [`lib/net/socket.cp`](file:///data/dev/cp/lib/net/socket.cp)：`SocketState`、`SocketStats`、`SafeSocket`（含 `> 240s` 自动拆分 `long_timeout_min` 及 RAII 释放）与兼容层 `Socket`；
  - 新增 [`lib/net/tcp_stream.cp`](file:///data/dev/cp/lib/net/tcp_stream.cp)：16KB `StreamBuffer` 软塞缓冲与 `TcpStream`；
  - 新增 [`lib/net/tcp_server.cp`](file:///data/dev/cp/lib/net/tcp_server.cp)：`ListenConfig`、`TcpServer` 与兼容层 `Server` / `listen()` / `connect()`；
  - 新增 [`lib/net/udp.cp`](file:///data/dev/cp/lib/net/udp.cp)：`UdpPacket` 与 `UdpSocket` 数据报套接字抽象；
  - 新增 [`lib/net/index.cp`](file:///data/dev/cp/lib/net/index.cp)：`cap:net` 统一导出入口。
- [`cap/test/test_net.cp`](file:///data/dev/cp/cap/test/test_net.cp):
  - 扩充为覆盖 `lib/net/` 全组件及兼容 API 的 6 大端到端测试套件（6/6 PASS）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs), [`cpc/src/codegen/borrow.rs`](file:///data/dev/cp/cpc/src/codegen/borrow.rs), [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs), [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 修正 `@net_listen`、`@net_accept_async`、`@net_write_async` 的 `I32` 返回类型推导及类方法状态机回调符号生成，统一 `self`/`this`/`_` 接收者参数过滤。
- [`lib/collections/`](file:///data/dev/cp/lib/collections/)（移除单文件 `lib/collections.cp`，新建扁平化高性能容器标准库目录）：
  - 新增 [`lib/collections/bounded_array.cp`](file:///data/dev/cp/lib/collections/bounded_array.cp)：`BoundedArray<T, const CAP: usize>` 零堆分配栈上定长缓冲容器；
  - 新增 [`lib/collections/baby_list.cp`](file:///data/dev/cp/lib/collections/baby_list.cp)：16 字节紧凑动态数组 `BabyList<T>` 与 `u32` 偏移字节列表 `OffsetByteList`；
  - 新增 [`lib/collections/linear_fifo.cp`](file:///data/dev/cp/lib/collections/linear_fifo.cp)：2 的幂次位掩码环形队列 `LinearFifo<T>`；
  - 新增 [`lib/collections/hive_array.cp`](file:///data/dev/cp/lib/collections/hive_array.cp)：基于硬件 `@ctz`/`@popCount` 的 `BitSet64` 与 $O(1)$ 槽位复用池 `HiveArray<T, const CAP: usize>`；
  - 新增 [`lib/collections/string_map.cp`](file:///data/dev/cp/lib/collections/string_map.cp)：按长度过滤的快速字符串映射表 `StringEntry<V>` 与 `FastStringMap<V, const CAP: usize>`；
  - 新增 [`lib/collections/index.cp`](file:///data/dev/cp/lib/collections/index.cp)：`cap:collections` 统一导出入口。
- [`cap/test/test_collections_bun.cp`](file:///data/dev/cp/cap/test/test_collections_bun.cp) & [`cap/test/test_collections_matrix.cp`](file:///data/dev/cp/cap/test/test_collections_matrix.cp):
  - 新增 `test_collections_bun.cp`（5 大套件 100% PASS），更新 `test_collections_matrix.cp` 为现代无 `mut` / `for (...)` 语法（7 大套件 100% PASS）。
- [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs) & [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 新增 `substitute_generic_type` 实现泛型结构体/类方法返回类型参数替换（支持 `Type::Ref("T")` 与参数化 `Type::Ref("BabyList<T>")`），修复静态泛型工厂方法（`BabyList<i64>.initCapacity`）返回指针类型推导；
  - 升级内置 `VecDeque` 与 `BinaryHeap` 只读方法接收者为 `*const`。
- [`lib/mem/`](file:///data/dev/cp/lib/mem/)（移除单文件 `lib/mem.cp`，新建扁平化内存分配器与智能指针标准库目录）：
  - 新增 [`lib/mem/_mimalloc.cp`](file:///data/dev/cp/lib/mem/_mimalloc.cp)：底层 `@c { ... }` Mimalloc C-FFI 声明与 `MiHeap` 不透明句柄；
  - 新增 [`lib/mem/allocator.cp`](file:///data/dev/cp/lib/mem/allocator.cp)：`StackFallbackAllocator<const CAP: usize>`、`BumpArena`、`MaybeOwnedBuf` 及向下兼容的 `Arena`、`get_arena()`、`SharedTensor`；
  - 新增 [`lib/mem/tagged_ptr.cp`](file:///data/dev/cp/lib/mem/tagged_ptr.cp)：64 位高 16 位标签指针 `TaggedPointer` 与低 3 位标签指针 `LowTagPointer`；
  - 新增 [`lib/mem/cow.cp`](file:///data/dev/cp/lib/mem/cow.cp)：写时复制字节切片 `CowBytes`；
  - 新增 [`lib/mem/rc.cp`](file:///data/dev/cp/lib/mem/rc.cp)：`DecrementResult`、`RefCounter`、`AtomicRefCounter` 与 `WeakControlBlock`；
  - 新增 [`lib/mem/index.cp`](file:///data/dev/cp/lib/mem/index.cp)：`cap:mem` 统一导出入口。
- [`cap/test/test_mem_bun.cp`](file:///data/dev/cp/cap/test/test_mem_bun.cp):
  - 新增覆盖 `lib/mem/` 全组件的 5 大端到端测试套件（5/5 PASS）。
- [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)（及 `cpc_dev/src/codegen/expr.rs` 镜像同步）:
  - 为 `cap:test` 的 `test("...", () => { ... })` 用例自动绑定栈上 `_test_arena` 并注入 `defer _test_arena.deinit()`，支持在测试闭包内直接使用 `new` 实例化类对象。
- [`lib/http/_picohttp.cp`](file:///data/dev/cp/lib/http/_picohttp.cp) & [`lib/http/index.cp`](file:///data/dev/cp/lib/http/index.cp):
  - 新增 `lib/http/_picohttp.cp`：吸收 Bun `picohttp_sys/picohttpparser.cp` 与 `picohttp/picohttp.cp`，提供 `@c { ... }` `phr_*` C-FFI 声明、C-ABI 结构体 `phr_header` / `phr_chunked_decoder` 及零拷贝 HTTP/1.x 报文头解析器 `PicoHttpParser`（`parseRequest`, `parseResponse`, `findHeader`），并在 `lib/http/index.cp` 统一导出。
- [`cap/test/test_picohttp.cp`](file:///data/dev/cp/cap/test/test_picohttp.cp):
  - 新增 `PicoHttpParser` 与 C-ABI 布局验证测试套件（3/3 PASS）。
- [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs) & [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 完善 `Type::String` 与不可变借用切片 `[]const u8`（`Type::BorrowType { inner: Array(U8), is_mut: false }`）及可变切片 `[]T` -> `[]const T` 的类型协变；
  - 修复 `catch <Lit>` 表达式代码生成，保留字面量原生无类型常量语义以兼容 `u16`/`u32` 无符号目标。
- [`lib/string/`](file:///data/dev/cp/lib/string/) & [`lib/encoding/index.cp`](file:///data/dev/cp/lib/encoding/index.cp):
  - 新增 [`lib/string/simd_scan.cp`](file:///data/dev/cp/lib/string/simd_scan.cp)：16 字节 `@Vector(16, u8)` 高速扫描器 `SimdScanner`；
  - 新增 [`lib/string/utf.cp`](file:///data/dev/cp/lib/string/utf.cp)：`Utf8`、`Utf16`、`DecodeResult` 与 `UnicodeWidth`；
  - 新增 [`lib/string/builder.cp`](file:///data/dev/cp/lib/string/builder.cp)：`HashedString`、15 字节栈内联 `SmolStr` 与两阶段零重分配 `StringBuilder`；
  - 新增 [`lib/string/escape.cp`](file:///data/dev/cp/lib/string/escape.cp)：`StringEscape`（HTML 实体与 RegExp 元字符转义）；
  - 新增 [`lib/string/index.cp`](file:///data/dev/cp/lib/string/index.cp) 并扩展 `lib/encoding/index.cp`。
- [`cap/test/test_string_unicode.cp`](file:///data/dev/cp/cap/test/test_string_unicode.cp):
  - 新增覆盖 SIMD 扫描、UTF-8/16 编解码、Unicode/ANSI 宽度、`SmolStr`/`StringBuilder` 与转义的 5 大测试套件（5/5 PASS）。
- [`cpc/src/codegen/types.rs`](file:///data/dev/cp/cpc/src/codegen/types.rs)（及 `cpc_dev/src/codegen/types.rs` 镜像同步）:
  - 修复 `is_class_field` 在局部变量或参数与字段同名时的遮蔽判定（结单 `task-20261114-field-shadow-readonly-e0201`）。
- [`lib/dns/`](file:///data/dev/cp/lib/dns/):
  - 新增 [`lib/dns/_cares.cp`](file:///data/dev/cp/lib/dns/_cares.cp)：c-ares C-FFI 绑定、`NSClass`、`NSType` 与 `CaresStatus`；
  - 新增 [`lib/dns/packet.cp`](file:///data/dev/cp/lib/dns/packet.cp)：RFC 1035 12 字节 `DnsHeader`、`DnsRCode`、`DecodedDomain` 与支持 `0xC0` 指针压缩的 `DnsWireCodec`；
  - 新增 [`lib/dns/cache.cp`](file:///data/dev/cp/lib/dns/cache.cp)：`DnsFamily`、`DnsBackend`、`ResolveOptions`、`DnsCacheEntry` 与定容 TTL 缓存 `DnsCache<const CAP: usize>`；
  - 新增 [`lib/dns/index.cp`](file:///data/dev/cp/lib/dns/index.cp)：`cap:dns` 对外统一出口。
- [`cap/test/test_dns.cp`](file:///data/dev/cp/cap/test/test_dns.cp):
  - 新增覆盖 c-ares 类型、RFC 1035 报文头、Wire 域名编解码与指针压缩、TTL 缓存的 4 大测试套件（4/4 PASS）。
- [`lib/compress/`](file:///data/dev/cp/lib/compress/):
  - 新增 [`lib/compress/_codecs.cp`](file:///data/dev/cp/lib/compress/_codecs.cp)：Zlib / Brotli / Zstd C-FFI 声明、`ZlibFlush`、`ZlibStatus`、`CompressionFormat` 与 `CompressBoundEstimator`；
  - 新增 [`lib/compress/checksum.cp`](file:///data/dev/cp/lib/compress/checksum.cp)：IEEE 802.3 `Crc32` 与 RFC 1950 `Adler32` 校验器；
  - 新增 [`lib/compress/gzip.cp`](file:///data/dev/cp/lib/compress/gzip.cp)：RFC 1952 `GzipHeader` 帧头/帧尾编解码与 `RleCodec` 字节压缩器；
  - 新增 [`lib/compress/index.cp`](file:///data/dev/cp/lib/compress/index.cp)：`cap:compress` 对外统一出口。
- [`cap/test/test_compress.cp`](file:///data/dev/cp/cap/test/test_compress.cp):
  - 新增覆盖压缩协商、CRC32/Adler32 标准向量、Gzip 帧头/帧尾与 RleCodec 的 4 大测试套件（4/4 PASS）。
- [`lib/sourcemap/`](file:///data/dev/cp/lib/sourcemap/):
  - 新增 [`lib/sourcemap/vlq.cp`](file:///data/dev/cp/lib/sourcemap/vlq.cp)：7 字节栈内联 Base64 `VLQ` 与 `VLQResult` 编解码器；
  - 新增 [`lib/sourcemap/line_offset.cp`](file:///data/dev/cp/lib/sourcemap/line_offset.cp)：`SourceLocation` 与二分行偏移查找表 `LineOffsetTable<const MAX_LINES: usize>`；
  - 新增 [`lib/sourcemap/mapping.cp`](file:///data/dev/cp/lib/sourcemap/mapping.cp)：`SourceMapping` 与 SourceMap v3 映射表解析器 `ParsedSourceMap<const CAP: usize>`；
  - 新增 [`lib/sourcemap/index.cp`](file:///data/dev/cp/lib/sourcemap/index.cp)：`cap:sourcemap` 对外统一出口。
- [`cap/test/test_sourcemap.cp`](file:///data/dev/cp/cap/test/test_sourcemap.cp):
  - 新增覆盖 Base64 VLQ、二分行偏移表与 SourceMap v3 映射反查的 3 大测试套件（3/3 PASS）。
- [`lib/s3/`](file:///data/dev/cp/lib/s3/):
  - 新增 [`lib/s3/types.cp`](file:///data/dev/cp/lib/s3/types.cp)：`S3Acl`、`S3StorageClass`、`S3SignErrorCode` 与 `S3Error`；
  - 新增 [`lib/s3/credentials.cp`](file:///data/dev/cp/lib/s3/credentials.cp)：`S3Credentials` 凭证校验与 Virtual-Hosted / Path-Style 端点构造；
  - 新增 [`lib/s3/signing.cp`](file:///data/dev/cp/lib/s3/signing.cp)：AWS Signature Version 4 (`AWS4-HMAC-SHA256`) 四级密钥派生与请求签名器 `SigV4Signer`；
  - 新增 [`lib/s3/index.cp`](file:///data/dev/cp/lib/s3/index.cp)：`cap:s3` 对外统一出口。
- [`cap/test/test_s3.cp`](file:///data/dev/cp/cap/test/test_s3.cp):
  - 新增覆盖 S3 ACL/StorageClass、凭证 URL 格式化与 AWS SigV4 官方测试向量的 3 大测试套件（3/3 PASS）。

- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs), [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/diagnostic.rs`](file:///data/dev/cp/cpc/src/diagnostic.rs), [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs), [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs), [`cpc/src/codegen/`](file:///data/dev/cp/cpc/src/codegen/)（及 `cpc_dev/src/*` 镜像同步）:
  - 结单 `task-20261114-enum-variant-name-and-top-string-const`（枚举显式 `name` 变体防冲突与顶层无类型字符串常量输出）；
  - 实装正交参数体系（接收者省略 `_` / `*_` / `move _`，具名参数 `v: T` / `v: *T` 编译期自动推导值拷贝或可变借用 / 前置 `move v: T`、`own v: *T`）；
  - 实装 `cpc compile`、`cpc check`、`cpc check --json` 单次全文件多错误聚合诊断输出；
  - 实装免 `import` 一等公民内置 `JSON`（`JSON.stringify`、`JSON.parse<T>`、`JSON.parse`）及 `sema.rs` 全局预注册 `std`/`builtin`；
  - 新增 `--nest`（`-n`）命令行标志与 `@is_nest` 编译期常量（兼容 `--beam` / `@is_beam`）；
  - 支持 `@c { ... }` / `pub @c { ... }` 内部免 `pub extern` 极简声明、`enum` 方法默认 `pub`、`= []` 空切片字面量与类 `_create` 默认字段值初始化、`* = expr;` 返回值丢弃及全场景省略 `: void`；
  - 新增 `E0103`/`E0104`/`E0115`/`E0116`/`E0118`/`E0119`/`E0120`/`E0121`/`E0122`/`E0123`/`E0125` 智能拦截与修复建议（保留 `while (cond) : (step)` 与 `@xxx` 宏）。
- [`lib/crypto/_boringssl.cp`](file:///data/dev/cp/lib/crypto/_boringssl.cp), [`lib/crypto/tls.cp`](file:///data/dev/cp/lib/crypto/tls.cp) & [`lib/crypto/index.cp`](file:///data/dev/cp/lib/crypto/index.cp):
  - 新增 BoringSSL 底垫片引用与 X.509 SAN / DNS 通配符主机名校验库 `tls.cp`，并在 `lib/crypto/index.cp` 统一导出；
  - 完成 `lib/` 下全部 144 个 `.cp` 标准库文件的正交语法净化（144/144 零错误通过 `cpc check`）。
- [`cp_ref.md`](file:///data/dev/cp/cp_ref.md) & [`cp_agent_ref.md`](file:///data/dev/cp/cp_agent_ref.md):
  - 同步更新双轨语法规范手册，写入正交参数修饰符、内置 `JSON`、`@c` 块、`@is_nest` 及 Top 16 绝对禁手清单。
- [`cap/test/test_syntax_orthogonality.cp`](file:///data/dev/cp/cap/test/test_syntax_orthogonality.cp):
  - 新增覆盖正交语法、`v: *T` 自动推导、内置 `JSON`、预注册 `std` 与 BoringSSL TLS 证书校验的 4 大端到端测试套件（4/4 PASS）。

- [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs), [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs), [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs), [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs), [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs), [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)（及 `cpc_dev/src/*` 镜像同步）:
  - 新增 `Token::PlusPlus` (`++`) 与 `BinaryOp::Concat` 支持（结单 `task-20261114-concat-plusplus-operator`）；
  - 为 prelude 辅助函数形参与局部变量统一添加 `__cp_` 前缀防止同名遮蔽（结单 `task-20261114-prelude-ptr-shadow`）；
  - 升级内置 `JSON.stringify`、`JSON.parse<T>`、`JSON.parse` 采用 `std.heap.smp_allocator` 与 `.allocate = .alloc_if_needed` 零拷贝借用；
  - 升级内置 `RegExp`（`emit_regex_prelude`）：内联 16 字节 `@Vector(16, u8)` SIMD 字面量前缀快跳与 `[2048]u8` 栈缓冲区零堆分配快速路径，修复正则字面量反斜杠转义；
  - 删除冗余空壳文件 `lib/regex.cp`；
  - 扩充 [`cap/test/test_syntax_orthogonality.cp`](file:///data/dev/cp/cap/test/test_syntax_orthogonality.cp) 第 5 项 `RegExp` 测试套件（5/5 PASS）。
- [`cpc/src/diagnostic.rs`](file:///data/dev/cp/cpc/src/diagnostic.rs), [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs), [`cpc/src/main.rs`](file:///data/dev/cp/cpc/src/main.rs), [`scripts/sync_cpc.sh`](file:///data/dev/cp/scripts/sync_cpc.sh)（及 `cpc_dev/src/*` 镜像同步）:
  - 新增 `cpc check --brief` (`-b`)、`cpc check --json --brief`、`cpc check --max <N>`、`cpc check --fix` 与 `cpc fix` 子命令；
  - 新增 `mark_cascades` 连锁错误识别与 `cascade_of` 标注/折叠，以及 `errors`、`root_errors`、`cascades_hidden`、`fixable` 汇总计数；
  - 为 `E0101`..`E0104`, `E0115`, `E0116`, `E0118`, `E0119`..`E0123`, `E0126` 填充 `{span_start, span_end, replacement}` 机械修复 `edits` 并实装 `apply_fixes` 多轮自动修复；
  - 新增 `if (let v = opt)` / `while (let v = opt)` 解析恢复与 `E0126` 自动转 `:=` 诊断；
  - 修复 `.{ key: val }`、`ErrorSet!void` 与 `error{ ... }` 解析，消除假性连锁报错。

## [2026-11-15] - 核心里程碑：`bun_agy` 4 项编译器急修结单、标准库 4 大目录瘦身合并与 10 大新模块 + 4 大模块增强全面落地

### 编译器协同热修 (Compiler Hotfixes & Resolver)
- [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs) & [`cpc_dev/src/parser.rs`](file:///data/dev/cp/cpc_dev/src/parser.rs):
  - 拦截 `let/const x: auto = undefined;` 组合并输出三段式诊断 `E0212: cannot infer type 'auto' from 'undefined'; explicit concrete type is required`（结单 `task-20261115-ban-auto-undefined-semantic`）。
- [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs) & [`cpc_dev/src/codegen/mod.rs`](file:///data/dev/cp/cpc_dev/src/codegen/mod.rs):
  - 新增 `lower_extra_zig_member()`：对 `struct`/`enum` 内部 `extra_zig` 常量/变量声明穿透执行完整表达式 lowering（如 `.{ had_flag: false }` -> `.{ .had_flag = false }`），结单 `task-20261115-nested-const-codegen-lowering`。
- [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs) & [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs):
  - 将 `Expr::Zig` 参数引用检测升级为精确词边界匹配 `code_contains_word`，补全非实例方法 `self` 形参、`comptime` 形参的 `_ = param;` 注入以及未引用局部 `const` 的 `_ = &var;` 注入（结单 `task-20261115-unused-param-codegen-silence` 与 `task-20261115-cpc-fix-e0120-e0122`）。
- [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs) & [`cpc_dev/src/resolver.rs`](file:///data/dev/cp/cpc_dev/src/resolver.rs):
  - 实现 `cap:col`/`cap:collections`、`cap:event`/`cap:event_loop`/`cap:events`、`cap:conf`/`cap:dotenv`/`cap:ini`、`cap:pack`/`cap:compress` 双向兼容模块路径映射。

### 标准库目录瘦身合并 (Phase A: Consolidated Modules)
- [`lib/col/`](file:///data/dev/cp/lib/col/) (`cap:col`)：由 `lib/collections/` 重命名瘦身，新增吸收自 Bun 的 256 位静态位集 [`lib/col/bit_set.cp`](file:///data/dev/cp/lib/col/bit_set.cp) (`BitSet256`)。
- [`lib/event/`](file:///data/dev/cp/lib/event/) (`cap:event`)：合并原 `lib/event_loop/` 与 `lib/events.cp`（迁入 [`lib/event/emitter.cp`](file:///data/dev/cp/lib/event/emitter.cp)）。
- [`lib/conf/`](file:///data/dev/cp/lib/conf/) (`cap:conf`)：合并原 `lib/dotenv/` 与 `lib/ini/`，并吸收 Bun 的 [`lib/conf/toml.cp`](file:///data/dev/cp/lib/conf/toml.cp)、[`lib/conf/yaml.cp`](file:///data/dev/cp/lib/conf/yaml.cp)、[`lib/conf/json5.cp`](file:///data/dev/cp/lib/conf/json5.cp)。
- [`lib/pack/`](file:///data/dev/cp/lib/pack/) (`cap:pack`)：由 `lib/compress/` 升级，保留流式压缩并吸收 Bun 的 [`lib/pack/_archive.cp`](file:///data/dev/cp/lib/pack/_archive.cp) 与 [`lib/pack/tar.cp`](file:///data/dev/cp/lib/pack/tar.cp)（POSIX USTAR 归档打包与零拷贝 `TarReader`）。

### 新建 10 大简洁标准库模块 (Phase B: New Concise Modules)
- [`lib/redis/`](file:///data/dev/cp/lib/redis/) (`cap:redis`)：`resp.cp`、`index.cp`（RESP2/RESP3 零拷贝协议帧解析与编码器）。
- [`lib/cli/`](file:///data/dev/cp/lib/cli/) (`cap:cli`)：`args.cp`、`tty.cp`、`index.cp`（命令行标志解析、ANSI 转义剥离与终端进度条 `ProgressBar`）。
- [`lib/watch/`](file:///data/dev/cp/lib/watch/) (`cap:watch`)：`watcher.cp`、`index.cp`（跨平台文件变更监控掩码与路径表 `FileWatcher`）。
- [`lib/patch/`](file:///data/dev/cp/lib/patch/) (`cap:patch`)：`diff.cp`、`index.cp`（Unified Diff 解析与 Hunk 渲染应用）。
- [`lib/bench/`](file:///data/dev/cp/lib/bench/) (`cap:bench`)：`timer.cp`、`index.cp`（纳秒级硬件单调计时器 `Stopwatch` 与分位数统计 `BenchStats`）。
- [`lib/elf/`](file:///data/dev/cp/lib/elf/) (`cap:elf`)：`format.cp`、`index.cp`（ELF64 / Mach-O / PE 二进制格式嗅探与头部解析）。
- [`lib/md/`](file:///data/dev/cp/lib/md/) (`cap:md`)：`markdown.cp`、`index.cp`（CommonMark/GFM 解析与 HTML/ANSI 渲染）。
- [`lib/html/`](file:///data/dev/cp/lib/html/) (`cap:html`)：`selector.cp`、`index.cp`（CSS 选择器解析与流式 `HtmlRewriter`）。
- [`lib/ffi/`](file:///data/dev/cp/lib/ffi/) (`cap:ffi`)：`_tcc.cp`、`dlopen.cp`、`index.cp`（动态库加载 `DynLib`、FFI 签名布局与 TinyCC 声明）。
- [`lib/sql/`](file:///data/dev/cp/lib/sql/) (`cap:sql`)：`protocol.cp`、`index.cp`（PostgreSQL v3 Wire 帧、MySQL 包头编解码与预编译语句绑定）。

### 现有 4 大模块能力扩充 (Phase C: Module Extensions)
- [`lib/http/`](file:///data/dev/cp/lib/http/)：新增 [`cookie.cp`](file:///data/dev/cp/lib/http/cookie.cp) (`CookieMap`) 与 [`form_data.cp`](file:///data/dev/cp/lib/http/form_data.cp) (`FormData`)。
- [`lib/net/`](file:///data/dev/cp/lib/net/)：新增 [`udp.cp`](file:///data/dev/cp/lib/net/udp.cp) (`UdpSocket`, `UdpPacket`) 与 `SocketAddress.parseIpv4`。
- [`lib/mem/`](file:///data/dev/cp/lib/mem/)：扩充 [`cow.cp`](file:///data/dev/cp/lib/mem/cow.cp) (`CowSlice`) 与 [`tagged_ptr.cp`](file:///data/dev/cp/lib/mem/tagged_ptr.cp) (`TaggedPtr`)。
- [`lib/string/`](file:///data/dev/cp/lib/string/)：新增 [`grapheme.cp`](file:///data/dev/cp/lib/string/grapheme.cp) (`Utf8Grapheme` UTF-8 校验与 UAX #29 字素簇计数)。
- 测试套件：新增 [`cap/test/test_stdlib_phase_b1.cp`](file:///data/dev/cp/cap/test/test_stdlib_phase_b1.cp)（8/8 PASS）与 [`cap/test/test_stdlib_phase_b2_c.cp`](file:///data/dev/cp/cap/test/test_stdlib_phase_b2_c.cp)（9/9 PASS）。

### 2026-11-15（续） 双子智能体并行深化 12 个标准库子模块与第 6 项 `bun_agy` 急修结单
- **编译器热修 (`cpc/src/` & `cpc_dev/src/`)**：
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：修复 `generate_zig_or_struct` 中泛型函数返回匿名 `struct`（`return struct { ... }`）内省略 `_` 的只读实例方法被误判为 `static` 的问题（结单 `task-20261115-generic-return-struct-readonly-receiver`）。
- **双子智能体并行深化标准库模块 (`lib/`)**：
  - [`lib/sql/postgres.cp`](file:///data/dev/cp/lib/sql/postgres.cp) & [`lib/sql/mysql.cp`](file:///data/dev/cp/lib/sql/mysql.cp)：PostgreSQL v3 认证挑战/数据行/命令完成帧与 MySQL `lenenc_int`/能力位图/OK/ERR 包解析；
  - [`lib/redis/command.cp`](file:///data/dev/cp/lib/redis/command.cp)：RESP2 命令编码器 `RedisCommand` 与批量管道 `RedisPipeline`；
  - [`lib/http/h2/hpack.cp`](file:///data/dev/cp/lib/http/h2/hpack.cp)：RFC 7541 HTTP/2 HPACK 整数前缀、静态表与首部字段编解码器 `HpackCodec`；
  - [`lib/col/object_pool.cp`](file:///data/dev/cp/lib/col/object_pool.cp)：零堆碎片侵入式空闲链表对象池 `ObjectPool<T, CAP>`；
  - [`lib/pack/zip.cp`](file:///data/dev/cp/lib/pack/zip.cp)：PKZIP Local/Central/EOCD 解析与零拷贝 `ZipReader`；
  - [`lib/md/table.cp`](file:///data/dev/cp/lib/md/table.cp)：GFM 表格对齐与 HTML `<table>` 渲染器 `MdTable`；
  - [`lib/html/escape.cp`](file:///data/dev/cp/lib/html/escape.cp)：零拷贝快跳 HTML 实体转义与反转义器 `HtmlEscape`；
  - [`lib/cli/table.cp`](file:///data/dev/cp/lib/cli/table.cp)：终端 ASCII 表格格式化渲染器 `CliTable`；
  - [`lib/watch/filter.cp`](file:///data/dev/cp/lib/watch/filter.cp)：文件监控忽略过滤 `WatchFilter` 与防抖器 `DebounceTracker`；
  - [`lib/patch/myers.cp`](file:///data/dev/cp/lib/patch/myers.cp)：行级差异比对与 Unified Diff 生成器 `LineDiffer`；
  - [`lib/bench/histogram.cp`](file:///data/dev/cp/lib/bench/histogram.cp)：8 桶对数纳秒延迟直方图 `LatencyHistogram`。
- **新增端到端测试套件**：
  - [`cap/test/test_sql_redis_hpack.cp`](file:///data/dev/cp/cap/test/test_sql_redis_hpack.cp)（4/4 PASS）；
  - [`cap/test/test_col_pack_md_html.cp`](file:///data/dev/cp/cap/test/test_col_pack_md_html.cp)（4/4 PASS）；
  - [`cap/test/test_cli_watch_patch_bench.cp`](file:///data/dev/cp/cap/test/test_cli_watch_patch_bench.cp)（4/4 PASS）。

### 2026-11-15（续 2） 第二波双子智能体并行深化 8 个标准库子模块
- **新增与更新标准库文件 (`lib/`)**：
  - [`lib/elf/macho_pe.cp`](file:///data/dev/cp/lib/elf/macho_pe.cp) & [`lib/elf/index.cp`](file:///data/dev/cp/lib/elf/index.cp)：Mach-O 64 位头部与 PE/COFF 头部解析器（`MachHeader64`, `MachoInspector`, `PeCoffHeader`, `PeInspector`）；
  - [`lib/ffi/layout.cp`](file:///data/dev/cp/lib/ffi/layout.cp) & [`lib/ffi/index.cp`](file:///data/dev/cp/lib/ffi/index.cp)：C-ABI 结构体字段偏移与自然对齐计算器（`CStructLayout`, `CStructField`）；
  - [`lib/net/proxy.cp`](file:///data/dev/cp/lib/net/proxy.cp) & [`lib/net/index.cp`](file:///data/dev/cp/lib/net/index.cp)：HTTP `CONNECT` 代理隧道与 SOCKS5 握手/连接帧编解码器（`HttpConnectTunnel`, `Socks5Codec`）；
  - [`lib/conf/getter.cp`](file:///data/dev/cp/lib/conf/getter.cp) & [`lib/conf/index.cp`](file:///data/dev/cp/lib/conf/index.cp)：配置值强类型解析器 `ConfValueParser`（布尔、端口、时长 `ms/s/m/h`、容量 `b/kb/mb/gb`）；
  - [`lib/encoding/hex.cp`](file:///data/dev/cp/lib/encoding/hex.cp) & [`lib/encoding/index.cp`](file:///data/dev/cp/lib/encoding/index.cp)：零堆分配查表十六进制编解码器 `Hex`；
  - [`lib/url/percent.cp`](file:///data/dev/cp/lib/url/percent.cp)、[`lib/url/query.cp`](file:///data/dev/cp/lib/url/query.cp) & [`lib/url/index.cp`](file:///data/dev/cp/lib/url/index.cp)：RFC 3986 URI 百分号编解码器 `PercentEncoding`；
  - [`lib/string/case.cp`](file:///data/dev/cp/lib/string/case.cp) & [`lib/string/index.cp`](file:///data/dev/cp/lib/string/index.cp)：标识符命名风格转换器 `CaseConverter`（`toSnakeCase`, `toKebabCase`, `toCamelCase`, `toPascalCase`, `toScreamingSnakeCase`）；
  - [`lib/router/pattern.cp`](file:///data/dev/cp/lib/router/pattern.cp)、[`lib/router/radix.cp`](file:///data/dev/cp/lib/router/radix.cp) & [`lib/router/index.cp`](file:///data/dev/cp/lib/router/index.cp)：路由模板参数提取与反向 URL 格式化器 `RouteFormatter`。
- **新增端到端测试套件**：
  - [`cap/test/test_elf_ffi_net_conf.cp`](file:///data/dev/cp/cap/test/test_elf_ffi_net_conf.cp)（4/4 PASS）；
  - [`cap/test/test_hex_url_case_router.cp`](file:///data/dev/cp/cap/test/test_hex_url_case_router.cp)（4/4 PASS）。

### 2026-11-15（续 5） 顶层原生类型遮蔽修复、双子智能体拓扑落地与任务全面汇报
- **编译器前端与 Codegen 修复 (`cpc/src/codegen/mod.rs`)**：
  - 在顶层变量遍历输出时增加 Zig 原生基本类型过滤（跳过 `u8`, `i8`, `u16`, `i16`, `u32`, `i32`, `u64`, `i64`, `usize`, `isize`, `f32`, `f64`, `bool`, `void`, `anyopaque`），彻底消除 `const u8 = undefined;` 导致的 `name shadows primitive 'u8'` 编译阻断；
  - 调用 `scripts/sync_cpc.sh` 原子构建并秒级部署至全局 `cap/bin/cpc` 与 `target/release/cpc`。
- **架构拓扑规范确立**：
  - 明确划分 `Hotfix Engineer`（基于 `cpc/` 直修发布，优先释放下游工单）与 `Core Compiler Researcher`（基于安全沙盒与 `cpc_dev` 专心自研攻坚）；
  - 常驻守护进程 `task-32193` 全天候监控下游工单信箱，确保通信零遗漏。

- **下游 CapSurf 紧急热修（3 项 100% 验证结单）**：
  - `task-capsurf-20261115-struct-associated-fn-lowering`：修复 `cpc/src/codegen/actor.rs` 关联方法降级逻辑，使结构体关联方法正确生成为静态函数；
  - `task-capsurf-20261115-enum-method-receiver`：修复 `cpc/src/codegen/mod.rs` 与 `types.rs` 中 enum 声明注册与方法接收者类型推断，避免类型退化为 `*anyopaque`；
  - `task-capsurf-20261115-instance-method-with-params`：修复带参数实例方法 receiver 判定顺序，使 `Point.add(x)` 正确保留 `self: Point`。
- **CSS Parser（7,333 行）核心自研攻坚突破**：
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：
    - 修复类型参数解析中跨 Token 空格保留机制（消灭 `[]constu8` 词法粘连）；
    - 修复 `parse_return_stmt` 支持 `else => return,` 无参带逗号 return；
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：
    - 在 `lower_struct_body` 中新增 `get_receiver_param_info`，全面支持 `*_` / `*this` / `*self` 以及带参数实例方法自动注入 `self: @This(), `；
    - 排除 `static fn` 的 receiver 注入；
    - 将 7,333 行 `css_parser.cp` 的 Zig AST 检查报错从 632+ 彻底消灭至仅剩 1 个借用冲突。
- **架构拓扑与多智能体分工**：
  - 启动后台常驻通信守护进程 `IPC_DAEMON`；
  - 启动热修专属子智能体（基于 `cpc` 目录极速结单下游工单）；
  - 启动自研攻关专属子智能体（基于 `cpc_dev` 目录彻底突破 CSS Parser 终局借用冲突并收敛标准库）。

- **多 AGY 隔离通信协议固化 (`GEMINI.md`)**：
  - [`GEMINI.md`](file:///data/dev/cp/GEMINI.md)：更新多工作区拓扑（新增 `capsurf_agy` `/data/dev/capsurf`）与第 七 章 4 条多 AGY 通信规则（双下游识别 `task-capsurf-*`、多 ID 批量 `resolve` 精准路由、`broadcast` 语法更新双端广播、`watch_task.py --status pending --creator all --timeout 0` 常驻监听）。
- **编译器热修与 CSS 语法自愈 (`cpc/src/` & `cpc_dev/src/`)**：
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：`has_top_string_decl` 检测 `ModuleItem::Import` 中的 `"string"` / `@"string"`，抑制顶部 `const string = []const u8;` 冲突（结单 `task-20261115-string-import-collision`）；
  - [`cpc/src/lexer.rs`](file:///data/dev/cp/cpc/src/lexer.rs)：新增字符字面量 `\xNN` / `\u{NNNN}` 转义与 `\\` 多行字符串词法支持；
  - [`cpc/src/ast.rs`](file:///data/dev/cp/cpc/src/ast.rs)、[`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)、[`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)、[`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：支持匿名嵌套 `struct { ... }` / `struct? { ... }`、内联可选泛型 `SmallList?(T, N)`、无载荷联合体变体、`ArrayList(T){}` / `@This(){}` 构造、`[]const u8` 类型实参、`Expr::If` 条件表达式 Walrus 解包捕获 `if (p := expr)`、`Decl::Class` 内 `extra_zig` 与 `nested_classes` 符号注册；
  - [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：注册 `"css" => "css".to_string()` 标准库通道映射。
- **新增 `cap:css` 标准库 (`lib/css/` 共 99 个 `.cp` 文件)**：
  - 顶层门面 API（5 个）：[`lib/css/index.cp`](file:///data/dev/cp/lib/css/index.cp)、[`lib/css/color.cp`](file:///data/dev/cp/lib/css/color.cp)、[`lib/css/length.cp`](file:///data/dev/cp/lib/css/length.cp)、[`lib/css/selector.cp`](file:///data/dev/cp/lib/css/selector.cp)、[`lib/css/stylesheet.cp`](file:///data/dev/cp/lib/css/stylesheet.cp)；
  - 完整吸收 `/data/dev/bun/src/css/` 全部 94 个 `.cp` 模块文件（根目录 15 个引擎文件 + `selectors/*.cp` 3 个 + `rules/*.cp` 22 个 + `values/*.cp` 22 个 + `properties/*.cp` 32 个），99/99 文件 100% 通过 `cpc check --brief`。
- **新增端到端测试套件**：
  - [`cap/test/test_css.cp`](file:///data/dev/cp/cap/test/test_css.cp)（5/5 100% PASS）。

- **Bun CSS 模块 95 源码文件端到端转译突破（通过率提升至 80.0% / 76 个文件 PASS）**：
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：
    - 在对象字面量解析（`parse_object_fields`）中支持前导 `Token::Dot`，全面兼容 `{ prop: "...", .ty = Value }`；
    - 在类体（`parse_class`）成员识别中放行 `comptime { ... }` 作为 `extra_zig` 原生直通 Zig 结构体；
    - 针对 `struct? { ... }` / `union? { ... }` 自动包装为 `Type::Optional`，解决可选类型匹配；
    - 宽容支持 `?T` 前缀可选类型语法（解除 E0115 硬阻断）；
    - 支持 `@This(){ ... }` 实例化表达式。
  - [`cpc/src/sema.rs`](file:///data/dev/cp/cpc/src/sema.rs)：
    - 在 `can_coerce` 中新增 `BorrowType` 到 `BorrowType` 双向兼容分支，支持解引用多级指针赋值。
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：
    - 修正 `get_func_receiver_mode`：优先判定 `func.is_static`，且过滤 `_: void` 等占位参数作为伪 receiver，杜绝 `self` 变量作用域遮蔽（`angle.cp`, `time.cp` 完全通过）。
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：
    - `has_top_string_decl` 检查 `Import` 声明避免 `duplicate struct member name 'string'`（`values.cp` 通过）；
    - 废除危险的 `: {` 匹配规则，杜绝 `blk: {`、`brk: {` 标签块被误替换为 `.{`；
    - 在 `lower_struct_body` 中将 `static fn` 去关键字化为 `pub fn`（`calc.cp` 通过）；
    - 为省略返回类型的函数精准补充 ` void `（`image.cp` 等通过）；
    - 新增 `lower_inline_struct_lit` 自动将单行 `.{ key: val }` 转为 `.{ .key = val }`。

### 2026-11-16
- **CSS 全量 95 源码模块纯正语法清洗与 100% 满贯达成 (95/95 PASS)**：
  - 严格捍卫 CP 纯正语法规范，在 `.cp` 源码端对标原始 `xxx_.zig` 消除全部非法语法；
  - **Values 模块群全面规范化**（9/9 PASS）：
    - [`values/color.cp`](file:///data/dev/bun/src/css/values/color.cp)、[`values/length.cp`](file:///data/dev/bun/src/css/values/length.cp)、[`values/syntax.cp`](file:///data/dev/bun/src/css/values/syntax.cp)：将所有 `pub union(enum)` 全量改造为纯正 CP Tagged Union `pub enum Name { ... }`；
    - [`values/length.cp`](file:///data/dev/bun/src/css/values/length.cp)：消除 Zig 风格 `const value, const unit = ...`，重构为标准元组解构 `let [value, unit] = ...`；具名结构体构造规范为 `TypeName{ field: val }`；
    - [`values/ident.cp`](file:///data/dev/bun/src/css/values/ident.cp)：重命名保留字 `ref` 为 `ref_val`，消除表达式层 `[]const u8` 为 `string`；
    - [`values/syntax.cp`](file:///data/dev/bun/src/css/values/syntax.cp)：拆分内联匿名结构体为 `pub struct RepeatedComponent`，规范状态机单点出口。
  - **Properties 模块群与 SmallList 全面规范化**（33/33 PASS）：
    - [`properties/align.cp`](file:///data/dev/bun/src/css/properties/align.cp) 等：匿名结构体字面量规范为 `.{ .result = ... }`，消除 switch 分支歧义；
    - [`small_list.cp`](file:///data/dev/bun/src/css/small_list.cp)：恢复 `pfi.slice()` 契约；
    - [`properties/animation.cp`](file:///data/dev/bun/src/css/properties/animation.cp)：4 处联合枚举重构为 `pub enum`。
  - **CSS Parser 核心巨构与媒体查询**（3/3 PASS）：
    - [`css_parser.cp`](file:///data/dev/bun/src/css/css_parser.cp)：7,400 行超级解析器核心消灭第 4467 行残留 Zig `if (...) |...|` 解包语法，修复 Tokenizer 闭合断点；
    - [`css_parser.d.cp`](file:///data/dev/bun/src/css/css_parser.d.cp)：废除 TS 箭头语法，重构为标准函数指针类型 `*const fn (*Parser) Result(T)`。
- **`lib/css/` 标准库全量同步**：
  - 完整同步 Bun CSS 成果至 [`lib/css/`](file:///data/dev/cp/lib/css/)，共计 100 个 `.cp` 文件（包含顶层门面 API），100/100 100% 通过 `cpc --lib` 与 `zig ast-check`。
- **两大重点资产群全量吸收与极简标准库落地 (100% PASS)**：
  - **资产群一（基础高并发与容器库）**：
    - [`lib/col/hash_map.cp`](file:///data/dev/cp/lib/col/hash_map.cp)：新增零堆分配 Robin-Hood 线性探查紧凑哈希表 `StaticHashMap<K, V, CAP>`；
    - [`lib/col/bounded_array.cp`](file:///data/dev/cp/lib/col/bounded_array.cp)、[`lib/col/bit_set.cp`](file:///data/dev/cp/lib/col/bit_set.cp)、[`lib/col/hive_array.cp`](file:///data/dev/cp/lib/col/hive_array.cp)、[`lib/col/object_pool.cp`](file:///data/dev/cp/lib/col/object_pool.cp)：规范方法 receiver，消除未使用参数警告与伪 receiver；
    - [`lib/string/smol.cp`](file:///data/dev/cp/lib/string/smol.cp)：新增 15 字节纯栈 SSO 小字符串 `SmolStr` 与堆回退；
    - [`lib/string/hashed.cp`](file:///data/dev/cp/lib/string/hashed.cp)：新增 O(1) 预哈希字符串 `HashedString`；
    - [`lib/string/path.cp`](file:///data/dev/cp/lib/string/path.cp)：新增零分配路径视图 `PathString`；
    - [`lib/string/joiner.cp`](file:///data/dev/cp/lib/string/joiner.cp)：新增高效字符串拼接器 `StringJoiner`；
    - [`lib/sync/pool.cp`](file:///data/dev/cp/lib/sync/pool.cp)：新增线程安全对象包裹器 `Guarded<T>` 与工作线程池队列 `WorkPool`；
    - [`lib/sync/futex.cp`](file:///data/dev/cp/lib/sync/futex.cp)、[`lib/sync/condition.cp`](file:///data/dev/cp/lib/sync/condition.cp)、[`lib/sync/wait_group.cp`](file:///data/dev/cp/lib/sync/wait_group.cp)、[`lib/sync/unbounded_queue.cp`](file:///data/dev/cp/lib/sync/unbounded_queue.cp)：消灭嵌套结构体 Zig 风格分号语法，规范为 CP 纯正语法与借用契约；
    - [`lib/event/task.cp`](file:///data/dev/cp/lib/event/task.cp)：新增动态任务执行器 `AnyTask` 与堆管理任务 `ManagedTask`；
    - 门面 API 同步导出：[`lib/col/index.cp`](file:///data/dev/cp/lib/col/index.cp)、[`lib/string/index.cp`](file:///data/dev/cp/lib/string/index.cp)、[`lib/sync/index.cp`](file:///data/dev/cp/lib/sync/index.cp)、[`lib/event/index.cp`](file:///data/dev/cp/lib/event/index.cp)。
  - **资产群二（应用与网络协议核心库）**：
    - [`lib/sql/sqlite.cp`](file:///data/dev/cp/lib/sql/sqlite.cp)：实现原生 SQLite3 数据库引擎抽象 `SqliteDatabase` 与查询游标 `SqliteRows`；
    - [`lib/sql/index.cp`](file:///data/dev/cp/lib/sql/index.cp)：对外统一聚合导出 MySQL, PostgreSQL, SQLite3 数据库协议与驱动；
    - [`lib/http/server.cp`](file:///data/dev/cp/lib/http/server.cp)：修复 `Request` 符号导入；
    - [`lib/shell/`](file:///data/dev/cp/lib/shell/)（全新吸收）：实现原生轻量嵌入式 Shell 执行引擎与解释器：
      - [`lib/shell/lexer.cp`](file:///data/dev/cp/lib/shell/lexer.cp)：Shell 词法分析器，识别指令、管道、重定向、引号与环境变量；
      - [`lib/shell/parser.cp`](file:///data/dev/cp/lib/shell/parser.cp)：Shell 语法解析器与 AST 语句结构树构建；
      - [`lib/shell/exec.cp`](file:///data/dev/cp/lib/shell/exec.cp)：执行调度引擎，实现多级管道进程间通信与 I/O 重定向；
      - [`lib/shell/builtin.cp`](file:///data/dev/cp/lib/shell/builtin.cp)：内置指令实现（`echo`, `cd`, `pwd`, `which`, `export`, `exit`）；
      - [`lib/shell/index.cp`](file:///data/dev/cp/lib/shell/index.cp)：门面导出极简 `sh(cmd)` 与 `Shell.run(cmd)`。
- **编译器与工程配置增强**：
  - [`cpc/src/codegen/prelude.rs`](file:///data/dev/cp/cpc/src/codegen/prelude.rs)：扫描 `f.default` 字段默认表达式与 `extra_zig` 内位移操作符；对 `@create_pending_task`、`@complete_task`、`@task_spawn`、`@task_join` 标记 `needs_actors = true`；
  - [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：增加 `cap:sync` / `cap:threading` 及 `cap:shell` / `cap:sh` 标准库别名与重映射路径；
  - [`cap.json`](file:///data/dev/cp/cap.json)：根工程加入 `build.link_libs: ["sqlite3"]`；
  - 编译器代码直通同步至 `cpc_dev`。
- **新增综合测试套件**：
  - [`cap/test/test_bun_absorbed_assets.cp`](file:///data/dev/cp/cap/test/test_bun_absorbed_assets.cp)：涵盖 `cap:string`, `cap:sync`, `cap:event`, `cap:sql`, `cap:shell` 综合端到端测试（6/6 100% PASS）；
  - [`cap/test/test_collections_bun.cp`](file:///data/dev/cp/cap/test/test_collections_bun.cp)：6/6 100% PASS。
- **新增原生图像处理标准库 `cap:image` (兼容 `cap:png` / `cap:img`)**：
  - [`lib/image/png.cp`](file:///data/dev/cp/lib/image/png.cp)：
    - 实现工业级纯 CP 原生 PNG 截图编码器 `Png.encodeRgba`（W3C 标准 8 字节魔数签名、IHDR 块构造、扫描线 None 滤波、Zlib 原生 Deflate 压缩生成 IDAT、IEND 闭合与 IEEE 802.3 CRC32 校验和）；
    - 实现 PNG 全滤镜逆运算解码器 `Png.decode`（IHDR 尺寸与颜色类型提取、IDAT 聚合流式解压、支持 None, Sub, Up, Average, Paeth 5 种滤波逆还原为 RGBA8 像素切片）；
    - 实现面向对象 `PngImage` 与像素结构体 `RgbaColor`（支持 `.getPixel(x, y)`、`.setPixel(x, y, r, g, b, a)`、`.encode()`、`.save(path)`）；
    - 实现同步文件系统保存 `Png.save(path, rgba, w, h)` 与加载 `Png.load(path)`；
  - [`lib/image/codecs.cp`](file:///data/dev/cp/lib/image/codecs.cp)：
    - 实现二进制魔数快速探测 `Codecs.sniff`（支持 PNG, JPEG, WebP, BMP, GIF, TIFF, AVIF, HEIC）；
    - 实现标准 MIME 映射 `Codecs.mime` 与文件后缀提取 `Codecs.fromExtension`；
    - 实现 EXIF Orientation 方向解析 `Codecs.getOrientationTransform`；
  - [`lib/image/bmp.cp`](file:///data/dev/cp/lib/image/bmp.cp)：
    - 实现 Windows DIB / BMP 格式解码器 `Bmp.decode` 与 32-bit 编码器 `Bmp.encodeRgba`（支持 4 字节跨距行对齐与倒序 Bottom-Up 扫描转换）；
  - [`lib/image/quantize.cp`](file:///data/dev/cp/lib/image/quantize.cp)：
    - 实现 Median-Cut 颜色量化器 `Quantizer.quantize`（RGBA8 转 256 色紧凑调色板与像素索引矩阵）；
  - [`lib/image/index.cp`](file:///data/dev/cp/lib/image/index.cp)：
    - 对外统一导出 `cap:image`、`cap:png`、`cap:img` 协议；
- **编译器与工程构建增强**：
  - [`cpc/src/resolver.rs`](file:///data/dev/cp/cpc/src/resolver.rs)：注册 `"image" | "img" | "png" => "image".to_string()`；
  - [`cap.json`](file:///data/dev/cp/cap.json)：引入 `"z"` 动态链接库支持原生硬件极速压缩；
  - 同步部署至 `cpc_dev`。
- **新增综合测试套件**：
  - [`cap/test/test_image_png.cp`](file:///data/dev/cp/cap/test/test_image_png.cp)：包含格式嗅探、RGBA8 截图编码、IHDR 签名校验、像素往返解码比对、文件保存与加载、BMP 编解码、颜色量化 6 大测试用例（**6/6 100% PASS**）。

---

### [2026-11-16] 编译器语义自愈升级、Zig 0.17 全套适配与 BEAM/集合类型深度解耦

- **编译器核心修改**：
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：
    - 解耦非 async 普通函数返回类型与 BEAM 模式，恢复强类型 `ret_ty` 返回签名；
    - 移除普通结构体函数在 BEAM 模式下无 return 时的错误原子发射；
    - 在状态机代码生成结束点（`emit_current_case` 闭合处）重置 `self.is_state_machine = false`，防止后续全局声明误用 `ctx.arena`；
    - 区分引用类（`ref_classes`）与值结构体（`value_classes`）的默认接收者推断；
    - 消除 Zig 0.17 `pointless discard of function parameter`；
  - [`cpc/src/codegen/stmt.rs`](file:///data/dev/cp/cpc/src/codegen/stmt.rs)：
    - 修复 `Stmt::Return(None)`：仅在 `self.is_beam_mode && self.is_state_machine` 时发射 `beam.Beam.makeAtom(ctx.self.env.?, "ok")`，普通函数及非 actor 结构体方法恢复标准 `return;`；
  - [`cpc/src/codegen/pattern.rs`](file:///data/dev/cp/cpc/src/codegen/pattern.rs)：
    - 在 `generate_switch` 顶层增加 `is_result_disc` 拦截，判别式为错误联合类型时禁止进入 Zig 原生 switch，转入安全解包流；
    - 消除 Zig 0.17 `unused capture`，未引用的 arm 绑定自动生成 `else =>`；
    - 降解 `err.Variant` 为 Zig 原生 `error.Variant`；
    - 修复 `generate_err_value` 中 `Type::BoxDyn` 移动语义与变量识别；
  - [`cpc/src/codegen/borrow.rs`](file:///data/dev/cp/cpc/src/codegen/borrow.rs)：
    - 在 `is_zig_heap_ptr_type` 中补齐 `VecDeque`、`BinaryHeap`、`BTreeMap`、`BTreeSet`、`HashMap`、`HashSet` 及 `ref_classes` 堆指针识别，防止集合类型借用传递时发生双重取地址；
  - [`cpc/src/codegen/expr.rs`](file:///data/dev/cp/cpc/src/codegen/expr.rs)：
    - 无参 `new Class()` 在 comptime 校验 init 参数个数；
    - 修正类型投射与成员借用的指针判定；
  - [`cpc/src/parser.rs`](file:///data/dev/cp/cpc/src/parser.rs)：
    - 解除双指针及多级原始指针（如 `pp.*.age`）字段访问的硬编码拦截；
  - [`cpc/src/diagnostic.rs`](file:///data/dev/cp/cpc/src/diagnostic.rs)：
    - 落地 `E0130`（只读接收者自动插入 `*_`）、`E0133`（只读形参赋值双分支诊断建议）、`E0134`（关键字与保留字重命名及转义）；
- **任务协同与热修闭环**：
  - 结单 [`task-capsurf-20261116-beam-non-actor-return`](file:///data/dev/cp/.tasks/resolved/capsurf_agy/task-capsurf-20261116-beam-non-actor-return.json)；
  - 后台任务常驻监听守卫重新启动并保持活跃状态；
- **测试套件回归验证**：
  - `test_unified_errors.cp`、`test_trait_class_pass.cp`、`test_semver.cp`、`test_dec_pass.cp`、`test_unsafe_pass.cp`、`test_blob.cp`、`test_collections_matrix.cp`、`test_all.cp`、`test_stdlib_phase_b2_c.cp`、`test_basic.cp`、`test_string.cp`、`test_json.cp`、`test_crypto.cp` **100% PASS**。

- **热修性能优化 (`task-capsurf-20261116-codegen-quadratic-line-count`)**：
  - [`cpc/src/codegen/mod.rs`](file:///data/dev/cp/cpc/src/codegen/mod.rs)：
    - 新增 `OutputBuffer` 结构体，封装 `content` 与 `newline_count`，实现 `std::ops::Deref`、`std::fmt::Write`、`std::fmt::Display` 与 `push_buffer`；
    - 将 `current_generated_line()` 从 O(N) 全量字节扫描重构为 O(1) 增量整型加法，消除语句数膨胀时的 O(N^2) 性能二次爆炸（10,000 条语句转译从 18.58s 缩短至 0.21s，提速 88 倍）；
  - [`cpc/src/codegen/actor.rs`](file:///data/dev/cp/cpc/src/codegen/actor.rs)：
    - 使用 `self.output.push_buffer(&body_str)` 零扫描高效拼接函数体；
  - 结单 `task-capsurf-20261116-codegen-quadratic-line-count` 并同步回写 `cpc_dev`。








