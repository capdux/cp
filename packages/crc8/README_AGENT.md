# crc8@1.0.0 (Agent Quickref)

> Zero-allocation CRC-8 checksum library (SMBus, Maxim 1-Wire, SAE-J1850, AUTOSAR, CDMA2000, Bluetooth HEC) in pure CP | License: MIT
> Import: `import { ... } from "crc8";` | Full types: `src/index.d.cp`

## Functions
- `export fn reflect8(b: u8): u8;`
- `export fn crc8Smbus(data: &[]u8): u8;`
- `export fn crc8Maxim(data: &[]u8): u8;`
- `export fn crc8SaeJ1850(data: &[]u8): u8;`
- `export fn crc8Autosar(data: &[]u8): u8;`
- `export fn crc8Cdma2000(data: &[]u8): u8;`
- `export fn crc8Bluetooth(data: &[]u8): u8;`
