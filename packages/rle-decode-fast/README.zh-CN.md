[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rle-decode-fast

Zero-allocation Apple/TIFF PackBits byte run-length compression and Prometheus/Gorilla-style Delta & Delta-of-Delta time-series integer encoding for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rle-decode-fast` ([https://github.com/okaneco/rle-decode-fast](https://github.com/okaneco/rle-decode-fast))（原库名 `rle-decode-fast` → CP 包名 `rle-decode-fast`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rle-decode-fast` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rle-decode-fast
```

```cp
import {
    encodePackBits,
    decodePackBits,
    encodeDeltaI64,
    decodeDeltaI64,
    encodeDeltaOfDeltaI64,
    decodeDeltaOfDeltaI64,
} from "rle-decode-fast";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `encodePackBits` | `(input: string, out: []u8): string?` | 通过 `encodePackBits` 编码或格式化输出，返回 `string?`。 |
| `decodePackBits` | `(encoded: string, out: []u8): string?` | 通过 `decodePackBits` 解析输入数据，返回 `string?`。 |
| `encodeDeltaI64` | `(values: &[]i64, out: []i64): usize` | 通过 `encodeDeltaI64` 编码或格式化输出，返回 `usize`。 |
| `decodeDeltaI64` | `(deltas: &[]i64, out: []i64): usize` | 通过 `decodeDeltaI64` 解析输入数据，返回 `usize`。 |
| `encodeDeltaOfDeltaI64` | `(values: &[]i64, out: []i64): usize` | 通过 `encodeDeltaOfDeltaI64` 编码或格式化输出，返回 `usize`。 |
| `decodeDeltaOfDeltaI64` | `(dod: &[]i64, out: []i64): usize` | 通过 `decodeDeltaOfDeltaI64` 解析输入数据，返回 `usize`。 |

## 最小调用示例

```cp
import { encodePackBits } from "rle-decode-fast";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodePackBits("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
