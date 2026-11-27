[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# bech32

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bech32` ([https://github.com/rust-bitcoin/rust-bech32](https://github.com/rust-bitcoin/rust-bech32))

Zero-allocation BIP-173 Bech32 and BIP-350 Bech32m encoder, decoder, 8-to-5 bit converter, and BCH checksum verifier for CP — clean-room MIT parity with Rust `bech32` and npm `bech32`.

零堆分配的 CP BIP-173 / BIP-350 校验和编解码引擎，支持 Bech32（常量 `1`）与 Bech32m（常量 `0x2bc830a3`）、8-bit ↔ 5-bit 严格位流转换与 6 字符 BCH 校验和防篡改校验，广泛用于 Age 密钥（`age1...`）、Nostr（`npub1...`）、Bitcoin SegWit/Taproot 与 Cosmos 地址（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add bech32
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Bech32Variant` | `pub enum { Bech32, Bech32m }` | BIP-173 / BIP-350 变体枚举 |
| `Bech32Decoded` | `pub struct { valid, variant, hrp, data }` | 解码结果结构体 |
| `to5BitWords` / `from5BitWords` | 8-bit ↔ 5-bit 转换 | 严格校验补位 `< 5` 位且全零 |
| `encodeBech32` | `(hrp, payload_bytes, variant, out) -> string?` | 编码为小写 `<hrp>1<data><checksum>` |
| `decodeBech32` | `(encoded, hrp_out, data_out) -> Bech32Decoded` | 自动识别 Bech32/Bech32m，拒绝混合大小写与任意单字符篡改 |
| `verifyBech32` | `(encoded: string) -> bool` | 快速校验结构与 6 字符 BCH 校验和 |

## Usage / 使用示例

```cp
import { Bech32Variant, encodeBech32, decodeBech32 } from "bech32";

let enc_buf: [128]u8 = undefined;
let hrp_buf: [32]u8 = undefined;
let data_buf: [64]u8 = undefined;

const addr = encodeBech32("npub", "0123456789abcdef", Bech32Variant.Bech32, enc_buf[0..128]).?;
const decoded = decodeBech32(addr, hrp_buf[0..32], data_buf[0..64]);
// decoded.valid == true, decoded.hrp == "npub", decoded.data == "0123456789abcdef"
```

## License

MIT
