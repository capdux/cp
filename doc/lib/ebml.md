# `cap:ebml` — 纯 CP RFC 8794 EBML / Matroska MKV / WebM 零分配解析库

## 概述

`cap:ebml` 提供纯 CP、零外部依赖、零堆分配的 RFC 8794 EBML（Extensible Binary Meta Language，Matroska MKV 与 WebM 底层容器格式）变长 VINT Element ID / Data Size 解码器、大端无符号整数解码器与可递归进入 Master Element 的流式元素迭代器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `EBML_ID_HEADER` / `EBML_ID_VERSION` / `EBML_ID_DOCTYPE` / `EBML_ID_SEGMENT` | 常用标准 Element ID 常量（`0x1A45DFA3`, `0x4286`, `0x4282`, `0x18538067`） |
| `EbmlVint` | VINT 解码结果：`value: u64`, `raw_id: u32`, `bytes_read: u8`, `is_unknown_size: bool` |
| `EbmlElement` | 零拷贝元素：`id: u32`, `size: u64`, `is_unknown_size: bool`, `payload: []const u8` |
| `decodeEbmlId(data)` | RFC 8794 §5：解码 1–4 字节 Element ID，拒绝首字节为 `0`、超过 4 字节及全 0 / 全 1 保留 ID |
| `decodeEbmlSize(data)` | RFC 8794 §6：解码 1–8 字节 VINT 数据长度，精准识别全 1 `is_unknown_size` |
| `isEbmlHeader(data)` | 4 字节魔数判定标准 EBML 头 `0x1A45DFA3` |
| `EbmlElementIter` | 流式元素迭代器：`init(data)`, `next(*_): EbmlElement?`；支持将 Master Element 的 `payload` 递归传入新迭代器，严格防越界 |
| `decodeEbmlUint(payload)` | RFC 8794 §7.2：解码 0–8 字节大端无符号整数（0 字节返回 `0`，>8 字节返回 `null`） |

## 示例

```cp
import { isEbmlHeader, EbmlElementIter, EBML_ID_HEADER, EBML_ID_DOCTYPE } from "cap:ebml";

fn extractDocType(webm: []const u8): ([]const u8)? {
    if (!isEbmlHeader(webm)) return null;
    let top = EbmlElementIter.init(webm);
    const hdr = top.next() ?? return null;
    if (hdr.id != EBML_ID_HEADER) return null;
    let inner = EbmlElementIter.init(hdr.payload);
    while (el := inner.next()) {
        if (el.id == EBML_ID_DOCTYPE) return el.payload;
    }
    return null;
}
```
