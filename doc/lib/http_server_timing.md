# cap:http_server_timing

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（W3C Server-Timing 规范），提供纯 CP、零外部依赖、零堆分配的 `Server-Timing` 响应头解析器与构造器。

## 导入方式

```cp
import {
    ServerTimingMetric,
    ServerTimingIter,
    findServerTiming,
    appendServerTiming,
} from "cap:http_server_timing";
```

## 核心类型与 API

- `ServerTimingMetric`：`name: []const u8`、`dur_us: u64?`（纯整数微秒，如 `dur=53.2` -> `53200`）、`desc: ([]const u8)?`。
- `ServerTimingIter`：`init(header)` / `next(*_): ServerTimingMetric?`（支持引号内逗号/分号与转义，参数大小写不敏感，重复参数首现优先）。
- `findServerTiming(header, name): ServerTimingMetric?`：精确按名称查找指标。
- `appendServerTiming(out, pos: *usize, name, dur_us, desc): bool`：自动格式化 `dur=ms.xxx`、对含空格/逗号/引号的 `desc` 自动加引号与转义，缓冲区不足时原子回滚 `pos` 并返回 `false`。

## 使用示例

```cp
import { appendServerTiming, findServerTiming } from "cap:http_server_timing";

fn example() {
    let buf: [256]u8 = undefined;
    let pos: usize = 0;
    * = appendServerTiming(&buf, &pos, "db", 53200, "SQL, read");
    * = appendServerTiming(&buf, &pos, "cache", null, "Hit");
    * = findServerTiming(buf[0..pos], "db");
}
```
