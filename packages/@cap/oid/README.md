# @cap/oid

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:const-oid` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))（原库名 `const-oid` → CP 包名 `@cap/oid`）

Zero-allocation ISO/IEC ASN.1 Object Identifier (OID) X.690 DER encoder/decoder and PKI registry for CP — clean-room MIT parity with Rust `const-oid` (690M downloads on crates.io).

纯 CP 零堆分配 ASN.1 对象标识符 (OID) X.690 DER 编解码与 X.509/TLS/PKI 命名查表引擎。

## Install / 安装

```bash
cap add @cap/oid
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Oid` | `struct { valid: bool, arcs: [16]u32, arc_count: usize }` | Fixed-capacity OID arc array (2..16 arcs) |
| `parseOid` | `(dot_str: string) -> Oid` | Parse & validate dot-decimal OID string |
| `formatOid` | `(oid: Oid, out: []u8) -> string` | Format OID into dot-decimal string |
| `oidEquals` | `(a: Oid, b: Oid) -> bool` | Structural equality check |
| `encodeOidDer` | `(oid: Oid, out: []u8) -> string?` | X.690 DER value bytes (Base-128 VLQ) |
| `decodeOidDer` | `(der_bytes: string) -> Oid` | Strict DER decoder (rejects non-canonical `0x80` padding & truncated streams) |
| `oidName` | `(dot_str: string) -> string?` | Built-in PKI/X.509/crypto OID name lookup (20 entries) |

## Usage / 使用示例

```cp
import { parseOid, encodeOidDer, decodeOidDer, oidName } from "@cap/oid";

const oid = parseOid("1.2.840.113549.1.1.11");
const name = oidName("1.2.840.113549.1.1.11"); // "sha256WithRSAEncryption"

let derBuf: [64]u8 = undefined;
const der = encodeOidDer(oid, derBuf[0..64]) ?? "";
const decoded = decodeOidDer(der);
```
