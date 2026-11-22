// Source: Absorbed from /data/dev/bun/src/valkey/valkey_protocol.cp | Synced: 2026-11-22T12:49:17Z
// lib/redis/protocol.cp
// Low-level RESP2 / RESP3 Protocol Types & Verbatim String Slice View in pure CP

pub struct VerbatimString {
    pub format: []const u8 = "txt";
    pub content: []const u8 = "";

    pub static fn parse(raw: []const u8): VerbatimString? {
        if (raw.len < 4 || raw[3] != ':') {
            return null;
        }
        return VerbatimString{
            format: raw[0..3],
            content: raw[4..raw.len],
        };
    }
}

pub enum SubscriptionPushMessage: u8 {
    Message = 0,
    Subscribe = 1,
    Unsubscribe = 2,
    PMessage = 3,
    PSubscribe = 4,
    PUnsubscribe = 5,

    pub static fn fromString(s: []const u8): SubscriptionPushMessage? {
        if (s == "message") return SubscriptionPushMessage.Message;
        if (s == "subscribe") return SubscriptionPushMessage.Subscribe;
        if (s == "unsubscribe") return SubscriptionPushMessage.Unsubscribe;
        if (s == "pmessage") return SubscriptionPushMessage.PMessage;
        if (s == "psubscribe") return SubscriptionPushMessage.PSubscribe;
        if (s == "punsubscribe") return SubscriptionPushMessage.PUnsubscribe;
        return null;
    }
}
