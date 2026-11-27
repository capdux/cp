[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# zbase32

Zero-allocation Zooko human-oriented z-base-32 encoder, decoder, and validator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:zbase32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:zbase32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `zbase32` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add zbase32
```

```cp
import {
    decodeChar,
    encodedLen,
    encode,
    decode,
    isValidZBase32,
} from "zbase32";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `decodeChar` | `(b: u8): i32` | 通过 `decodeChar` 解析输入数据，返回 `i32`。 |
| `encodedLen` | `(byte_len: usize): usize` | 通过 `encodedLen` 编码或格式化输出，返回 `usize`。 |
| `encode` | `(data: &[]u8, out: []u8): usize` | 通过 `encode` 编码或格式化输出，返回 `usize`。 |
| `decode` | `(input: string, out: []u8): i64` | 通过 `decode` 解析输入数据，返回 `i64`。 |
| `isValidZBase32` | `(input: string): bool` | 执行 `isValidZBase32` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { decode } from "zbase32";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decode("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
