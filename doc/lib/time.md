# CP 标准库：时间、RFC 3339 / ISO 8601、Cron 调度与限流套件手册 (`cap:time`)

> 参考来源：`/data/dev/bun/docs/runtime/utils.mdx`, `/data/dev/bun/src/bun_core/time.cp` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:time` 是 CP 语言官方内置的工业生产级纯 CP 时间、日历转换、Cron 调度与限流退避标准库，**零外部依赖、零 `std.posix`、零 `@import("bun")`、零堆分配**，适用于 BEAM 调度器、分布式定时任务、API 网关限流与重试退避场景。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 用户层时间与调度应用 (cap:time)                  │
├───────────────────┬─────────────────────┬───────────────────────────────┤
│ 日历与 RFC 3339    │ POSIX Cron 调度器   │ 限流与抖动退避引擎              │
│ CivilDateTime     │ CronExpr            │ TokenBucket                   │
│ Iso8601           │ matches()           │ SlidingWindowLimiter          │
│ IsoDuration       │ nextAfter()         │ ExponentialBackoff            │
├───────────────────┴─────────────────────┴───────────────────────────────┤
│  Duration / WallClock (std.os.linux.clock_gettime & Howard Hinnant)     │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/time/` 下全部 5 个源文件首行均严格标记 `// Source:` 谱系注释：

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/time/clock.cp` | `// Source: cp/lib/time/clock.cp (Monotonic Clock, Wall Clock & Duration Primitives)` | `TimeUnit`、`Duration`（纳秒/微秒/毫秒/秒/分/时/天算术）与基于 `std.os.linux.clock_gettime` 的 `WallClock` |
| `lib/time/iso8601.cp` | `// Source: cp/lib/time/iso8601.cp (RFC 3339 / ISO 8601 DateTime & Duration)` | `CivilDateTime`（Howard Hinnant 日历与 Epoch 双向纯算法转换）、`Iso8601` 严格解析/格式化与 `IsoDuration` |
| `lib/time/cron.cp` | `// Source: cp/lib/time/cron.cp (POSIX 5-Field Cron Expression Scheduler)` | `CronExpr` 位掩码解析（支持 `*`、`*/n`、`a-b`、`a-b/n`、逗号列表、`JAN-DEC`/`SUN-SAT` 及 `@yearly`/`@monthly`/`@weekly`/`@daily`/`@hourly`）、`matches()` 与跳步式 `nextAfter()` |
| `lib/time/ratelimit.cp` | `// Source: cp/lib/time/ratelimit.cp (TokenBucket, SlidingWindow & ExponentialBackoff)` | `TokenBucket` 令牌桶、`SlidingWindowLimiter` 滑动窗口计数器与 `ExponentialBackoff` 确定性全抖动/等抖动退避 |
| `lib/time/index.cp` | `// Source: cp/lib/time/index.cp (Unified Public Exports for cap:time)` | `cap:time` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. `CivilDateTime`、`Iso8601` 与 `IsoDuration`（`iso8601.cp`）

- **`CivilDateTime`**：包含 `year`, `month`, `day`, `hour`, `minute`, `second`, `nanosecond`, `tz_offset_minutes`。
  - **Howard Hinnant 算法**（`daysFromCivil` / `civilFromDays`）：纯整数运算，无查表、无堆分配，精确支持 1970 年前后（含负时间戳与负年份）、400 年闰年周期（`1900`/`2100` 非闰年、`2000`/`2024` 闰年）。
  - **双向转换**：`toUnixSeconds()` / `toUnixMillis()` / `fromUnixSeconds(sec)` / `fromUnixMillis(ms)` / `fromUnixSecondsWithOffset(sec, offset)` / `toUtc()` / `withOffset(offset)` / `dayOfWeek()`（0=Sunday..6=Saturday）。
