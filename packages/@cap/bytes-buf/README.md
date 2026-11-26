# @cap/bytes-buf

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bytes` ([https://github.com/tokio-rs/bytes](https://github.com/tokio-rs/bytes))（原库名 `bytes` → CP 包名 `@cap/bytes-buf`）

零堆分配的 CP 原生字节缓冲读写游标库，Clean-Room MIT 实现对标 crates.io Top 25 顶流包 `bytes`（18 亿+ 下载）。

## 特性

- `BytesReader`（对标 `bytes::Buf`）：零拷贝游标、大端/小端 `u8`/`u16`/`u32`/`u64` 读取、`advance`、`copyToSlice`
- `BytesMut`（对标 `bytes::BytesMut`/`BufMut`）：固定栈 512 字节缓冲、大端/小端写入、`putSlice`、`splitTo`、`truncate`、`clear`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`tokio-rs/bytes`](https://github.com/tokio-rs/bytes)。
