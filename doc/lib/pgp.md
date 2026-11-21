# `cap:pgp` — 零分配 OpenPGP RFC 4880/9580 Armor & Packet 解析标准库

`cap:pgp` 是纯 CP、零堆分配、零外部依赖的 OpenPGP（RFC 4880 / RFC 9580）ASCII Armor（含 Radix-64 CRC-24 强制校验）与二进制 Old/New Format Packet 帧解析库。

---

## 快速导入

```typescript
import {
    PGP_CRC24_INIT,
    PGP_CRC24_POLY,
    TAG_SIGNATURE,
    TAG_PUBLIC_KEY,
    TAG_LITERAL_DATA,
    TAG_USER_ID,
    PgpArmorHeader,
    PgpPacket,
    PgpPacketIter,
    PgpPublicKeyInfo,
    PgpSignatureInfo,
    PgpLiteralData,
    pgpCrc24,
    parsePgpArmor,
    decodePgpArmor,
    parsePgpPublicKey,
    parsePgpSignature,
    parsePgpLiteralData,
} from "cap:pgp";
```

---

## 核心 API

- **CRC-24**：`pgpCrc24(data: []const u8): u32`（`PGP_CRC24_INIT = 0xB704CE`, `PGP_CRC24_POLY = 0x1864CFB`）
- **ASCII Armor**：`parsePgpArmor(text): PgpArmorHeader?`、`decodePgpArmor(text, out: *[]u8): usize?`（剥离头尾与空行并强制验证 `=xxxx` CRC-24）
- **二进制包流式迭代**：`PgpPacketIter.init(data)` / `next(*_): PgpPacket?`（同时支持 Old Format 1B/2B/4B/Indeterminate 与 New Format `<192`/`192..223`/`255` 长度编码）
- **高频 Packet 快捷解析**：
  - `parsePgpPublicKey(body): PgpPublicKeyInfo?`（v4 公钥创建时间戳与算法）
  - `parsePgpSignature(body): PgpSignatureInfo?`（v4 签名类型、算法与 Hashed/Unhashed Subpackets）
  - `parsePgpLiteralData(body): PgpLiteralData?`（Tag 11 格式、文件名、时间戳与负载）