- **`Iso8601`**：
  - `Iso8601.parseDateTime(input)`：严格解析 `YYYY-MM-DDTHH:MM:SS[.fff...][Z|+HH:MM|-HH:MM]`（校验闰日、月份天数、时分秒范围及合法偏移），非法输入返回 `null`。
  - `Iso8601.parseDate(input)`：解析 `YYYY-MM-DD`。
  - `Iso8601.formatRfc3339(dt, out)`：零堆分配格式化为规范 RFC 3339 字符串。
- **`IsoDuration`**：
  - `IsoDuration.parse("P1DT2H30M15S")`：支持负号、年/月/周/日、时间段 `T`（时/分/秒及小数毫秒如 `PT1.5S`），严格校验字段顺序。
  - `toMilliseconds()` / `toMillisecondsExact()`（若包含非零年/月则返回 `null`）/ `toSeconds()` / `format(out)`。

### 2. `CronExpr`（`cron.cp`）

- **5 字段解析 (`CronExpr.parse`)**：`分(0-59) 时(0-23) 日(1-31) 月(1-12/JAN-DEC) 周(0-7/SUN-SAT)`。
  - 支持 `*`、`*/n`、`a-b`、`a-b/n`、`a/n`、逗号列表，以及 `@yearly`/`@annually`、`@monthly`、`@weekly`、`@daily`/`@midnight`、`@hourly`。
  - 遵循 POSIX / Vixie Cron 语义：当“日”与“周”均受限时采用“或（OR）”匹配，任一为 `*` 时采用“与（AND）”匹配。
- **匹配与调度**：
  - `matches(dt: CivilDateTime): bool` 与 `matchesEpochSec(epoch_sec: i64): bool`。
  - `nextAfter(from_epoch_sec: i64): i64?`：采用字段跳跃推进算法，O(1) 跨越无效月份与非闰年（例如 `0 0 29 2 *` 从 `2096-02-29` 精确跳至 `2104-02-29`）；对永不满足的表达式（如 `0 0 31 2 *`）立即返回 `null`。

### 3. `TokenBucket`、`SlidingWindowLimiter` 与 `ExponentialBackoff`（`ratelimit.cp`）

- **`TokenBucket`**：
  - `init(capacity, refill_per_sec, now_ms)` / `initWithTokens(...)`；
  - `tryAcquire(tokens, now_ms)` / `tryAcquireOne(now_ms)` / `retryAfterMs(tokens, now_ms)` / `available(now_ms)` / `reset(now_ms)`。
  - 外部显式注入 `now_ms`，无需系统调用即可进行 100% 确定性单元测试。
- **`SlidingWindowLimiter`**：
  - O(1) 空间双窗口加权计数算法，防止固定窗口跨边界 2 倍突发流量；
  - 提供 `tryAcquire(now_ms)`、`tryAcquireN(count, now_ms)`、`currentCount(now_ms)`、`remaining(now_ms)`、`retryAfterMs(now_ms)`、`reset(now_ms)`。
- **`ExponentialBackoff`**：
  - 配置 `base_ms`、`max_ms`、`multiplier` 与状态字段 `attempt`；
  - `nextDelayMs(seed)`：基于 SplitMix64 的确定性 AWS Full Jitter（区间 `[0, min(max_ms, base_ms * multiplier^attempt)]`）并自增 `attempt`；
  - `nextEqualJitterMs(seed)`、`peekDelayMs(seed)`、`rawDelayMs()`、`reset()`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    CivilDateTime,
    Iso8601,
    IsoDuration,
    CronExpr,
    TokenBucket,
    SlidingWindowLimiter,
    ExponentialBackoff,
} from "cap:time";

