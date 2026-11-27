[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/bitvec

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bitvec` ([https://github.com/ferrilab/bitvec](https://github.com/ferrilab/bitvec))

零堆分配的 CP 原生定长位向量库（支持最多 512 位），Clean-Room MIT 实现对标 crates.io Top 210 包 `bitvec`（6 亿+ 下载）。

## 特性

- 8 × `u64` 紧凑存储，O(1) 位读写 `get`/`set`/`toggle`/`push`/`pop`
- 字级位运算 `andWith`/`orWith`/`xorWith`/`notAll`、快速 `countOnes`/`countZeros`/`any`/`all`/`firstOne`/`lastOne` 与二进制字符串格式化 `toBinStr`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`ferrilab/bitvec`](https://github.com/ferrilab/bitvec)。
