[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @faker-js/faker

Zero-allocation deterministic fake data generator (xoshiro256** PRNG, person/network/text/commerce/UUIDv4/Luhn card/collections) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@faker-js/faker` ([https://github.com/faker-js/faker](https://github.com/faker-js/faker))（原库名 `@faker-js/faker` → CP 包名 `@faker-js/faker`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `@faker-js/faker` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @faker-js/faker
```

```cp
import {
    Faker,
    splitmixStep,
    rotl7,
    rotl45,
    appendByte,
    appendBytes,
    appendLowerAscii,
    appendCapitalized,
} from "@faker-js/faker";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Faker` | `class` | 字段: `s0: u64, s1: u64, s2: u64, s3: u64` · 方法: `init, reseed, nextU64, nextInt, nextBool, pickStr, +23 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `splitmixStep` | `(state: &mut u64): u64` | 调用 `splitmixStep` 并返回 `u64`。 |
| `rotl7` | `(x: u64): u64` | 调用 `rotl7` 并返回 `u64`。 |
| `rotl45` | `(x: u64): u64` | 调用 `rotl45` 并返回 `u64`。 |
| `appendByte` | `(out: []u8, pos: &mut usize, b: u8)` | 就地执行 `appendByte` 操作。 |
| `appendBytes` | `(out: []u8, pos: &mut usize, data: &[]u8)` | 就地执行 `appendBytes` 操作。 |
| `appendLowerAscii` | `(out: []u8, pos: &mut usize, data: &[]u8)` | 就地执行 `appendLowerAscii` 操作。 |
| `appendCapitalized` | `(out: []u8, pos: &mut usize, data: &[]u8)` | 就地执行 `appendCapitalized` 操作。 |
| `appendU64Dec` | `(out: []u8, pos: &mut usize, val: u64)` | 就地执行 `appendU64Dec` 操作。 |
| `appendDecPadded` | `(out: []u8, pos: &mut usize, val: u64, width: usize)` | 就地执行 `appendDecPadded` 操作。 |
| `appendHexByte` | `(out: []u8, pos: &mut usize, b: u8)` | 就地执行 `appendHexByte` 操作。 |
| `isLeapYear` | `(year: u64): bool` | 执行 `isLeapYear` 条件判定，返回 `bool`。 |
| `daysInMonth` | `(year: u64, month: u64): u64` | 调用 `daysInMonth` 并返回 `u64`。 |
| `isValidLuhn` | `(digits: &[]u8): bool` | 执行 `isValidLuhn` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isLeapYear } from "@faker-js/faker";

pub fn main() {
    let result = isLeapYear(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
