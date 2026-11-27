[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @sindresorhus/fnv1a

Zero-heap FNV-1 and FNV-1a 32-bit and 64-bit streaming non-cryptographic hash functions for CP (compatible with Rust fnv & npm @sindresorhus/fnv1a)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@sindresorhus/fnv1a` ([https://github.com/sindresorhus/fnv1a](https://github.com/sindresorhus/fnv1a)) / `crates.io:const-fnv1a-hash` ([https://github.com/jonasbb/const-fnv1a-hash](https://github.com/jonasbb/const-fnv1a-hash))（原库名 `@sindresorhus/fnv1a` → CP 包名 `@sindresorhus/fnv1a`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `@sindresorhus/fnv1a` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @sindresorhus/fnv1a
```

```cp
import {
    fnv64OffsetBasis,
    fnv64Prime,
    mulWrap32,
    mulFnv64Prime,
    fnv1a32Update,
    fnv1a32,
    fnv132,
    fnv1a64Update,
} from "@sindresorhus/fnv1a";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fnv64OffsetBasis` | `(): u64` | 调用 `fnv64OffsetBasis` 并返回 `u64`。 |
| `fnv64Prime` | `(): u64` | 调用 `fnv64Prime` 并返回 `u64`。 |
| `mulWrap32` | `(a: u32, b: u32): u32` | 调用 `mulWrap32` 并返回 `u32`。 |
| `mulFnv64Prime` | `(h: u64): u64` | 调用 `mulFnv64Prime` 并返回 `u64`。 |
| `fnv1a32Update` | `(state: u32, data: &[]u8): u32` | 调用 `fnv1a32Update` 并返回 `u32`。 |
| `fnv1a32` | `(data: &[]u8): u32` | 调用 `fnv1a32` 并返回 `u32`。 |
| `fnv132` | `(data: &[]u8): u32` | 调用 `fnv132` 并返回 `u32`。 |
| `fnv1a64Update` | `(state: u64, data: &[]u8): u64` | 调用 `fnv1a64Update` 并返回 `u64`。 |
| `fnv1a64` | `(data: &[]u8): u64` | 调用 `fnv1a64` 并返回 `u64`。 |
| `fnv164` | `(data: &[]u8): u64` | 调用 `fnv164` 并返回 `u64`。 |
| `formatHex64` | `(h: u64, out: []u8): usize` | 通过 `formatHex64` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatHex64 } from "@sindresorhus/fnv1a";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatHex64(0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
