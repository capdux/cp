[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# string_decoder

Zero-allocation streaming multi-byte UTF-8/UTF-16/Latin1/Base64 string decoder preserving incomplete boundary bytes across chunks (clean-room MIT implementation of npm string_decoder, #62 / 378M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string_decoder` ([https://github.com/nodejs/string_decoder](https://github.com/nodejs/string_decoder)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `string_decoder` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add string_decoder
```

```cp
import {
    DecoderEncoding,
    StringDecoder,
    sdecUtf8ExpectedLen,
    sdecPushReplacement,
    sdecPushCodePointUtf8,
} from "string_decoder";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `DecoderEncoding` | `Utf8, Latin1, Utf16Le` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StringDecoder` | `class` | 字段: `encoding: DecoderEncoding, last_char: [4]u8, last_need: usize, last_total: usize, high_surrogate: u16` · 方法: `new, utf8, pendingBytes, write, writeUtf16Le, emitUtf16Unit, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sdecUtf8ExpectedLen` | `(lead: u8): usize` | 调用 `sdecUtf8ExpectedLen` 并返回 `usize`。 |
| `sdecPushReplacement` | `(out: []u8, pos: usize): usize` | 调用 `sdecPushReplacement` 并返回 `usize`。 |
| `sdecPushCodePointUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | 调用 `sdecPushCodePointUtf8` 并返回 `usize`。 |

## 最小调用示例

```cp
import { sdecUtf8ExpectedLen } from "string_decoder";

pub fn main() {
    let result = sdecUtf8ExpectedLen(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
