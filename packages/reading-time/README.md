[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/text-stats

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:reading-time` ([https://github.com/ngryman/reading-time](https://github.com/ngryman/reading-time))（原库名 `reading-time` → CP 包名 `@cap/text-stats`）

Zero-allocation bilingual English & CJK document statistics, reading-time estimator, and LLM token estimator for CP — clean-room MIT parity with npm `reading-time` and `word-count`.

零堆分配的 CP 中英文混合文档统计与阅读时长估算引擎，一次遍历即可输出字节数、字符数、英文词数、CJK 字数、行数、非空行数、段落数、句子数、预估阅读秒/分钟以及 LLM Token 估算数，适用于 CMS、知识库与 AI RAG 流水线（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/text-stats
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `TextStats` | `pub struct { bytes, chars, ascii_words, cjk_chars, total_words, lines, non_empty_lines, paragraphs, sentences, est_reading_sec, est_reading_min, est_llm_tokens: usize }` | 文档统计结果结构体 |
| `analyzeText` | `(text: string) -> TextStats` | 默认速度（英文 200 WPM / 中文 350 字/分）分析文档 |
| `analyzeTextWithWpm` | `(text: string, en_wpm: usize, cjk_cpm: usize) -> TextStats` | 按自定义中英文阅读速度分析文档 |
| `formatReadingTime` | `(stats: TextStats, zh: bool, out: []u8) -> string` | 格式化阅读时长（如 `"3 min read"` / `"约 3 分钟读完"`、不足 1 分钟输出 `"< 1 min read"` / `"不足 1 分钟"`） |

## Usage / 使用示例

```cp
import { analyzeText, analyzeTextWithWpm, formatReadingTime } from "@cap/text-stats";

const doc = "CP 是一门高性能系统语言。It compiles to native Zig!";
const stats = analyzeText(doc);
// stats.ascii_words == 6, stats.cjk_chars == 10, stats.total_words == 16, stats.sentences == 2

let buf: [64]u8 = undefined;
const label_en = formatReadingTime(stats, false, buf[0..64]); // "< 1 min read"
const label_zh = formatReadingTime(stats, true, buf[0..64]);  // "不足 1 分钟"
```

## License

MIT
