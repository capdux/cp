# cap:http_upgrade

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9110 §7.6.1/§7.8 与 RFC 6455 规范），提供纯 CP、零外部依赖、零堆分配的 `Upgrade` / `Connection` 协议升级、逐跳头识别与 WebSocket 握手判定库。

## 导入方式

```cp
import {
    UpgradeProtocol,
    UpgradeIter,
    ConnectionTokenIter,
    connectionHasToken,
    upgradeHasProtocol,
    isHopByHopHeader,
    isWebSocketUpgrade,
} from "cap:http_upgrade";
```

## 核心类型与 API

- `UpgradeProtocol`：`name: []const u8`、`version: ([]const u8)?`。
- `UpgradeIter`：`init(header)` / `next(*_): UpgradeProtocol?`（解析 `HTTP/2.0, SHTTP/1.3, websocket`）。
- `ConnectionTokenIter`：`init(header)` / `next(*_): ([]const u8)?`。
- `connectionHasToken(connection_header, token): bool` 与 `upgradeHasProtocol(upgrade_header, proto_name): bool`（均大小写不敏感）。
- `isHopByHopHeader(header_name, connection_header): bool`：识别 8 个 RFC 9110 标准逐跳头及 `Connection` 动态声明的自定义逐跳头。
- `isWebSocketUpgrade(connection_header, upgrade_header, ws_version): bool`：校验 WebSocket 升级握手条件与可选版本 `"13"`。

## 使用示例

```cp
import { isWebSocketUpgrade, isHopByHopHeader } from "cap:http_upgrade";

fn example() {
    * = isWebSocketUpgrade("keep-alive, Upgrade", "websocket", "13"); // true
    * = isHopByHopHeader("X-Internal-Hop", "keep-alive, x-internal-hop"); // true
}
```
