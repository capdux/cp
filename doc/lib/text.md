# CP 标准库：AI 文本分块、模糊匹配与 Token 预算引擎手册 (`cap:text` / `cap.text`)

## 一、 架构总览与设计准则

`cap:text` 是 CP 语言官方内置的纯原生文本处理与 AI Agent 上下文工程标准库，专为 RAG 文档切分、命令面板模糊搜索与 LLM 上下文窗口预算控制打造。包含三大核心子模块：

1. **`TextChunker` + `TextChunk` + `ChunkOptions`**（`lib/text/chunker.cp`）：零拷贝、UTF-8 码点边界安全的递归文本切分器与 Markdown 章节切分器。按 `\n\n` -> `\n` -> `. ` -> ` ` -> UTF-8 码点边界优先级切分，绝不截断多字节 UTF-8 字符，支持滑动重叠窗口 (`overlap_chars`) 与围栏代码块感知的 `#`/`##`/`###` 章节标题保留 (`TextChunk.heading`)。
2. **编辑距离与模糊匹配引擎**（`lib/text/fuzzy.cp`）：提供 Unicode 码点级 `levenshtein`、无限制（True）`damerauLevenshtein`、零堆分配 `jaroWinkler`，以及带连续命中/词首分隔符/camelCase 边界加分的交互式子序列打分器 `fuzzyScore` 与 `fuzzyTopK`。
3. **`TokenEstimator` + `TextStats`**（`lib/text/tokens.cp`）：单遍 $O(n)$ 零堆分配 LLM Token 预算估算器与 UTF-8 安全前缀截断器，区分 ASCII 词、数字、标点、空白符与 CJK 字符，严格保证 `estimateTokens(truncateToTokenBudget(text, max_tokens)) <= max_tokens`。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:text"` 与 `import cap from "cap"; cap.text.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/text/chunker.cp` | `// Source: cp/lib/text/chunker.cp (UTF-8 Safe Recursive Text & Markdown RAG Chunker)` | `TextErr`、`TextChunk`、`ChunkOptions`、`TextChunker` |
| `lib/text/fuzzy.cp` | `// Source: cp/lib/text/fuzzy.cp (Edit Distance, Jaro-Winkler & Fuzzy Subsequence Matcher)` | `FuzzyMatch`、`levenshtein`、`damerauLevenshtein`、`jaroWinkler`、`fuzzyScore`、`fuzzyTopK` |
| `lib/text/tokens.cp` | `// Source: cp/lib/text/tokens.cp (Fast LLM Token Budget Estimator & UTF-8 Safe Truncation)` | `TextStats`、`TokenEstimator` |
| `lib/text/index.cp` | `// Source: cp/lib/text/index.cp (Unified Exports for cap:text)` | `cap:text` 与 `cap.text` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. UTF-8 安全递归与 Markdown 分块（`chunker.cp`）

- **`TextErr`**：`InvalidParameter`、`OutOfMemory`
- **`TextChunk`**：
  - 字段：`text: string`、`start_byte: usize`、`end_byte: usize`、`chunk_index: usize`、`heading: string`
  - 零拷贝契约：`text` 与 `heading` 均为原始输入字符串的只读子切片，调用方仅需 `alloc.free(chunks)` 释放返回的切片容器本身。
- **`ChunkOptions`**：`max_chars: usize = 512`、`overlap_chars: usize = 0`
- **`TextChunker`**：
  - `splitRecursive(alloc: std.mem.Allocator, text: string, opts: ChunkOptions = ChunkOptions{}): ([]TextChunk)!`
  - `splitMarkdown(alloc: std.mem.Allocator, md: string, max_chars: usize = 512): ([]TextChunk)!`（自动忽略 ```` ``` ```` / `~~~` 围栏代码块内部的 `#` 注释行，提取剥离 `#` 前缀的章节标题存入 `TextChunk.heading`）

### 2. 编辑距离、相似度与模糊搜索（`fuzzy.cp`）

- **`levenshtein(alloc: std.mem.Allocator, a: string, b: string): usize!`**
  - Unicode 码点级 Levenshtein 编辑距离，$O(\min(m, n))$ 空间复杂度。
- **`damerauLevenshtein(alloc: std.mem.Allocator, a: string, b: string): usize!`**
  - Unicode 码点级无限制 True Damerau-Levenshtein 距离（Lowrance-Wagner 算法，支持相邻字符交换）。
- **`jaroWinkler(a: string, b: string): f64`**
  - 零堆分配 Unicode 码点级 Jaro-Winkler 相似度，返回 `[0.0, 1.0]`（Winkler 前缀系数 `p = 0.1`，最大前缀长度 `4`）。
- **`FuzzyMatch`**：`index: usize`、`score: i32`、`text: string`
- **`fuzzyScore(query: string, candidate: string): i32?`**
  - 大小写不敏感子序列匹配最优对齐打分；包含连续命中 (`+15`)、词首/分隔符 (`_`, `-`, `/`, `.`, ` `, `:`) 边界 (`+15`)、camelCase 边界 (`+12`) 与精确大小写一致 (`+2`) 加分；未匹配返回 `null`。
- **`fuzzyTopK(alloc: std.mem.Allocator, query: string, candidates: []const string, top_k: usize): ([]FuzzyMatch)!`**
  - 按 `score` 降序返回前 `top_k` 个匹配项（同分按更短长度、更小原索引稳定排序），调用方使用 `alloc.free(matches)` 释放。

### 3. LLM Token 预算估算与截断（`tokens.cp`）

- **`TextStats`**：`bytes: usize`、`codepoints: usize`、`words: usize`、`lines: usize`、`cjk_chars: usize`、`estimated_tokens: usize`
- **`TokenEstimator`**：
  - `analyze(text: string): TextStats`：零堆分配单遍扫描统计全部文本指标与估算 Token 数。
  - `estimateTokens(text: string): usize`：快速估算文本 Token 数（区分 ASCII 词、数字、标点、换行/缩进与 CJK/Emoji 码点）。
  - `truncateToTokenBudget(text: string, max_tokens: usize): string`：零堆分配返回满足 `estimateTokens(res) <= max_tokens` 的最长前缀，严格在有效 UTF-8 码点边界截断。

---

## 三、 使用示例

```typescript
import cap from "cap";
import { TextChunker, ChunkOptions, TokenEstimator, fuzzyTopK } from "cap:text";

pub fn demoTextEngine(alloc: std.mem.Allocator): ! {
    const md = "# RAG Guide\n\nChunking preserves UTF-8 boundaries like 你好世界.\n\n## Budget\nKeep prompts compact.";
    const chunks = TextChunker.splitMarkdown(alloc, md, 64)?;
    defer alloc.free(chunks);

    for (c of chunks) {
        const fitted = TokenEstimator.truncateToTokenBudget(c.text, 16);
        * = fitted;
    }

    const cmds: [3]string = ["openFile", "saveAllFiles", "searchWorkspace"];
    const hits = cap.text.fuzzyTopK(alloc, "sf", &cmds, 2)?;
    defer alloc.free(hits);
}
```

---

## 零分配文本排版与缩进子模块（收敛自原 `text_*` 系列）

- **多行缩进检测与格式化 (`indent.cp`)**：
  - `minCommonIndent`、`dedent`、`indentLines(text, prefix, indent_empty, out)`、`stripCommonPrefix`（保留 `\n`/`\r\n`）。
- **自动换行与 UTF-8 安全省略 (`wrap.cp`)**：
  - `wrapText`、`indentNonEmptyLines(text, prefix, out)`、`commonIndent`、`dedentLines`、`truncateEllipsis`。
