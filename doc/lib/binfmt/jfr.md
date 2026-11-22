# `cap:jfr` — 零分配 JVM Java Flight Recorder (`.jfr`) 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> **模块路径**：`lib/jfr/`（导入方式：`import { ... } from "cap:binfmt";`）  
> **特性**：100% 纯 CP、零堆分配、完整遵循 JDK Flight Recorder v2.0 规范（68B 大端 Chunk 头、9 字节压缩无符号整数 LEB128 与多 Chunk/事件帧流式迭代）。

---

## 一、 核心常量

| 常量名 | 值 | 说明 |
| :--- | :--- | :--- |
| `JFR_MAGIC` | `"FLR\x00"` | 4 字节 Chunk 魔数 |
| `JFR_CHUNK_HEADER_SIZE` | `68` | 固定大端 Chunk 头字节数 |
| `JFR_EVENT_METADATA` | `0` | Metadata 事件类型 ID |
| `JFR_EVENT_CHECKPOINT` | `1` | Checkpoint（常量池）事件类型 ID |
| `JFR_FLAG_COMPRESSED_INTS` | `0x01` | 压缩整数特性位 |
| `JFR_FLAG_FINAL_CHUNK` | `0x02` | 最终 Chunk 特性位 |

---

## 二、 核心 API

- `isJfr(data)` / `parseJfrChunkHeader(data, offset): JfrChunkHeader?`  
  `JfrChunkHeader` 提供只读方法 `usesCompressedInts()`, `isFinalChunk()`, `durationMs()`。
- `readJfrUleb128(data, pos): JfrVarint?`  
  兼容 JFR 最多 9 字节压缩无符号整数（前 8 字节低 7 位有效，第 9 字节全 8 位有效）。
- `JfrChunkIter.init(data)` / `next(*_): JfrChunk?` — 迭代多 Chunk 拼接的 `.jfr` 文件。
- `JfrEventIter.init(chunk_slice)` / `next(*_): JfrEventFrame?` — 零拷贝迭代 Chunk 偏移 68 起的事件帧。
- `summarizeJfr(data): JfrStats?` — 单遍统计 Chunk 数、事件总数、Metadata/Checkpoint 事件数及总持续毫秒数。

---

## 三、 使用示例

```typescript
import { JfrChunkIter, JfrEventIter, summarizeJfr } from "cap:binfmt";

fn inspectJfr(bytes: []const u8) {
    let chunks = JfrChunkIter.init(bytes);
    while (chunk := chunks.next()) {
        let events = JfrEventIter.init(chunk.slice);
        while (ev := events.next()) {
            * = ev.event_type_id;
        }
    }
    if (stats := summarizeJfr(bytes)) {
        * = stats.event_count;
    }
}
```
