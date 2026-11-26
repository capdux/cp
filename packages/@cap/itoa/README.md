# @cap/itoa

零堆分配的 CP 原生高速整数转字符串库，Clean-Room MIT 实现对标 crates.io Top 35 顶流包 `itoa`（17 亿+ 下载）。

## 特性

- 200 字节两位查表法（2 digits/div），将十进制除法次数减半
- 支持 `formatU64` / `formatI64`（安全处理 `i64::MIN = -9223372036854775808` 无溢出）、`formatU32` / `formatI32` / `formatUsize`
- 附带 `formatHexU64`（大小写）与 `formatRadixU64`（2..36 任意进制）

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`dtolnay/itoa`](https://github.com/dtolnay/itoa)。
