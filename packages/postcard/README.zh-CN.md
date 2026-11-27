[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# postcard

Zero-heap compact binary wire protocol codecs (Varint, ZigZag, and COBS framing)

| 元信息 | 值 |
| --- | --- |
| 包名 | `postcard` |
| 版本 | `1.1.3` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add postcard
```

```cp
import {
    VarintU64Res,
    VarintI64Res,
    pcrdU64Max,
    encodeVarintU64,
    decodeVarintU64,
    zigzagEncode,
    zigzagDecode,
    encodeVarintI64,
} from "postcard";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `VarintU64Res` | `class` | 字段: `ok: bool, value: u64, consumed: usize` |
| `VarintI64Res` | `class` | 字段: `ok: bool, value: i64, consumed: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pcrdU64Max` | `(): u64` | 调用 `pcrdU64Max` 并返回 `u64`。 |
| `encodeVarintU64` | `(val: u64, out: []u8): usize` | 通过 `encodeVarintU64` 编码或格式化输出，返回 `usize`。 |
| `decodeVarintU64` | `(data: &[]u8): VarintU64Res` | 通过 `decodeVarintU64` 解析输入数据，返回 `VarintU64Res`。 |
| `zigzagEncode` | `(val: i64): u64` | 调用 `zigzagEncode` 并返回 `u64`。 |
| `zigzagDecode` | `(val: u64): i64` | 调用 `zigzagDecode` 并返回 `i64`。 |
| `encodeVarintI64` | `(val: i64, out: []u8): usize` | 通过 `encodeVarintI64` 编码或格式化输出，返回 `usize`。 |
| `decodeVarintI64` | `(data: &[]u8): VarintI64Res` | 通过 `decodeVarintI64` 解析输入数据，返回 `VarintI64Res`。 |
| `cobsEncode` | `(src: &[]u8, out: []u8): usize` | 调用 `cobsEncode` 并返回 `usize`。 |
| `cobsDecode` | `(src: &[]u8, out: []u8): i64` | 调用 `cobsDecode` 并返回 `i64`。 |

## 最小调用示例

```cp
import { encodeVarintU64 } from "postcard";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeVarintU64(0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
