[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rs_merkle

Zero-allocation RFC 6962 domain-separated SHA-256 Merkle tree and inclusion proof verifier (Rust rs_merkle / merkletreejs parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rs_merkle` ([https://github.com/antouhou/rs-merkle](https://github.com/antouhou/rs-merkle))（原库名 `rs_merkle` → CP 包名 `rs_merkle`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rs_merkle` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rs_merkle
```

```cp
import {
    Hash32,
    Sha256State,
    MerkleProof,
    MerkleTree,
    hexLower,
    zeroHash,
    add32,
    pow2_64,
} from "rs_merkle";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Hash32` | `class` | 字段: `bytes: [32]u8` · 方法: `equals, toHex` |
| `Sha256State` | `class` | 字段: `h: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64` |
| `MerkleProof` | `class` | 字段: `siblings: [MAX_PROOF_DEPTH]Hash32, is_right: [MAX_PROOF_DEPTH]bool, depth: usize` |
| `MerkleTree` | `class` | 字段: `leaves: [MAX_LEAVES]Hash32, count: usize` · 方法: `init, addLeaf, leafCount, root, generateProof` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hexLower` | `(nibble: u8): u8` | 调用 `hexLower` 并返回 `u8`。 |
| `zeroHash` | `(): Hash32` | 调用 `zeroHash` 并返回 `Hash32`。 |
| `add32` | `(a: u32, b: u32): u32` | 写入或更新状态数据。 |
| `pow2_64` | `(n: u32): u64` | 调用 `pow2_64` 并返回 `u64`。 |
| `rotr32` | `(x: u32, n: u32): u32` | 调用 `rotr32` 并返回 `u32`。 |
| `shr32` | `(x: u32, n: u32): u32` | 调用 `shr32` 并返回 `u32`。 |
| `bitNot32` | `(x: u32): u32` | 调用 `bitNot32` 并返回 `u32`。 |
| `sha256K` | `(i: usize): u32` | 调用 `sha256K` 并返回 `u32`。 |
| `sha256Init` | `(): Sha256State` | 调用 `sha256Init` 并返回 `Sha256State`。 |
| `sha256Compress` | `(st: &mut Sha256State)` | 就地执行 `sha256Compress` 操作。 |
| `sha256UpdateByte` | `(st: &mut Sha256State, byte: u8)` | 就地执行 `sha256UpdateByte` 操作。 |
| `sha256UpdateSlice` | `(st: &mut Sha256State, data: string)` | 就地执行 `sha256UpdateSlice` 操作。 |
| `sha256Finalize` | `(st: &mut Sha256State): Hash32` | 调用 `sha256Finalize` 并返回 `Hash32`。 |
| `sha256` | `(data: string): Hash32` | 调用 `sha256` 并返回 `Hash32`。 |
| `hashLeaf` | `(data: string): Hash32` | 执行 `hashLeaf` 条件判定，返回 `bool`。 |
| `hashNode` | `(left: Hash32, right: Hash32): Hash32` | 执行 `hashNode` 条件判定，返回 `bool`。 |
| `emptyProof` | `(): MerkleProof` | 调用 `emptyProof` 并返回 `MerkleProof`。 |
| `verifyProof` | `(leaf_data: string, proof: MerkleProof, expected_root: Hash32): bool` | 校验输入合法性，返回 `bool`。 |

## 最小调用示例

```cp
import { hashLeaf } from "rs_merkle";

pub fn main() {
    let result = hashLeaf("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
