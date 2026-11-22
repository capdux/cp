# CP 标准库：ASN.1 DER 编解码与 X.509 v3 证书解析手册 (`cap:asn1`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:auth`，导入请使用 `import { ... } from "cap:auth"`。

## 一、 架构总览与设计准则

`cap:asn1` 是 CP 语言官方内置的纯 CP ITU-T X.690 ASN.1 DER 编解码与 X.509 v3 证书内省标准库，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，既可通过 `import { ... } from "cap:auth"` 导入，也支持 `import cap from "cap"; cap.asn1.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    CP 用户层 ASN.1 DER & X.509 引擎 (cap:asn1)           │
├──────────────────────────────────┬──────────────────────────────────────┤
│ DER TLV & OID 编解码 (der.cp)     │ X.509 v3 证书内省 (x509.cp)           │
│ DerReader (零拷贝 + 规范长度校验)  │ X509Inspector.parseDer (Arena 零泄漏) │
│ DerWriter (最短定长 DER 构建器)    │ SAN DNS / BasicConstraints / 有效期  │
│ OidCodec (点分 OID <-> VLQ + 别名) │ Sha256 指纹 & UTCTime/GeneralizedTime │
└──────────────────────────────────┴──────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/asn1/der.cp` | `// Source: cp/lib/asn1/der.cp (ITU-T X.690 ASN.1 DER Encoder, Decoder & OID Codec)` | `Asn1Error`、标准 DER Tag 常量、`DerElement`、`DerReader`、`DerWriter`、`OidCodec` |
| `lib/asn1/x509.cp` | `// Source: cp/lib/asn1/x509.cp (Zero-C-Dep X.509 v3 Certificate Inspector)` | `X509CertInfo`、`X509Inspector`（DER 证书解析、时间转换、SHA-256 指纹） |
| `lib/asn1/index.cp` | `// Source: cp/lib/asn1/index.cp (Unified Exports for cap:asn1)` | `cap:asn1` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与 Tag 常量（`der.cp`）

- **`Asn1Error`**：`UnexpectedEof`、`InvalidLength`、`NonCanonicalLength`、`TagMismatch`、`InvalidOid`、`InvalidTime`、`MaxDepthExceeded`、`MalformedCert`、`OutOfMemory`。
- **常用 DER Tag 常量**：`TAG_BOOLEAN` (`0x01`)、`TAG_INTEGER` (`0x02`)、`TAG_BIT_STRING` (`0x03`)、`TAG_OCTET_STRING` (`0x04`)、`TAG_NULL` (`0x05`)、`TAG_OID` (`0x06`)、`TAG_UTF8_STRING` (`0x0C`)、`TAG_PRINTABLE_STRING` (`0x13`)、`TAG_T61_STRING` (`0x14`)、`TAG_IA5_STRING` (`0x16`)、`TAG_UTC_TIME` (`0x17`)、`TAG_GENERALIZED_TIME` (`0x18`)、`TAG_BMP_STRING` (`0x1E`)、`TAG_SEQUENCE` (`0x30`)、`TAG_SET` (`0x31`)。
- **`ASN1_MAX_DEPTH = 64`**：嵌套容器最大深度保护阈值。

### 2. 零拷贝 `DerElement` 与 `DerReader`（`der.cp`）

- **`DerElement`**：`tag: u8`、`content: []const u8`、`raw: []const u8`，提供 `reader()`、`asBoolean()`、`asIntegerBytes()`、`asUint64()`、`asOctetString()`、`asBitString()`、`asBitStringUnusedBits()`、`asString()`。
- **`DerReader`**：
  - 严格执行 X.690 DER 规范：拒绝不定长编码 `0x80`、保留长度 `0xFF`、非最短多字节长度（如 `<128` 却使用 `0x81` 或含前导 `0x00`）与越界截断 TLV；
  - 常用读取方法：`readElement()`、`expectTag(tag)`、`readSequence()`、`readSet()`、`readContextExplicit(ctx_num)`、`tryReadContextExplicit(ctx_num)`、`readBoolean()`、`readIntegerBytes()`、`readUint64()`、`readOctetString()`、`readBitString()`、`readNull()`、`readOidBytes()`、`readOid(alloc)`、`readString()`、`readAnyStringAlloc(alloc)`、`skip()`。

### 3. 规范构建器 `DerWriter` 与 `OidCodec`（`der.cp`）

- **`DerWriter`**：`init(alloc)` / `deinit()` / `reset()` / `asSlice()` / `toOwnedSlice()`，支持 `writeTlv`、`writeBoolean`、`writeIntegerBytes`（自动剔除冗余前导 `0x00` 并按需补充符号保护字节 `0x00`）、`writeUint64`、`writeOctetString`、`writeBitString`、`writeUtf8String`、`writePrintableString`、`writeIa5String`、`writeUtcTime`、`writeGeneralizedTime`、`writeNull`、`writeOid`、`writeSequence`、`writeSet`、`writeContextExplicit`、`writeContextImplicit`。
- **`OidCodec`**：
  - `encode(alloc, dotted)` / `decode(alloc, bytes)`：严格校验首弧 `0..2`、次弧边界、Base-128 规范无冗余前导 `0x80`；
  - `knownName(dotted)`：内置映射常见 RSA/ECDSA/Ed25519 算法、X.500 DN 属性（`CN`/`O`/`OU`/`C`/`L`/`ST`）与 X.509 v3 扩展（`subjectAltName`/`basicConstraints`/`keyUsage`/`extKeyUsage` 等）。

### 4. X.509 v3 证书内省（`x509.cp`）

- **`X509CertInfo`**：
  - 字段：`version: u8`（`1`/`2`/`3`）、`serial_hex`、`sig_alg_oid`、`issuer_cn`、`issuer_org`、`subject_cn`、`subject_org`、`not_before_sec: i64`、`not_after_sec: i64`、`pub_key_alg_oid`、`san_dns: []string`、`is_ca: bool`、`sha256_fingerprint_hex: [64]u8`；
  - 方法：`isValidAt(now_sec): bool`、`fingerprintSlice(): []const u8`（需要可变绑定 `let info`）、`fingerprintHex(): [64]u8`、`deinit()`（内部 `ArenaAllocator` $O(1)$ 整块释放）。
- **`X509Inspector`**：
  - `parseDer(alloc, der_bytes): X509CertInfo!`：解析完整 DER `Certificate`，提取元数据、SAN dNSName 列表、BasicConstraints `cA` 并基于 `std.crypto.hash.sha2.Sha256` 计算 64 位小写 SHA-256 指纹；
  - `parseTimeToUnixSec(tag, bytes): i64!`：纯整数历法解析 `UTCTime`（`YYMMDDHHMMSSZ`，RFC 5280 50 年滑窗）与 `GeneralizedTime`（`YYYYMMDDHHMMSSZ`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { DerWriter, OidCodec, X509Inspector } from "cap:auth";
import cap from "cap";

pub fn demoAsn1AndX509(): ! {
    const alloc = std.heap.page_allocator;

    // 1. OID 编解码与别名映射
    const oid_der = OidCodec.encode(alloc, "1.2.840.113549.1.1.11")?;
    defer alloc.free(oid_der);
    const alg_name = OidCodec.knownName("1.2.840.113549.1.1.11").?; // "sha256WithRSAEncryption"

    // 2. 通过 cap.asn1 动态挂载解析 DER 证书
    let w = cap.asn1.DerWriter.init(alloc);
    defer w.deinit();
    w.writeUtf8String("cp-lang.org")?;
    * = alg_name;
}
```
