# `cap:shamir` — GF(2^8) Shamir 门限密钥分片标准库

## 1. 概述
`cap:shamir`（动态挂载 `cap.shamir`）是纯 CP 实现的信息论安全 `(k, n)` Shamir 门限密钥分片（Shamir's Secret Sharing）标准库。基于 `std.crypto.random` CSPRNG 生成随机多项式系数并在 GF(2^8) 上进行拉格朗日插值恢复，任意 `k` 份分片可无损还原原始密钥，少于 `k` 份无法泄露任何比特信息。

## 2. 核心类型
- `ShamirError`：`EmptySecret`、`InvalidThreshold`、`InsufficientShares`、`DuplicateShareIndex`、`InvalidShareFormat`、`ShareLengthMismatch`、`InvalidShareIndex`、`OutOfMemory`
- `Share`（含 `index`, `threshold`, `data`, `encode`/`decode`/`deinit`）

## 3. API 速查
- `splitSecret(alloc, secret, threshold, total_shares)` -> `[]Share`
- `combineShares(alloc, shares)` -> `[]u8`
- `freeShares(alloc, shares)`

## 4. 示例
```cp
import { splitSecret, combineShares, freeShares } from "cap:shamir";
```
