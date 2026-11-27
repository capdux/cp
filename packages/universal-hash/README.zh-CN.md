[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# universal-hash

Zero-allocation NIST SP 800-38D GF(2^128) universal hash (GHASH for AES-GCM) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:universal-hash` ([https://github.com/RustCrypto/universal-hashes](https://github.com/RustCrypto/universal-hashes))（原库名 `universal-hash` → CP 包名 `universal-hash`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `universal-hash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add universal-hash
```

```cp
import {
    Block128,
    Ghash,
    bitToMask64,
    readBeU64,
    writeBeU64,
    gf128Mul,
} from "universal-hash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Block128` | `class` | 字段: `hi: u64, lo: u64` · 方法: `zero, one, fromBytes, toBytes, xor, eq` |
| `Ghash` | `class` | 字段: `h: Block128, y: Block128` · 方法: `new, reset, updateBlock, updatePadded, updateLengths, finalize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bitToMask64` | `(bit: u64): u64` | 调用 `bitToMask64` 并返回 `u64`。 |
| `readBeU64` | `(buf: &[]u8, off: usize): u64` | 调用 `readBeU64` 并返回 `u64`。 |
| `writeBeU64` | `(val: u64, out: []u8, off: usize)` | 就地执行 `writeBeU64` 操作。 |
| `gf128Mul` | `(x: Block128, y: Block128): Block128` | 调用 `gf128Mul` 并返回 `Block128`。 |

## 最小调用示例

```cp
import { bitToMask64 } from "universal-hash";

pub fn main() {
    let result = bitToMask64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
