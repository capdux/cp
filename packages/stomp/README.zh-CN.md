[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# stomp

Zero-allocation STOMP 1.2 messaging protocol frame encoder, decoder, and header escaper in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:stomp` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `stomp` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add stomp
```

```cp
import {
    StompHeader,
    StompFrame,
    escapeHeader,
    unescapeHeader,
    encodeFrame,
    parseContentLength,
    decodeFrame,
    getHeader,
} from "stomp";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StompHeader` | `class` | 字段: `key: string, value: string` |
| `StompFrame` | `class` | 字段: `command: string, headers: [16]StompHeader, header_count: usize, body: string, consumed: usize` · 方法: `empty` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `escapeHeader` | `(input: string, out: []u8): usize` | 调用 `escapeHeader` 并返回 `usize`。 |
| `unescapeHeader` | `(input: string, out: []u8): usize` | 调用 `unescapeHeader` 并返回 `usize`。 |
| `encodeFrame` | `(command: string, headers: &[]StompHeader, body: string, out: []u8): usize` | 通过 `encodeFrame` 编码或格式化输出，返回 `usize`。 |
| `parseContentLength` | `(val: string): i64` | 通过 `parseContentLength` 解析输入数据，返回 `i64`。 |
| `decodeFrame` | `(data: string, out_frame: &mut StompFrame): i32` | 通过 `decodeFrame` 解析输入数据，返回 `i32`。 |
| `getHeader` | `(frame: StompFrame, key: string): string` | 查询或读取目标数据，返回 `string`。 |

## 最小调用示例

```cp
import { parseContentLength } from "stomp";

pub fn main() {
    let result = parseContentLength("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
