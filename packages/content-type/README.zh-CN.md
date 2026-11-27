[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# content-type

Zero-heap RFC 7231 HTTP Content-Type header parser and formatter with quoted-string unescaping (clean-room port of content-type)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:content-type` ([https://github.com/jshttp/content-type](https://github.com/jshttp/content-type))

| 元信息 | 值 |
| --- | --- |
| 包名 | `content-type` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add content-type
```

```cp
import {
    ContentTypeParam,
    ContentType,
    ctypIsOws,
    ctypIsTokenChar,
    ctypIsValidToken,
    ctypIsQuotedAllowed,
    ctypWriteByte,
    ctypWriteSlice,
} from "content-type";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ContentTypeParam` | `class` | 字段: `name: string, value: string` · 方法: `new` |
| `ContentType` | `class` | 字段: `valid: bool, media_type: string, params: [8]ContentTypeParam, param_count: usize` · 方法: `invalid, getParam, getParamUnescaped, charset, boundary` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ctypIsOws` | `(c: u8): bool` | 调用 `ctypIsOws` 并返回 `bool`。 |
| `ctypIsTokenChar` | `(c: u8): bool` | 调用 `ctypIsTokenChar` 并返回 `bool`。 |
| `ctypIsValidToken` | `(s: string): bool` | 调用 `ctypIsValidToken` 并返回 `bool`。 |
| `ctypIsQuotedAllowed` | `(c: u8): bool` | 调用 `ctypIsQuotedAllowed` 并返回 `bool`。 |
| `ctypWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `ctypWriteByte` 并返回 `usize`。 |
| `ctypWriteSlice` | `(out: []u8, pos: usize, s: string): usize` | 调用 `ctypWriteSlice` 并返回 `usize`。 |
| `isValidMediaType` | `(s: string): bool` | 执行 `isValidMediaType` 条件判定，返回 `bool`。 |
| `unescapeQuotedString` | `(raw: string, out: []u8): usize` | 调用 `unescapeQuotedString` 并返回 `usize`。 |
| `parseContentType` | `(header: string): ContentType` | 通过 `parseContentType` 解析输入数据，返回 `ContentType`。 |
| `formatContentType` | `(media_type: string, params: &[]ContentTypeParam, out: []u8): usize` | 通过 `formatContentType` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidMediaType } from "content-type";

pub fn main() {
    let result = isValidMediaType("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
