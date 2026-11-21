# `cap:pcapng` — 纯 CP PCAPng 下一代网络抓包容器零分配解析库

## 概述

`cap:pcapng` 提供纯 CP、零外部依赖、零堆分配的 PCAPng 抓包容器解析器，支持自动识别 Little-Endian / Big-Endian Section Header Block（SHB）、严格首尾长度与 4 字节对齐校验的通用 Block 流式迭代、SHB/IDB/EPB 核心块解码、64-bit 时间戳组合以及 4 字节对齐 TLV Options 迭代器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `BLOCK_SHB` / `BLOCK_IDB` / `BLOCK_SPB` / `BLOCK_NRB` / `BLOCK_ISB` / `BLOCK_EPB` / `BYTE_ORDER_MAGIC` | 标准 PCAPng 块类型与字节序魔数常量 |
| `isPcapng(data)` | 校验首块为 SHB (`0x0A0D0D0A`) 且匹配 LE 或 BE `0x1A2B3C4D` |
| `PcapngBlock` / `PcapngBlockIter` | 通用 Block 零拷贝视图与流式迭代器（自动切换大小端、校验首尾长度与 4B 对齐） |
| `ShbInfo` / `parseShb(body, is_be)` | 解析 SHB 主次版本号与 `section_length: i64` |
| `IdbInfo` / `parseIdb(body, is_be)` | 解析 IDB `link_type: u16` 与 `snap_len: u32` |
| `EpbPacket` / `parseEpb(body, is_be)` | 解析 EPB 报文元数据与零拷贝 `packet_data`，提供 `timestamp64()` |
| `PcapngOption` / `PcapngOptionIter` | 4 字节对齐 TLV Option 迭代器（遇 `code == 0` 终止） |

## 示例

```cp
import { isPcapng, PcapngBlockIter, BLOCK_EPB, parseEpb } from "cap:pcapng";

fn countPackets(capture: []const u8): usize {
    if (!isPcapng(capture)) return 0;
    let it = PcapngBlockIter.init(capture);
    let count: usize = 0;
    while (blk := it.next()) {
        if (blk.block_type == BLOCK_EPB) {
            if (parseEpb(blk.body, blk.is_big_endian) != null) count += 1;
        }
    }
    return count;
}
```
