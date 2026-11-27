[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# crypto-bigint

Zero-allocation constant-time fixed-width 256-bit unsigned integer (U256) arithmetic and modular operations (RustCrypto crypto-bigint parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crypto-bigint` ([https://github.com/RustCrypto/crypto-bigint](https://github.com/RustCrypto/crypto-bigint))

| 元信息 | 值 |
| --- | --- |
| 包名 | `crypto-bigint` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0 OR MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add crypto-bigint
```

```cp
import {
    CbiAddRes,
    CbiSubRes,
    U256,
    cbiMask64,
    cbiAllOnes64,
    cbiCtMask,
    cbiReadBeU64,
    cbiReadLeU64,
} from "crypto-bigint";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CbiAddRes` | `class` | 字段: `val: U256, carry: u64` |
| `CbiSubRes` | `class` | 字段: `val: U256, borrow: u64` |
| `U256` | `class` | 字段: `limbs: [4]u64` · 方法: `fromU64, fromBeBytes, fromLeBytes, toBeBytes, toLeBytes, fromBeHex, +12 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cbiMask64` | `(): u128` | 调用 `cbiMask64` 并返回 `u128`。 |
| `cbiAllOnes64` | `(): u64` | 调用 `cbiAllOnes64` 并返回 `u64`。 |
| `cbiCtMask` | `(cond: bool): u64` | 调用 `cbiCtMask` 并返回 `u64`。 |
| `cbiReadBeU64` | `(b: &[]u8, off: usize): u64` | 调用 `cbiReadBeU64` 并返回 `u64`。 |
| `cbiReadLeU64` | `(b: &[]u8, off: usize): u64` | 调用 `cbiReadLeU64` 并返回 `u64`。 |
| `cbiWriteBeU64` | `(v: u64, out: []u8, off: usize)` | 就地执行 `cbiWriteBeU64` 操作。 |
| `cbiWriteLeU64` | `(v: u64, out: []u8, off: usize)` | 就地执行 `cbiWriteLeU64` 操作。 |
| `cbiHexNibble` | `(ch: u8): i32` | 调用 `cbiHexNibble` 并返回 `i32`。 |
| `cbiHexChar` | `(nibble: u8): u8` | 调用 `cbiHexChar` 并返回 `u8`。 |
| `cbiBitLen64` | `(v_in: u64): usize` | 调用 `cbiBitLen64` 并返回 `usize`。 |
| `cbiAddWithCarry` | `(a: U256, b: U256): CbiAddRes` | 调用 `cbiAddWithCarry` 并返回 `CbiAddRes`。 |
| `cbiSubWithBorrow` | `(a: U256, b: U256): CbiSubRes` | 调用 `cbiSubWithBorrow` 并返回 `CbiSubRes`。 |
| `addMod` | `(a: U256, b: U256, m: U256): U256` | 写入或更新状态数据。 |
| `subMod` | `(a: U256, b: U256, m: U256): U256` | 调用 `subMod` 并返回 `U256`。 |

## 最小调用示例

```cp
import { cbiMask64 } from "crypto-bigint";

pub fn main() {
    let result = cbiMask64();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
