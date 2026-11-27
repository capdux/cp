[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# magnet-url

Zero-heap BitTorrent Magnet URI (BEP-9 / BEP-53 btih & btmh) parser, tracker extractor, and builder for CP (compatible with npm magnet-uri & Rust magnet-url)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:magnet-url` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `magnet-url` → CP 包名 `magnet-url`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `magnet-url` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add magnet-url
```

```cp
import {
    MagnetLink,
    TopicMatch,
    startsWithIgnoreCase,
    isHexStr,
    isBase32Str,
    parsePositiveI64,
    classifyExactTopic,
    parseMagnet,
} from "magnet-url";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MagnetLink` | `class` | 字段: `hash_kind: u8, info_hash: string, display_name: string, exact_length: i64, trackers: [8]string, tracker_count: usize, +1 项` |
| `TopicMatch` | `class` | 字段: `kind: u8, hash: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `startsWithIgnoreCase` | `(s: string, prefix: string): bool` | 调用 `startsWithIgnoreCase` 并返回 `bool`。 |
| `isHexStr` | `(s: string): bool` | 执行 `isHexStr` 条件判定，返回 `bool`。 |
| `isBase32Str` | `(s: string): bool` | 执行 `isBase32Str` 条件判定，返回 `bool`。 |
| `parsePositiveI64` | `(s: string): i64` | 通过 `parsePositiveI64` 解析输入数据，返回 `i64`。 |
| `classifyExactTopic` | `(xt: string): TopicMatch` | 调用 `classifyExactTopic` 并返回 `TopicMatch`。 |
| `parseMagnet` | `(uri: string): MagnetLink` | 通过 `parseMagnet` 解析输入数据，返回 `MagnetLink`。 |
| `appendBytes` | `(dst: []u8, pos: usize, src: string): usize` | 调用 `appendBytes` 并返回 `usize`。 |
| `formatMagnetV1` | `(info_hash: string, display_name: string, trackers: &[]string, out: []u8): usize` | 通过 `formatMagnetV1` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isHexStr } from "magnet-url";

pub fn main() {
    let result = isHexStr("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
