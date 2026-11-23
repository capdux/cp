# CP 标准库：AI 文本分块、模糊匹配与 Token 预算引擎手册 (`cap:text` / `cap.text`)

> 参考来源：`/data/dev/bun/src/string/`, `/data/dev/bun/src/unicode/` | 吸收日期：2026-11-22

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


---

## 命名风格转换与 Unicode Case Folding（已收敛至 `cap:text`）

- **`lib/text/caseconv.cp`**：`WordIter`, `toSnakeCase`, `toScreamingSnakeCase`, `toKebabCase`, `toCamelCase`, `toPascalCase`
- **`lib/text/casefold.cp`**：`FoldInfo`, `caseFold`, `caseFoldEqual`

---

## 零堆分配 RAG 递归文档分块与引用定位 (`lib/text/ragChunker.cp`)

专为高并发 AI Agent / RAG 管线打造的零堆分配、零拷贝 Markdown 感知分块器：

- **`RagChunkConfig`**：`{ max_bytes: usize = 512, overlap_bytes: usize = 64, respect_code_fences: bool = true, split_on_headings: bool = true }`（提供 `RagChunkConfig.default()`）
- **`RagChunkSpan`**：`{ start: usize, end: usize, index: u32, est_tokens: u32, line_start: u32, line_end: u32 }`（方法 `slice(doc: string): string` 返回零拷贝原文切片）
- **`chunkDocumentRag(doc: string, cfg: RagChunkConfig, out_spans: *[]RagChunkSpan): usize?`**：按 Markdown 标题 `\n#` -> 段落 `\n\n` -> 换行 `\n` -> 句末标点 `. `/`。` -> 空格 -> UTF-8 码点边界递归切分；`respect_code_fences == true` 时避免从 ```` ``` ```` / `~~~` 代码块中间腰斩；非法配置（`max_bytes == 0` 或 `overlap_bytes >= max_bytes`）或缓冲不足返回 `null`。
- **`estimateRagTokens(text: string): u32`**：零堆分配估算近似 Token 数，精准区分 ASCII 词、标点与 3-4 字节 CJK/Emoji 字符密度。
- **`locateLineRange(doc: string, start: usize, end: usize): [2]u32`**：返回 1-based `[line_start, line_end]` 引用行号范围。

```typescript
import { RagChunkConfig, RagChunkSpan, chunkDocumentRag } from "cap:text";

pub fn demoRagChunking(doc: string) {
    let spans_buf: [32]RagChunkSpan = undefined;
    let spans: []RagChunkSpan = spans_buf[0..];
    const cfg = RagChunkConfig{ max_bytes: 256, overlap_bytes: 32, respect_code_fences: true, split_on_headings: true };
    if (n := chunkDocumentRag(doc, cfg, spans)) {
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const text = spans[i].slice(doc);
            * = text;
        }
    }
}
```

---

## 零堆分配字节级 BPE 分词器 (`lib/text/bpe.cp`)

专为 AI Agent / LLM 推理前端打造的纯 CP 零堆分配字节级 Byte-Pair Encoding（BPE）编解码器，以 `0..255` 原始字节为基础词表，支持任意 UTF-8 与二进制字节串比特无损往返：

- **`BpeMerge`**：`{ left: u32, right: u32, merged_id: u32, rank: u32 }`（`rank` 越小优先级越高，支持多级嵌套合并树）
- **`SpecialToken`**：`{ text: string, id: u32 }`（如 `<|endoftext|>`、`<|im_start|>`、`<|im_end|>`）
- **`BpeVocab`**：`{ merges: []const BpeMerge, specials: []const SpecialToken }`，提供 `BpeVocab.init(merges, specials)`
- **`bpeEncodeSegment(vocab: BpeVocab, bytes: []const u8, work_tokens: *[]u32): usize?`**：将原始字节初始化为 `0..255` token 序列后原地按最低 `rank` 迭代贪心合并，返回最终 token 数；`work_tokens` 容量不足返回 `null`。
- **`bpeEncodeWithSpecials(vocab: BpeVocab, text: string, out_tokens: *[]u32, seg_work: *[]u32): usize?`**：优先识别 `specials` 中最长匹配控制词发射原子 ID（不被 BPE 拆散），其余正文段调用 `bpeEncodeSegment`；任一缓冲不足返回 `null`。
- **`bpeDecode(vocab: BpeVocab, tokens: []const u32, out_bytes: *[]u8, expand_stack: *[]u32): usize?`**：无递归迭代栈展开，将 `0..255` 基础字节、`merged_id` 合并树与 `specials` 还原为原始字节流；内置未知 token ID、直接/间接循环合并规则与缓冲/栈溢出防护（异常时返回 `null`）。
- **`bpeCountTokens(vocab: BpeVocab, text: string, seg_work: *[]u32): usize?`**：无需输出 token 缓冲即可精确计算 `text` 的 BPE token 总数（与 `bpeEncodeWithSpecials` 输出长度严格一致）。

```typescript
import {
    BpeMerge,
    SpecialToken,
    BpeVocab,
    bpeEncodeWithSpecials,
    bpeDecode,
    bpeCountTokens,
} from "cap:text";

