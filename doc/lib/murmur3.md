# `cap:murmur3` — MurmurHash3 x86_32 & x64_128 零堆非加密哈希标准库

## 1. 概述
`cap:murmur3`（动态挂载 `cap.murmur3`）是纯 CP 零堆分配实现的 MurmurHash3 (`x86_32` 与 `x64_128`) 标准库，支持一次性切片与流式分块增量计算，适用于布隆过滤器、分片路由与 HyperLogLog。

## 2. 核心类型
- `Murmur3_32Stream` / `Murmur3_128Stream`：零堆流式增量哈希器

## 3. API 速查
- `murmur3_32(data, seed)` -> `u32`
- `murmur3_128(data, seed)` -> `[16]u8`
- `Murmur3_32Stream.init(seed)` -> `update(chunk)` -> `final()`
- `Murmur3_128Stream.init(seed)` -> `update(chunk)` -> `final()`

## 4. 示例
```cp
import { murmur3_32, murmur3_128 } from "cap:murmur3";
```
