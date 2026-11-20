# CP 标准库：抗 ReDoS 正则表达式引擎手册 (`cap:regex`)

## 一、 架构总览与设计准则

`cap:regex` 是 CP 语言官方内置的纯 CP、零 C 依赖的 **抗 ReDoS 线性时间正则表达式引擎**。底层采用 **Thompson NFA 字节码编译** + **Pike VM 同步多状态步进（带稀疏集合/代数去重）**，从根本上杜绝回溯指数灾难（如 `(a+)+b` 匹配长串无 `b` 文本恒为 $O(N \cdot M)$ 线性时间完成）。既可通过 `import { Regex, RegexError, MatchSpan, Captures } from "cap:regex"` 静态导入，也支持 `import cap from "cap"; cap.regex.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/regex/compiler.cp` | `// Source: cp/lib/regex/compiler.cp (Linear NFA Regex Bytecode Compiler)` | `RegexError`、`CharClass`（256 位位图）、`Opcode`/`Inst` 与 Thompson NFA 字节码编译器 |
| `lib/regex/vm.cp` | `// Source: cp/lib/regex/vm.cp (Anti-ReDoS Pike VM Regex Engine)` | `MatchSpan`、`Captures`、`Regex` 与 $O(N \cdot M)$ Pike VM 执行器 |
| `lib/regex/index.cp` | `// Source: cp/lib/regex/index.cp (Unified Exports for cap:regex)` | `cap:regex` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与匹配结构

- **`RegexError`**：
  - `InvalidPattern`：空模式或非法语法结构（如反向字符区间 `[z-a]`、游离 `}`）。
  - `UnmatchedParen`：圆括号 `(` 或 `)` 未闭合或多余。
  - `UnclosedClass`：字符类 `[...]` 未闭合。
  - `InvalidQuantifier`：量词缺少前导原子（如 `*a`）、嵌套未加括号（如 `a**`）或 `{m,n}` 中 `m > n`。
  - `InvalidEscape`：尾部孤立 `\` 或不支持的转义序列。
  - `MaxComplexityExceeded`：指令数（> 16384）、捕获组数（> 32）、重复次数（> 1000）或嵌套深度（> 64）超限。
  - `OutOfMemory`：底层内存分配失败。
- **`MatchSpan`**：
  - `start: usize`：匹配起始字节偏移（左闭）。
  - `end: usize`：匹配结束字节偏移（右开）。
  - `text: string`：命中的原始切片视图（零拷贝）。
- **`Captures`**：
  - `group(idx: usize): MatchSpan?`：`group(0)` 返回完整匹配，`group(1..N)` 返回对应捕获组。
  - `count(): usize`：总组数（含组 `0`）。
  - `deinit(*_)`：释放捕获组切片内存。

### 2. `Regex` 核心方法

- **`Regex.compile(alloc: std.mem.Allocator, pattern: []const u8): Regex ! RegexError`**：
  将正则表达式编译为线性 NFA 字节码程序。
- **`deinit(*_)`**：释放编译后的指令与字符类内存。
- **`isMatch(text: []const u8): bool`**：判断 `text` 中是否存在匹配子串。
- **`isFullMatch(text: []const u8): bool`**：判断整个 `text` 是否从头到尾完全匹配。
- **`find(text: []const u8): MatchSpan?`**：返回首个最左匹配区间，无匹配返回 `null`。
- **`findAll(alloc: std.mem.Allocator, text: []const u8): ([]MatchSpan)!`**：返回全部非重叠匹配区间切片（调用方使用 `alloc.free(spans)` 释放）。
- **`captures(alloc: std.mem.Allocator, text: []const u8): (Captures?)!`**：返回首个匹配的完整捕获组集合（调用方需调用 `caps.deinit()`）。
- **`replaceAll(alloc: std.mem.Allocator, text: []const u8, replacement: []const u8): ([]u8)!`**：全局替换所有匹配，支持 `$0`、`$1`..`$9` 捕获组反向引用与 `$$` 字面 `$`。
- **`split(alloc: std.mem.Allocator, text: []const u8): ([]string)!`**：按匹配分隔符切分文本为字符串切片数组。

---

## 三、 支持的正则语法特性

- **字面字符与转义**：`\d`, `\D`, `\w`, `\W`, `\s`, `\S`, `\n`, `\r`, `\t`, `\\`, `\.`, `\*`, `\+`, `\?`, `\(`, `\)`, `\[`, `\]`, `\{`, `\}`, `\|`, `\^`, `\$`, `\/`, `\-`, `\_`
- **字符类**：`[abc]`, `[a-z0-9_-]`, `[^0-9]`（否定字符类），支持类内 `\d`/`\w`/`\s`
- **分组与分支**：`(...)` 捕获组（`1..N`）、`(?:...)` 非捕获组、多路选择分支 `a|b|c`
- **锚点**：`^`（行/串首）、`$`（行/串尾）、`.`（除 `\n` 外任意字节）
- **量词（贪婪与惰性）**：`*`, `+`, `?`, `{m}`, `{m,}`, `{m,n}` 及惰性形式 `*?`, `+?`, `??`, `{m,n}?`

---

## 四、 可运行代码示例

```typescript
const std = @import("std");
import { Regex } from "cap:regex";
import cap from "cap";

pub fn demoRegex(): ! {
    const alloc = std.heap.page_allocator;
    let re = Regex.compile(alloc, "(\\d{4})-(\\d{2})-(\\d{2})")?;
    defer re.deinit();

    const reformatted = re.replaceAll(alloc, "date: 2026-11-20", "$2/$3/$1")?;
    defer alloc.free(reformatted);
    // reformatted == "date: 11/20/2026"

    let fast = cap.regex.Regex.compile(alloc, "(a+)+b")?;
    defer fast.deinit();
    // 线性时间免疫 ReDoS，瞬时返回 false
    const matched = fast.isMatch("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaac");
    * = matched;
}
```
