# CP 标准库：通配符主题消息代理引擎手册 (`cap:pubsub` / `cap.pubsub`)

## 一、 架构总览与设计准则

`cap:pubsub` 是 CP 语言官方内置的纯原生进程内通配符主题消息代理（In-Process Wildcard Pub/Sub Broker），专为百万级 AI Agent 本地主题扇出、NATS/MQTT 双协议通配符路由、有界背压队列控制与保留消息（Retained Messages）即时回放打造。

核心特性：
1. **双协议零分配通配符引擎（`TopicMatcher`）**：
   - **NATS 风格（`TopicStyle.Nats`）**：分隔符 `.`，单层通配符 `*`，尾段多层通配符 `>`（匹配 1 层及以上，如 `agent.>` 匹配 `agent.1` 与 `agent.1.log`，不匹配 `agent`）。
   - **MQTT 风格（`TopicStyle.Mqtt`）**：分隔符 `/`，单层通配符 `+`，尾段多层通配符 `#`（遵循 MQTT 语义匹配 0 层及以上含父级，如 `sensors/room1/#` 同时匹配 `sensors/room1` 与 `sensors/room1/temp`）。
   - **严格合法性拦截**：自动拦截空模式、空段（前导/尾随/连续分隔符）、半截通配符（如 `foo*`/`a+`）、非尾段多层通配符（`a.>.b` / `a/#/b`），以及发布主题中包含通配符的越界调用。
2. **有界背压队列（Bounded Backpressure）**：
   - 每个订阅者在 `subscribe(pattern, max_queue, drop_policy)` 时绑定独立有界 FIFO 队列，彻底杜绝慢消费者拖垮内存：
     - `DropPolicy.DropOldest`：队列满时淘汰最旧消息并接收新消息（适合状态快照/实时遥测）。
     - `DropPolicy.DropNewest`：队列满时丢弃新到达消息并保留存量消息（适合严格防抖/限流）。
   - 实时累计 `BrokerStats`（`published`、`delivered`、`dropped`、`active_subscriptions`、`retained_topics`）。
3. **保留消息与 MQTT 清空语义（Retained Messages）**：
   - `publishRetained(topic, payload)` 保存主题最新保留快照并同步投递给当前在线订阅者；新订阅者在 `subscribe` 时若匹配已有保留消息，立即自动投递（`retained = true`）。
   - 发布空载荷 `publishRetained(topic, "")` 遵循 MQTT 协议语义：向当前在线订阅者投递空载荷并立即清除该主题的保留快照。
4. **O(1) 订阅索引与所有权安全**：
   - 内部通过哈希索引表 + 紧凑数组实现 `subscribe`、`unsubscribe`、`poll`、`pendingCount` 均摊 $O(1)$ 操作；`BrokerMessage` 独立持有 `topic`/`payload` 内存，调用方消费完毕后调用 `msg.deinit()` 确定性释放。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:pubsub"` 与 `import cap from "cap"; cap.pubsub.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/pubsub/topic.cp` | `// Source: cp/lib/pubsub/topic.cp (NATS & MQTT Wildcard Topic Matching Engine)` | `TopicStyle`、`TopicMatcher` |
| `lib/pubsub/broker.cp` | `// Source: cp/lib/pubsub/broker.cp (In-Process Wildcard Pub/Sub Broker with Backpressure & Retained Messages)` | `PubSubError`、`DropPolicy`、`BrokerMessage`、`BrokerStats`、`PubSubBroker` |
| `lib/pubsub/index.cp` | `// Source: cp/lib/pubsub/index.cp (Unified Exports for cap:pubsub)` | `cap:pubsub` 与 `cap.pubsub` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 主题通配符匹配引擎（`topic.cp`）

- **`TopicStyle: u8`**：`Nats = 0`、`Mqtt = 1`
- **`TopicMatcher`**：
  - `static isValidPattern(style: TopicStyle, pattern: string): bool` — 校验订阅模式合法性（拦截空串、空段、非单段通配符及非尾段 `>`/`#`）。
  - `static isValidPublishTopic(style: TopicStyle, topic: string): bool` — 校验发布主题合法性（拒绝空串、空段及任何通配符字符）。
  - `static matches(style: TopicStyle, pattern: string, topic: string): bool` — 零分配判断 `pattern` 是否匹配 `topic`（非法模式或含通配符发布主题一律返回 `false`）。

### 2. 消息代理、背压策略与统计（`broker.cp`）

- **`PubSubError`**：`InvalidPattern`、`InvalidTopic`、`SubscriptionNotFound`、`OutOfMemory`
- **`DropPolicy: u8`**：`DropOldest = 0`、`DropNewest = 1`
- **`BrokerMessage`**：
  - 字段：`seq: u64`、`topic: string`、`payload: string`、`retained: bool`
  - 方法：`deinit(*_)` — 幂等释放消息持有的 `topic` 与 `payload` 内存
- **`BrokerStats`**：
  - 字段：`published: u64`、`delivered: u64`、`dropped: u64`、`active_subscriptions: usize`、`retained_topics: usize`
- **`PubSubBroker`**：
  - 构造与销毁：
    - `static init(alloc: std.mem.Allocator, style: TopicStyle): PubSubBroker`
    - `deinit(*_)`
  - 订阅管理：
    - `subscribe(*_, pattern: string, max_queue: usize, drop_policy: DropPolicy): u64 ! PubSubError`（返回唯一 `sub_id`；自动投递匹配的保留消息；`max_queue == 0` 时所有消息均被背压丢弃）
    - `unsubscribe(*_, sub_id: u64): bool`
  - 发布与保留消息：
    - `publish(*_, topic: string, payload: string): usize ! PubSubError`（返回成功入队的订阅数）
    - `publishRetained(*_, topic: string, payload: string): usize ! PubSubError`（非空更新保留消息；空 `payload` 清除该主题保留消息并投递在线订阅者）
  - 消费与观测：
    - `poll(*_, sub_id: u64): BrokerMessage?`
    - `pendingCount(sub_id: u64): usize`
    - `stats(): BrokerStats`

---

## 三、 使用示例

```typescript
import cap from "cap";
import { TopicStyle, DropPolicy, PubSubBroker } from "cap:pubsub";

const std = @import("std");

pub fn demoPubSub(): ! {
    const alloc = std.heap.page_allocator;
    let broker = PubSubBroker.init(alloc, TopicStyle.Nats);
    defer broker.deinit();

    // 1. 发布保留消息（尚无订阅者，返回投递数 0）
    * = broker.publishRetained("agent.planner.state", "ready")?;

    // 2. 通配符订阅：立即自动收到匹配的保留消息 (retained == true)
    const sub_id = broker.subscribe("agent.*.state", 64, DropPolicy.DropOldest)?;
    if (msg := broker.poll(sub_id)) {
        let owned = msg;
        defer owned.deinit();
        // owned.topic == "agent.planner.state", owned.payload == "ready", owned.retained == true
    }

    // 3. 普通扇出发布
    const delivered = broker.publish("agent.worker.state", "running")?;
    * = delivered;

    // 4. cap.pubsub 动态挂载等价用法
    let mqtt_broker = cap.pubsub.PubSubBroker.init(alloc, cap.pubsub.TopicStyle.Mqtt);
    defer mqtt_broker.deinit();
    const msub = mqtt_broker.subscribe("sensors/#", 16, cap.pubsub.DropPolicy.DropNewest)?;
    * = mqtt_broker.publish("sensors/rack1/temp", "41.5")?;
    * = mqtt_broker.unsubscribe(msub);
}
```
