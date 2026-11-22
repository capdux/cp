# `cap:alt_svc` — RFC 7838 HTTP `Alt-Svc` 头部解析与 HTTP/3 发现缓存

> 参考来源：/data/dev/bun/src/http/h3_client/AltSvc.cp | 吸收日期：2026-11-22

## 概述

`cap:alt_svc` 是吸收自 Bun HTTP/3 客户端（`src/http/h3_client/AltSvc.cp`）的纯 CP 零堆分配 RFC 7838 `Alt-Svc` 头部解析器与 TTL 缓存库，用于在 HTTP/1.1 或 HTTP/2 响应中发现同源 HTTP/3 替代服务端口并按生存期（`ma=`）缓存。

## 上游安全策略与行为契约

- **同源主机限制（防跨主机劫持）**：Bun 上游 `AltSvc.parse` 在自动升级 HTTP/3 时，**仅采纳同源主机（`host` 为空，即 `h3=":443"` 形式）的 `h3` 替代服务**，跳过带外部主机名（如 `h3="other.example:443"`）的条目，防止恶意或误配响应将流量诱导至第三方主机。CP 版在 `AltSvcHeader.parse` 中完整保留了多协议与跨主机条目的解析结果（最多 8 条），同时提供 `firstSameHostH3()` 严格对齐 Bun 的安全筛选策略。
- **主动清空指令（`Alt-Svc: clear`）**：收到 `clear`（大小写不敏感）时，`AltSvcHeader.is_clear` 置为 `true`，`AltSvcCache.record` 会立即清除对应 `(origin_host, origin_port)` 的缓存记录。
- **默认 TTL 与端口校验**：依据 RFC 7838 §3.1，未指定 `ma=` 参数时默认生存期为 `DEFAULT_MAX_AGE = 86400` 秒（24 小时）；`port == 0` 的条目视为无效并被忽略；非法 `ma=` 数值回退为默认值。
- **有界缓存与惰性过期清理**：上游 Bun 缓存上限 256 源并在满载时触发 `sweepExpired`；CP 版 `AltSvcCache` 采用固定 64 槽位栈上零分配数组，在 `record` 与 `lookup` 时自动扫描并回收 `expires_at <= now_sec` 的过期槽位，彻底避免堆分配与内存无界增长。

## 导出符号与 API

### 常量

- `DEFAULT_MAX_AGE: u32 = 86400` — RFC 7838 默认缓存新鲜度（24 小时）。

### `AltSvcEntry`

单条解析后的替代服务记录：

| 字段 / 方法 | 类型 / 签名 | 说明 |
| :--- | :--- | :--- |
| `protocol` | `[]const u8` | 协议标识（如 `"h3"`、`"h3-29"`、`"h2"`） |
| `host` | `[]const u8` | 替代主机名（空切片表示与源站同主机） |
| `port` | `u16` | 目标端口（`1..65535`） |
| `max_age` | `u32` | 缓存有效秒数（`ma=`，默认 `86400`） |
| `persist` | `bool` | 是否声明了 `persist=1` |
| `isH3()` | `bool` | 大小写不敏感判断协议是否为 `"h3"` |
| `isSameHost()` | `bool` | 判断 `host.len == 0`（同源主机） |

### `AltSvcHeader`

- 字段：`is_clear: bool`、`entries: [8]AltSvcEntry`、`len: usize`
- `static fn parse(field_value: []const u8): AltSvcHeader` — 解析 RFC 7838 `Alt-Svc` 字段值（支持 `clear`、逗号分隔多条目、引号包裹的 `host:port` 权威、`ma=` 与 `persist=1` 参数）。
- `fn firstSameHostH3(): AltSvcEntry?` — 返回首个同源 `h3` 替代服务条目（严格对齐 Bun 升级策略）。

### `AltSvcCache`

固定 64 源零堆分配 TTL 缓存：

- `static fn init(): AltSvcCache` — 初始化空缓存。
- `fn record(*_, origin_host: []const u8, origin_port: u16, field_value: []const u8, now_sec: i64)` — 解析并写入源站 H3 端口映射；遇到 `clear` 时立即删除已有记录。
- `fn lookup(*_, origin_host: []const u8, origin_port: u16, now_sec: i64): u16?` — 查询未过期的 H3 目标端口，若已过期则自动清理并返回 `null`。
- `fn remove(*_, origin_host: []const u8, origin_port: u16)` — 主动移除指定源站的缓存记录。

## 边界注意事项

1. **主机名长度上限**：`AltSvcCache` 单槽位存储源主机名上限为 128 字节（`origin_host.len == 0 || origin_host.len > 128` 时 `record` 静默忽略）。
2. **条目上限截断**：单条 `Alt-Svc` 头部最多保留前 8 个有效条目，超出的条目被安全忽略而不会溢出错。
3. **显式时钟注入**：`record` 与 `lookup` 均显式接收 `now_sec: i64`，便于单元测试进行确定性 TTL 过期验证。

## 使用示例

```cp
import { AltSvcHeader, AltSvcCache, DEFAULT_MAX_AGE } from "cap:alt_svc";

const hdr = AltSvcHeader.parse("h3=\":443\"; ma=2592000, h3-29=\":443\"; ma=2592000");
if (h3 := hdr.firstSameHostH3()) {
    // h3.port == 443, h3.max_age == 2592000
    * = h3;
}

let cache = AltSvcCache.init();
cache.record("example.com", 443, "h3=\":8443\"; ma=3600", 1_700_000_000);
const port = cache.lookup("example.com", 443, 1_700_000_100); // 8443
* = port;

// 收到 clear 指令后自动失效
cache.record("example.com", 443, "clear", 1_700_000_200);
// cache.lookup("example.com", 443, 1_700_000_201) == null
* = DEFAULT_MAX_AGE;
```
