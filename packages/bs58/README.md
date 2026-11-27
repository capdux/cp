[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/bs58

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bs58` ([https://github.com/Nullus157/bs58-rs](https://github.com/Nullus157/bs58-rs))

零堆分配的 CP Base58 多字母表编解码引擎 — 洁净室 MIT 实现，对标 Rust `bs58` 与 npm `bs58`，完整支持 Bitcoin、Ripple、Flickr 三套标准字母表，广泛用于 Solana、Bitcoin、IPFS/CID 与分布式节点 ID。

Zero-allocation Base58 multi-alphabet encoder/decoder (`Bitcoin`, `Ripple`, `Flickr`) for CP — clean-room MIT parity with Rust `bs58` and npm `bs58`.

## 安装 / Install

```bash
cap add @cap/bs58
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `Base58Alphabet` | 枚举：`Bitcoin` / `Ripple` / `Flickr` |
| `encodeBase58(input, out)` | 使用 Bitcoin 默认字母表编码 |
| `decodeBase58(encoded, out)` | 使用 Bitcoin 默认字母表解码 |
| `encodeBase58With(input, alpha, out)` | 使用指定字母表编码 |
| `decodeBase58With(encoded, alpha, out)` | 使用指定字母表解码 |
| `isValidBase58(encoded, alpha)` | 校验字符串是否全部属于指定 Base58 字母表 |

## 使用示例 / Usage

```cp
import { encodeBase58, decodeBase58, Base58Alphabet, encodeBase58With } from "@cap/bs58";

let buf: [128]u8 = undefined;
const enc = encodeBase58("Hello World!", buf[0..128]).?; // "2NEpo7TZRRrLZSi2U"

let dec_buf: [128]u8 = undefined;
const raw = decodeBase58(enc, dec_buf[0..128]).?;        // "Hello World!"
```

## License

MIT
