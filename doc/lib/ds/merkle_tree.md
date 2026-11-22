# `cap:merkle_tree` — 密码学 SHA-256 Merkle Tree 标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 概述

`cap:merkle_tree`（支持 `import { ... } from "cap:ds"` 与 `cap.merkle_tree` 动态挂载）提供纯 CP 实现的工业级 SHA-256 Merkle Tree 与零信任包容性证明（Inclusion Proof）验证能力。

核心安全设计：
- **域分离（Domain Separation）**：叶子节点使用 `SHA256(0x00 || data)`，内部节点使用 `SHA256(0x01 || left || right)`，彻底防止第二原像攻击（Second-Preimage Attack）。
- **奇数叶子兼容**：当某一层节点数为奇数时，自动复制最后一个节点配对，保证任意正整数叶子数均能构建完整 Merkle Tree。
- **确定性内存安全**：所有树节点与证明步骤均通过显式 `Allocator` 分配与 `deinit` 释放，无隐藏全局状态。

## 核心类型

- **`ProofStep`**：单个证明步，包含 `{ hash: [32]u8, is_left: bool }`。
- **`MerkleProof`**：包容性证明，包含 `{ leaf_index: usize, leaf_count: usize, steps: []ProofStep }`，提供 `deinit(alloc)` 释放步骤切片。
- **`MerkleTree`**：SHA-256 Merkle Tree 实例，内部按层级存储节点哈希。

## API 速查

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `hashLeaf` | `hashLeaf(data: []const u8): [32]u8` | 计算域分离叶子哈希 `SHA256(0x00 \|\| data)` |
| `hashInternal` | `hashInternal(left: [32]u8, right: [32]u8): [32]u8` | 计算域分离内部节点哈希 `SHA256(0x01 \|\| left \|\| right)` |
| `MerkleTree.init` | `init(alloc, leaves: []const []const u8): !MerkleTree` | 构建 Merkle Tree（支持空树、单叶、任意数量叶子） |
| `MerkleTree.deinit` | `deinit(&self)` | 释放树内全部分配内存 |
| `MerkleTree.rootHash` | `rootHash(self): [32]u8` | 返回 32 字节根哈希（空树返回全零 `[32]u8`） |
| `MerkleTree.leafCount` | `leafCount(self): usize` | 返回叶子节点数量 |
| `MerkleTree.generateProof` | `generateProof(self, alloc, index: usize): !MerkleProof` | 为指定索引生成包容性证明，越界返回 `error.IndexOutOfBounds` |
| `verifyProof` | `verifyProof(root_hash: [32]u8, leaf_data: []const u8, proof: MerkleProof): bool` | 无状态验证叶子与证明是否匹配给定根哈希 |

## 示例

```cp
import { MerkleTree, verifyProof } from "cap:ds";
const std = @import("std");

pub fn example(alloc: std.mem.Allocator): !void {
    const leaves = [_][]const u8{ "tx1", "tx2", "tx3", "tx4", "tx5" };
    let tree = try MerkleTree.init(alloc, &leaves);
    defer tree.deinit();

    const root = tree.rootHash();
    let proof = try tree.generateProof(alloc, 2);
    defer proof.deinit(alloc);

    const ok = verifyProof(root, "tx3", proof);
    _ = ok;
}
```
