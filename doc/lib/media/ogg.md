# `cap:ogg` — 纯 CP RFC 3533 Ogg 音频/多媒体容器零分配分页解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

## 概述

`cap:ogg` 提供纯 CP、零外部依赖、零堆分配的 RFC 3533 Ogg 容器（Ogg Vorbis / Ogg Opus / Ogg FLAC）分页解析器，支持 `"OggS"` 魔数与版本探测、27 字节分页头与段表零拷贝视图解码、BOS/EOS/Continuation 标志判断以及标准多项式 `0x04C11DB7` 非反射 CRC-32 完整性验证。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `OggPage` | 零拷贝 Ogg 分页：`version`, `header_type`, `granule_position`, `serial_number`, `page_sequence`, `checksum`, `page_segments`, `segment_table`, `payload`, `raw_page`；只读方法：`isContinuation()`, `isBos()`, `isEos()`, `verifyChecksum()` |
| `oggCrc32(page_bytes)` | RFC 3533 非反射 CRC-32（`0x04C11DB7`，自动将偏移 `22..26` 按 0 字节计算） |
| `isOgg(data)` | 校验 `"OggS"` 魔数与 `version == 0` |
| `OggPageIter` | 流式分页迭代器：`init(data)`、`next(*_): OggPage?`（自动校验边界并推进流偏移） |

## 示例

```cp
import { isOgg, OggPageIter } from "cap:media";

fn verifyAllPages(stream: []const u8): bool {
    if (!isOgg(stream)) return false;
    let it = OggPageIter.init(stream);
    let count: usize = 0;
    while (page := it.next()) {
        if (!page.verifyChecksum()) return false;
        count += 1;
    }
    return count > 0;
}
```
