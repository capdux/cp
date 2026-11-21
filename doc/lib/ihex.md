# `cap:ihex` — 纯 CP Intel HEX (I8HEX/I16HEX/I32HEX) 嵌入式固件零分配编解码库

## 概述

`cap:ihex` 提供纯 CP、零外部依赖、零堆分配的 Intel HEX（`:LLAAAATTDD...CC`）编解码与 32-bit 扩展地址流式解析器，完整支持 I8HEX、I16HEX（记录类型 02/03）与 I32HEX（记录类型 04/05）：

- **严格单行解析**：校验 `:` 起始符、偶数十六进制长度、精确字节数 `11 + 2*LL`、二补码校验和与规范类型约束；
- **32-bit 绝对地址流式迭代**：`IhexIter` 自动维护 `upper_linear_base`（类型 04 `val << 16`）与 `segment_base`（类型 02 `val << 4`），产出精确 32-bit 绝对地址 `IhexChunk` 并捕获入口地址 `start_linear_address`；
- **零分配编码器**：`encodeIhexLine` 输出标准大写 `:LLAAAATT...CC\n`；
- **固件统计汇总**：`summarizeIhex` 统计数据记录数、总字节数、闭区间地址范围 `[min_addr, max_addr]`、EOF 完整性与入口点。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `IhexRecordType` / `ihexRecordTypeFromByte(b)` | 记录类型枚举：`data=0`, `eof=1`, `ext_segment_addr=2`, `start_segment_addr=3`, `ext_linear_addr=4`, `start_linear_addr=5`, `unknown=255` |
| `IhexRecord` | 单条记录：`byte_count`, `address`, `record_type`, `data: [255]u8`, `checksum`；附 `dataByte(idx): u8?` 与 `dataSlice(): []const u8` |
| `ihexChecksum(byte_count, address, rec_type, payload): u8` | 计算 Intel HEX 二补码校验和 `(~sum + 1) & 0xFF` |
| `parseIhexLine(line): IhexRecord?` | 严格解析单条 Intel HEX 记录行（支持 `\r\n` 行尾） |
| `IhexChunk` / `IhexIter` | 32-bit 绝对地址流式迭代器，维护 `upper_linear_base` / `segment_base` / `start_linear_address` / `has_eof` / `had_error` |
| `encodeIhexLine(address, rec_type, payload, out): usize?` | 零分配编码大写 `:LLAAAATT...CC\n` |
| `IhexStats` / `summarizeIhex(text): IhexStats` | 统计 `data_records`, `total_bytes`, `min_addr`, `max_addr`, `has_eof`, `entry_addr` |

## 示例

```cp
import std from "std";
import { IhexRecordType, IhexIter, encodeIhexLine, summarizeIhex } from "cap:ihex";

fn inspectFirmware(hex_text: []const u8): usize {
    const stats = summarizeIhex(hex_text);
    if (!stats.has_eof) return 0;

    let it = IhexIter.init(hex_text);
    let bytes: usize = 0;
    while (chunk := it.next()) {
        * = chunk.abs_address;
        bytes += chunk.record.dataSlice().len;
    }

    let line_buf: [64]u8 = std.mem.zeroes([64]u8);
    const payload: [2]u8 = [0x08, 0x00];
    * = encodeIhexLine(0x0000, IhexRecordType.ext_linear_addr, payload[0..], line_buf[0..]);
    return bytes;
}
```
