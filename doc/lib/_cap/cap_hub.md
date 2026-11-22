# `cap` 根命名空间惰性挂载容器设计规范与验收报告

> [!NOTE]
> 本设计规范已归档至统一微核心目录 `doc/lib/_cap/`，根命名空间导入请使用 `import cap from "cap"`。

## 1. 核心目标与架构总览

通过 `import cap from "cap";`（或 `import c from "cap";`、`const cap = @import("cap");`）提供统一根命名空间挂载容器，并在 `cpc` 编译器中实现**编译期按需惰性模块挂载**：

- **零重依赖微核心（`lib/_cap/core.cp`）**：仅包含极薄的高频零依赖原语（内存/OOM、平台环境常量、断言、数值/切片原语、底层路径与 FD 常量、轻量输出与编译期映射），解析与转译开销极小。
- **静态路由表 + 词法精准扫描 + 传递不动点 BFS（`cpc/src/resolver.rs` / `cpc/src/modular.rs` / `cpc/src/main.rs`）**：
  - 自动跳过 `//` 单行注释、`/* ... */` 块注释、`"... "` 字符串字面量、`` `...` `` 模板字面量及 `'...'` 字符字面量，仅提取真实代码中的 `<alias>.<ident>` 成员访问。
  - 仅当 `<ident>` 命中权威路由表（59 个标准库映射）且磁盘对应 `.cp` 文件真实存在时，才将该子库纳入编译期解析与转译队列。
  - 支持传递不动点发现：被挂载的子库如果自身引用 `cap.<other>`，也会在同一轮 BFS / 不动点循环中自动发现并挂载。
  - 未被引用的 50+ 标准库（如 `css`/`http`/`sql`/`shell`/`md`/`s3`/`pack` 等）连磁盘都不读、不转译、不进入 Zig 编译。

---

## 2. 修改与新增文件清单

