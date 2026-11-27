[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# convert-source-map

Zero-allocation inline sourceMappingURL comment extractor, Base64 decoder/encoder, and comment stripper (clean-room MIT implementation of npm convert-source-map, #96 / 307M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:convert-source-map` ([https://github.com/thlorenz/convert-source-map](https://github.com/thlorenz/convert-source-map)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `convert-source-map` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add convert-source-map
```

```cp
import {
    SourceMapCommentKind,
    SourceMapComment,
    csmStartsWith,
    csmPush,
    csmB64Val,
    csmB64Encode,
    csmB64Decode,
    findSourceMapComment,
} from "convert-source-map";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SourceMapCommentKind` | `None, InlineBase64, ExternalFile` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SourceMapComment` | `class` | 字段: `kind: SourceMapCommentKind, comment_start: usize, comment_end: usize, payload: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `csmStartsWith` | `(s: string, pos: usize, lit: string): bool` | 调用 `csmStartsWith` 并返回 `bool`。 |
| `csmPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `csmPush` 并返回 `usize`。 |
| `csmB64Val` | `(b: u8): i32` | 调用 `csmB64Val` 并返回 `i32`。 |
| `csmB64Encode` | `(data: &[]u8, out: []u8, start_pos: usize): usize` | 调用 `csmB64Encode` 并返回 `usize`。 |
| `csmB64Decode` | `(b64: string, out: []u8): i64` | 调用 `csmB64Decode` 并返回 `i64`。 |
| `findSourceMapComment` | `(src: string): SourceMapComment` | 查询或读取目标数据，返回 `SourceMapComment`。 |
| `fromSource` | `(src: string, out: []u8): i64` | 调用 `fromSource` 并返回 `i64`。 |
| `toComment` | `(json_map: string, multiline: bool, out: []u8): usize` | 调用 `toComment` 并返回 `usize`。 |
| `removeComments` | `(src: string, out: []u8): usize` | 调用 `removeComments` 并返回 `usize`。 |

## 最小调用示例

```cp
import { csmStartsWith } from "convert-source-map";

pub fn main() {
    let result = csmStartsWith("example", 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
