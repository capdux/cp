[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/crc8`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crc8` ([https://github.com/mrhooray/crc-rs](https://github.com/mrhooray/crc-rs))

Zero-heap-allocation CRC-8 checksum library in pure CP, covering six industry-standard polynomials plus custom configurations.

## Supported Standards (RevEng `"123456789"` Check Vectors)

| Standard | Poly | Init | RefIn | RefOut | XorOut | Check |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **SMBus** | `0x07` | `0x00` | `false` | `false` | `0x00` | `0xF4` |
| **Maxim / 1-Wire** | `0x31` | `0x00` | `true` | `true` | `0x00` | `0xA1` |
| **SAE-J1850** | `0x1D` | `0xFF` | `false` | `false` | `0xFF` | `0x4B` |
| **AUTOSAR** | `0x2F` | `0xFF` | `false` | `false` | `0xFF` | `0xDF` |
| **CDMA2000** | `0x9B` | `0xFF` | `false` | `false` | `0x00` | `0xDA` |
| **Bluetooth HEC** | `0xA7` | `0x00` | `true` | `true` | `0x00` | `0x26` |

## API

```cp
import {
    Crc8Hasher,
    crc8Smbus,
    crc8Maxim,
    crc8SaeJ1850,
    crc8Autosar,
    crc8Cdma2000,
    crc8Bluetooth,
} from "@cap/crc8";
```

## License

MIT
