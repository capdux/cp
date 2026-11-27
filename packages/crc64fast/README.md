# @cap/crc64

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crc64fast` ([https://github.com/srijs/rust-crc64fast](https://github.com/srijs/rust-crc64fast))（原库名 `crc64fast` → CP 包名 `@cap/crc64`）

零堆分配的 CP 64 位循环冗余校验引擎 — 洁净室 MIT 实现，完整支持 **ECMA-182**、**CRC-64/XZ** 与 **CRC-64/GO-ISO** 三大工业标准，采用 256 项查找表加速，广泛用于对象存储（S3/OSS）、Redis RDB、XZ 压缩与 NVMe 数据完整性校验。

Zero-allocation 64-bit CRC checksum engine (ECMA-182, XZ, GO-ISO) for CP — clean-room MIT parity with Rust `crc` and `crc64fast`.

## 安装 / Install

```bash
cap add @cap/crc64
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description | `"123456789"` 标准校验值 |
| --- | --- | --- |
| `crc64Ecma(data)` / `Crc64Hasher.initEcma()` | ECMA-182 标准 | `0x6C40DF5F0B497347` |
| `crc64Xz(data)` / `Crc64Hasher.initXz()` | CRC-64/XZ 标准 | `0x995DC9BBDF1939FA` |
| `crc64Iso(data)` / `Crc64Hasher.initIso()` | CRC-64/GO-ISO 标准 | `0xB90956C775A41001` |
| `Crc64Hasher` | 流式校验器：`update(*_, data)`, `finalize()`, `reset(*_)` | — |

## 使用示例 / Usage

```cp
import { crc64Ecma, crc64Xz, crc64Iso, Crc64Hasher } from "@cap/crc64";

const sum = crc64Xz("123456789"); // 0x995DC9BBDF1939FA

let h = Crc64Hasher.initEcma();
h.update("1234");
h.update("56789");
const streamed = h.finalize();    // 0x6C40DF5F0B497347
```

## License

MIT
