# `cap:clf` — NCSA Common & Apache/Nginx Combined HTTP 访问日志零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:obs`，导入请使用 `import { ... } from "cap:obs"`。

`cap:clf` 提供纯 CP、零堆分配的 NCSA Common Log Format 与 Apache/Nginx Combined Log Format HTTP 访问日志流式解析与流量统计能力，兼容 IPv4/IPv6 主机、带空格与 `\"` 转义引号的请求行/User-Agent、`"-"` 缺省字节数以及 `\r\n`/`\n` 换行。

## 导入方式

```cp
import {
    ClfEntry,
    ClfStats,
    ClfIter,
    isClf,
    parseClfLine,
    summarizeClf,
} from "cap:obs";
```

## 数据结构

### `ClfEntry`
- `host`, `ident`, `user`, `timestamp: []const u8`（零拷贝切片，`timestamp` 已剥离 `[` `]`）
- `request_line`, `method`, `path`, `protocol: []const u8`（自动拆分请求行）
- `status: u16`（3 位 HTTP 状态码 `100..599`）
- `bytes_sent: u64`, `has_bytes: bool`（`"-"` 解析为 `0` 且 `has_bytes = false`）
- `referer`, `user_agent: []const u8`, `is_combined: bool`
- 方法：`is2xx()`, `is3xx()`, `is4xx()`, `is5xx(): bool`

### `ClfStats`
- `total_requests`, `status_2xx`, `status_3xx`, `status_4xx`, `status_5xx: u32`
- `total_bytes_sent: u64`
- `combined_count`, `malformed_lines: u32`

## 核心函数与迭代器

- `isClf(data: []const u8): bool` — 快速检测首个非空行是否为合法 CLF/Combined 日志
- `parseClfLine(raw_line: []const u8): ClfEntry?` — 解析单条访问日志行
- `ClfIter.init(data)` / `.next()` — 逐行零分配迭代（跳过空行，畸形行递增 `skipped_malformed` 并继续）
- `summarizeClf(data): ClfStats` — 单遍汇总流量、状态码分布与畸形行数

## 快速示例

```cp
import { ClfIter, summarizeClf } from "cap:obs";

fn analyzeAccessLog(log_bytes: []const u8) {
    let it = ClfIter.init(log_bytes);
    while (entry := it.next()) {
        if (entry.is5xx()) {
            * = entry.path;
        }
    }
    const stats = summarizeClf(log_bytes);
    * = stats.total_bytes_sent;
}
```
