[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jsesc

Zero-allocation JavaScript & JSON string escaper with ES6 code point, script-context, and minimal modes (clean-room MIT implementation of npm jsesc v3, #148 / 250M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsesc` ([https://github.com/mathiasbynens/jsesc](https://github.com/mathiasbynens/jsesc)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `jsesc` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jsesc
```

```cp
import {
    JsescOptions,
    JescDecoded,
    jescHexChar,
    jescPush,
    jescPush2,
    jescPushHex2,
    jescPushUnicode4,
    jescPushEs6CodePoint,
} from "jsesc";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `JsescOptions` | `class` | 字段: `quote: u8, wrap: bool, escape_everything: bool, minimal: bool, is_script_context: bool, json: bool, +2 项` · 方法: `default, jsonDefault` |
| `JescDecoded` | `class` | 字段: `cp: u32, len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `jescHexChar` | `(nibble: u32, lowercase: bool): u8` | 调用 `jescHexChar` 并返回 `u8`。 |
| `jescPush` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `jescPush` 并返回 `usize`。 |
| `jescPush2` | `(out: []u8, pos: usize, b0: u8, b1: u8): usize` | 调用 `jescPush2` 并返回 `usize`。 |
| `jescPushHex2` | `(out: []u8, pos: usize, val: u32, lowercase: bool): usize` | 调用 `jescPushHex2` 并返回 `usize`。 |
| `jescPushUnicode4` | `(out: []u8, pos: usize, val: u32, lowercase: bool): usize` | 调用 `jescPushUnicode4` 并返回 `usize`。 |
| `jescPushEs6CodePoint` | `(out: []u8, pos: usize, cp: u32, lowercase: bool): usize` | 调用 `jescPushEs6CodePoint` 并返回 `usize`。 |
| `jescDecodeUtf8` | `(input: string, idx: usize): JescDecoded` | 调用 `jescDecodeUtf8` 并返回 `JescDecoded`。 |
| `jescAsciiLower` | `(b: u8): u8` | 调用 `jescAsciiLower` 并返回 `u8`。 |
| `jescMatchesHtmlComment` | `(input: string, idx: usize): bool` | 调用 `jescMatchesHtmlComment` 并返回 `bool`。 |
| `jescMatchesCloseScript` | `(input: string, idx: usize): bool` | 调用 `jescMatchesCloseScript` 并返回 `bool`。 |
| `jescEscapeCodePoint` | `(out: []u8, pos: usize, cp: u32, is_json: bool, use_es6: bool, lowercase: bool): usize` | 调用 `jescEscapeCodePoint` 并返回 `usize`。 |
| `jsescWithOptions` | `(input: string, opts: JsescOptions, out: []u8): usize` | 调用 `jsescWithOptions` 并返回 `usize`。 |
| `jsesc` | `(input: string, out: []u8): usize` | 调用 `jsesc` 并返回 `usize`。 |

## 最小调用示例

```cp
import { jescDecodeUtf8 } from "jsesc";

pub fn main() {
    let result = jescDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
