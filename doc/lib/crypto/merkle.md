# CP 标准库：RFC 6962 Merkle 哈希树引擎手册 (`cap:merkle`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

## 一、 架构总览与设计准则

`cap:merkle` 是 CP 语言官方内置的纯 CP **RFC 6962 Merkle 哈希树引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于 `cap:crypto` 与 `std.crypto.hash.sha2.Sha256` 为分布式文件同步、不可变审计日志（配合 `cap:audit`）及轻客户端密码学验证提供工业级哈希树实现。支持 `import { ... } from "cap:crypto"` 与 `import cap from "cap"; cap.merkle.*` 动态挂载调用。

### 1. 域分离前缀与防二阶原像攻击

为彻底防御二阶原像攻击（Second-Preimage Attack），所有节点计算严格遵循 RFC 6962 域分离规范：
- **叶子节点**：`SHA256(0x00 || data)`
- **内部节点**：`SHA256(0x01 || left_hash || right_hash)`
- **非 2 次幂叶子数**：逐层向上配对合并；当某层节点数为奇数时，末尾节点直接提升（Promote）至上一层，不与自身重复配对，保证任意数量（`1, 2, 3, 5, 7, 17, 100...`）叶子下的树结构与根哈希完全确定性。

### 2. 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/merkle/merkle.cp` | `// Source: cp/lib/merkle/merkle.cp (RFC 6962 Merkle Hash Tree Engine)` | `MerkleError`、`ProofSide`、`ProofStep`、`MerkleProof`、`MerkleTree`、`hashLeaf`、`hashNode`、`verifyProof`、`diffTrees` |
| `lib/merkle/index.cp` | `// Source: cp/lib/merkle/index.cp (Unified Exports for cap:merkle)` | `cap:merkle` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与基础类型

- **`MerkleError`**：
  - `EmptyTree`：传入空数据切片构建树。
  - `IndexOutOfBounds`：`generateProof` 请求的叶子索引越界。
  - `InvalidProof`：无效证明错误。
  - `OutOfMemory`：内存分配失败。
- **`ProofSide`**：`Left` / `Right`（指示兄弟节点位于当前路径节点的左侧或右侧）。
- **`ProofStep`**：`sibling_hash: [32]u8`、`sibling_side: ProofSide`。
- **`MerkleProof`**：`leaf_index: usize`、`leaf_count: usize`、`leaf_hash: [32]u8`、`steps: []ProofStep`，提供 `deinit()` 释放内部步骤切片。

### 2. `MerkleTree` 核心方法

- **`MerkleTree.build(alloc, items: []const []const u8): MerkleTree ! MerkleError`**：构建不可变 Merkle 树；空切片返回 `MerkleError.EmptyTree`。
- **`deinit()`**：释放所有层级的堆内存。
- **`rootHash(): [32]u8`**：返回 32 字节原始根哈希。
- **`rootHex(): [64]u8`**：返回 64 字节小写十六进制根哈希。
- **`leafCount(): usize`**：返回叶子节点数量。
- **`generateProof(alloc, leaf_index: usize): MerkleProof ! MerkleError`**：生成 $O(\log N)$ 包含证明；越界返回 `MerkleError.IndexOutOfBounds`。
- **`MerkleTree.verifyProof(root_hash: [32]u8, leaf_data: []const u8, proof: &MerkleProof): bool`**：无树状态纯计算验证；校验叶子哈希、每步 `sibling_side` 拓扑一致性及最终根哈希。
- **`MerkleTree.diffTrees(alloc, a: &MerkleTree, b: &MerkleTree): []usize ! MerkleError`**：自顶向下比对节点哈希，跳过整棵哈希一致的子树，以 $O(k \log N)$ 复杂度快速定位所有内容变化、新增或被截断的叶子索引（返回升序切片，调用方使用 `alloc.free(diffs)` 释放）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { MerkleTree } from "cap:crypto";
import cap from "cap";

pub fn demoMerkle(): ! {
    const alloc = std.heap.page_allocator;
    const chunks: [3][]const u8 = ["block-0", "block-1", "block-2"];
    let tree = MerkleTree.build(alloc, &chunks)?;
    defer tree.deinit();

    const root = tree.rootHash();
    let proof = tree.generateProof(alloc, 1)?;
    defer proof.deinit();

    const valid = cap.merkle.MerkleTree.verifyProof(root, "block-1", &proof);
    * = valid;
}
```