pub fn demoBpeTokenizer() {
    const merges: [2]BpeMerge = [
        BpeMerge{ left: 104, right: 105, merged_id: 256, rank: 0 }, // "hi"
        BpeMerge{ left: 256, right: 33, merged_id: 257, rank: 1 },  // "hi!"
    ];
    const specials: [1]SpecialToken = [
        SpecialToken{ text: "<|im_start|>", id: 1000 },
    ];
    const vocab = BpeVocab.init(merges[0..], specials[0..]);

    let out_toks_buf: [64]u32 = undefined;
    let seg_work_buf: [64]u32 = undefined;
    let out_toks: []u32 = out_toks_buf[0..];
    let seg_work: []u32 = seg_work_buf[0..];

    const input = "<|im_start|>hi!";
    if (n := bpeEncodeWithSpecials(vocab, input, out_toks, seg_work)) {
        let out_bytes_buf: [64]u8 = undefined;
        let stack_buf: [32]u32 = undefined;
        let out_bytes: []u8 = out_bytes_buf[0..];
        let stack: []u32 = stack_buf[0..];
        if (m := bpeDecode(vocab, out_toks[0..n], out_bytes, stack)) {
            * = out_bytes[0..m]; // "<|im_start|>hi!"
        }
    }
    * = bpeCountTokens(vocab, input, seg_work);
}
```

---

## 零堆分配 WordPiece 子词分词与编码器输入打包器 (`lib/text/wordpiece.cp`)

专为 BERT / 语义嵌入（Embedding）与交叉编码器重排（Cross-Encoder Reranker）打造的纯 CP 零堆分配 WordPiece 分词与张量输入打包器：

- **`WordPieceConfig`**：`{ unk_id: u32 = 100, cls_id: u32 = 101, sep_id: u32 = 102, pad_id: u32 = 0, lowercase: bool = true, max_word_chars: usize = 100 }`（提供 `WordPieceConfig.default()`）
- **`WordPieceVocab`**：`{ tokens: []const string }`（索引即 token ID），提供 `WordPieceVocab.init(tokens)` 与只读查找方法 `lookup(piece: string): u32?`
- **`EncodedSequenceStats`**：`{ total_len: usize, real_tokens: usize, truncated: bool }`
- **`wordPieceEncodeWord(vocab, word, cfg, out_ids, work_buf): usize?`**：对单个基础词执行贪心最长子词切分（非首子词自动拼接 `"##"` 前缀），超 `max_word_chars` 或任意子段未登录时整个词回退单个 `cfg.unk_id`；缓冲不足返回 `null`。
- **`wordPieceEncodeText(vocab, text, cfg, out_ids, work_buf): usize?`**：按空白符与 ASCII 标点预分词（标点独立作为单字符 token），逐词执行 WordPiece 切分。
- **`wordPieceEncodePair(vocab, text_a, text_b, cfg, max_seq_len, pad_to_max, input_ids_out, attention_mask_out, token_type_ids_out, work_buf): EncodedSequenceStats?`**：
  - 单句模式（`text_b == ""`）：打包 `[CLS] A [SEP]`；
  - 双句模式（`text_b != ""`）：打包 `[CLS] A [SEP] B [SEP]`；
  - 精确生成 `attention_mask`（有效 token 为 `1`，`[PAD]` 为 `0`）与 `token_type_ids`（A 段及首个 `[SEP]` 为 `0`，B 段及末尾 `[SEP]` 为 `1`，`[PAD]` 为 `0`）；
  - 超长时采用 BERT 标准 `longest_first` 截断策略并严格保留 `[CLS]` 与 `[SEP]`，可选 `[PAD]` 填充至 `max_seq_len`。

```typescript
import {
    WordPieceConfig,
    WordPieceVocab,
    wordPieceEncodePair,
} from "cap:text";

