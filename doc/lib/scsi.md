# `cap:scsi` — SCSI SPC-5 / SBC-4 CDB、INQUIRY、CAPACITY & Sense 零分配编解码库

纯 CP、零堆分配（zero-alloc）的 SCSI SPC-5 / SBC-4 存储协议核心编解码库，支持 6/10/12/16 字节 CDB 长度查表与编码、36B INQUIRY 解析、READ CAPACITY(10/16) 128-bit 大容量计算以及 Fixed/Descriptor Sense Data 解析。

## 导入

```cp
import {
    OP_TEST_UNIT_READY,
    OP_REQUEST_SENSE,
    OP_INQUIRY,
    OP_READ_CAPACITY_10,
    OP_READ_10,
    OP_WRITE_10,
    OP_SYNCHRONIZE_CACHE_10,
    OP_READ_16,
    OP_WRITE_16,
    OP_SERVICE_ACTION_IN_16,
    STATUS_GOOD,
    STATUS_CHECK_CONDITION,
    STATUS_BUSY,
    STATUS_RESERVATION_CONFLICT,
    STATUS_TASK_SET_FULL,
    SK_NO_SENSE,
    SK_RECOVERED_ERROR,
    SK_NOT_READY,
    SK_MEDIUM_ERROR,
    SK_HARDWARE_ERROR,
    SK_ILLEGAL_REQUEST,
    SK_UNIT_ATTENTION,
    SK_DATA_PROTECT,
    SK_ABORTED_COMMAND,
    PDT_DIRECT_ACCESS,
    PDT_SEQUENTIAL_ACCESS,
    PDT_CD_DVD,
    PDT_ENCLOSURE,
    cdbLengthForOpcode,
    ScsiInquiryData,
    parseInquiry,
    ScsiCapacity,
    parseReadCapacity10,
    parseReadCapacity16,
    ScsiSense,
    parseSense,
    encodeInquiryCdb,
    encodeRead10Cdb,
    encodeWrite10Cdb,
    encodeRead16Cdb,
    encodeWrite16Cdb,
} from "cap:scsi";
```

## 核心 API

- **CDB 查表与编码**：`cdbLengthForOpcode(opcode): u8?`、`encodeInquiryCdb`、`encodeRead10Cdb`、`encodeWrite10Cdb`、`encodeRead16Cdb`、`encodeWrite16Cdb`。
- **INQUIRY**：`parseInquiry(data): ScsiInquiryData?`（`vendorSlice()`, `productSlice()`, `revSlice()` 自动剥离尾部空格）。
- **READ CAPACITY(10/16)**：`parseReadCapacity10(data)`、`parseReadCapacity16(data)`（`sectorCount(): u64`, `totalBytes(): u128`）。
- **Sense Data**：`parseSense(data): ScsiSense?`（同时支持 Fixed `0x70/0x71` 与 Descriptor `0x72/0x73`，`isCurrent()`, `isOk()`）。

## 示例

```cp
import std from "std";
import { encodeRead16Cdb, parseReadCapacity16 } from "cap:scsi";

fn example(cap16: []const u8) {
    let cdb: [16]u8 = std.mem.zeroes([16]u8);
    * = encodeRead16Cdb(0x100000000, 128, cdb[0..]);
    if (cap := parseReadCapacity16(cap16)) {
        * = cap.totalBytes();
    }
}
```
