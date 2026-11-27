[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# crc8

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CRC-8 checksum library (SMBus, Maxim 1-Wire, SAE-J1850, AUTOSAR, CDMA2000, Bluetooth HEC) in pure CP

## 安装与导入

```bash
cap add crc8
```

```cp
import { reflect8 } from "crc8";
```

## 核心导出 API

- `export fn reflect8(b: u8): u8;`
- `export fn crc8Smbus(data: &[]u8): u8;`
- `export fn crc8Maxim(data: &[]u8): u8;`
- `export fn crc8SaeJ1850(data: &[]u8): u8;`
- `export fn crc8Autosar(data: &[]u8): u8;`
- `export fn crc8Cdma2000(data: &[]u8): u8;`
- `export fn crc8Bluetooth(data: &[]u8): u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`crc8@1.0.0`)，许可证：MIT
