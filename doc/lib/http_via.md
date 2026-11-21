# cap:http_via

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9110 §7.6.2/§7.6.3 规范），提供纯 CP、零外部依赖、零堆分配的 `Via` 代理链解析、环路检测、追加构造与 `Max-Forwards` 控制库。

## 导入方式

```cp
import {
    ViaEntry,
    ViaIter,
    viaHopCount,
    viaContainsProxy,
    appendVia,
    parseMaxForwards,
    decrementMaxForwards,
} from "cap:http_via";
```

## 核心类型与 API

- `ViaEntry`：`protocol_name: ([]const u8)?`（省略默认 `"HTTP"`）、`protocol_version`、`received_by`、`comment: ([]const u8)?` 及 `effectiveProtocolName(): []const u8`。
- `ViaIter`：`init(header)` / `next(*_): ViaEntry?`（安全处理 `(...)` 注释内逗号与嵌套括号）。
- `viaHopCount(header): usize` 与 `viaContainsProxy(header, received_by): bool`（大小写不敏感环路检测）。
- `appendVia(existing, protocol_version, received_by, out): ([]const u8)?`。
- `parseMaxForwards(header): u32?` 与 `decrementMaxForwards(val): u32?`（0 返回 `null` 停止转发）。

## 使用示例

```cp
import { viaContainsProxy, appendVia, parseMaxForwards, decrementMaxForwards } from "cap:http_via";

fn example() {
    const via = "1.1 edge1.cp.dev";
    if (!viaContainsProxy(via, "edge2.cp.dev")) {
        let buf: [128]u8 = undefined;
        * = appendVia(via, "1.1", "edge2.cp.dev", &buf);
    }
    if (mf := parseMaxForwards("3")) {
        * = decrementMaxForwards(mf); // 2
    }
}
```
