[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# siphash

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed 64-bit HashDoS-resistant PRF for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:siphash` ([https://github.com/jedisct1/siphash-js](https://github.com/jedisct1/siphash-js))（原库名 `siphash` → CP 包名 `siphash`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `siphash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add siphash
```

```cp
import {
    SipState,
    shU64,
    shAdd64,
    shRotl64,
    shReadLe64,
    shRound,
    shRun,
    siphash24,
} from "siphash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SipState` | `class` | 字段: `v0: u64, v1: u64, v2: u64, v3: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `shU64` | `(hi: u32, lo: u32): u64` | 调用 `shU64` 并返回 `u64`。 |
| `shAdd64` | `(a: u64, b: u64): u64` | 调用 `shAdd64` 并返回 `u64`。 |
| `shRotl64` | `(x: u64, bits: u32): u64` | 调用 `shRotl64` 并返回 `u64`。 |
| `shReadLe64` | `(data: &[]u8, offset: usize): u64` | 调用 `shReadLe64` 并返回 `u64`。 |
| `shRound` | `(st: &mut SipState)` | 就地执行 `shRound` 操作。 |
| `shRun` | `(k0: u64, k1: u64, data: &[]u8, c_rounds: usize, d_rounds: usize): u64` | 调用 `shRun` 并返回 `u64`。 |
| `siphash24` | `(k0: u64, k1: u64, data: &[]u8): u64` | 调用 `siphash24` 并返回 `u64`。 |
| `siphash13` | `(k0: u64, k1: u64, data: &[]u8): u64` | 调用 `siphash13` 并返回 `u64`。 |
| `siphash24Key16` | `(key: [16]u8, data: &[]u8): u64` | 调用 `siphash24Key16` 并返回 `u64`。 |
| `siphash13Key16` | `(key: [16]u8, data: &[]u8): u64` | 调用 `siphash13Key16` 并返回 `u64`。 |

## 最小调用示例

```cp
import { shU64 } from "siphash";

pub fn main() {
    let result = shU64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
