# CP 标准库：高速拼写纠错与语音编码引擎手册 (`cap:spell`)

## 一、 架构总览与设计准则

`cap:spell` 是 CP 语言官方内置的纯 CP **高速拼写纠错引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 CP 搜索引擎「Did you mean」、CLI 参数智能纠错与 AI Prompt 拼写容错设计。既可通过 `import { ... } from "cap:spell"` 导入，也支持 `import cap from "cap"; cap.spell.*` 动态懒挂载调用。

### 核心特性
- **SymSpell 对称删除索引 (`SymSpellIndex`)**：在词典入库阶段沿 `prefix_length` 前缀预生成 `1..max_edit_distance` 删字变体倒排索引，查询时仅需在输入词及其删字变体上检索并复核距离，实现 $O(1)$ 毫秒级高频纠错。
- **BK-Tree 度量空间树 (`BkTree`)**：基于三角不等式 `[d - max_dist, d + max_dist]` 剪枝遍历，支持任意编辑距离的精确全量召回。
- **OSA Damerau-Levenshtein 距离 (`damerauLevenshtein`)**：支持插入、删除、替换及**相邻字符换位**（如 `"teh"` -> `"the"` 距离为 `1` 而非 `2`），采用滚动三行缓冲区实现 $O(\min(m, n))$ 空间复杂度。
- **American Soundex 语音编码 (`soundex`)**：生成经典 4 字符英文语音编码（如 `"Robert"` / `"Rupert"` 均编码为 `"R163"`），辅助同音拼写召回。
- **确定性排序**：所有候选建议严格按 `(distance ASC, frequency DESC, term ASC)` 排序。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/spell/spell.cp` | `// Source: cp/lib/spell/spell.cp (...)` | `SpellError`、`SuggestVerbosity`、`SpellSuggestion`、`damerauLevenshtein`、`soundex`、`SymSpellIndex`、`BkTree` |
| `lib/spell/index.cp` | `// Source: cp/lib/spell/index.cp (...)` | `cap:spell` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与基础类型
- `pub err SpellError { InvalidMaxDistance, EmptyWord, OutOfMemory }`
  - `InvalidMaxDistance`：`SymSpellIndex.init` 传入超过上限（`> 8`）的编辑距离，或 `lookup` 传入 `max_distance > max_edit_distance`。
  - `EmptyWord`：`addWord`、`lookup`、`BkTree.insert` 或 `BkTree.search` 传入空字符串。
  - `OutOfMemory`：底层词表、索引或结果切片内存分配失败。
- `pub enum SuggestVerbosity { Top, Closest, All }`
  - `Top`：仅返回最优的 1 条建议。
  - `Closest`：返回与最小发现编辑距离并列的全部建议。
  - `All`：返回 `max_distance` 范围内的全部建议。
- `pub struct SpellSuggestion { term: string; distance: u32; frequency: u64; }`
  - `term` 指向索引内部持久持有的归一化词条切片（在索引 `deinit()` 前持续有效）。

### 2. 距离与语音函数
- `damerauLevenshtein(alloc: std.mem.Allocator, a: string, b: string): u32 ! SpellError`
- `soundex(word: string): [4]u8`

### 3. `SymSpellIndex`
- `SymSpellIndex.init(alloc: std.mem.Allocator, max_edit_distance: u32, prefix_length: usize): SymSpellIndex ! SpellError`
- `deinit(*_)`
- `wordCount(): usize`
- `addWord(*_, word: string, freq: u64): void ! SpellError`（自动小写归一化；重复词条累加词频）
- `lookup(alloc: std.mem.Allocator, input: string, verbosity: SuggestVerbosity, max_distance: u32): []SpellSuggestion ! SpellError`（调用方需使用 `alloc.free` 释放返回切片）

### 4. `BkTree`
- `BkTree.init(alloc: std.mem.Allocator): BkTree`
- `deinit(*_)`
- `wordCount(): usize`
- `insert(*_, word: string, freq: u64): void ! SpellError`
- `search(alloc: std.mem.Allocator, query: string, max_dist: u32): []SpellSuggestion ! SpellError`（调用方需使用 `alloc.free` 释放返回切片）

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SymSpellIndex, BkTree, SuggestVerbosity, damerauLevenshtein, soundex } from "cap:spell";
import cap from "cap";

pub fn demoSpell(): ! {
    const alloc = std.heap.page_allocator;

    const d = damerauLevenshtein(alloc, "teh", "the")?; // 1
    const s = soundex("Robert");                        // "R163"
    * = d;
    * = s;

    let idx = SymSpellIndex.init(alloc, 2, 7)?;
    defer idx.deinit();
    idx.addWord("the", 10000)?;
    idx.addWord("there", 5000)?;
    idx.addWord("their", 3000)?;

    const top = idx.lookup(alloc, "teh", .Top, 2)?;
    defer alloc.free(top);

    let tree = cap.spell.BkTree.init(alloc);
    defer tree.deinit();
    tree.insert("algorithm", 800)?;
    const hits = tree.search(alloc, "algoritm", 2)?;
    defer alloc.free(hits);
}
```
