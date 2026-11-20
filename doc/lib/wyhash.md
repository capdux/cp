# `cap:wyhash` / `cap.wyhash` 标准库参考文档

## 1. 概述

`lib/wyhash/` 提供纯 CP 实现的工业级 Wyhash v4 高速 64-bit 非加密哈希引擎与 WyRand 伪随机数生成器，支持全无状态一次性哈希、任意分块流式增量哈希、Lemire 无偏差范围随机整数生成以及常用字符串/整数哈希辅助函数，零堆分配。

导入方式：

```cp
import { wyhash, wyhash64, WyhashStream, WyRand, hashString, combineHashes } from "cap:wyhash";
// 或动态挂载：
import cap from "cap";
// 使用 cap.wyhash.wyhash(...)
```

---

## 2. 核心类型

- **`WyhashStateless`**：无内部缓冲区的高速 Wyhash 状态机（32-byte 对齐块迭代 + 尾部终结）。
- **`WyhashStream`**（别名 `Wyhash`）：带 32-byte 内部缓冲区的流式增量哈希器，支持任意 1..N 字节分块 `update(chunk)` 与 `final()`，输出与单次 `wyhash(seed, data)` 100% 比特一致。
- **`WyRand`**：基于 Wyhash 混合原语的高速 64-bit 伪随机数生成器，支持 `nextU64`、`nextU32`、Lemire 无偏差 `nextRange`、`nextF64` 与 `fillBytes`。

---

## 3. API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `wyhash` | `fn wyhash(seed: u64, input: string): u64` | 一次性计算 64-bit Wyhash 摘要 |
| `wyhash64` | `fn wyhash64(a: u64, b: u64): u64` | 两个 64-bit 整数的快速混合哈希 |
| `wyhashFinal4` | `fn wyhashFinal4(a: u64, b: u64): u64` | Wyhash final4 混合函数 |
| `wyhashMum` | `fn wyhashMum(a: u64, b: u64): u64` | 128-bit 乘法折叠混合原语 |
| `WyhashStream.init` | `static fn init(seed: u64): WyhashStream` | 初始化流式哈希器 |
| `WyhashStream.update` | `fn update(*_, chunk: string)` | 增量喂入任意长度数据块 |
| `WyhashStream.final` | `fn final(*_): u64` | 终结计算并返回 64-bit 摘要 |
| `WyRand.init` | `static fn init(seed: u64): WyRand` | 初始化伪随机数生成器 |
| `WyRand.nextU64` | `fn nextU64(*_): u64` | 生成下一个 64-bit 伪随机无符号整数 |
| `WyRand.nextU32` | `fn nextU32(*_): u32` | 生成下一个 32-bit 伪随机无符号整数 |
| `WyRand.nextRange` | `fn nextRange(*_, bound: u64): u64` | Lemire 无偏差 `[0, bound)` 范围随机整数 |
| `WyRand.nextF64` | `fn nextF64(*_): f64` | `[0.0, 1.0)` 均匀分布双精度浮点数 |
| `WyRand.fillBytes` | `fn fillBytes(*_, buf: []u8)` | 填充任意长度字节切片 |
| `hashString` | `fn hashString(s: string): u64` | 默认种子 `0` 计算字符串哈希 |
| `hashInt` | `fn hashInt(val: u64): u64` | 64-bit 整数哈希 |
| `combineHashes` | `fn combineHashes(h1: u64, h2: u64): u64` | 合并两个 64-bit 哈希值 |

---

## 4. 使用示例

```cp
import { wyhash, WyhashStream, WyRand, combineHashes } from "cap:wyhash";

fn example(): u64 {
    let h1 = wyhash(42, "hello world");

    let stream = WyhashStream.init(42);
    stream.update("hello");
    stream.update(" ");
    stream.update("world");
    let h2 = stream.final(); // h1 == h2

    let rng = WyRand.init(12345);
    let r = rng.nextRange(100);
    return combineHashes(h1, r);
}
```
