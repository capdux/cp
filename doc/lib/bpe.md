# `cap:bpe` — Byte-Pair Encoding (BPE) LLM Tokenizer & Token Budget Engine

## 1. 概述
`cap:bpe`（`lib/bpe/`）是纯 CP 实现的零 C 依赖工业级 Byte-Level BPE（Tiktoken 风格）分词器与 Token 预算控制引擎。内置 0..255 单字节兜底词表保证永不 OOV，支持最小 rank 优先相邻对合并、特殊 Token 切分与防注入普通编码、Tiktoken Base64 词表解析、语料自训练以及 UTF-8 边界安全的 Token 预算截断。

## 2. 核心类型
- `BpeError`: 词表与编解码错误枚举（`InvalidFormat`、`InvalidBase64`、`UnknownTokenId`、`DuplicateToken`、`InvalidTargetVocabSize`、`OutOfMemory`）。
- `SpecialToken`: 特殊控制 Token 结构（`text: []const u8`、`id: u32`）。
- `BpeMerge`: BPE 相邻合并规则（`left_id`、`right_id`、`merged_id`、`rank`）。
- `BpeVocab`: 词表、Merge 规则与特殊 Token 注册中心，内部统一托管所有堆分配字节切片的生命周期。
- `BpeTokenizer`: 核心 BPE 编解码、语料训练与 Token 预算截断引擎。

## 3. API 速查
- **词表构建 (`BpeVocab`)**：
  - `BpeVocab.init(alloc)` / `deinit()`
  - `addByteTokens()`: 注册 0..255 单字节基础词表
  - `addToken(bytes, rank_id)` / `addMerge(left_bytes, right_bytes)` / `addSpecialToken(text, id)`
  - `loadTiktokenText(alloc, content)`: 解析 `<base64_token> <rank>` 格式 Tiktoken 词表
- **编解码与预算 (`BpeTokenizer`)**：
  - `BpeTokenizer.init(alloc)` / `fromVocab(vocab)` / `loadTiktoken(alloc, content)` / `deinit()`
  - `trainFromCorpus(alloc, text, target_vocab_size)`: 从语料迭代统计最高频相邻对并生成 Merge 规则
  - `encode(alloc, text)`: 识别 `special_tokens` 切分后执行 Byte-Level BPE 合并
  - `encodeOrdinary(alloc, text)`: 忽略 `special_tokens` 按普通文本编码（防 Prompt 注入）
  - `decode(alloc, token_ids)`: 无损还原为 UTF-8 字节流（未知 ID 返回 `BpeError.UnknownTokenId`）
  - `countTokens(alloc, text)`: 统计文本 Token 数
  - `truncateToMaxTokens(alloc, text, max_tokens)`: 按 Token 预算截断并保证截断后 UTF-8 边界完整不乱码

## 4. 示例
```cp
import { BpeTokenizer } from "cap:bpe";

let tok = try BpeTokenizer.trainFromCorpus(alloc, "hello hello world", 260);
defer tok.deinit();

const ids = try tok.encode(alloc, "hello world");
defer alloc.free(ids);

const decoded = try tok.decode(alloc, ids);
defer alloc.free(decoded);
```
