# @cap/hashbrown

零堆分配的 CP 原生 SwissTable 风格控制字节哈希表与集合，Clean-Room MIT 实现对标 crates.io **全站第 1 名**包 `hashbrown`（32.6 亿+ 下载），跨包复用 `@cap/fnv-hash`。

## 特性

- 1 字节 H2 控制标签数组（`0x80` 空槽 / `0xFE` 墓碑 / `0x00..0x7F` H2 哈希指纹）高速缓存友好探测
- `HashMapU64`（128 槽、75% 负载上限 96 项）：O(1) `insert`/`get`/`containsKey`/`remove`/`clear`
- `HashSetU64`：基于 `HashMapU64` 的零重复集合封装

## 致谢

Clean-Room MIT 实现，架构灵感对标开源生态 [`rust-lang/hashbrown`](https://github.com/rust-lang/hashbrown)。
