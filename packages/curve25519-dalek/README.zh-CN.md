[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# curve25519-dalek

Zero-allocation constant-time RFC 7748 X25519 elliptic curve Diffie-Hellman and GF(2^255-19) field arithmetic (curve25519-dalek parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:curve25519-dalek` ([https://github.com/dalek-cryptography/curve25519-dalek](https://github.com/dalek-cryptography/curve25519-dalek))

| 元信息 | 值 |
| --- | --- |
| 包名 | `curve25519-dalek` |
| 版本 | `1.0.0` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add curve25519-dalek
```

```cp
import {
    FieldElement,
    c25Carry,
    c25Sel,
    clampScalar,
    x25519,
    x25519Basepoint,
} from "curve25519-dalek";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FieldElement` | `class` | 字段: `limbs: [16]i64` · 方法: `fromBytes, toBytes, add, sub, mul, square, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `c25Carry` | `(o: []i64)` | 就地执行 `c25Carry` 操作。 |
| `c25Sel` | `(p: []i64, q: []i64, b: i64)` | 就地执行 `c25Sel` 操作。 |
| `clampScalar` | `(k: [32]u8): [32]u8` | 调用 `clampScalar` 并返回 `[32]u8`。 |
| `x25519` | `(scalar: [32]u8, u_coord: [32]u8): [32]u8` | 调用 `x25519` 并返回 `[32]u8`。 |
| `x25519Basepoint` | `(scalar: [32]u8): [32]u8` | 调用 `x25519Basepoint` 并返回 `[32]u8`。 |

## 最小调用示例

```cp
import { clampScalar } from "curve25519-dalek";

pub fn main() {
    let result = clampScalar(/* k: [32]u8 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