pub fn demoWordPieceReranker() {
    const tokens: [10]string = [
        "[PAD]", "[UNK]", "[CLS]", "[SEP]",
        "un", "##aff", "##able", "hello", "world", "!",
    ];
    const vocab = WordPieceVocab.init(tokens[0..]);
    const cfg = WordPieceConfig{
        unk_id: 1, cls_id: 2, sep_id: 3, pad_id: 0,
        lowercase: true, max_word_chars: 100,
    };

    let ids_arr: [12]u32 = undefined;
    let mask_arr: [12]u8 = undefined;
    let types_arr: [12]u8 = undefined;
    let work_arr: [64]u8 = undefined;
    let ids: []u32 = ids_arr[0..];
    let mask: []u8 = mask_arr[0..];
    let types: []u8 = types_arr[0..];
    let work: []u8 = work_arr[0..];

    if (stats := wordPieceEncodePair(vocab, "Unaffable!", "Hello world", cfg, 12, true, ids, mask, types, work)) {
        * = stats; // total_len=12, real_tokens=9, truncated=false
    }
}
```

---

## 零堆分配 SentencePiece Unigram 语言模型分词器 (`lib/text/unigram.cp`)

专为 T5 / LLaMA-1 / XLNet / Multilingual SentencePiece 模型打造的纯 CP 零堆分配 Unigram 分词器，采用精确 Viterbi 动态规划求解全局最大对数似然切分路径，完整补齐现代 AI 三大分词范式（BPE + WordPiece + Unigram）：

- **`UnigramPiece`**：`{ piece: string, log_prob: f64, id: u32 }`
- **`UnigramVocab`**：`{ pieces: []const UnigramPiece, unk_id: u32, unk_penalty: f64 }`（提供 `UnigramVocab.init(pieces, unk_id, unk_penalty)`）
- **`UnigramResult`**：`{ token_count: usize, total_log_prob: f64, unk_count: usize }`
- **`normalizeSentencePiece(text: string, add_dummy_prefix: bool, out_buf: *[]u8): usize?`**：将 ASCII 空白替换为 SentencePiece `▁`（`0xE2 0x96 0x81`），并在非空串首可选注入引导 `▁`。
- **`unigramEncodeViterbi(vocab, text, out_ids, dp_score_s, dp_prev_s, dp_id_s): UnigramResult?`**：零堆分配 Viterbi 动态规划最优切分（DP 缓冲长度需 `>= text.len + 1`）；支持 UTF-8 码点级未知字符回退 `unk_id`（不截断多字节中文/Emoji），内置 `NaN`/Inf 概率与缓冲不足防护。
- **`unigramDecode(vocab, ids, strip_leading_space, out_bytes): usize?`**：将 token 序列还原为 UTF-8 字节流，自动将 `▁` 还原为空格并可选剥离首部引导空格；未知 ID 或缓冲不足返回 `null`。

```typescript
import {
    UnigramPiece,
    UnigramVocab,
    normalizeSentencePiece,
    unigramEncodeViterbi,
    unigramDecode,
} from "cap:text";

pub fn demoUnigramViterbi() {
    const pieces: [4]UnigramPiece = [
        UnigramPiece{ piece: "▁hello", log_prob: -1.2, id: 1 },
        UnigramPiece{ piece: "▁world", log_prob: -1.4, id: 2 },
        UnigramPiece{ piece: "▁", log_prob: -2.0, id: 3 },
        UnigramPiece{ piece: "!", log_prob: -1.5, id: 4 },
    ];
    const vocab = UnigramVocab.init(pieces[0..], 0, -10.0);

    let norm_arr: [64]u8 = undefined;
    let norm: []u8 = norm_arr[0..];
    if (norm_len := normalizeSentencePiece("hello world!", true, norm)) {
        let out_ids_arr: [32]u32 = undefined;
        let dp_s_arr: [65]f64 = undefined;
        let dp_p_arr: [65]usize = undefined;
        let dp_id_arr: [65]u32 = undefined;
        let out_ids: []u32 = out_ids_arr[0..];
        let dp_s: []f64 = dp_s_arr[0..];
        let dp_p: []usize = dp_p_arr[0..];
        let dp_id: []u32 = dp_id_arr[0..];
        if (res := unigramEncodeViterbi(vocab, norm[0..norm_len], out_ids, dp_s, dp_p, dp_id)) {
            let dec_arr: [64]u8 = undefined;
            let dec_buf: []u8 = dec_arr[0..];
            * = unigramDecode(vocab, out_ids[0..res.token_count], true, dec_buf);
        }
    }
}
```

---

## 零堆分配 RAG Markdown 结构感知分块与标题面包屑索引器 (`lib/text/markdownChunk.cp`)

专为 RAG 知识库索引打造的纯 CP 零堆分配 Markdown 结构分块器，在切分同时自动维护 6 级标题层级路径（面包屑），并保护围栏代码块与 GFM 表格不被撕裂：

- **`MdSectionKind`**：`Prose` \| `CodeBlock` \| `Table` \| `List`
- **`MdChunkConfig`**：`{ max_chars: usize = 512, min_chars: usize = 64, keep_code_blocks_intact: bool = true, keep_tables_intact: bool = true }`（提供 `MdChunkConfig.default()`）
- **`MdHeadingTrail`**：固定 6 级 ATX 标题栈（`H1..H6` 字节起止偏移），提供：
  - `setHeading(*_, level, start, end)` / `clear(*_)` / `depth()` / `headingText(src, level): string?`
  - `formatBreadcrumb(src, out_buf: *[]u8): usize?`：零堆分配格式化层级路径（如 `"Intro > Setup > Install"`），缓冲不足返回 `null`。
- **`MdChunkSpan`**：`{ start, end, start_line, end_line, kind, heading_level, trail: MdHeadingTrail }`（1-based 精确行号，提供 `slice(src): string` 零拷贝切片方法）
- **`chunkMarkdownStructured(src, cfg, out_chunks: *[]MdChunkSpan): usize`**：
  - 识别非代码块内的 `#`..`######` 标题并更新面包屑（```` ``` ```` / `~~~` 围栏代码块内部 `#` 注释行绝不误判为标题）；
  - `keep_code_blocks_intact` / `keep_tables_intact` 保护围栏代码块与 GFM 表格完整不撕裂；
  - 超长段落与列表按换行、空格与 UTF-8 码点边界安全切分，同标题下相邻短段落按 `min_chars` 合并减少碎片；
  - 输出切片截断安全，始终返回总块数（`src.len == 0` 或 `max_chars == 0` 返回 `0`）。

