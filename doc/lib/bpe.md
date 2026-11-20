# CP 标准库：Byte-Level BPE Tokenizer 引擎手册 (`cap:bpe`)

## 一、 架构总览与设计准则

`cap:bpe` 是 CP 语言官方内置的纯 CP **字节级 BPE（Byte Pair Encoding）分词引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 CP 原生 AI 推理、LLM Token 精确计数与无损文本压缩设计。既可通过 `import { ... } from "cap:bpe"` 导入，也支持 `import cap from "cap"; cap.bpe.*` 动态懒挂载调用。

### 核心特性
- **256 字节无损基础词表**：初始化自动预置 `0..255` 全部单字节基础 Token，任何 UTF-8 中英文、Emoji、二进制控制字节均**永不产生 `<UNK>`**，严格保证 `decode(encode(text)) == text`。
- **确定性 BPE 训练 (`train`)**：统计语料内相邻 Token 对频次，每次选取最高频（频次相同按 `(left, right)` 字典序确定性打破平局）的相邻对合并并分配递增 `rank`，直至达到目标词表大小或无频次 `>= 2` 的对。
- **Merge Ranks 优先级编码 (`encode`)**：普通字节片段按最低 `rank` 优先反复贪心合并。
- **Special Tokens 保护**：通过 `addSpecialToken` 注册特殊标记（如 `<|endoftext|>`, `<|im_start|>`），在编码与训练切分阶段采用最长匹配保护，绝不被内部拆分或参与合并。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/bpe/bpe.cp` | `// Source: cp/lib/bpe/bpe.cp (...)` | `BpeError`、`MergePair`、`BpeTokenizer` |
| `lib/bpe/index.cp` | `// Source: cp/lib/bpe/index.cp (...)` | `cap:bpe` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `BpeError`
- `InvalidVocabSize`：`train` 传入 `target_vocab_size < 256`。
- `UnknownTokenId`：`decode` 遇到越界 Token ID（`id >= vocabSize()`）。
- `DuplicateSpecialToken`：`addSpecialToken` 重复注册同一特殊标记。
- `InvalidMergeRule`：`addMerge` 传入越界 ID、特殊标记 ID、重复合并对，或 `addSpecialToken` 传入空字符串。
- `OutOfMemory`：底层词表、哈希表或输出切片内存分配失败。

### 2. 结构 `MergePair`
```typescript
pub struct MergePair {
    left: u32;
    right: u32;
    new_id: u32;
    rank: u32;
}
```

### 3. `BpeTokenizer`
- **生命周期**：
  - `BpeTokenizer.init(alloc: std.mem.Allocator): BpeTokenizer ! BpeError`（自动预置 `0..255` 单字节 Token）
  - `deinit(*_)`
- **词表与规则构建**：
  - `addSpecialToken(*_, token_str: string): u32 ! BpeError`
  - `addMerge(*_, left_id: u32, right_id: u32): u32 ! BpeError`
  - `train(*_, corpus: string, target_vocab_size: usize): void ! BpeError`
- **编解码与查询**：
  - `encode(alloc: std.mem.Allocator, text: string): []u32 ! BpeError`（调用方需使用 `alloc.free` 释放）
  - `decode(alloc: std.mem.Allocator, ids: []const u32): []u8 ! BpeError`（调用方需使用 `alloc.free` 释放）
  - `countTokens(alloc: std.mem.Allocator, text: string): usize ! BpeError`
  - `vocabSize(): usize`
  - `tokenBytes(id: u32): ([]const u8)?`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { BpeTokenizer } from "cap:bpe";
import cap from "cap";

pub fn demoBpe(): ! {
    const alloc = std.heap.page_allocator;

    let tok = BpeTokenizer.init(alloc)?;
    defer tok.deinit();

    const im_start = tok.addSpecialToken("<|im_start|>")?;
    const im_end = tok.addSpecialToken("<|im_end|>")?;
    * = im_start;
    * = im_end;

    tok.train("hello world hello cp-lang hello bpe!", 270)?;

    const input = "<|im_start|>hello cp-lang!<|im_end|>";
    const ids = tok.encode(alloc, input)?;
    defer alloc.free(ids);

    const restored = tok.decode(alloc, ids)?;
    defer alloc.free(restored);

    const n = cap.bpe.BpeTokenizer.init(alloc)?;
    n.deinit();
}
```
