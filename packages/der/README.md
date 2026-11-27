[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`der`](https://github.com/RustCrypto/formats)（`MIT OR Apache-2.0`）（原库名 `der`，CP 包名 `der`）。

# `der`

Zero-allocation ITU-T X.690 ASN.1 DER encoder and strict decoder in pure CP (RustCrypto `der` parity). Reuses `@cap/const-oid` for canonical OID TLV (`0x06`) handling.

## API

### Universal Tags
`TAG_BOOLEAN` (`0x01`), `TAG_INTEGER` (`0x02`), `TAG_BIT_STRING` (`0x03`), `TAG_OCTET_STRING` (`0x04`), `TAG_NULL` (`0x05`), `TAG_OID` (`0x06`), `TAG_UTF8_STRING` (`0x0C`), `TAG_PRINTABLE_STRING` (`0x13`), `TAG_IA5_STRING` (`0x16`), `TAG_UTC_TIME` (`0x17`), `TAG_GENERALIZED_TIME` (`0x18`), `TAG_SEQUENCE` (`0x30`), `TAG_SET` (`0x31`).

### `DerReader` (Zero-Copy Cursor)
- `DerReader.new(buf: []const u8): DerReader`
- `remaining()`, `isEmpty()`, `peekTag()`
- `readHeader(*_): DerHeader` — Strict minimal definite length decoding
- `readTlv(*_): DerTlvResult`
- `readBool(*_): DerBoolResult` — Strictly accepts only `0x00` / `0xFF`
- `readU64(*_): DerU64Result` — Strictly rejects redundant leading `0x00`/`0xFF` and negative values
- `readOctetString(*_): DerSliceResult`
- `readBitString(*_): DerBitStringResult` — Validates `unused_bits <= 7` and zeroed trailing unused bits
- `readNull(*_): bool`
- `readOid(*_): OidParseResult` — Delegates to `@cap/const-oid` (`ObjectIdentifier.fromDer`)
- `readSequence(*_): DerSequenceResult` — Returns a nested zero-copy `DerReader`

### `DerWriter` (Fixed-Slice Writer)
- `DerWriter.new(buf: []u8): DerWriter`
- `DerWriter.encodedLengthSize(len: usize): usize`
- `DerWriter.encodeLength(len: usize, out: []u8): usize`
- `writeTlv`, `writeBool`, `writeU64`, `writeOctetString`, `writeBitString`, `writeNull`, `writeOid` (via `@cap/const-oid` `toDerTlv`), `writeSequence`
