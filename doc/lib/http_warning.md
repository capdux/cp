# cap:http_warning

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 7234 §5.5 `Warning` 头部规范），提供纯 CP、零外部依赖、零堆分配的 `Warning` 响应头解析与格式化库。

## 导入方式

```cp
import { WarningItem, WarningIter, isStaleWarning, formatWarning } from "cap:http_warning";
```

## 核心类型与 API

- `WarningItem`：`code: u16`、`agent: []const u8`、`text: []const u8`、`date: ([]const u8)?`。
- `WarningIter`：`init(header)` / `next(*_): WarningItem?`（支持引号内逗号与 `\"` 转义，正确解析带逗号的 HTTP-date）。
- `isStaleWarning(code: u16): bool`：识别 110 / 111 / 112 / 113 缓存陈旧类告警码。
- `formatWarning(code, agent, text, date, out): ([]const u8)?`：规范化输出 `<code> <agent> "<text>"[ "<date>"]`。

## 使用示例

```cp
import { formatWarning, WarningIter, isStaleWarning } from "cap:http_warning";

fn example() {
    let buf: [128]u8 = undefined;
    if (hdr := formatWarning(110, "edge.cp.dev", "Response is Stale", "Wed, 21 Oct 2015 07:28:00 GMT", &buf)) {
        let it = WarningIter.init(hdr);
        if (item := it.next()) {
            * = isStaleWarning(item.code); // true
        }
    }
}
```
