[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rs_merkle

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6962 domain-separated SHA-256 Merkle tree and inclusion proof verifier (Rust rs_merkle / merkletreejs parity) for CP

## 安装与导入

```bash
cap add rs_merkle
```

```cp
import { sha256 } from "rs_merkle";
```

## 核心导出 API

- `export fn sha256(data: string): Hash32;`
- `export fn hashLeaf(data: string): Hash32;`
- `export fn hashNode(left: Hash32, right: Hash32): Hash32;`
- `export fn verifyProof(leaf_data: string, proof: MerkleProof, expected_root: Hash32): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rs_merkle@1.0.0`)，许可证：MIT
