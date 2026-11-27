[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# snowflake

零堆分配的 64 位 Snowflake 分布式 ID 生成器、编解码器与字符串解析器（兼容 Twitter / Discord 纪元）。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `snowflake` |
| **CP 包名** | `snowflake` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态保持原包名不变，无需任何 `@cap/*` 前缀或改名即可平滑迁移：

```rust
// Rust (crates.io: snowflake)
use snowflake::SnowflakeIdGenerator;
let mut gen = SnowflakeIdGenerator::new(1, 2);
let id = gen.real_time_generate();
```

```typescript
// CP（保持原包名 "snowflake"）
import { SnowflakeGenerator, TWITTER_EPOCH_MS } from "snowflake";
let mut gen = SnowflakeGenerator.withTwitterEpoch(1, 2);
let id = gen.nextId(1700000000000);
```

## 安装

```bash
cap add snowflake
```

## 64 位位布局

- **41 bit**：相对自定义纪元的毫秒时间戳（`SF_TIMESTAMP_SHIFT = 22`，上限 `SF_MAX_TIMESTAMP`）
- **5 bit**：数据中心 ID（`0..31`，`SF_MAX_DATACENTER = 31`，`SF_DATACENTER_SHIFT = 17`）
- **5 bit**：工作节点 ID（`0..31`，`SF_MAX_WORKER = 31`，`SF_WORKER_SHIFT = 12`）
- **12 bit**：同毫秒内自增序列号（`0..4095`，`SF_MAX_SEQUENCE = 4095`）

内置纪元常量：
- `TWITTER_EPOCH_MS = 1288834974657`（2010-11-04T01:42:54.657Z）
- `DISCORD_EPOCH_MS = 1420070400000`（2015-01-01T00:00:00.000Z）

## 快速上手与实战示例

### 1. 有状态生成器（`SnowflakeGenerator`）

```typescript
import { SnowflakeGenerator } from "snowflake";

fn generateIds(): void {
    // 支持 Twitter 纪元、Discord 纪元或自定义纪元
    let mut twitter_gen = SnowflakeGenerator.withTwitterEpoch(1, 5);
    let mut discord_gen = SnowflakeGenerator.withDiscordEpoch(0, 3);
    let mut custom_gen = SnowflakeGenerator.new(1704067200000, 2, 7);

    let now_ms: u64 = 1705000000000;
    if (let id = twitter_gen.nextId(now_ms)) {
        let ts = twitter_gen.timestamp(id);
        let parts = twitter_gen.decompose(id);
        // parts.timestamp_ms, parts.datacenter_id, parts.worker_id, parts.sequence
    }

    // 批量生成到栈缓冲区（零堆分配），返回实际生成数量
    let mut batch: [16]u64 = undefined;
    let produced = discord_gen.nextBatch(now_ms, batch[0..16]);

    // 使用当前生成器的 epoch/datacenter/worker 直接组装 ID
    let manual_id = custom_gen.compose(now_ms, 42);
    custom_gen.reset();
}
```

### 2. 无状态编解码与字段提取

```typescript
import {
    TWITTER_EPOCH_MS,
    encodeSnowflake,
    compose,
    decodeSnowflake,
    decompose,
    timestamp,
    datacenterId,
    workerId,
    sequence,
    minIdForTimestamp,
    maxIdForTimestamp,
} from "snowflake";

fn inspectSnowflake(): void {
    let ts: u64 = 1700000000000;
    let id = encodeSnowflake(TWITTER_EPOCH_MS, ts, 3, 12, 99);
    // `compose` 为 `encodeSnowflake` 别名
    let same = compose(TWITTER_EPOCH_MS, ts, 3, 12, 99);

    let parts = decodeSnowflake(id, TWITTER_EPOCH_MS); // 别名：decompose
    let extracted_ts = timestamp(id, TWITTER_EPOCH_MS);
    let dc = datacenterId(id); // 3
    let wk = workerId(id);     // 12
    let seq = sequence(id);    // 99

    // 生成数据库时间范围索引边界（WHERE id BETWEEN lo AND hi）
    let lo = minIdForTimestamp(TWITTER_EPOCH_MS, ts);
    let hi = maxIdForTimestamp(TWITTER_EPOCH_MS, ts);
}
```

### 3. 十进制字符串格式化与解析

```typescript
import { formatSnowflake, parseSnowflake, parse, isValidSnowflakeString } from "snowflake";

fn textCodec(id: u64): void {
    let mut buf: [24]u8 = undefined;
    let n = formatSnowflake(id, buf[0..24]); // 写入 ASCII 数字，返回字节长度

    if (isValidSnowflakeString("175928847299117063")) {
        if (let parsed = parseSnowflake("175928847299117063")) {
            // `parse` 为 `parseSnowflake` 别名
            let same = parse("175928847299117063");
        }
    }
}
```

## API 速览

### 常量
- `TWITTER_EPOCH_MS: u64`（`1288834974657`）、`DISCORD_EPOCH_MS: u64`（`1420070400000`）
- `SF_MAX_DATACENTER: u16`（`31`）、`SF_MAX_WORKER: u16`（`31`）、`SF_MAX_SEQUENCE: u16`（`4095`）
- `SF_TIMESTAMP_SHIFT: u6`（`22`）、`SF_DATACENTER_SHIFT: u6`（`17`）、`SF_WORKER_SHIFT: u6`（`12`）
- `SF_MAX_TIMESTAMP: u64`（`(1 << 41) - 1`）

### 核心类型
- `SnowflakeParts { timestamp_ms: u64, datacenter_id: u16, worker_id: u16, sequence: u16 }`
- `SnowflakeGenerator`：
  - `new(epoch_ms: u64, datacenter_id: u16, worker_id: u16)` / `withTwitterEpoch(dc, wk)` / `withDiscordEpoch(dc, wk)`
  - `nextId(now_ms: u64): u64?`、`nextBatch(now_ms: u64, out: []u64): usize`
  - `compose(timestamp_ms: u64, seq: u16): u64`、`decompose(id: u64): SnowflakeParts`、`timestamp(id: u64): u64`、`reset()`

### 独立函数
- `encodeSnowflake` / `compose`、`decodeSnowflake` / `decompose`
- `timestamp`、`datacenterId`、`workerId`、`sequence`
- `minIdForTimestamp`、`maxIdForTimestamp`
- `formatSnowflake(id: u64, out: []u8): usize`、`parseSnowflake(s: string): u64?` / `parse(s: string): u64?`、`isValidSnowflakeString(s: string): bool`
