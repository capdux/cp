# cap:proc_net_route

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux `/proc/net/route` 路由表解析器），提供纯 CP、零外部依赖、零堆分配的 Linux 内核路由表流式解析与默认网关查找库。

## 导入方式

```cp
import {
    RTF_UP,
    RTF_GATEWAY,
    RTF_HOST,
    parseHexIpv4Le,
    RouteEntry,
    ProcRouteIter,
    findDefaultGateway,
} from "cap:proc_net_route";
```

## 核心常量、类型与 API

### 路由标志位常量

- `RTF_UP: u32 = 0x0001`（路由启用）
- `RTF_GATEWAY: u32 = 0x0002`（下一跳为网关）
- `RTF_HOST: u32 = 0x0004`（主机路由）

### `parseHexIpv4Le`

- `fn parseHexIpv4Le(hex8: []const u8): ([4]u8)?`
- 解析 `/proc/net/route` 中 8 位十六进制小端主机字节序 IPv4 地址（如 `"0100A8C0"` → `[192, 168, 0, 1]`），非法长度或非十六进制字符返回 `null`。

### `RouteEntry`

```cp
pub struct RouteEntry {
    iface: []const u8;
    destination: [4]u8;
    gateway: [4]u8;
    flags: u32;
    metric: u32;
    mask: [4]u8;
    mtu: u32;

    fn isUp(): bool;
    fn isGateway(): bool;
    fn isDefaultRoute(): bool;
    fn prefixLen(): u8;
}
```

### `ProcRouteIter` 与 `findDefaultGateway`

| API | 签名 | 说明 |
| --- | --- | --- |
| `ProcRouteIter.init` | `fn init(content: []const u8): ProcRouteIter` | 初始化零分配路由表迭代器 |
| `ProcRouteIter.next` | `fn next(*_): RouteEntry?` | 自动跳过表头 `Iface\tDestination...`、空行与畸形行，解析标准 11 列 |
| `findDefaultGateway` | `fn findDefaultGateway(content: []const u8): RouteEntry?` | 扫描路由表并返回 `metric` 最小的活跃默认网关路由条目 |

## 使用示例

```cp
import { findDefaultGateway } from "cap:proc_net_route";

fn example(proc_route_text: []const u8) {
    if (gw := findDefaultGateway(proc_route_text)) {
        * = gw.iface;   // "eth0"
        * = gw.gateway; // [192, 168, 0, 1]
    }
}
```
