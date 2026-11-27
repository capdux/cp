[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# snowflake

纯 CP 实现的零堆 64 位 Snowflake 分布式 ID 生成器、位域解码器、时间窗口范围计算器及十进制格式化/解析库（兼容 Twitter 与 Discord Epoch）。

- **源包对齐**：参考 Rust [`snowflake`](https://crates.io/crates/snowflake)（`crates.io`，`MIT`）及标准 Twitter/Discord 41-5-5-12 位布局。
- **包版本**：`1.0.0`（依赖 `itoa`）。

## 安装与导入

```bash
cap add snowflake
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "snowflake": "^1.0.0"
  }
}
```

```typescript
import {
    SnowflakeGenerator, SnowflakeParts,
    TWITTER_EPOCH_MS, DISCORD_EPOCH_MS,
    SF_MAX_DATACENTER, SF_MAX_WORKER, SF_MAX_SEQUENCE,
    encodeSnowflake, compose, decodeSnowflake, decompose,
    timestamp, datacenterId, workerId, sequence,
    minIdForTimestamp, maxIdForTimestamp,
    formatSnowflake, parseSnowflake, parse, isValidSnowflakeString
} from "snowflake";
```

## 核心 API 参考

### 常量与 `class SnowflakeParts`

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| `TWITTER_EPOCH_MS` | `1288834974657` | Twitter Snowflake 纪元（2010-11-04 UTC） |
| `DISCORD_EPOCH_MS` | `1420070400000` | Discord Snowflake 纪元（2015-01-01 UTC） |
| `SF_MAX_DATACENTER` / `SF_MAX_WORKER` / `SF_MAX_SEQUENCE` | `31` / `31` / `4095` | 5 位数据中心、5 位机器节点、12 位序列号上限 |
| `SF_TIMESTAMP_SHIFT` / `SF_DATACENTER_SHIFT` / `SF_WORKER_SHIFT` / `SF_MAX_TIMESTAMP` | `22` / `17` / `12` / `(1 << 41) - 1` | 位移与时间戳上限常量 |
| `SnowflakeParts` | `timestamp_ms: u64`、`datacenter_id: u16`、`worker_id: u16`、`sequence: u16` | 解码后的 ID 各分量 |

### `class SnowflakeGenerator`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `new` | `SnowflakeGenerator.new(epoch_ms: u64, datacenter_id: u16, worker_id: u16): SnowflakeGenerator` | 使用自定义纪元创建生成器 |
| `withTwitterEpoch` / `withDiscordEpoch` | `SnowflakeGenerator.withTwitterEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator` | 使用 Twitter 或 Discord 纪元创建生成器 |
| `nextId` | `nextId(*_, now_ms: u64): u64?` | 生成下一个单调递增 ID；时钟回拨或同毫秒序列号溢出（`> 4095`）返回 `null` |
| `nextBatch` | `nextBatch(*_, now_ms: u64, out: []u64): usize` | 在 `now_ms` 批量生成至 `out`，返回成功生成数量 |
| `compose` / `decompose` / `timestamp` | `compose(timestamp_ms: u64, seq: u16): u64` / `decompose(id: u64): SnowflakeParts` / `timestamp(id: u64): u64` | 使用生成器自身纪元与节点 ID 编解码 |
| `reset` | `reset(*_)` | 重置 `last_ts_ms` 与 `sequence` 为 `0` |

### 独立函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `encodeSnowflake` / `compose` | `encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64` | 按位组装 64 位 Snowflake ID |
| `decodeSnowflake` / `decompose` | `decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts` | 解码 ID 为 `SnowflakeParts` |
| `timestamp` / `datacenterId` / `workerId` / `sequence` | `timestamp(id: u64, epoch_ms: u64): u64` / `datacenterId(id: u64): u16` / ... | 提取单个字段 |
| `minIdForTimestamp` / `maxIdForTimestamp` | `minIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64` | 计算时间点对应的最小/最大 ID，便于数据库范围检索 |
| `formatSnowflake` | `formatSnowflake(id: u64, out: []u8): usize` | 零堆格式化为十进制 ASCII 字符串写入 `out` |
| `parseSnowflake` / `parse` | `parse(s: string): u64?` | 解析十进制字符串为 `u64?`（非法或溢出返回 `null`） |
| `isValidSnowflakeString` | `isValidSnowflakeString(s: string): bool` | 校验字符串是否为合法 `u64` 十进制 Snowflake ID |

## 使用示例

### 1. 生成单调 ID 并解码格式化

```typescript
import { SnowflakeGenerator, formatSnowflake } from "snowflake";

fn generateOrderId(now_ms: u64, out: []u8): usize {
    let mut gen = SnowflakeGenerator.withTwitterEpoch(1, 7);
    let id = gen.nextId(now_ms) orelse return 0;

    let parts = gen.decompose(id);
    // parts.datacenter_id == 1, parts.worker_id == 7, parts.timestamp_ms == now_ms
    return formatSnowflake(id, out);
}
```

### 2. 时间窗口范围查询与字符串解析

```typescript
import { DISCORD_EPOCH_MS, minIdForTimestamp, maxIdForTimestamp, parse } from "snowflake";

fn isInWindow(raw_id: string, start_ms: u64, end_ms: u64): bool {
    let id = parse(raw_id) orelse return false;
    let lo = minIdForTimestamp(DISCORD_EPOCH_MS, start_ms);
    let hi = maxIdForTimestamp(DISCORD_EPOCH_MS, end_ms);
    return id >= lo && id <= hi;
}
```

## 与源包差异与 CP 安全亮点

- **显式时间注入（`now_ms: u64`）**：生成器不隐式执行系统时钟调用，热路径零 syscall 且 100% 可确定性测试。
- **可选类型防回拨与防溢出**：时钟回拨或同毫秒超 4096 个 ID 时返回 `null`（`u64?`），不阻塞调度器线程也不 panic。
- **零堆格式化**：`formatSnowflake` 借助 `itoa` 直接写入调用方 `[]u8`（最多 20 字节）。
