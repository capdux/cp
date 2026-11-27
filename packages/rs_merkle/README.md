[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# rs_merkle

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rs_merkle` ([https://github.com/antouhou/rs-merkle](https://github.com/antouhou/rs-merkle))（原库名 `rs_merkle` → CP 包名 `rs_merkle`）

Zero-allocation RFC 6962 domain-separated SHA-256 Merkle tree and inclusion proof verifier for CP — clean-room MIT parity with Rust `rs_merkle` and npm `merkletreejs`.

纯 CP 零堆分配默克尔哈希树与审计证明引擎，内置标准 FIPS 180-4 SHA-256，严格采用 RFC 6962 域分离前缀（`0x00 || leaf` / `0x01 || left || right`）防御二次原像攻击，适用于 `capdux.org` 透明日志、分布式状态同步与数据防篡改。

## Install / 安装

```bash
cap add rs_merkle
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Hash32` | `struct { bytes: [32]u8 }` | 256-bit digest with constant-time `equals(other)` and `toHex(out)` |
| `MerkleProof` | `struct { siblings: [8]Hash32, is_right: [8]bool, depth: usize }` | Inclusion proof path (up to 64 leaves) |
| `MerkleTree` | `init()`, `addLeaf(*_, data) -> bool`, `leafCount() -> usize`, `root() -> Hash32?`, `generateProof(idx) -> MerkleProof?` | Zero-allocation Merkle tree |
| `verifyProof` | `(leaf_data: string, proof: MerkleProof, expected_root: Hash32) -> bool` | Independent O(log N) inclusion verification |
| `sha256` / `hashLeaf` / `hashNode` | `-> Hash32` | Built-in FIPS 180-4 SHA-256 and RFC 6962 domain-separated helpers |

## Usage / 使用示例

```cp
import { MerkleTree, verifyProof } from "rs_merkle";

let tree = MerkleTree.init();
* = tree.addLeaf("pkg-a@1.0.0");
* = tree.addLeaf("pkg-b@1.0.0");
* = tree.addLeaf("pkg-c@1.0.0");

const root = tree.root();
const proof = tree.generateProof(1 as usize);
// verifyProof("pkg-b@1.0.0", proof, root) == true
```
