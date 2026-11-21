# cap:dns_srv_record

> 源码吸收自 `/data/dev/bun/src/dns.zig`（RFC 2782 DNS SRV 服务发现记录规范），提供纯 CP、零外部依赖、零堆分配的 SRV 文本/线缆头解析、`_service._proto.domain` 查询名解析与确定性优先级/权重选择库。

## 导入方式

```cp
import {
    SrvWireHeader,
    SrvRecord,
    SrvServiceName,
    compareSrvOrder,
    selectSrvByRoll,
} from "cap:dns_srv_record";
```

## 核心类型与 API

- `SrvWireHeader`：`priority: u16`, `weight: u16`, `port: u16`, `target_wire: []const u8`。
- `SrvRecord`：`priority: u16`, `weight: u16`, `port: u16`, `target: []const u8`：
  - `isServiceUnavailable(): bool`：RFC 2782 约定 `target == "."` 表示服务不可用。
  - `SrvRecord.parseText(rdata: []const u8): SrvRecord?`：解析 `"priority weight port target"`，严格校验 `u16` 范围与合法目标域名。
  - `SrvRecord.parseWireHeader(rdata: []const u8): SrvWireHeader?`：解析前 6 字节大端三元组与剩余目标域名线缆切片。
- `SrvServiceName`：`service: []const u8`, `proto: []const u8`, `domain: []const u8`：
  - `SrvServiceName.parse(qname: []const u8): SrvServiceName?`：解析 `_http._tcp.example.com.` 等 RFC 2782 服务查询名（`service`/`proto` 去除前导 `_`）。
- `compareSrvOrder(a: SrvRecord, b: SrvRecord): i32`：按 `priority` 升序、同优先级 `weight` 降序比较（返回 `-1`/`0`/`1`）。
- `selectSrvByRoll(records: []const SrvRecord, priority: u16, roll: u32): usize?`：按 RFC 2782 对指定优先级有效记录执行确定性权重落点选择（自动忽略 `target == "."` 不可用项，全零权重均匀轮询）。

## 使用示例

```cp
import { SrvRecord, SrvServiceName, selectSrvByRoll } from "cap:dns_srv_record";

fn example() {
    if (svc := SrvServiceName.parse("_https._tcp.api.example.com.")) {
        * = svc.service; // "https"
        * = svc.proto;   // "tcp"
    }
    if (r1 := SrvRecord.parseText("10 70 443 a.example.com.")) {
        if (r2 := SrvRecord.parseText("10 30 443 b.example.com.")) {
            const list: [2]SrvRecord = [r1, r2];
            * = selectSrvByRoll(list[0..2], 10, 75); // 1 (b.example.com.)
        }
    }
}
```
