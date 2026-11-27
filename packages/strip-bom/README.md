[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# strip-bom

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strip-bom` ([https://github.com/sindresorhus/strip-bom](https://github.com/sindresorhus/strip-bom)) / `npm:strip-bom-buf` ([https://github.com/sindresorhus/strip-bom-buf](https://github.com/sindresorhus/strip-bom-buf))

Pure CP, zero-allocation Byte Order Mark (BOM) detector, zero-copy stripper, and idempotent UTF-8 BOM prepender (parity with npm `strip-bom` & `strip-bom-buf`).

## Supported BOMs

| Constant | Encoding | Bytes |
| --- | --- | --- |
| `BOM_UTF32_LE` (4) | UTF-32 Little Endian | `FF FE 00 00` |
| `BOM_UTF32_BE` (5) | UTF-32 Big Endian | `00 00 FE FF` |
| `BOM_UTF8` (1) | UTF-8 | `EF BB BF` |
| `BOM_UTF16_LE` (2) | UTF-16 Little Endian | `FF FE` |
| `BOM_UTF16_BE` (3) | UTF-16 Big Endian | `FE FF` |
| `BOM_NONE` (0) | None | — |

## API

- `detectBom(data: []u8): BomInfo`
- `hasUtf8Bom(s: string): bool`
- `stripUtf8Bom(s: string): string`
- `stripAnyBomOffset(data: []u8): usize`
- `prependUtf8Bom(s: string, out: []u8): usize`

## License

MIT
