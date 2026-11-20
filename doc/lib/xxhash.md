# cap:xxhash — 工业级 xxHash 与流式分块哈希标准库

## 概述

`cap:xxhash`（亦可通过 `cap.xxhash` 访问）提供纯 CP 实现的工业级非加密哈希算法套件，完整覆盖 **XXH32**、**XXH64**、**XXH3_64bits** 与 **XXH3_128bits** 规范，并提供零堆分配的流式增量哈希状态机 `Xxh32Stream` 与 `Xxh64Stream`。

## 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `Xxh32Stream` | 零堆分配 XXH32 流式哈希上下文（内置 16 字节环形缓冲与 4 路累加器） |
| `Xxh64Stream` | 零堆分配 XXH64 流式哈希上下文（内置 32 字节环形缓冲与 4 路累加器） |
| `Xxh128` | 128 位哈希结果结构体，包含 `low: u64` 与 `high: u64` 字段及 `eql(other: Xxh128): bool` 方法 |
| `XXH3_DEFAULT_SECRET` | 官方标准 192 字节 XXH3 默认密钥常量 (`[192]u8`) |

## API 速查

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `xxh32` | `pub fn xxh32(input: []const u8, seed: u32): u32` | 单次计算 XXH32 哈希 |
| `Xxh32Stream.init` | `pub fn init(seed: u32): Xxh32Stream` | 初始化 XXH32 流式计算器 |
| `Xxh32Stream.reset` | `pub fn reset(*_, seed: u32)` | 重置流式状态并更换种子 |
| `Xxh32Stream.update` | `pub fn update(*_, input: []const u8)` | 增量投喂任意长度分块数据 |
| `Xxh32Stream.digest` | `pub fn digest(): u32` | 输出当前累计哈希值（不破坏内部状态） |
| `xxh64` | `pub fn xxh64(input: []const u8, seed: u64): u64` | 单次计算 XXH64 哈希 |
| `Xxh64Stream.init` | `pub fn init(seed: u64): Xxh64Stream` | 初始化 XXH64 流式计算器 |
| `Xxh64Stream.reset` | `pub fn reset(*_, seed: u64)` | 重置流式状态并更换种子 |
| `Xxh64Stream.update` | `pub fn update(*_, input: []const u8)` | 增量投喂任意长度分块数据 |
| `Xxh64Stream.digest` | `pub fn digest(): u64` | 输出当前累计哈希值（不破坏内部状态） |
| `xxh3_64` | `pub fn xxh3_64(input: []const u8): u64` | 计算 XXH3_64bits（seed = 0） |
| `xxh3_64WithSeed` | `pub fn xxh3_64WithSeed(input: []const u8, seed: u64): u64` | 带自定义种子的 XXH3_64bits |
| `xxh3_128` | `pub fn xxh3_128(input: []const u8): Xxh128` | 计算 XXH3_128bits（seed = 0） |
| `xxh3_128WithSeed` | `pub fn xxh3_128WithSeed(input: []const u8, seed: u64): Xxh128` | 带自定义种子的 XXH3_128bits |

## 示例

```cp
import { xxh32, xxh64, Xxh64Stream, xxh3_64, xxh3_128 } from "cap:xxhash";
import cap from "cap";

fn example() {
    const h32 = xxh32("hello cp", 0);
    const h64 = xxh64("hello cp", 0);

    let stream = Xxh64Stream.init(0);
    stream.update("hello ");
    stream.update("cp");
    const streamed = stream.digest(); // 与 h64 完全一致

    const h3 = xxh3_64("fast hash");
    const h128 = xxh3_128("128-bit hash");
    const via_ns = cap.xxhash.xxh3_64("fast hash");
}
```