pub fn demoTimeSuite() {
    // 1. RFC 3339 解析与转换
    const dt = Iso8601.parseDateTime("2026-11-20T22:30:00.5+08:00").?;
    const epoch_ms = dt.toUnixMillis();
    let out: [64]u8 = [0; 64];
    const utc_str = dt.toUtc().formatRfc3339(&out) catch unreachable;

    // 2. ISO Duration 与 Cron 调度
    const dur = IsoDuration.parse("P1DT2H30M15S").?;
    const dur_ms = dur.toMilliseconds();
    const cron = CronExpr.parse("0 0 29 2 *").?;
    const next_leap = cron.nextAfter(CivilDateTime.utc(2024, 2, 29, 0, 0, 0).toUnixSeconds()).?;

    // 3. 限流与退避
    let bucket = TokenBucket.init(10.0, 5.0, 0);
    const ok = bucket.tryAcquire(4.0, 100);

    let limiter = SlidingWindowLimiter.init(100, 1000, 0);
    const allowed = limiter.tryAcquire(50);

    let backoff = ExponentialBackoff.init(100, 5000, 2.0);
    const wait_ms = backoff.nextDelayMs(42);

    * = epoch_ms;
    * = utc_str;
    * = dur_ms;
    * = next_leap;
    * = ok;
    * = allowed;
    * = wait_ms;
}
```

---

## 零分配时长与 RFC 3339 子模块（收敛自 `duration_parse` / `rfc3339_time`）

- **人类可读时长解析与格式化 (`duration.cp`)**：`NS_PER_US`..`NS_PER_WEEK`、`parseDurationNs`、`parseDurationMs`、`formatDurationNs`、`formatDurationCompact`。
- **RFC 3339 / ISO 8601 时间戳编解码 (`rfc3339.cp`)**：`isLeapYear`、`daysInMonth`、`Rfc3339Time`。

---

## Batch 21 时间/时钟/时区/时序/调度域收敛子模块

以下 16 个纯 CP 子模块已统一收敛至 `lib/time/`（0 覆盖、0 符号冲突，Bun 资产 `duration.cp` / `rfc3339.cp` 保持最高优先权）：

| 目标文件 | 原目录 | 核心导出符号与职责 |
| :--- | :--- | :--- |
| `lib/time/tz.cp` | `lib/tz` | `TzError`, `DstRule`, `LocalDateTime`, `Timezone`, `isValidDateTime`, `daysFromCivil`, `weekdayFromDays`（POSIX TZ & DST 规则引擎；`isLeapYear`/`daysInMonth` 沿用 `rfc3339.cp`） |
| `lib/time/hlc.cp` | `lib/hlc` | `HlcTimestamp`, `HlcClock`, `formatHlc`, `parseHlc`（Kulkarni 混合逻辑时钟 HLC） |
| `lib/time/ptp.cp` | `lib/ptp` | `PtpTimestamp`, `PtpPortIdentity`, `PtpHeader`, `PtpPacket`, `parsePtp`, `encodePtpHeader` 等（IEEE 1588-2008/2019 PTPv2/v2.1 精密时间协议编解码） |
| `lib/time/supervisor.cp` | `lib/supervisor` | `SupervisorError`, `RestartStrategy`, `ChildRestartPolicy`, `SupervisorConfig`, `SupervisorTree`（OTP 监督树与重启风暴防护） |
| `lib/time/supervisor_resilience.cp` | `lib/supervisor` | `BackoffKind`, `BackoffPolicy`, `Bulkhead`, `RetryBudget`（退避策略、舱壁隔离与重试预算） |
| `lib/time/timeseries.cp` | `lib/timeseries` | `TimeSeriesError`, `AggOp`, `DataPoint`, `AnomalyPoint`, `SeriesSummary`, `TimeSeries`（时序存储、对齐降采样、SMA/EMA 与 Z-Score 异常检测） |
| `lib/time/prometheus.cp` | `lib/prometheus` | `PromError`, `MetricType`, `LabelPair`, `ParsedSample`, `PromRegistry`, `parseExposition`, `freeParsedSamples`（Prometheus 时序指标与 Exposition 解析） |
| `lib/time/promql.cp` | `lib/promql` | `PromqlError`, `MatchOp`, `VectorSelector`, `MatrixSelector`, `AggregationExpr`, `BinaryExpr`, `Expr`, `parseDuration`, `parse`（PromQL 时序查询词法与 AST 解析） |
| `lib/time/interval.cp` | `lib/interval` | `IntervalError`, `Interval`, `IntervalSet`, `IntervalEntry`, `IntervalTree`（半开区间集合代数与增广区间树） |
| `lib/time/tween.cp` | `lib/tween` | `TweenError`, `EaseKind`, `Easing`, `Tween`, `Keyframe`, `KeyframeTrack`, `SpringDamper`（时间轴缓动、关键帧轨道与弹簧阻尼引擎） |
| `lib/time/health.cp` | `lib/health` | `HealthError`, `ProbeKind`, `HealthStatus`, `CheckOutcome`, `ProbeReport`, `HealthRegistry`, `ShutdownCoordinator`（探针聚合器与优雅停机协调） |
| `lib/time/uuid_v7.cp` | `lib/uuid_v7` | `UuidError`, `Uuid`, `NAMESPACE_DNS` 等（RFC 9562 单调时间戳 UUID v7 / v4 / v5） |
| `lib/time/job.cp`, `lib/time/job_scheduler.cp` | `lib/queue` | `QueueError`, `JobPriority`, `JobState`, `RetryPolicy`, `Job`, `QueueStats`, `JobQueue`（优先级、延迟调度、租约超时与死信作业队列） |
| `lib/time/vcd.cp` | `lib/vcd` | `VcdTimeUnit`, `VcdVarType`, `VcdEventKind`, `VcdTimescale`, `VcdHeader`, `VcdEvent`, `parseVcdTimescale`, `parseVcdHeader`, `VcdVarIter`, `VcdValueIter`（IEEE 1364 VCD 时序波形解析） |
| `lib/time/pqueue.cp` | `lib/pqueue` | `PQueueError`, `PriorityItem`, `TopKEntry`, `IndexedMinHeap`, `BoundedTopK`, `StreamingMedian`（可索引最小堆与流式分位数） |
| `lib/time/pool.cp` | `lib/pool` | `PoolError`, `BorrowOrder`, `PoolConfig`, `PooledHandle`, `PoolStats`, `ResourcePool`（带空闲/生命周期超时的有界资源池） |
| `lib/time/union_interval_.cp` *(Reference Only)* | `lib/union_interval` | 参考归档（与 `lib/time/interval.cp` 功能重叠，不从 `index.cp` 导出，保留用于对比分析） |

---

## 上游行为契约与时间边界注意事项

1. **单调时钟 vs 墙钟严格区分（`WallClock` / `Duration`）**：
   - 测量耗时、限流窗口（`TokenBucket` / `SlidingWindowLimiter`）与超时控制必须使用单调递增时间戳，避免 NTP 校时或夏令时回拨导致负耗时或限流误判；仅在日志与外部协议序列化时使用墙钟 UTC 时间戳。
2. **公历闰年与 RFC 3339 严格校验（`CivilDateTime` / `Iso8601` / `Rfc3339Time`）**：
   - 采用 Howard Hinnant 纯算法实现公历与 Unix Epoch 双向转换，精确处理 400 年闰年周期（`isLeapYear` / `daysInMonth`）并拦截 `2025-02-29` 等非法日期与越界时区偏移。
3. **POSIX Cron 日/周并集与跳步推进（`CronExpr`）**：
   - 遵循 POSIX Cron 规范：当“日（DOM）”与“星期（DOW）”均非通配符 `*` 时，满足任一即触发；`nextAfter` 按月/日/时/分位掩码跳步推进，无需逐分钟空转扫描。
4. **抖动退避防惊群（`ExponentialBackoff`）**：
   - 分布式重试场景推荐传入确定性随机种子启用抖动（Jitter），避免多节点在故障恢复瞬间同步重连引发雪崩。

---

## 零堆分配确定性定时与周期调度器（`recurring_scheduler.cp`）

面向服务端定时任务、事件循环超时计算与 AI Agent 周期巡检的 100% 纯 CP 零堆分配调度引擎：

- **核心类型**：`ScheduleKind`（`OneShot | FixedInterval | FixedDelay`）、`ScheduleState`（`Active | Paused | Completed | Failed | Cancelled`）、`ScheduledTaskSlot`、`SchedulerStats`、`RecurringScheduler`
- **核心能力**：
  - `scheduleOneShot` / `scheduleInterval` / `scheduleFixedDelay`：支持一次性任务、漂移无关的固定间隔任务（超时跳帧防惊群）与完成后固定延迟任务，内置确定性 SplitMix64 哈希抖动（`jitter_window_ms`）；
  - `nextDueMs(): i64?`：O(N) 精确返回下一个最近触发时间戳，可直接喂给事件循环 `epoll`/`poll`；
  - `pollDue(*_, now_ms, out_task_ids): usize`：按 `(next_run_ms asc, task_id asc)` 确定性提取全部到期任务并标记 `in_flight` 防止重入并发；
  - `reportResult(*_, task_id, succeeded, now_ms): bool`：成功推进 `run_count` 与下一周期（达 `max_runs` 自动标记 `Completed`）；失败按指数退避（`retry_backoff_ms * 2^(retry-1)`）自动重排直至 `max_retries` 后标记 `Failed`；
  - `pause` / `resumeTask` / `cancel` / `releaseTerminal` / `stats`。

### QuickStart 示例

```cp
import { ScheduledTaskSlot, RecurringScheduler } from "cap:time";

