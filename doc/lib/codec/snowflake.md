# `cap:snowflake` — 分布式 64-bit Snowflake ID 生成与 Base62 编码标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

## 1. 概述
`cap:snowflake`（动态挂载 `cap.snowflake`）是纯 CP 线程安全的分布式 64-bit Snowflake ID 生成器（41-bit 毫秒时间戳 + 5-bit 数据中心 + 5-bit 工作节点 + 12-bit 毫秒内序列号），支持时钟回拨保护、字段反解与零堆 Base62 编解码。

## 2. 核心类型
- `SnowflakeError`：`InvalidNodeId`、`ClockMovedBackwards`、`InvalidIdString`
- `SnowflakeParts` / `SnowflakeGenerator`

## 3. API 速查
- `SnowflakeGenerator.init(datacenter_id, worker_id, epoch_ms)` / `nextId()` / `nextIdAt(now_ms)`
- `decomposeId(id, epoch_ms)` -> `SnowflakeParts`
- `encodeBase62(id, &buf)` / `decodeBase62(str)`

## 4. 示例
```cp
import { SnowflakeGenerator, DEFAULT_EPOCH_MS, decomposeId } from "cap:codec";
```
