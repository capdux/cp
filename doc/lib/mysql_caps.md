# `cap:mysql_caps` — MySQL 32-bit 能力位与 16-bit 服务器状态标志库

> 来源：吸收自 `/data/dev/bun/src/sql/mysql/{Capabilities,StatusFlags}.cp`

`cap:mysql_caps` 提供纯 CP、零外部依赖、零堆分配的 MySQL 4.1–8.4 协议 32-bit 能力位集合（`Capabilities`）与 16-bit 服务器状态标志集合（`StatusFlags`），支持客户端/服务端握手协商交集、未支持扩展位剔除、人类可读格式化及事务/多结果集状态判定。

## 核心类型

### `CapabilityFlag` & `Capabilities`

```cp
pub enum CapabilityFlag: u32 {
    CLIENT_LONG_PASSWORD = 1,
    // ... 覆盖全部 32 个标准 MySQL 4.1–8.4 能力位 ...
    CLIENT_REMEMBER_OPTIONS = 2147483648,

    fn bitName(): string
}

pub struct Capabilities {
    raw: u32 = 0,

    static fn fromInt(v: u32): Capabilities
    static fn defaultClient(): Capabilities
    fn toInt(): u32
    fn has(flag: CapabilityFlag): bool
    fn set(*_, flag: CapabilityFlag)
    fn clear(*_, flag: CapabilityFlag)
    fn intersect(server_caps: Capabilities): Capabilities
    fn rejectUnsupported(*_)
    fn formatList(out: []u8): []const u8
}
```

### `StatusFlag` & `StatusFlags`

```cp
pub enum StatusFlag: u16 {
    SERVER_STATUS_IN_TRANS = 1,
    SERVER_STATUS_AUTOCOMMIT = 2,
    SERVER_MORE_RESULTS_EXISTS = 8,
    SERVER_STATUS_NO_GOOD_INDEX_USED = 16,
    SERVER_STATUS_NO_INDEX_USED = 32,
    SERVER_STATUS_CURSOR_EXISTS = 64,
    SERVER_STATUS_LAST_ROW_SENT = 128,
    SERVER_STATUS_DB_DROPPED = 256,
    SERVER_STATUS_NO_BACKSLASH_ESCAPES = 512,
    SERVER_STATUS_METADATA_CHANGED = 1024,
    SERVER_QUERY_WAS_SLOW = 2048,
    SERVER_PS_OUT_PARAMS = 4096,
    SERVER_STATUS_IN_TRANS_READONLY = 8192,
    SERVER_SESSION_STATE_CHANGED = 16384,
}

pub struct StatusFlags {
    raw: u16 = 0,

    static fn fromInt(v: u16): StatusFlags
    fn toInt(): u16
    fn has(flag: StatusFlag): bool
    fn inTransaction(): bool
    fn isAutoCommit(): bool
    fn hasMoreResults(): bool
    fn isReadOnlyTransaction(): bool
}
```

## 使用示例

```cp
import { Capabilities, CapabilityFlag, StatusFlags } from "cap:mysql_caps";

let client = Capabilities.defaultClient();
let server = Capabilities.fromInt(0xFFFFFFFF);
server.rejectUnsupported();
const negotiated = client.intersect(server);

const status = StatusFlags.fromInt(0x000B); // IN_TRANS | AUTOCOMMIT | MORE_RESULTS
// status.inTransaction() == true, status.hasMoreResults() == true
```