pub fn demoRecurringScheduler() {
    let slots: [8]ScheduledTaskSlot = [ScheduledTaskSlot{}; 8];
    let slots_s: []ScheduledTaskSlot = slots[0..];
    let sched = RecurringScheduler.init(slots_s) ?? return;

    * = sched.scheduleOneShot(1, "warmup", 1000);
    * = sched.scheduleInterval(2, "heartbeat", 1000, 5000, 0, 200);
    * = sched.setRetryPolicy(2, 3, 500);

    if (next_ms := sched.nextDueMs()) {
        let due: [4]u64 = [0; 4];
        let due_s: []u64 = due[0..];
        const n = sched.pollDue(next_ms, due_s);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            * = sched.reportResult(due[i], true, next_ms + 20);
        }
    }
    * = sched.stats();
}
```

---

## 高精度多租户 GCRA 速率限流器（`gcra_limiter.cp`）

对标 Stripe / Cloudflare 工业级限流架构的 100% 纯 CP、零堆分配 GCRA（Generic Cell Rate Algorithm）平滑突发限流与多租户配额表：单租户仅需单个 `tat_ms`（Theoretical Arrival Time）时间戳槽位，零后台定时器，内置开放寻址与最旧 TAT 淘汰防止多租户内存膨胀：

- **核心类型**：
  - `GcraConfig`（`emission_interval_ms`, `burst_capacity`, `max_cost_per_call`，提供 `init(...)`、`isValid()`、`burstWindowMs()`）
  - `GcraDecision`（`allowed`, `remaining_burst`, `retry_after_ms`, `reset_after_ms`, `tat_ms` —— 直接映射 HTTP `X-RateLimit-Remaining`、`Retry-After` 与 `X-RateLimit-Reset` 响应头）
  - `GcraTenantSlot`（`key_hash`, `tat_ms`, `occupied`）
  - `GcraStats`（`capacity`, `active_tenants`, `total_allowed`, `total_rejected`, `total_evictions`）
- **核心能力（`GcraLimiter`）**：
  - `init(slots_buf: *[]GcraTenantSlot, cfg: GcraConfig): GcraLimiter?`
  - `checkAndConsume(*_, key_hash: u64, cost: u32, now_ms: i64): GcraDecision`（支持加权 `cost >= 1` 扣减、精确 `retry_after_ms` 到点瞬间放行、时钟回拨钳制与满槽最旧 TAT 淘汰）
  - `peek(key_hash: u64, cost: u32, now_ms: i64): GcraDecision`（只读试探，零副作用）
  - `resetTenant(*_, key_hash: u64): bool` / `resetAll(*_)` / `stats(): GcraStats`

### QuickStart 示例

```cp
import { GcraConfig, GcraTenantSlot, GcraLimiter } from "cap:time";

