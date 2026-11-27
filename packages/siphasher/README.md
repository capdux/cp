[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`siphasher`](https://github.com/jedisct1/rust-siphash)（`MIT OR Apache-2.0`）（原库名 `siphasher`，CP 包名 `@cap/siphash`）。

# @cap/siphash

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed streaming PRF/hashers (HashDoS-resistant) for CP — clean-room MIT parity with Rust `siphasher` and the Aumasson-Bernstein SipHash specification.

零堆分配的 CP 防 HashDoS 带密钥流式哈希引擎，完整实现经典 SipHash-2-4 与 Rust `std::collections::HashMap` 默认高速变体 SipHash-1-3，比特级通过全部官方论文测试向量（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/siphash
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `SipHasher24` | `init(k0, k1)`, `write(*_, data)`, `finish() -> u64` | SipHash-2-4 流式哈希器（2 压缩轮 + 4 终结轮） |
| `siphash24` | `(k0: u64, k1: u64, data: string) -> u64` | SipHash-2-4 单次快捷哈希 |
| `SipHasher13` | `init(k0, k1)`, `write(*_, data)`, `finish() -> u64` | SipHash-1-3 流式哈希器（1 压缩轮 + 3 终结轮） |
| `siphash13` | `(k0: u64, k1: u64, data: string) -> u64` | SipHash-1-3 单次快捷哈希 |

## Usage / 使用示例

```cp
import { siphash24, siphash13, SipHasher24 } from "@cap/siphash";

const k0: u64 = 0x0706050403020100;
const k1: u64 = 0x0f0e0d0c0b0a0908;
const digest = siphash24(k0, k1, "hello");
const fast = siphash13(k0, k1, "hello");
```

## License

MIT
