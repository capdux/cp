[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# whatwg-mimetype

Zero-allocation WHATWG MIME Sniffing Standard MIMEType parser, parameter map, essence, and serializer (clean-room MIT implementation of npm whatwg-mimetype, #200 / 215M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:whatwg-mimetype` ([https://github.com/jsdom/whatwg-mimetype](https://github.com/jsdom/whatwg-mimetype)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `whatwg-mimetype` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add whatwg-mimetype
```

```cp
import {
    MimeParam,
    MimeType,
    wmimeIsHttpWs,
    wmimeIsTokenChar,
    wmimeEndsWithIgnoreCase,
    wmimePushLower,
    parseMimeType,
} from "whatwg-mimetype";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MimeParam` | `class` | 字段: `name: string, value: string` |
| `MimeType` | `class` | 字段: `valid: bool, main_type: string, subtype: string, params: [8]MimeParam, param_count: usize` · 方法: `paramCount, getParam, essence, isHTML, isXML, isJavaScript, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wmimeIsHttpWs` | `(b: u8): bool` | 调用 `wmimeIsHttpWs` 并返回 `bool`。 |
| `wmimeIsTokenChar` | `(b: u8): bool` | 调用 `wmimeIsTokenChar` 并返回 `bool`。 |
| `wmimeEndsWithIgnoreCase` | `(s: string, suffix: string): bool` | 调用 `wmimeEndsWithIgnoreCase` 并返回 `bool`。 |
| `wmimePushLower` | `(out: []u8, pos: usize, s: string): usize` | 调用 `wmimePushLower` 并返回 `usize`。 |
| `parseMimeType` | `(input: string): MimeType` | 通过 `parseMimeType` 解析输入数据，返回 `MimeType`。 |

## 最小调用示例

```cp
import { parseMimeType } from "whatwg-mimetype";

pub fn main() {
    let result = parseMimeType("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
