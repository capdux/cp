# @cap/tiny-keccak

Zero-allocation Keccak-f[1600] 24-round sponge implementation providing NIST FIPS 202 `Sha3_256` and original Ethereum `Keccak256` for CP — clean-room MIT parity with Rust `tiny-keccak` v2 (crates.io #160).

纯 CP 零堆分配 Keccak-f[1600] 24 轮海绵置换实现（完整 theta/rho/pi/chi/iota 五步），同时支持标准 NIST FIPS 202 `Sha3_256`（域分隔 `0x06`）与以太坊 `Keccak256`（域分隔 `0x01`）。

## Install / 安装

```bash
cap add @cap/tiny-keccak
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `KECCAK256_DIGEST_LEN` | `usize = 32` | Output digest length in bytes (256 bits) |
| `KECCAK256_RATE` | `usize = 136` | Sponge rate in bytes (1088 bits) |
| `Sha3_256` | `new()`, `update(*_, data)`, `updateStr(*_, s)`, `finalize(*_, out32)`, `finalizeHex(*_, out64)` | NIST FIPS 202 SHA3-256 streaming hasher |
| `Keccak256` | `new()`, `update(*_, data)`, `updateStr(*_, s)`, `finalize(*_, out32)`, `finalizeHex(*_, out64)` | Original Keccak-256 (Ethereum) streaming hasher |
| `sha3_256Hex` | `(s: string, out64: []u8) -> usize` | One-shot lowercase hex SHA3-256 |
| `keccak256Hex` | `(s: string, out64: []u8) -> usize` | One-shot lowercase hex Keccak-256 |

## Usage / 使用示例

```cp
import { Sha3_256, Keccak256, sha3_256Hex, keccak256Hex } from "@cap/tiny-keccak";

let hex: [64]u8 = undefined;
* = sha3_256Hex("abc", hex[0..64]);
* = keccak256Hex("transfer(address,uint256)", hex[0..64]); // starts with "a9059cbb"
```
