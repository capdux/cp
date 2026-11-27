[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# data-uri-to-buffer

Zero-allocation RFC 2397 Data URI parser, Base64/percent decoder, and encoder for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:data-uri-to-buffer` ([https://github.com/TooTallNate/proxy-agents](https://github.com/TooTallNate/proxy-agents))（原库名 `data-uri-to-buffer` → CP 包名 `data-uri-to-buffer`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `data-uri-to-buffer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add data-uri-to-buffer
```

```cp
import {
    DataUri,
    appendByte,
    appendStr,
    isUnreservedUriByte,
    parseDataUri,
    isDataUri,
    b64Val,
    decodeBase64Payload,
} from "data-uri-to-buffer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DataUri` | `class` | 字段: `ok: bool, media_type: string, charset: string, is_base64: bool, raw_payload: string` · 方法: `invalid` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `isUnreservedUriByte` | `(c: u8): bool` | 执行 `isUnreservedUriByte` 条件判定，返回 `bool`。 |
| `parseDataUri` | `(uri: string): DataUri` | 通过 `parseDataUri` 解析输入数据，返回 `DataUri`。 |
| `isDataUri` | `(s: string): bool` | 执行 `isDataUri` 条件判定，返回 `bool`。 |
| `b64Val` | `(c: u8): u8?` | 调用 `b64Val` 并返回 `u8?`。 |
| `decodeBase64Payload` | `(payload: string, out: []u8): ([]u8)?` | 通过 `decodeBase64Payload` 解析输入数据，返回 `([]u8)?`。 |
| `decodePercentPayload` | `(payload: string, out: []u8): ([]u8)?` | 通过 `decodePercentPayload` 解析输入数据，返回 `([]u8)?`。 |
| `decodeDataUri` | `(uri: string, out: []u8): ([]u8)?` | 通过 `decodeDataUri` 解析输入数据，返回 `([]u8)?`。 |
| `encodeDataUriBase64` | `(media_type: string, payload: []u8, out: []u8): string` | 通过 `encodeDataUriBase64` 编码或格式化输出，返回 `string`。 |
| `encodeDataUriText` | `(media_type: string, text: string, out: []u8): string` | 通过 `encodeDataUriText` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { parseDataUri } from "data-uri-to-buffer";

pub fn main() {
    let result = parseDataUri("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
