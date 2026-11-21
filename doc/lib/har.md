# `cap:har` — W3C HTTP Archive (`.har`) 1.2 零分配解析与性能统计库

`cap:har` 提供纯 CP、零堆分配、无外部依赖的 W3C HAR 1.2 浏览器网络抓包日志解析与性能统计能力，通过零拷贝 JSON 扫描内核安全跳过嵌套 `headers`/`cookies`/`content.text`（含转义字符），精准提取 `log`/`creator`/`browser`/`entries`/`request`/`response`/`timings`。

## 导入方式

```cp
import {
    HarHeader,
    HarTimings,
    HarEntry,
    HarStats,
    HarEntryIter,
    isHar,
    parseHarHeader,
    summarizeHar,
} from "cap:har";
```

## 数据结构

### `HarHeader`
- `version: []const u8`（如 `"1.2"`）
- `creator_name`, `creator_version: []const u8`
- `browser_name`, `browser_version: []const u8`（可选）
- `entries_slice: []const u8`（零拷贝指向 `"entries": [...]` 数组切片）

### `HarTimings`
- `blocked_ms`, `dns_ms`, `connect_ms`, `ssl_ms`, `send_ms`, `wait_ms` (TTFB), `receive_ms: f64`（HAR 规范中 `-1.0` 表示未发生）

### `HarEntry`
- `started_date_time: []const u8`, `time_ms: f64`, `server_ip: []const u8`
- `method`, `url`, `http_version: []const u8`, `req_body_size: i64`
- `status: u16`, `status_text`, `mime_type: []const u8`, `resp_body_size`, `content_size: i64`
- `timings: HarTimings`
- 方法 `isSuccess(): bool`（`200..299`）、`isError(): bool`（`>= 400 || == 0`）

### `HarStats`
- `entry_count`, `success_count`, `redirect_count`, `client_error_count`, `server_error_count: u32`
- `total_transfer_bytes: u64`
- `total_time_ms`, `max_time_ms`, `avg_ttfb_ms: f64`

## 核心函数与迭代器

- `isHar(data: []const u8): bool` — 校验顶层 JSON 对象含 `"log"` 且内含 `"version"` 或 `"entries"`
- `parseHarHeader(data: []const u8): HarHeader?` — 提取 `log` 头部元数据与 `entries_slice`
- `HarEntryIter.init(entries_slice_or_har)` / `.next()` — 零分配迭代全部网络请求条目
- `summarizeHar(data: []const u8): HarStats` — 单遍统计状态码分布、传输字节与 TTFB/耗时指标

## 快速示例

```cp
import { parseHarHeader, HarEntryIter, summarizeHar } from "cap:har";

fn inspectTrace(har_json: []const u8) {
    if (hdr := parseHarHeader(har_json)) {
        let it = HarEntryIter.init(hdr.entries_slice);
        while (entry := it.next()) {
            if (entry.isError()) {
                * = entry.url;
            }
        }
    }
    const stats = summarizeHar(har_json);
    * = stats.avg_ttfb_ms;
}
```
