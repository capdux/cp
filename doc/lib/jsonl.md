# cap:jsonl

> 源码吸收自 `/data/dev/bun/src/`（JSONL / NDJSON 流式行处理逻辑），提供纯 CP、零外部依赖、零堆分配的 JSON Lines 流式行迭代、增量分块边界扫描、平衡性校验与帧写入库。

## 导入方式

```cp
import {
    JsonlChunkResult,
    JsonlIter,
    scanCompleteJsonlChunk,
    isBalancedJsonRecord,
    appendJsonlRecord,
} from "cap:jsonl";
```

## 核心类型与 API

- `JsonlChunkResult`：结构体，包含 `consumed_bytes: usize`、`line_count: usize`、`has_incomplete_tail: bool`。
- `JsonlIter`：
  - `init(input: []const u8): JsonlIter`：自动跳过开头 UTF-8 BOM（`0xEF 0xBB 0xBF`）。
  - `next(*_): ([]const u8)?`：按 `\n` / `\r\n` 流式产出非空 JSONL 记录切片，保留原始行内容并自动跳过空白行。
- `scanCompleteJsonlChunk(chunk: []const u8): JsonlChunkResult`：用于网络流/文件流增量拼接，仅统计以 `\n` 结尾的完整非空记录数与已消费字节偏移，识别末尾未完结半行。
- `isBalancedJsonRecord(line: []const u8): bool`：零分配校验单行 JSON 记录的括号/引号嵌套平衡与非空性，拦截未闭合字符串、截断对象/数组、嵌套错配、粘连对象及裸换行符。
- `appendJsonlRecord(record: []const u8, out: []u8, pos: *usize): bool`：校验记录非空且不含裸 `\r`/`\n`，并将 `record + "\n"` 追加至输出缓冲。

## 使用示例

```cp
import {
    JsonlIter,
    scanCompleteJsonlChunk,
    isBalancedJsonRecord,
    appendJsonlRecord,
} from "cap:jsonl";

fn example() {
    const chunk = "{\"id\":1}\n{\"id\":2}\n{\"id\":3";
    const res = scanCompleteJsonlChunk(chunk);
    * = res.line_count; // 2
    * = res.has_incomplete_tail; // true

    let it = JsonlIter.init(chunk[0..res.consumed_bytes]);
    while (line := it.next()) {
        * = isBalancedJsonRecord(line); // true
    }
}
```