pub fn demoGcraRateLimiter() {
    // 每 100ms 补充 1 配额（10 req/s），突发上限 3，单次最高消耗 3
    const cfg = GcraConfig.init(100, 3, 3) ?? return;
    let slots: [128]GcraTenantSlot = [GcraTenantSlot{}; 128];
    let slots_s: []GcraTenantSlot = slots[0..];
    let limiter = GcraLimiter.init(slots_s, cfg) ?? return;

    const tenant_hash: u64 = 0xA1B2C3D4;
    const d1 = limiter.checkAndConsume(tenant_hash, 1, 1000); // allowed=true, remaining=2
    const d2 = limiter.checkAndConsume(tenant_hash, 2, 1000); // allowed=true, remaining=0
    const d3 = limiter.checkAndConsume(tenant_hash, 1, 1000); // allowed=false, retry_after_ms=100
    const d4 = limiter.checkAndConsume(tenant_hash, 1, 1000 + d3.retry_after_ms); // allowed=true!
    * = d1;
    * = d2;
    * = d4;
}
```

---

## 零分配分层哈希时间轮 O(1) 定时器引擎（`timing_wheel.cp`）

面向百万级并发 Agent 超时控制、RPC 截止时间与 TCP Keepalive 的 Kafka / Netty / Linux 内核级 100% 纯 CP 零动态堆分配 3 级 64 槽分层哈希时间轮（总跨度 `64^3 = 262,144` ticks，O(1) 调度、O(1) ABA 防护取消、自动级联降级）：

- **常量与模型**：`TW_SLOTS_PER_LEVEL` (`64`)、`TW_LEVELS` (`3`)、`TW_MAX_TIMERS` (`256`)、`TW_INVALID_IDX` (`0xFFFF`)、`TimerHandle`（`slot_idx` + `generation` 防 ABA）、`WheelTimerEntry`、`TimingWheelStats`。
- **核心引擎 `HierarchicalTimingWheel`**：
  - `init(start_tick: u64): HierarchicalTimingWheel`
  - `schedule(*_, delay_ticks, user_tag): TimerHandle?` / `schedule_periodic(*_, delay_ticks, interval_ticks, user_tag): TimerHandle?`（O(1) 侵入式入桶，超 `262,143` ticks 或满 256 槽安全返回 `null`）
  - `cancel(*_, handle): bool`（严格校验 `generation` 防 ABA，O(1) 双向链表摘除并归还空闲栈）
  - `advance_to(*_, target_tick, out_fired_tags): usize`（逐 tick 推进并自动触发 Level 2 -> Level 1 -> Level 0 级联降级，到期输出 `user_tag`，周期定时器自动重新入轮；时间倒退安全空操作）
  - `next_expiry_tick(): u64?` / `stats(): TimingWheelStats`

### QuickStart 示例

```cp
import { HierarchicalTimingWheel } from "cap:time";

