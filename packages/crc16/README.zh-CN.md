[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# crc16

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap industrial CRC-16 checksums (CCITT-FALSE, XMODEM, MODBUS, KERMIT, ARC/IBM, USB) in pure CP

## 安装与导入

```bash
cap add crc16
```

```cp
import { reflectU8 } from "crc16";
```

## 核心导出 API

- `export fn reflectU8(v: u8): u8;`
- `export fn reflectU16(v: u16): u16;`
- `export fn crc16CcittFalse(data: &[]u8): u16;`
- `export fn crc16Xmodem(data: &[]u8): u16;`
- `export fn crc16Modbus(data: &[]u8): u16;`
- `export fn crc16Kermit(data: &[]u8): u16;`
- `export fn crc16Arc(data: &[]u8): u16;`
- `export fn crc16Usb(data: &[]u8): u16;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`crc16@1.0.0`)，许可证：MIT