```typescript
import {
    MdChunkConfig,
    MdChunkSpan,
    chunkMarkdownStructured,
} from "cap:text";

pub fn demoMarkdownStructuredChunk(doc: string) {
    let chunks_buf: [16]MdChunkSpan = [MdChunkSpan{}; 16];
    let chunks: []MdChunkSpan = chunks_buf[0..];
    const cfg = MdChunkConfig{
        max_chars: 256,
        min_chars: 32,
        keep_code_blocks_intact: true,
        keep_tables_intact: true,
    };

    const total = chunkMarkdownStructured(doc, cfg, chunks);
    let crumb_buf: [128]u8 = [0; 128];
    let crumb: []u8 = crumb_buf[0..];
    let i: usize = 0;
    while (i < total && i < chunks.len) : (i += 1) {
        if (n := chunks[i].trail.formatBreadcrumb(doc, crumb)) {
            * = crumb[0..n];
            * = chunks[i].slice(doc);
        }
    }
}
```

---

## 零堆分配 Unified Diff 补丁解析与原子应用器 (`lib/text/unifiedDiff.cp`)

专为 AI 编程智能体代码补丁验证与安全落盘打造，采用两遍原子设计（Pass 1 全量校验并测算输出字节数，Pass 2 写入），失败时保证 `out_buf` 零污染：

- **`DiffHunkHeader`**：`{ old_start: u32, old_count: u32, new_start: u32, new_count: u32 }`
- **`parseHunkHeader(line: string): DiffHunkHeader?`**：解析 `@@ -a,b +c,d @@`（支持省略 `,count` 默认 1 如 `@@ -10 +10,2 @@`、空文件插入 `@@ -0,0 +1,2 @@` 与可选函数名尾注）
- **`DiffLineKind`**：`Context` | `Remove` | `Add`；**`DiffOp`**：`{ kind: DiffLineKind, text: string }`（零拷贝行视图）；**`parseDiffOp(line: string): DiffOp?`**
- **`PatchApplyStatus`**：`Ok` | `ContextMismatch` | `LineOutOfRange` | `InvalidHunk` | `BufferTooSmall`
- **`PatchApplyResult`**：`{ status, written_bytes, hunks_applied, lines_added, lines_removed }`
- **`applyUnifiedPatch(original: string, patch: string, *out_buf: []u8): PatchApplyResult`**：支持多 Hunk 顺序应用、严格校验上下文与待删除行匹配、自动忽略 `---`/`+++`/`diff`/`index` 文件头与处理 `\ No newline at end of file`，任一错误时返回 `written_bytes == 0` 且不修改 `out_buf`。

```cp
import { PatchApplyStatus, applyUnifiedPatch } from "cap:text";

pub fn demoUnifiedDiffApply() {
    const orig = "fn main() {\n    let x = 1;\n    return x;\n}\n";
    const patch =
        "--- a/main.cp\n" ++
        "+++ b/main.cp\n" ++
        "@@ -1,4 +1,4 @@\n" ++
        " fn main() {\n" ++
        "-    let x = 1;\n" ++
        "+    let x = 42;\n" ++
        "     return x;\n" ++
        " }\n";

    let out_buf: [256]u8 = [0; 256];
    let out: []u8 = out_buf[0..];
    const res = applyUnifiedPatch(orig, patch, out);
    if (res.status == PatchApplyStatus.Ok) {
        const patched = out[0..res.written_bytes];
        * = patched;
    }
}
```

