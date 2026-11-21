# `cap:crc` — 全谱系 CRC-8 / CRC-16 / CRC-32 / CRC-32C / CRC-64 校验和标准库

## 1. 概述
`cap:crc`（动态挂载 `cap.crc`）是纯 CP 零堆分配实现的多标准 CRC 校验和标准库，覆盖 SMBUS、CCITT-FALSE、XMODEM、MODBUS、IEEE 802.3 (`crc32Combine` O(log N) 合并)、Castagnoli iSCSI CRC-32C、XZ 与 ECMA-182 CRC-64。

## 2. 核心类型
- `Crc32Stream`（`initIeee` / `initCastagnoli`）
- `Crc64Stream`（`initXz` / `initEcma182`）

## 3. API 速查
- `crc8Smbus`、`crc16CcittFalse`、`crc16Xmodem`、`crc16Modbus`
- `crc32Ieee`、`crc32Castagnoli`、`crc32Combine(crc1, crc2, len2)`
- `crc64Xz`、`crc64Ecma182`

## 4. 示例
```cp
import { crc32Ieee, crc32Castagnoli, crc32Combine } from "cap:crc";
```
