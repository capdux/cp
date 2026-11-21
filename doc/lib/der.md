# `cap:der` — 零分配 ITU-T X.690 ASN.1 DER 严格编解码标准库

`cap:der` 是纯 CP、零堆分配的 ASN.1 DER（Distinguished Encoding Rules）严格规范编解码库，面向 X.509 证书、PKCS、CMS、OCSP 与密码学密钥结构解析与构造，严格执行 X.690 唯一规范编码约束（拒绝不定长 `0x80`、非最小长度编码、整型冗余前导 `0x00`/`0xFF`、非规范布尔值与位串非零未用位）。

---

## 快速导入

```typescript
import {
    TAG_BOOLEAN,
    TAG_INTEGER,
    TAG_BIT_STRING,
    TAG_OCTET_STRING,
    TAG_NULL,
    TAG_OID,
    TAG_UTF8_STRING,
    TAG_SEQUENCE,
    DerTlv,
    DerBitString,
    DerReader,
    DerWriter,
    parseDerTlv,
    formatDerOid,
} from "cap:der";
```

---

## Universal Tag 与掩码常量

`TAG_BOOLEAN=0x01`, `TAG_INTEGER=0x02`, `TAG_BIT_STRING=0x03`, `TAG_OCTET_STRING=0x04`, `TAG_NULL=0x05`, `TAG_OID=0x06`, `TAG_UTF8_STRING=0x0C`, `TAG_PRINTABLE_STRING=0x13`, `TAG_IA5_STRING=0x16`, `TAG_UTC_TIME=0x17`, `TAG_GENERALIZED_TIME=0x18`, `TAG_SEQUENCE=0x30`, `TAG_SET=0x31`, `CLASS_MASK=0xC0`, `CONSTRUCTED_MASK=0x20`。

---

## 核心 API

### 1. `DerTlv` & `parseDerTlv(data: []const u8): DerTlv?`
- 返回 `tag: u8`、`is_constructed: bool`、`value: []const u8`、`raw: []const u8`、`total_bytes: usize`。
- 支持单字节与高编号多字节 Tag；严格校验 DER 最小长度编码与缓冲区边界。

### 2. `DerReader`（零拷贝只读游标）
- `DerReader.init(data)`、`remaining()`、`isEmpty()`
- `nextTlv(*_): DerTlv?`
- `readBool(*_): bool?`（仅接受 `0x00` / `0xFF`）
- `readU64(*_): u64?`、`readI64(*_): i64?`（严格拒绝冗余前导填充）
- `readNull(*_): bool`、`readOctetString(*_): ([]const u8)?`、`readOid(*_): ([]const u8)?`
- `readBitString(*_): DerBitString?`（校验 `unused_bits <= 7` 且末字节低 `unused_bits` 位全为 0）
- `readSequence(*_): DerReader?`、`readContextExplicit(*_, ctx_tag_num: u8): DerReader?`

### 3. OID 格式化
- `formatDerOid(oid_bytes: []const u8, out: *[]u8): usize?`：解码 DER OID 字节为点分十进制字符串（严格拒绝 base-128 首字节 `0x80` 非规范填充）。

### 4. `DerWriter`（溢出原子失败写入器）
- `DerWriter.init(buf)`、`written(): []const u8`
- `writeTlv`, `writeBool`, `writeU64`, `writeNull`, `writeOctetString`, `writeUtf8String`, `writeSequenceHeader`

---

## 使用示例

```typescript
import { DerReader, DerWriter, formatDerOid } from "cap:der";

fn roundTripExample(): bool {
    let buf: [64]u8 = [0; 64];
    let w = DerWriter.init(buf[0..64]);
    if (!w.writeU64(65537)) return false;
    if (!w.writeBool(true)) return false;

    let r = DerReader.init(w.written());
    const exp = r.readU64() ?? return false;
    const flag = r.readBool() ?? return false;
    return exp == 65537 && flag;
}
```
