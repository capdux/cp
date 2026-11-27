[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# ulid

纯 CP 实现的零堆分配 **ULID（全局唯一字典序可排序标识符）** 编解码器、校验器、比较器与 80-bit 单调递增器，采用 Crockford Base32 编码。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [crates.io](https://crates.io/crates/ulid) & [npm](https://www.npmjs.com/package/ulid) |
| **原包名与版本** | `ulid`（`ulid-rs` v1 / npm `ulid` v2 / `ulidx` 对齐） |
| **上游仓库** | https://github.com/dylanhart/ulid-rs |
| **CP 包名与版本** | `ulid`（`1.0.0`） |
| **开源协议** | MIT |

在 CP 中保持原包名直接导入，无需改名：

```rust
// Rust (crates.io)
use ulid::Ulid;
```

```typescript
// TypeScript (npm)
import { ulid, decodeTime } from "ulid";

// CP (cap)
import { Ulid, fromParts, timestampMs, encode, decode, isValidUlid, increment, compare } from "ulid";
```

## 安装

```bash
cap add ulid
```

## 核心特性与性能亮点

- **标准 128-bit 布局**：48-bit 大端 Unix 毫秒时间戳 + 80-bit 随机数，紧凑存储于 `Ulid { bytes: [16]u8 }`。
- **零堆分配**：`encode` 直接写入调用方 26 字节缓冲区，`decode` 原地填充 `&mut Ulid`，零动态内存分配。
- **Crockford Base32 规范解码**：大小写不敏感，兼容易混淆字符（`I`/`L` -> `1`，`O` -> `0`），并严格拦截首位大于 `'7'` 的 128-bit 溢出串。
- **同毫秒单调递增**：`increment(u, &mut next)` 对低 80 位随机区加 1，溢出时返回 `false`，保证高并发同毫秒严格有序。

## 快速上手

```typescript
import {
    Ulid,
    fromParts,
    timestampMs,
    encode,
    decode,
    isValidUlid,
    increment,
    compare,
} from "ulid";

fn main(): void {
    const entropy: [10]u8 = [0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x01, 0x02];
    const id = fromParts(1700000000000, entropy);

    let text_buf: [26]u8 = undefined;
    const written = encode(id, text_buf[0..]); // written == 26
    _ = written;

    // 同毫秒单调递增
    let next_id: Ulid = undefined;
    if (increment(id, &mut next_id)) {
        const ord = compare(id, next_id); // ord < 0
        _ = ord;
    }

    // 校验并解码
    const sample = "01ARZ3NDEKTSV4RRFFQ69G5FAV";
    if (isValidUlid(sample)) {
        let parsed: Ulid = undefined;
        if (decode(sample, &mut parsed)) {
            const ts = timestampMs(parsed); // 1469918176385
            _ = ts;
        }
    }
}
```

## API 速览

| 导出符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `Ulid` | `class { bytes: [16]u8 }` | 128-bit 二进制 ULID |
| `fromParts` | `(timestamp_ms: u64, rand_bytes: [10]u8) -> Ulid` | 由毫秒时间戳与 10 字节随机数构造 ULID |
| `timestampMs` | `(u: Ulid) -> u64` | 提取 48-bit Unix 毫秒时间戳 |
| `encode` | `(u: Ulid, out: []u8) -> usize` | 编码为 26 字节 Crockford Base32（缓冲区不足返回 `0`） |
| `decode` | `(s: string, out: &mut Ulid) -> bool` | 解码 26 字符 ULID 到 `out` |
| `isValidUlid` | `(s: string) -> bool` | 校验 ULID 长度、字符集与 128-bit 边界 |
| `increment` | `(u: Ulid, out: &mut Ulid) -> bool` | 80-bit 随机段单调加 1（溢出返回 `false`） |
| `compare` | `(a: Ulid, b: Ulid) -> i32` | 按字节字典序比较（`-1` / `0` / `1`） |

## 开源协议

MIT