pub fn demoTimingWheel() {
    let tw = HierarchicalTimingWheel.init(0);
    const h1 = tw.schedule(5, 1001) ?? return;
    const h2 = tw.schedule_periodic(10, 10, 2002) ?? return;
    * = tw.schedule(5000, 3003); // Level 2 远程定时器

    let fired: [8]u64 = [0; 8];
    let fired_s: []u64 = fired[0..];
    * = tw.advance_to(25, fired_s); // 触发 1001(@5), 2002(@10), 2002(@20)
    * = tw.cancel(h2);              // O(1) 停止周期定时器
    * = h1;
}
```

---

## 零分配 RFC 5545 重复日程规则 `RRULE` 解析与迭代触发调度器（`rrule_scheduler.cp`）

面向日历同步、定时提醒、周期计费与 AI Agent 长期日程编排的 100% 纯 CP 零动态堆分配 RFC 5545 `RRULE` 解析、规范化往返格式化与纯整数公历调度器：

- **常量与枚举**：
  - `RRULE_MAX_ITER_STEPS = 200_000`（单次查询防无限循环安全步数上限）
  - `RRuleFreq`（`daily | weekly | monthly | yearly`）、`RRuleWeekday`（`mo | tu | we | th | fr | sa | su`）
- **纯整数公历辅助函数（零浮点误差）**：
  - `RRuleDateTime`、`isLeapYear(year)`、`daysInMonth(year, month)`、`rruleDateTimeToUnix(dt)`、`rruleUnixToDateTime(ts)`、`rruleWeekdayOfUnix(ts)`、`parseRRuleUtcTimestamp(s)`（解析 `YYYYMMDDTHHMMSSZ`）
- **核心规则模型与调度 API**：
  - `parseRRule(input): RRule?`：支持 `FREQ`、`INTERVAL`、`COUNT`、`UNTIL`、`BYDAY`、`BYMONTHDAY`、`BYMONTH`、`BYHOUR`、`BYMINUTE`、`WKST`（兼容 `"RRULE:"` 前缀）；严格按 RFC 5545 拒绝缺失 `FREQ`、`INTERVAL=0`、`COUNT` 与 `UNTIL` 同时出现、重复键值与越界参数。
  - `formatRRule(rule, out_buf: *[]u8): usize?`：规范化往返输出标准 `RRULE` 字符串。
  - `RRule.nextOccurrence(dtstart, after_ts): i64?` / `rruleNextOccurrence`：精确计算下一次触发时间戳，完美处理闰年 2 月 29 日与 31 日跳过规则。
  - `RRule.expandOccurrences(dtstart, window_start, window_end, out_ts: *[]i64): usize` / `rruleExpandOccurrences`：在闭区间 `[window_start, window_end]` 批量展开触发点并严格遵守 `COUNT` / `UNTIL`。

### QuickStart 示例

```cp
import {
    RRuleDateTime,
    rruleDateTimeToUnix,
    parseRRule,
} from "cap:time";

