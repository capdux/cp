[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# enumflags2

Zero-allocation 64-bit type-safe enum bitflags set with valid-mask protection for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:enumflags2` ([https://github.com/meithecatte/enumflags2](https://github.com/meithecatte/enumflags2))

| 元信息 | 值 |
| --- | --- |
| 包名 | `enumflags2` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add enumflags2
```

```cp
import { BitFlags64, ef2Popcount64, ef2Ctz64 } from "enumflags2";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BitFlags64` | `class` | 字段: `bits: u64, valid_mask: u64` · 方法: `empty, all, fromBitsTruncate, fromBitsStrict, raw, len, +15 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ef2Popcount64` | `(val: u64): usize` | 调用 `ef2Popcount64` 并返回 `usize`。 |
| `ef2Ctz64` | `(val: u64): u8` | 调用 `ef2Ctz64` 并返回 `u8`。 |

## 最小调用示例

```cp
import { ef2Popcount64 } from "enumflags2";

pub fn main() {
    let result = ef2Popcount64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