---

## 零堆分配多语言源码符号大纲与 Repo-Map 提取器 (`lib/text/codeOutline.cp`)

专为 AI 编程智能体构建紧凑代码仓库地图（Repo-Map）与精准行号定位打造，单遍零堆分配扫描 CP / TypeScript / Rust / Zig 源码：

- **`SymbolKind`**：`Function` | `Class` | `Struct` | `Enum` | `Trait` | `Const` | `Import` | `Test`
- **`CodeSymbolSpan`**：`{ kind, name_start, name_end, start_line, end_line, brace_depth, is_public }`，提供零拷贝 `nameSlice(src: string): string`
- **`extractCodeOutline(src: string, *out_symbols: []CodeSymbolSpan): usize`**：自动跳过 `//` 单行注释、`/* ... */` 块注释及双/单/反引号字符串字面量内的伪关键字与伪花括号（兼容 Rust `'a` 生命周期），识别 `pub`/`export`/`static`/`async` 修饰的声明并通过花括号栈追踪精确 `[start_line, end_line]` 作用域范围；输出截断安全并返回总符号数。
- **`findSymbolByName(src, symbols, target): CodeSymbolSpan?`**：按精确符号名零拷贝检索首个匹配符号。
- **`formatRepoMapLine(src, sym, *out_buf: []u8): usize?`**：格式化为 `pub fn applyUnifiedPatch [L42-L98]` 风格单行 Repo-Map 摘要；缓冲不足返回 `null`。

```cp
import {
    CodeSymbolSpan,
    extractCodeOutline,
    findSymbolByName,
    formatRepoMapLine,
} from "cap:text";

pub fn demoRepoMap(src: string) {
    let syms_buf: [32]CodeSymbolSpan = [CodeSymbolSpan{}; 32];
    let syms: []CodeSymbolSpan = syms_buf[0..];
    const total = extractCodeOutline(src, syms);
    const written = if (total < syms.len) total else syms.len;

    if (hit := findSymbolByName(src, syms[0..written], "applyUnifiedPatch")) {
        let line_buf: [128]u8 = [0; 128];
        let line: []u8 = line_buf[0..];
        if (n := formatRepoMapLine(src, hit, line)) {
            * = line[0..n]; // e.g. "pub fn applyUnifiedPatch [L42-L98]"
        }
    }
}
```

---

## 零堆分配 Token 感知代码模式匹配与 `$VAR` 捕获器 (`lib/text/codePattern.cp`)

面向 AI 编程智能体结构化代码审计与重构检索（轻量对标 ast-grep / semgrep），忽略注释与排版空白差异，支持同名元变量跨位置一致性约束：

- **`CodeTokenKind`**：`Ident` | `Number` | `String` | `Punct` | `MetaVar`（`$NAME`）
- **`CodeToken`**：`{ kind, start, end, line }`，提供零拷贝 `slice(src: string): string`
- **`tokenizeCodeLite(src: string, *out_tokens: []CodeToken): usize`**：跳过空白与 `//`/`/* ... */` 注释，切分标识符、数字、字符串、多字符操作符（`==`/`!=`/`<=`/`>=`/`->`/`=>`/`::` 等）与 `$VAR` 元变量（截断安全并返回总 Token 数）
- **`MetaCapture`**：`{ var_name: string, captured_text: string }`；**`PatternMatchHit`**：`{ start_byte, end_byte, start_line, end_line, capture_start, capture_count }`
- **`findCodePatternMatches(src, pattern, *src_tok_buf, *pat_tok_buf, *out_hits, *out_captures): usize`**：非重叠扫描全部匹配，支持 `$X = $X + 1` 同名元变量一致性约束与 `$_` 单 Token 通配符，返回总命中数（截断安全）。

```cp
import {
    CodeToken,
    MetaCapture,
    PatternMatchHit,
    findCodePatternMatches,
} from "cap:text";

pub fn demoCodePatternMatch(src: string) {
    let src_toks: [128]CodeToken = [CodeToken{}; 128];
    let pat_toks: [16]CodeToken = [CodeToken{}; 16];
    let hits: [8]PatternMatchHit = [PatternMatchHit{}; 8];
    let caps: [16]MetaCapture = [MetaCapture{}; 16];

    // 匹配自增赋值 `$X = $X + 1`（自动要求两处 $X 文本一致，忽略注释与换行）
    const n = findCodePatternMatches(
        src,
        "$X = $X + 1",
        src_toks[0..],
        pat_toks[0..],
        hits[0..],
        caps[0..],
    );
    * = n;
}
```