pub fn demoRRuleScheduler() {
    const rule = parseRRule("FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE,FR;COUNT=4") ?? return;
    const dtstart = rruleDateTimeToUnix(RRuleDateTime{ year: 2026, month: 1, day: 5, hour: 9, minute: 0, second: 0 }) ?? return;

    let out: [8]i64 = [0; 8];
    let out_s: []i64 = out[0..];
    const n = rule.expandOccurrences(dtstart, dtstart, dtstart + 60 * 86400, *out_s);
    * = n; // 4 (2026-01-05 MO, 01-07 WE, 01-09 FR, 01-19 MO)
}
```

---

## 收敛子模块深度文档

- [health.md](time/health.md)
- [hlc.md](time/hlc.md)
- [interval.md](time/interval.md)
- [pool.md](time/pool.md)
- [pqueue.md](time/pqueue.md)
- [prometheus.md](time/prometheus.md)
- [promql.md](time/promql.md)
- [ptp.md](time/ptp.md)
- [queue.md](time/queue.md)
- [supervisor.md](time/supervisor.md)
- [timeseries.md](time/timeseries.md)
- [tween.md](time/tween.md)
- [tz.md](time/tz.md)
- [union_interval.md](time/union_interval.md)
- [uuid_v7.md](time/uuid_v7.md)
- [vcd.md](time/vcd.md)
