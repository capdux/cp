# `cap:lucene` — 零分配 Apache Lucene `CodecUtil` 索引文件解析库

> **模块路径**：`lib/lucene/`（导入方式：`import { ... } from "cap:lucene";`）  
> **特性**：100% 纯 CP、零堆分配、完整遵循 Apache Lucene `CodecUtil` 大端头/尾规范与 `DataInput` VInt/VLong/ZigZag 编解码。

---

## 一、 核心常量

| 常量名 | 值 | 说明 |
| :--- | :--- | :--- |
| `LUCENE_CODEC_MAGIC` | `0x3FD76C17` | 4 字节大端头部魔数 |
| `LUCENE_FOOTER_MAGIC` | `0xC02893E8` | 4 字节大端尾部魔数（`~LUCENE_CODEC_MAGIC`） |
| `LUCENE_FOOTER_LENGTH` | `16` | 尾部固定字节数（4B magic + 4B algorithmID=0 + 8B CRC32） |
| `LUCENE_ID_LENGTH` | `16` | 段对象唯一 ID 长度（16 字节） |

---

## 二、 核心 API

- `readLuceneVInt(data, pos): LuceneVInt?` — 最多 5 字节 u32 变长整数解码
- `readLuceneVLong(data, pos): LuceneVLong?` — 最多 9 字节非负 u63 变长整数解码
- `luceneZigZagDecode32(v: u32): i32` / `luceneZigZagDecode64(v: u64): i64`
- `readLuceneString(data, pos): LuceneString?` — VInt 长度前缀零拷贝字符串读取
- `parseLuceneHeader(data): LuceneCodecHeader?` — 解析基础 CodecHeader（magic + codec_name + version）
- `parseLuceneIndexHeader(data): LuceneIndexHeader?` — 解析 5.0+ 标准 IndexHeader（含 16B `object_id` 与 ASCII `object_suffix`）
- `parseLuceneFooter(data): LuceneFooter?` — 校验末尾 16B Footer（`algorithm_id == 0` 且 `checksum >> 32 == 0`）
- `isLuceneFile(data): bool` — 同时校验头部、尾部及首尾不重叠合法性

---

## 三、 使用示例

```typescript
import {
    isLuceneFile,
    parseLuceneIndexHeader,
    parseLuceneFooter,
} from "cap:lucene";

fn inspectLuceneSegment(bytes: []const u8) {
    if (!isLuceneFile(bytes)) return;
    if (hdr := parseLuceneIndexHeader(bytes)) {
        * = hdr.codec_name.len;
        * = hdr.version;
    }
    if (ft := parseLuceneFooter(bytes)) {
        * = ft.crc32;
    }
}
```