---

## 上游行为契约与文本处理边界注意事项

1. **UTF-8 码点边界安全不变式**：
   - `TextChunker`、`TokenEstimator.truncateToTokenBudget` 与 `truncateEllipsis` 在任意多字节 UTF-8（中文/Emoji）输入下均严格对齐码点起始字节，绝不产出非法半截 UTF-8 字节序列。
2. **RAG 块重叠约束（`ChunkOptions`）**：
   - `overlap_chars` 必须严格小于目标块大小，避免切分游标原地踏步；Markdown 模式下围栏代码块（```` ``` ````）内部的 `#` 注释行不会被误判为章节标题（`TextChunk.heading`）。
3. **Unicode 完整大小写折叠（`caseFoldEqual`）**：
   - 相比仅限 ASCII 的大小写比较，`caseFoldEqual` 正确处理德语 `ß == SS`、希腊语与多码点折叠映射，适合国际化搜索与标识符去重。

---

## 扩展模板引擎、NLP、全文检索、字符串算法与格式化模块（零冲突归并）

以下 12 个纯 CP 文本/模板/NLP/字符串算法/三路合并/终端表格子模块已统一收敛至 `lib/text/` 并零冲突导出至 `cap:text`：

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/text/mustache.cp` | 零分配 Mustache 模板引擎 | `renderMustache`, `validateMustache`, `MustacheIter`, `MustacheVar`, `MustacheToken` |
| `lib/text/templateValue.cp` / `templateEngine.cp` | Mustache 兼容 Prompt/HTML/代码生成模板 | `TemplateEngine`, `CompiledTemplate`, `TemplateContext`, `TemplateValue`, `TemplateError` |
| `lib/text/soundex.cp` | 语音编码 (Soundex/Metaphone/NYSIIS/Cologne) | `soundex`, `soundexEquals`, `soundexDifference`, `metaphone`, `nysiis`, `colognePhonetic` |
| `lib/text/porter.cp` | Porter 英文词干提取器 | `porterStem`, `porterStemEquals`, `porterMeasure`, `PORTER_MAX_WORD_LEN` |
| `lib/text/bm25Tokenizer.cp` / `bm25.cp` / `bm25Rrf.cp` | BM25 全文检索、分词与 RRF 融合排序 | `Bm25Index`, `Bm25Config`, `Bm25Hit`, `rrfFuse`, `tokenizeTerms`, `defaultStopwords` |
| `lib/text/tableWidth.cp` / `table.cp` | ANSI & 东亚宽字符感知表格渲染 | `Table`, `ColumnSpec`, `TableStyle`, `DisplayWidth`, `TextAlign`, `TableError` |
| `lib/text/diff3.cp` | 三路文本合并与冲突块检测 | `mergeLines`, `mergeTexts`, `hasConflicts`, `MergeResult`, `MergeHunk`, `ConflictBlock`, `ConflictStyle` |
| `lib/text/nfaRegex.cp` | 线性时间 Thompson NFA 正则引擎 | `NfaRegex`, `MatchSpan`, `RegexError`, `MAX_NFA_STATES` |
| `lib/text/aho.cp` | Aho-Corasick 多模式字符串匹配自动机 | `AhoCorasick`, `AhoBuilder`, `AhoMatch`, `MatchKind`, `AhoError` |
| `lib/text/kmpZAlgo.cp` | KMP / Z-Algorithm / 最长回文子串 | `computeLps`, `kmpFindFirst`, `kmpFindAll`, `zArray`, `longestPalindrome`, `PalindromeSpan` |
| `lib/text/suffixarray.cp` | 后缀数组、Kasai LCP 与子串索引 | `SuffixArray`, `SuffixArrayError` |
| `lib/text/spell.cp` | SymSpell / BK-Tree 拼写纠错 | `SymSpellIndex`, `BkTree`, `SpellSuggestion`, `SuggestVerbosity`, `spellDamerauLevenshtein`, `spellSoundex` |

> **重叠对照参考归档说明**：`lib/text/fuzzy_.cp`（与 `fuzzy.cp` 重叠）与 `lib/text/suffix_array_.cp`（与 `suffixarray.cp` 重叠）为对照参考副本（文件首行带 `// Reference Only:` 声明），不在 `lib/text/index.cp` 冲突导出。

---

## Batch 34 国际化、语法高亮、通配符、标记图谱与文本记录格式子模块（零冲突归并）

