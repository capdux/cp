[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pure-rand

Zero-allocation pure functional immutable PRNG (Xoroshiro128Plus, SplitMix64 seeding, jump, and unbiased uniformIntDistribution) (clean-room MIT implementation of npm pure-rand, #443 / 124M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pure-rand` ([https://github.com/dubzzz/pure-rand](https://github.com/dubzzz/pure-rand)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `pure-rand` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pure-rand
```

```cp
import {
    Xoroshiro128Plus,
    NextU64Result,
    NextI32Result,
    NextIntRangeResult,
    prRotl64,
    prSplitMix64Step,
    uniformIntDistribution,
} from "pure-rand";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Xoroshiro128Plus` | `class` | 字段: `s0: u64, s1: u64` · 方法: `fromSeed, fromState, nextU64, nextI32, jump` |
| `NextU64Result` | `class` | 字段: `value: u64, next: Xoroshiro128Plus` |
| `NextI32Result` | `class` | 字段: `value: i32, next: Xoroshiro128Plus` |
| `NextIntRangeResult` | `class` | 字段: `value: i64, next: Xoroshiro128Plus` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `prRotl64` | `(x: u64, k: u6): u64` | 调用 `prRotl64` 并返回 `u64`。 |
| `prSplitMix64Step` | `(state: u64): u64` | 调用 `prSplitMix64Step` 并返回 `u64`。 |
| `uniformIntDistribution` | `(min: i64, max: i64, rng: Xoroshiro128Plus): NextIntRangeResult` | 调用 `uniformIntDistribution` 并返回 `NextIntRangeResult`。 |

## 最小调用示例

```cp
import { prRotl64 } from "pure-rand";

pub fn main() {
    let result = prRotl64(0, /* k: u6 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
