[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# subtitle

Zero-heap SRT & WebVTT subtitle parser, timecode formatter, cue time-shifter, and SRT<->VTT converter

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:subtitle` ([https://github.com/gsantiago/subtitle.js](https://github.com/gsantiago/subtitle.js))

| 元信息 | 值 |
| --- | --- |
| 包名 | `subtitle` |
| 版本 | `1.0.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add subtitle
```

```cp
import {
    SubtitleFormat,
    Cue,
    CueList,
    parseUint,
    parseTimestampMs,
    put2,
    put3,
    formatTimestampMs,
} from "subtitle";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SubtitleFormat` | `Srt, WebVtt` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Cue` | `class` | 字段: `index: u32, start_ms: i64, end_ms: i64, text: string` |
| `CueList` | `class` | 字段: `cues: [64]Cue, count: usize, format: SubtitleFormat` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseUint` | `(s: string): i64?` | 通过 `parseUint` 解析输入数据，返回 `i64?`。 |
| `parseTimestampMs` | `(raw: string): i64?` | 通过 `parseTimestampMs` 解析输入数据，返回 `i64?`。 |
| `put2` | `(out: []u8, pos: &mut usize, v: i64)` | 写入或更新状态数据。 |
| `put3` | `(out: []u8, pos: &mut usize, v: i64)` | 写入或更新状态数据。 |
| `formatTimestampMs` | `(ms_in: i64, fmt: SubtitleFormat, out: []u8): string` | 通过 `formatTimestampMs` 编码或格式化输出，返回 `string`。 |
| `parseSubtitles` | `(input: string): CueList` | 通过 `parseSubtitles` 解析输入数据，返回 `CueList`。 |
| `shiftCues` | `(list: &mut CueList, offset_ms: i64)` | 就地执行 `shiftCues` 操作。 |
| `formatSubtitles` | `(list: CueList, target_fmt: SubtitleFormat, out: []u8): string` | 通过 `formatSubtitles` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { parseUint } from "subtitle";

pub fn main() {
    let result = parseUint("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