以下 16 个纯 CP 文本/国际化/高亮/通配符/标记语言与记录解析子模块已统一收敛至 `lib/text/` 并零冲突导出至 `cap:text`：

| 子模块文件 | 原目录 | 核心职责 |
| :--- | :--- | :--- |
| `lib/text/i18n.cp` | `lib/i18n` | Locale Negotiator、CLDR 复数规则与层级消息目录 (`I18nCatalog`) |
| `lib/text/intlNumber.cp`, `intlUnits.cp` | `lib/intl` | 分组/紧凑/序数/百分比数字格式化与双向人类可读单位解析 |
| `lib/text/punycode.cp`, `idna.cp` | `lib/idna` | RFC 3492 Punycode 与 IDNA 国际化域名编解码及 RFC 1034/1123 校验 |
| `lib/text/highlight.cp` | `lib/highlight` | 多语言词法语法高亮器 |
| `lib/text/glob.cp`, `globset.cp` | `lib/globset` | 单模式与多模式 Glob NFA 编译器与匹配集 |
| `lib/text/mermaid.cp` | `lib/mermaid` | 声明式 Mermaid 流程图/时序图文本构建与校验 |
| `lib/text/dot.cp` | `lib/dot` | Graphviz DOT 图谱文本解析器与序列化器 |
| `lib/text/ris.cp` | `lib/ris` | Research Information Systems `.ris` 文献引文文本解析器 |
| `lib/text/gedcom.cp` | `lib/gedcom` | GEDCOM 5.5.1 / 7.0 族谱文本记录解析器 |
| `lib/text/marc.cp` | `lib/marc` | ISO 2709 / MARC 21 图书馆书目记录解析器 |
| `lib/text/mobi.cp` | `lib/mobi` | Amazon Mobipocket / Kindle `.mobi` 电子书文档解析器 |
| `lib/text/yenc.cp` | `lib/yenc` | yEnc v1.3 二进制转文本编解码器与头尾解析 |
| `lib/text/abc.cp` | `lib/abc` | ABC 音乐记谱法 v2.1 文本标记解析与事件词法器 |
| `lib/text/smi.cp` | `lib/smi` | Daylight SMILES / OpenSMILES 线性化学分子记法解析器 |
| `lib/text/lucene.cp` | `lib/lucene` | Apache Lucene CodecUtil 全文索引文件头解析器 |
| `lib/text/igc.cp` | `lib/igc` | FAI IGC 纯文本飞行记录仪日志解析器 |

---

## 收敛子模块深度文档

- [abc.md](text/abc.md)
- [aho.md](text/aho.md)
- [bm25.md](text/bm25.md)
- [diff3.md](text/diff3.md)
- [dot.md](text/dot.md)
- [gedcom.md](text/gedcom.md)
- [globset.md](text/globset.md)
- [highlight.md](text/highlight.md)
- [i18n.md](text/i18n.md)
- [idna.md](text/idna.md)
- [igc.md](text/igc.md)
- [intl.md](text/intl.md)
- [kmp_z_algo.md](text/kmp_z_algo.md)
- [lucene.md](text/lucene.md)
- [marc.md](text/marc.md)
- [mermaid.md](text/mermaid.md)
- [mobi.md](text/mobi.md)
- [mustache.md](text/mustache.md)
- [nfa_regex.md](text/nfa_regex.md)
- [porter.md](text/porter.md)
- [ris.md](text/ris.md)
- [smi.md](text/smi.md)
- [soundex.md](text/soundex.md)
- [spell.md](text/spell.md)
- [suffixarray.md](text/suffixarray.md)
- [table.md](text/table.md)
- [template.md](text/template.md)
- [yenc.md](text/yenc.md)

## 历史文本/格式化/编码域散装模块收敛（W29-Sync R3）

以下 12 个原独立顶层目录已统一收敛至 `cap:text`（`lib/text/`），可直接从 `"cap:text"` 导入：

