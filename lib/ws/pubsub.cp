// Source: Absorbed from /data/dev/bun/src/runtime/server/WebSocketServerContext.cp | Synced: 2026-11-22T06:46:43Z
// Pure-CP zero-allocation WebSocket Pub-Sub topic broker:
// subscribe / unsubscribe / unsubscribeAll (with empty-topic reclamation),
// `publish_to_self` semantics (`ws.publish` vs `server.publish`), subscriberCount, and topic listing.

/// Pub-Sub 代理支持的最大活跃主题数量
pub const WS_PUBSUB_MAX_TOPICS: usize = 64;

/// 单个主题支持的最大并发订阅者数量
pub const WS_PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC: usize = 64;

/// WebSocket Pub-Sub 错误集
pub err WsPubSubError {
    EmptyTopic,
    InvalidClientId,
    TopicTableFull,
    SubscriberTableFull,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// WebSocket Pub-Sub 配置（对齐 Bun `HandlerFlags.publish_to_self`）
pub struct WsPubSubConfig {
    /// `false`（默认）：`ws.publish(topic, msg)` 不回发给发送者自身；
    /// `true`：发送者若订阅了该主题也会收到消息。`server.publish` 传入 `sender_id = null` 始终全量投递。
    publish_to_self: bool = false,
}

/// 单次主题发布的目标收集结果
pub struct WsPublishResult {
    delivered_count: usize = 0,
    skipped_self: bool = false,
    topic_found: bool = false,
}

/// 单个主题的订阅者桶（固定容量零堆分配）
struct WsTopicBucket {
    topic: []const u8 = "",
    subscribers: [WS_PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC]u64 = [0; WS_PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC],
    len: usize = 0,

    fn findClient(client_id: u64): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.subscribers[i] == client_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn addClient(*_, client_id: u64): bool! {
        if (_.findClient(client_id) != null) {
            return false;
        }
        if (_.len >= WS_PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC) {
            return err.SubscriberTableFull;
        }
        _.subscribers[_.len] = client_id;
        _.len += 1;
        return true;
    }

    fn removeClient(*_, client_id: u64): bool {
        const idx = _.findClient(client_id) ?? return false;
        let i = idx;
        while (i + 1 < _.len) {
            _.subscribers[i] = _.subscribers[i + 1];
            i += 1;
        }
        _.len -= 1;
        _.subscribers[_.len] = 0;
        return true;
    }
}

/// 零堆分配 WebSocket 主题发布/订阅路由器（对齐 Bun `WebSocketServerContext` Pub-Sub 语义）
pub struct WsPubSubBroker {
    config: WsPubSubConfig = WsPubSubConfig{},
    topics: [WS_PUBSUB_MAX_TOPICS]WsTopicBucket = [WsTopicBucket{}; WS_PUBSUB_MAX_TOPICS],
    topic_len: usize = 0,
    total_subscriptions: usize = 0,

    static fn init(cfg: WsPubSubConfig): WsPubSubBroker {
        return WsPubSubBroker{ config: cfg };
    }

    fn topicCount(): usize {
        return _.topic_len;
    }

    fn findTopicIndex(topic: []const u8): usize? {
        let i: usize = 0;
        while (i < _.topic_len) {
            if (bytesEql(_.topics[i].topic, topic)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 查询指定主题当前订阅人数（对齐 Bun `server.subscriberCount(topic)`）
    fn subscriberCount(topic: []const u8): usize {
        const idx = _.findTopicIndex(topic) ?? return 0;
        return _.topics[idx].len;
    }

    /// 判断客户端是否已订阅指定主题（对齐 Bun `ws.isSubscribed(topic)`）
    fn isSubscribed(client_id: u64, topic: []const u8): bool {
        const idx = _.findTopicIndex(topic) ?? return false;
        return _.topics[idx].findClient(client_id) != null;
    }

    /// 订阅主题（幂等：新订阅返回 `true`，已订阅返回 `false`）
    fn subscribe(*_, client_id: u64, topic: []const u8): bool! {
        if (topic.len == 0) {
            return err.EmptyTopic;
        }
        if (client_id == 0) {
            return err.InvalidClientId;
        }

        if (idx := _.findTopicIndex(topic)) {
            const added = _.topics[idx].addClient(client_id)?;
            if (added) {
                _.total_subscriptions += 1;
            }
            return added;
        }

        if (_.topic_len >= WS_PUBSUB_MAX_TOPICS) {
            return err.TopicTableFull;
        }
        let bucket = WsTopicBucket{ topic: topic };
        * = bucket.addClient(client_id)?;
        _.topics[_.topic_len] = bucket;
        _.topic_len += 1;
        _.total_subscriptions += 1;
        return true;
    }

    fn reclaimTopicSlot(*_, idx: usize) {
        const last = _.topic_len - 1;
        if (idx < last) {
            _.topics[idx] = _.topics[last];
        }
        _.topics[last] = WsTopicBucket{};
        _.topic_len -= 1;
    }

    /// 取消订阅主题；若该主题已无剩余订阅者则自动回收主题槽位
    fn unsubscribe(*_, client_id: u64, topic: []const u8): bool {
        const idx = _.findTopicIndex(topic) ?? return false;
        if (!_.topics[idx].removeClient(client_id)) {
            return false;
        }
        if (_.total_subscriptions > 0) {
            _.total_subscriptions -= 1;
        }
        if (_.topics[idx].len == 0) {
            _.reclaimTopicSlot(idx);
        }
        return true;
    }

    /// 连接关闭时一键退订该客户端的所有主题并回收空主题，返回退订主题数
    fn unsubscribeAll(*_, client_id: u64): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < _.topic_len) {
            if (_.topics[i].removeClient(client_id)) {
                removed += 1;
                if (_.total_subscriptions > 0) {
                    _.total_subscriptions -= 1;
                }
                if (_.topics[i].len == 0) {
                    _.reclaimTopicSlot(i);
                    continue;
                }
            }
            i += 1;
        }
        return removed;
    }

    /// 收集指定主题的投递目标客户端 ID 列表
    ///
    /// - `sender_id == null`：等效 `server.publish(topic, msg)`，全量投递；
    /// - `sender_id != null`：等效 `ws.publish(topic, msg)`，当 `publish_to_self == false` 时自动跳过发送者。
    fn collectRecipients(topic: []const u8, sender_id: u64?, out_clients: []u64): WsPublishResult {
        const idx = _.findTopicIndex(topic) ?? return WsPublishResult{};
        const bucket = _.topics[idx];
        let res = WsPublishResult{ topic_found: true };
        let i: usize = 0;
        while (i < bucket.len) {
            const cid = bucket.subscribers[i];
            if (sid := sender_id) {
                if (sid == cid && !_.config.publish_to_self) {
                    res.skipped_self = true;
                    i += 1;
                    continue;
                }
            }
            if (res.delivered_count < out_clients.len) {
                out_clients[res.delivered_count] = cid;
                res.delivered_count += 1;
            }
            i += 1;
        }
        return res;
    }

    /// 列出指定客户端当前已订阅的所有主题名称（对齐 Bun `ws.subscriptions`）
    fn listClientTopics(client_id: u64, out_topics: [][]const u8): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.topic_len) {
            if (_.topics[i].findClient(client_id) != null) {
                if (count < out_topics.len) {
                    out_topics[count] = _.topics[i].topic;
                    count += 1;
                }
            }
            i += 1;
        }
        return count;
    }
}
