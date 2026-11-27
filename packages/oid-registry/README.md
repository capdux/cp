[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# oid-registry

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:oid-registry` ([https://github.com/rusticata/oid-registry](https://github.com/rusticata/oid-registry))

Zero-allocation static X.509 / PKCS / TLS / NIST / RFC Object Identifier (OID) database with ITU-T X.690 DER encoder and decoder for CP — clean-room parity with Rust `oid-registry` (221M+ downloads on crates.io).

纯 CP 零堆分配 X.509 / PKCS / TLS / NIST / RFC 标准 OID 静态注册表（内置 36 个工业级核心 OID），附带完整的 DER OID 内容字节编解码器，适用于 X.509 证书解析、ASN.1/DER 编解码与密码算法标识映射。

## Install / 安装

```bash
cap add oid-registry
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `OidEntry` | `struct { oid, short_name, description, category }` | Static OID metadata record |
| `lookupOid` | `(dotted: string) -> OidEntry?` | Looks up entry by dotted OID string |
| `lookupShortName` | `(sn: string) -> OidEntry?` | Looks up entry by short name (e.g. `"CN"`, `"Ed25519"`) |
| `oidEntryCount` | `() -> usize` | Total number of built-in OID entries (`36`) |
| `encodeOidDer` | `(dotted: string, out: []u8) -> usize` | Encodes dotted OID into DER content octets |
| `decodeOidDer` | `(der: []const u8, out: []u8) -> usize` | Decodes DER content octets into dotted OID string |

## Usage / 使用示例

```cp
import { lookupOid, lookupShortName, encodeOidDer, decodeOidDer } from "oid-registry";

if (entry := lookupOid("1.3.101.112")) {
    // entry.short_name == "Ed25519"
}
let der: [32]u8 = undefined;
const n = encodeOidDer("1.2.840.113549.1.1.11", der[0..32]);
let dotted: [64]u8 = undefined;
const m = decodeOidDer(der[0..n], dotted[0..64]);
```
