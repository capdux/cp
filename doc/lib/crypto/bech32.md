# `cap:bech32` — BIP-173 Bech32 & BIP-350 Bech32m 纠错码标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

## 1. 概述
`cap:bech32`（动态挂载 `cap.bech32`）是纯 CP 实现的 BIP-173 Bech32 与 BIP-350 Bech32m 编解码标准库，支持 5-bit/8-bit `convertBits`、通用字节载荷编解码（如 Nostr `npub`/`nsec`、Age 公钥）与 SegWit v0/v1 地址编码及混合大小写/校验和错误拦截。

## 2. 核心类型
- `Bech32Variant`：`bech32` / `bech32m`
- `Bech32Error`：`InvalidHrp`、`InvalidLength`、`MixedCase`、`MissingSeparator`、`InvalidCharacter`、`InvalidChecksum`、`InvalidPadding`、`InvalidWitnessVersion`、`InvalidWitnessProgram`、`OutOfMemory`
- `DecodedBech32` / `DecodedBytes`

## 3. API 速查
- `encode(alloc, hrp, data5, variant)` / `decode(alloc, str)`
- `convertBits(alloc, data, from_bits, to_bits, pad)`
- `encodeBytes(alloc, hrp, raw_bytes, variant)` / `decodeBytes(alloc, str)`
- `encodeSegwitAddr(alloc, hrp, witness_ver, witness_prog)`

## 4. 示例
```cp
import { encodeBytes, decodeBytes, Bech32Variant } from "cap:crypto";
```
