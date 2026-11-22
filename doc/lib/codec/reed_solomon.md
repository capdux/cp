# `cap:reed_solomon` — GF(2^8) Cauchy Reed-Solomon 纠删码标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

## 1. 概述
`cap:reed_solomon`（动态挂载 `cap.reed_solomon`）是纯 CP 实现的系统性 Cauchy Reed-Solomon 纠删码库（支持 `data_shards + parity_shards <= 256`），采用 GF(2^8) 有限域与高斯-约旦求逆，可从任意缺失 `<= parity_shards` 个数据或校验分片中无损恢复全部原始数据。

## 2. 核心类型
- `RsError`：`InvalidShardCount`、`ShardCountMismatch`、`EmptyShard`、`ShardSizeMismatch`、`TooFewShards`、`SingularMatrix`、`OutOfMemory`
- `ReedSolomon`

## 3. API 速查
- `gfAdd` / `gfMul` / `gfPow` / `gfInv` / `gfDiv`
- `ReedSolomon.init(alloc, data_shards, parity_shards)` / `deinit()`
- `rs.encode(shards)` / `rs.verify(alloc, shards)` / `rs.reconstruct(alloc, shards, present)`

## 4. 示例
```cp
import { ReedSolomon } from "cap:codec";
```
