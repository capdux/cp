# CP 标准库：多模式 Glob 与 `.gitignore` 规则匹配引擎手册 (`cap:globset`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

## 一、 架构总览与设计准则

`cap:globset` 是 CP 语言官方内置的纯 CP **多模式通配符与忽略规则匹配库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。底层采用线性时间 NFA 状态集模拟（零堆分配匹配、免疫指数回溯），支持单模式 `GlobMatcher`、支持嵌套大括号展开的多模式集合 `GlobSet`/`GlobSetBuilder`，以及兼容 `.gitignore` 语义的 `IgnoreMatcher`。支持 `import { ... } from "cap:text"` 与 `import cap from "cap"; cap.globset.*` 动态挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/globset/glob.cp` | `// Source: cp/lib/globset/glob.cp (...)` | `GlobError`、`GlobOptions`、`GlobMatcher` 单模式编译与线性 NFA 匹配 |
| `lib/globset/globset.cp` | `// Source: cp/lib/globset/globset.cp (...)` | `GlobSetBuilder`、`GlobSet`（嵌套大括号展开与 `matchesWhich`）及 `IgnoreMatcher`（`.gitignore` 引擎） |
| `lib/globset/index.cp` | `// Source: cp/lib/globset/index.cp (...)` | `cap:globset` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与配置

- **`GlobError`**：
  - `InvalidPattern`：非法转义（如末尾孤立 `\`）、空字符类 `[]` 或倒序范围 `[z-a]`。
  - `UnclosedBracket`：未闭合的字符类 `[...` 。
  - `UnclosedBrace`：未闭合或多余的 `{...}` 大括号。
  - `EmptyPattern`：空模式字符串 `""`。
  - `OutOfMemory`：内存分配失败。
- **`GlobOptions`**：
  - `case_insensitive: bool = false`：ASCII 大小写不敏感匹配。
  - `literal_separator: bool = true`：为 `true` 时 `*`、`?` 与否定字符类 `[!...]` 不跨越 `/`；使用 `**` 跨多级目录。

### 2. `GlobMatcher` 单模式匹配器

- **`compile(alloc: std.mem.Allocator, pattern: string, opts: GlobOptions): GlobMatcher ! GlobError`**：
  支持 `*`、`**`（含 `a/**/b` 零或多级目录、`**/` 前缀）、`?`、字符类 `[abc]` / `[a-z]` / `[!0-9]` / `[^a-f]` 及反斜杠转义（`\*`、`\?`、`\[` 等）。
- **`matches(path: string): bool`**：零堆分配线性时间匹配。
- **`deinit(*_)`**：释放编译后的 token 缓冲。

### 3. `GlobSetBuilder` & `GlobSet` 多模式集合

- **`GlobSetBuilder.init(alloc)` / `initWithOptions(alloc, opts)`**：创建构建器。
- **`add(*_, pattern: string): void ! GlobError`** / **`addWithOptions(*_, pattern, opts)`**：
  支持嵌套大括号展开（如 `*.{ts,tsx,{c,h}}`），所有展开子模式共享同一 0-based 模式索引。
- **`build(*_): GlobSet ! GlobError`**：构建 `GlobSet` 并转移所有权。
- **`GlobSet.matches(path: string): bool`**：任一模式命中即返回 `true`。
- **`GlobSet.matchesWhich(alloc: std.mem.Allocator, path: string): []usize ! GlobError`**：返回所有命中的 0-based 模式索引（升序去重），调用方需通过 `alloc.free(...)` 释放。
- **`GlobSet.len(): usize` / `isEmpty(): bool` / `deinit(*_)`**。

### 4. `IgnoreMatcher`（`.gitignore` 规则引擎）

- **`init(alloc: std.mem.Allocator): IgnoreMatcher`** / **`deinit(*_)`**
- **`addRule(*_, raw_line: string): void ! GlobError`** / **`parse(*_, content: string): void ! GlobError`**：
  - 忽略 `#` 注释行与空白行；
  - 支持 `!` 否定恢复规则（后规则覆盖前规则，Last-Match-Wins）；
  - 支持尾部 `/` 仅匹配目录（`is_dir == true`）；
  - 支持前导 `/` 或含内部 `/` 锚定根目录，无 `/` 规则自动匹配任意深度。
- **`isIgnored(path: string, is_dir: bool): bool`**：判定路径是否被忽略。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { GlobMatcher, GlobOptions, GlobSetBuilder, IgnoreMatcher } from "cap:text";

pub fn demoGlobset(): ! {
    const alloc = std.heap.page_allocator;

    let m = GlobMatcher.compile(alloc, "src/**/test_[a-z]*.cp", GlobOptions{})?;
    defer m.deinit();
    const ok = m.matches("src/core/test_alpha.cp");
    * = ok;

    let builder = GlobSetBuilder.init(alloc);
    defer builder.deinit();
    builder.add("*.{ts,tsx,{c,h}}")?;
    builder.add("src/**")?;

    let set = builder.build()?;
    defer set.deinit();
    const hits = set.matchesWhich(alloc, "src/main.tsx")?;
    defer alloc.free(hits);

    let ig = IgnoreMatcher.init(alloc);
    defer ig.deinit();
    ig.parse("# comment\n*.log\n!keep.log\nbuild/\n/root.tmp\n")?;
    const ignored = ig.isIgnored("build", true);
    * = ignored;
}
```