| 文件路径 | 职责说明 |
| :--- | :--- |
| [core.cp](file:///data/dev/cp/lib/_cap/core.cp) | 新建：零依赖微核心原语实现（纯正 CP + Zig 0.17 原生 API） |
| [index.cp](file:///data/dev/cp/lib/_cap/index.cp) | 新建：文档与静态检查入口（导出 `core.cp` 并注释惰性挂载路由清单） |
| [resolver.rs](file:///data/dev/cp/cpc/src/resolver.rs) | 新增 `CAP_HUB` 路由表 `resolve_cap_hub_target`、精确扫描器 `scan_cap_hub_members`、`extract_cap_hub_aliases`、`extract_top_level_zig_pub_names` 及单文件惰性收集 `resolve_imports_with_cap_hub` |
| [modular.rs](file:///data/dev/cp/cpc/src/modular.rs) | 模块模式（`--emit-dir`）惰性挂载：缓存版本升级至 v2，BFS 不动点发现按需子模块，自动合成 `_lib/_cap/index.zig`（仅在内容变化时写入保护增量缓存 `mtime`；不使用 Zig 0.17 已移除的 `usingnamespace`） |
| [main.rs](file:///data/dev/cp/cpc/src/main.rs) | 单文件模式（`cap test` / `cap build` / `cpc compile --output`）惰性挂载：将 `core.cp` 与按需子模块分别编译为独立隔离的 `.zig` 命名空间文件（`__cap_hub_<stem>/`）并合成 `index.zig`（stdout 预览模式提供内联 fallback） |
| [mod.rs](file:///data/dev/cp/cpc/src/codegen/mod.rs) | Codegen 支持单文件与模块模式下 `import <alias> from "cap"`、`const <alias> = @import("cap")` 及嵌入式子模块标记 `single_file_cap_submod` |
| [builtin.rs](file:///data/dev/cp/cpc/src/codegen/expr/builtin.rs) | 支持 `@import("cap")` / `@import("cap:core")` 映射，并修复 `@expect_eq` 在模块化模式下通过 `self.rt_sym("cp_eq")` 引用共享运行时 |
| [test_cap_lazy_hub.cp](file:///data/dev/cp/cap/test/test_cap_lazy_hub.cp) | 新建：覆盖微核心原语、默认 `cap` / 自定义别名 `c` / `@import("cap")` 别名及注释字符串隔离验证的综合测试 |

---

## 3. 微核心原语矩阵（`lib/_cap/core.cp`）

- **内存与错误**：`OOM`、`default_allocator`、`page_allocator`、`DefaultAllocator`、`handleOom`、`HandleOomReturn`、`outOfMemory`、`create`、`destroy`、`destroyWith`、`take`、`copy`、`zero`
- **环境与平台常量**：`Environment`（`isLinux`, `isMac`, `isDarwin`, `isWindows`, `isFreeBSD`, `isWasm`, `isPosix`, `isDebug`, `isRelease`, `isTest`, `allow_assert`, `isX64`, `isX86`, `isAarch64`, `is64Bit`, `os_tag`, `arch`, `mode`）
- **断言与全局**：`assert`、`debugAssert`、`assertWithLocation`、`Global`（`exit`, `panic`）
- **数值与切片原语**：`clamp`、`cast`、`range`、`asByteSlice`、`constStrToU8`、`sliceTo`
- **底层路径与 FD 基础类型**：`FD`（`i32`）、`invalid_fd`（`-1`）、`Mode`（`u32`）、`MAX_PATH_BYTES`（`4096`）、`PathBuffer`（`[4096]u8`）、`O`（基于 `std.os.linux.O` 的 `RDONLY`/`WRONLY`/`RDWR`/`CREAT`/`TRUNC`/`APPEND`/`EXCL`/`NONBLOCK`/`CLOEXEC`/`DIRECTORY`）、`S`（POSIX 权限与文件类型位常量）
- **轻量输出与编译期映射**：`Output`（`print`/`println`/`errorln`/`debug`/`flush`）、`ComptimeStringMap`、`fmtIntSize`

---

## 4. 按需挂载实测铁证

### 4.1 模块化模式产物验证（`cpc compile --emit-dir temp/verify_cap_lazy cap/test/test_cap_lazy_hub.cp -I /data/dev/cp/lib`）

1. **合成的 `_lib/_cap/index.zig`**：
   仅包含 `core.zig` 顶层导出与实际被引用的 3 个子命名空间（`bits`、`errno`、`hash`）；测试文件中在注释/字符串/模板字面量提及的 `css`/`http`/`sql`/`shell`/`md`/`s3`/`pack`/`redis`/`elf` 均未被挂载：
   ```zig
   pub const bits = @import("../meta/bits.zig");
   pub const errno = @import("../errno/index.zig");
   pub const hash = @import("../hash/index.zig");
   ```
2. **模块清单 `.cp_modular_manifest.txt`（仅 7 个模块）**：
   - `cap/test/test_cap_lazy_hub.cp -> test/test_cap_lazy_hub.zig`
   - `lib/_cap/core.cp -> _lib/_cap/core.zig`
   - `lib/meta/bits.cp -> _lib/meta/bits.zig`
   - `lib/errno/index.cp -> _lib/errno/index.zig`
   - `lib/hash/index.cp -> _lib/hash/index.zig`
   - `lib/test.cp -> _lib/test.zig`
   - `lib/errno/errno.cp -> _lib/errno/errno.zig`
3. **Zig 编译与执行**：
   `zig build-exe "$ENTRY" -O Debug -fno-llvm -lc` 链接成功，运行输出 `Total: 2, Passed: 2, Failed: 0`。

### 4.2 `cap test` 单文件模式与回归测试结果

- `cap test cap/test/test_cap_lazy_hub.cp` — **PASS**（2/2 用例通过）
- `cap test cap/test/test_struct_name_and_nested_repeat.cp` — **PASS**
- `cap test cap/test/test_event_loop.cp` — **PASS**
