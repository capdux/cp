# cap:http_alt_svc

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 7838 HTTP `Alt-Svc` 备选服务 / HTTP/3 QUIC 发现规范），提供纯 CP、零外部依赖、零堆分配的 `Alt-Svc` 头解析与格式化库。

## 导入方式

```cp
import {
    AltSvcEntry,
    isAltSvcClear,
    AltSvcIter,
    formatAltSvcEntry,
} from "cap:http_alt_svc";
```

## 核心类型与 API

- `AltSvcEntry`：`protocol_id: []const u8`（如 `"h3"`、`"h3-29"`、`"h2"`）、`host: []const u8`（空表示同源主机，IPv6 保留方括号如 `"[::1]"`）、`port: u16`、`max_age: u64`（默认 RFC 7838 `86400`）、`persist: bool`（`persist=1`）：
  - `isHttp3(): bool`：匹配 `"h3"` 或 `"h3-*"`。
- `isAltSvcClear(header: []const u8): bool`：大小写不敏感匹配 `"clear"`。
- `AltSvcIter`：`init(header: []const u8)` / `next(*_): AltSvcEntry?`（解析多条目 `Alt-Svc` 头，支持 IPv6 `[::1]:443`、引号感知与 `ma`/`persist` 参数，拦截端口 `0` / `>65535` 与缺失端口）。
- `formatAltSvcEntry(entry: AltSvcEntry, out: []u8): ([]const u8)?`：规范化输出 RFC 7838 `Alt-Svc` 条目（缓冲不足或字段非法返回 `null`）。

## 使用示例

```cp
import { AltSvcIter, formatAltSvcEntry } from "cap:http_alt_svc";

fn example() {
    let it = AltSvcIter.init("h3=\":443\"; ma=2592000; persist=1, h2=\"alt.example.com:8443\"");
    while (entry := it.next()) {
        if (entry.isHttp3()) {
            let buf: [128]u8 = undefined;
            * = formatAltSvcEntry(entry, &buf);
        }
    }
}
```
