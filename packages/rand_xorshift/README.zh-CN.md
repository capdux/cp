[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rand_xorshift

Pure CP zero-allocation Marsaglia Xorshift PRNG family: XorShift128 (rand_xorshift parity), XorShift64Star, and XorShift32

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_xorshift` ([https://github.com/rust-random/rngs](https://github.com/rust-random/rngs)) / `npm:xorshift` ([https://github.com/AndreasMadsen/xorshift](https://github.com/AndreasMadsen/xorshift))（原库名 `rand_xorshift` → CP 包名 `rand_xorshift`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rand_xorshift` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rand_xorshift
```

```cp
import {
    XorShift128,
    XorShift64Star,
    XorShift32,
    xsU64FromHalves,
    xsAddWrap64,
    xsMulWrap64,
    xsSplitMixNext,
    xsReadU32Le,
} from "rand_xorshift";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `XorShift128` | `class` | 字段: `x: u32, y: u32, z: u32, w: u32` · 方法: `fromSeed, fromU64, nextU32, nextU64, nextF64, fillBytes` |
| `XorShift64Star` | `class` | 字段: `state: u64` · 方法: `new, nextU64, nextU32` |
| `XorShift32` | `class` | 字段: `state: u32` · 方法: `new, nextU32` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `xsU64FromHalves` | `(hi: u32, lo: u32): u64` | 调用 `xsU64FromHalves` 并返回 `u64`。 |
| `xsAddWrap64` | `(a: u64, b: u64): u64` | 调用 `xsAddWrap64` 并返回 `u64`。 |
| `xsMulWrap64` | `(a: u64, b: u64): u64` | 调用 `xsMulWrap64` 并返回 `u64`。 |
| `xsSplitMixNext` | `(st: &mut u64): u64` | 调用 `xsSplitMixNext` 并返回 `u64`。 |
| `xsReadU32Le` | `(b: [16]u8, off: usize): u32` | 调用 `xsReadU32Le` 并返回 `u32`。 |

## 最小调用示例

```cp
import { xsU64FromHalves } from "rand_xorshift";

pub fn main() {
    let result = xsU64FromHalves(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