| 原顶层目录 | 收敛目标文件 | 主要导出符号 |
|---|---|---|
| `cap:byte_size` | `lib/text/byteSize.cp` | `parseByteSize`, `formatBytesIec`, `formatBytesSi` |
| `cap:hex_dump` | `lib/text/hexDump.cp` | `encodeHexLower`, `encodeHexUpper`, `decodeHexStrict`, `decodeHex`, `formatHexDumpLine`, `formatHexDump` |
| `cap:human_fmt` | `lib/text/humanFmt.cp` | `HumanTableStyle`, `formatBytes`, `formatDurationNs`, `redactUrl`, `formatTableBorder`, `formatTableRow` |
| `cap:number_format` | `lib/text/numberFmt.cp` | `formatGroupedU64`, `formatGroupedI64`, `ordinalSuffix`, `formatOrdinal`, `formatRadixU64`, `formatFixedPoint` |
| `cap:char_freq` | `lib/text/charFreq.cp` | `HEAD_CHAR_COUNT`, `TAIL_CHAR_COUNT`, `CHAR_FREQ_COUNT`, `CharAndCount`, `CharFreq`, `NameMinifier` |
| `cap:charset_label` | `lib/text/charsetLabel.cp` | `CharsetKind`, `resolveCharsetLabel`, `canonicalCharsetName`, `isUtf8Compatible`, `isAsciiSuperset`, `sniffBomCharset` |
| `cap:cow_str` | `lib/text/cowStr.cp` | `CowStrKind`, `CowStr` |
| `cap:small_string` | `lib/text/smallStr.cp` | `MAX_INLINE_LEN`, `SmallString`, `ExternalString`, `StringInternPool` |
| `cap:smol_str` | `lib/text/smolStr.cp` | `INLINE_MAX_LEN`, `SmolStr` |
| `cap:grapheme_cluster` | `lib/text/grapheme.cp` | `DecodedCodepoint`, `GraphemeBreakProperty`, `GraphemeBreakState`, `GraphemeIterator`, `classifyCodepoint`, `shouldBreak`, `decodeUtf8Step`, `count`, `sliceByGraphemes` |
| `cap:utf8_dfa` | `lib/text/utf8Dfa.cp` | `UTF8_ACCEPT`, `UTF8_REJECT`, `REPLACEMENT_CHAR`, `DecodeStep`, `decodeByte`, `isDoneDecoding`, `Utf8Iterator`, `isValidUtf8`, `countCodepoints`, ASCII 分类/大小写转换 |
| `cap:glob_escape` | `lib/text/globEscape.cp` | `isGlobMagic`, `hasGlobMagic`, `escapeGlob`, `unescapeGlob`, `staticBaseDir`, `isNegatedPattern`, `stripNegation` |

## Wave 33 工业级安全加固与 16-Lane SIMD 加速（`glob.cp` & `globset.cp` 对标 `BurntSushi/ripgrep` MIT/Unlicense）

`lib/text/glob.cp` 与 `lib/text/globset.cp` 已对照 `BurntSushi/ripgrep`（`crates/globset`，MIT/Unlicense）完成全量语义加固、组合爆炸防护、线性时间防回溯 NFA 修复与 16-lane `@Vector(16, u8)` SIMD 加速，保持 100% 向后兼容：

| 加固项 / 新增符号 | 所在文件 | 工程价值 |
|---|---|---|
| `LeadingDoubleStarSlash` 双状态边界自动机 | `lib/text/glob.cp` | 修复 `a/**/b` 误匹配 `a/xb` 与 `**/foo` 误匹配 `foofoo` 的跨段穿透缺陷；严格区分零层目录跳过与段内消费，保持 $O(NM)$ 无回溯线性时间 |
| `literal_separator` 字符类严格隔离 | `lib/text/glob.cp` | 无论正向 `[.-0]`/`[/]` 或否定 `[!a]`，在 `literal_separator = true` 下均禁止匹配 `/`，与 `globset` `SLASHLIT` 向量 100% 对齐 |
| `GLOB_MAX_ALTERNATES = 64` / `GLOB_MAX_BRACE_DEPTH = 16` / `GlobError.TooManyAlternates` | `lib/text/glob.cp`, `lib/text/globset.cp` | 拦截 `{a,b}{a,b}...` 等花括号组合爆炸与深层嵌套，防内存耗尽；`addWithOptions` 具备 `errdefer` 原子回滚 |
| `simdIndexOfByte` / `simdIndexOfAny4` / `simdIndexOfGlobMagic` / `simdBytesEqual` | `lib/text/glob.cp`, `lib/text/globset.cp` | 纯 CP 16-lane `@Vector(16, u8)` 魔术字符/分隔符扫描与静态字面量前缀/后缀快路径（配标量参考函数 0 差异对拍）；`IgnoreMatcher` 新增 `ruleCount()` |

- **权威回归与 Fuzz 套件**：`cap/test/testTextGlobRustRefSuite.cp` 继承 `BurntSushi/ripgrep` `globset` 测试向量，覆盖 `*`/`?`/`**`（零层与多层）/`[a-z]`/`[!0-9]`/`[.-0]`/`{a,b}`/转义/大小写不敏感/`.gitignore` 否定与目录规则、4 类病理模式毫秒级防回溯与组合爆炸拦截、16-lane SIMD vs 标量 0 差异对拍，以及 1,600 组确定性 PRNG 畸形模式与路径 Fuzz。
