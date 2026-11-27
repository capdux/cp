[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/crc16`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crc16` ([https://github.com/mrhooray/crc-rs](https://github.com/mrhooray/crc-rs))

Zero-heap industrial CRC-16 checksum engine in pure CP supporting CCITT-FALSE, XMODEM, MODBUS, KERMIT, ARC/IBM, and USB standards, plus custom polynomial configuration and streaming updates.

零堆分配纯 CP 工业级 CRC-16 校验和引擎，覆盖 CCITT-FALSE、XMODEM、MODBUS、KERMIT、ARC/IBM 与 USB 六大标准，支持自定义多项式与流式分片更新。

## Installation

```bash
cap add @cap/crc16
```

## Supported Standards (`"123456789"` Check Vectors)

| Function | Standard | Check Value |
| --- | --- | :---: |
| `crc16CcittFalse(data)` | CRC-16/IBM-3740 (CCITT-FALSE) | `0x29B1` |
| `crc16Xmodem(data)` | CRC-16/XMODEM | `0x31C3` |
| `crc16Modbus(data)` | CRC-16/MODBUS | `0x4B37` |
| `crc16Kermit(data)` | CRC-16/KERMIT | `0x2189` |
| `crc16Arc(data)` | CRC-16/ARC (IBM) | `0xBB3D` |
| `crc16Usb(data)` | CRC-16/USB | `0xB4C8` |

## License

MIT
