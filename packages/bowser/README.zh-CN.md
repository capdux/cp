[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bowser

Zero-allocation User-Agent parser for browser, OS, engine, and platform detection (npm bowser parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`bowser`](https://github.com/bowser-js/bowser)（`MIT`）（包名 `bowser`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `bowser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bowser
```

```cp
import {
    PlatformType,
    ParsedUA,
    BwsPair,
    bwsIsVersionChar,
    bwsFindSubstrCI,
    bwsContains,
    bwsContainsCI,
    bwsExtractVersionAt,
} from "bowser";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `PlatformType` | `Desktop, Mobile, Tablet, Bot, Unknown` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedUA` | `class` | 字段: `browser_name: string, browser_version: string, os_name: string, os_version: string, engine_name: string, platform: PlatformType` |
| `BwsPair` | `class` | 字段: `name: string, version: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bwsIsVersionChar` | `(c: u8): bool` | 调用 `bwsIsVersionChar` 并返回 `bool`。 |
| `bwsFindSubstrCI` | `(haystack: string, needle: string): usize?` | 调用 `bwsFindSubstrCI` 并返回 `usize?`。 |
| `bwsContains` | `(haystack: string, needle: string): bool` | 调用 `bwsContains` 并返回 `bool`。 |
| `bwsContainsCI` | `(haystack: string, needle: string): bool` | 调用 `bwsContainsCI` 并返回 `bool`。 |
| `bwsExtractVersionAt` | `(ua: string, start: usize): string` | 调用 `bwsExtractVersionAt` 并返回 `string`。 |
| `bwsExtractAfter` | `(ua: string, token: string): string` | 调用 `bwsExtractAfter` 并返回 `string`。 |
| `bwsDetectBrowser` | `(ua: string): BwsPair` | 调用 `bwsDetectBrowser` 并返回 `BwsPair`。 |
| `bwsDetectOS` | `(ua: string): BwsPair` | 调用 `bwsDetectOS` 并返回 `BwsPair`。 |
| `bwsDetectEngine` | `(ua: string): string` | 调用 `bwsDetectEngine` 并返回 `string`。 |
| `bwsDetectPlatform` | `(ua: string, os_name: string): PlatformType` | 调用 `bwsDetectPlatform` 并返回 `PlatformType`。 |
| `parseUserAgent` | `(ua: string): ParsedUA` | 通过 `parseUserAgent` 解析输入数据，返回 `ParsedUA`。 |

## 最小调用示例

```cp
import { parseUserAgent } from "bowser";

pub fn main() {
    let result = parseUserAgent("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
