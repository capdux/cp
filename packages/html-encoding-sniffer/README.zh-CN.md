[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# html-encoding-sniffer

Zero-allocation WHATWG HTML 13.2.3.2 byte stream encoding sniffer and label normalizer (npm html-encoding-sniffer parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:html-encoding-sniffer` ([https://github.com/jsdom/html-encoding-sniffer](https://github.com/jsdom/html-encoding-sniffer))（原库名 `html-encoding-sniffer` → CP 包名 `html-encoding-sniffer`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `html-encoding-sniffer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add html-encoding-sniffer
```

```cp
import {
    SniffOptions,
    hesToLower,
    hesEqIgnoreCase,
    hesIsAsciiWs,
    hesTrimAsciiWs,
    hesStartsWithIgnoreCase,
    normalizeEncodingLabel,
    hesMapMetaEncoding,
} from "html-encoding-sniffer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SniffOptions` | `class` | 字段: `transport_layer_encoding: string, default_encoding: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hesToLower` | `(ch: u8): u8` | 调用 `hesToLower` 并返回 `u8`。 |
| `hesEqIgnoreCase` | `(a: string, b: string): bool` | 调用 `hesEqIgnoreCase` 并返回 `bool`。 |
| `hesIsAsciiWs` | `(ch: u8): bool` | 调用 `hesIsAsciiWs` 并返回 `bool`。 |
| `hesTrimAsciiWs` | `(s: string): string` | 调用 `hesTrimAsciiWs` 并返回 `string`。 |
| `hesStartsWithIgnoreCase` | `(bytes: &[]u8, pos: usize, needle: string): bool` | 调用 `hesStartsWithIgnoreCase` 并返回 `bool`。 |
| `normalizeEncodingLabel` | `(label: string): string` | 调用 `normalizeEncodingLabel` 并返回 `string`。 |
| `hesMapMetaEncoding` | `(label: string): string` | 调用 `hesMapMetaEncoding` 并返回 `string`。 |
| `hesExtractCharsetFromContent` | `(content: string): string` | 调用 `hesExtractCharsetFromContent` 并返回 `string`。 |
| `hesPrescanMeta` | `(bytes: &[]u8): string` | 调用 `hesPrescanMeta` 并返回 `string`。 |
| `sniffHtmlEncoding` | `(bytes: &[]u8, opts: SniffOptions): string` | 调用 `sniffHtmlEncoding` 并返回 `string`。 |

## 最小调用示例

```cp
import { hesEqIgnoreCase } from "html-encoding-sniffer";

pub fn main() {
    let result = hesEqIgnoreCase("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
