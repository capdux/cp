# crc16@1.0.0 (Agent Quickref)

> Zero-heap industrial CRC-16 checksums (CCITT-FALSE, XMODEM, MODBUS, KERMIT, ARC/IBM, USB) in pure CP | License: MIT
> Import: `import { ... } from "crc16";` | Full types: `src/index.d.cp`

## Functions
- `export fn reflectU8(v: u8): u8;`
- `export fn reflectU16(v: u16): u16;`
- `export fn crc16CcittFalse(data: &[]u8): u16;`
- `export fn crc16Xmodem(data: &[]u8): u16;`
- `export fn crc16Modbus(data: &[]u8): u16;`
- `export fn crc16Kermit(data: &[]u8): u16;`
- `export fn crc16Arc(data: &[]u8): u16;`
- `export fn crc16Usb(data: &[]u8): u16;`
