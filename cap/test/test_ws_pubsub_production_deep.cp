import { test, expect, expect_eq } from "cap:test";
import {
    WS_PUBSUB_MAX_TOPICS,
    WS_PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC,
    WsPubSubError,
    WsPubSubConfig,
    WsPublishResult,
    WsPubSubBroker,
} from "cap:ws";

fn strEql(a: []const u8, b: []const u8): bool {
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

test("ws pubsub: idempotent subscribe, subscriberCount, and isSubscribed", fn()! {
    expect_eq(WS_PUBSUB_MAX_TOPICS, 64);
    expect_eq(WS_PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC, 64);

    let broker = WsPubSubBroker.init(WsPubSubConfig{});
    expect_eq(broker.topicCount(), 0);
    expect_eq(broker.subscriberCount("chat"), 0);
    expect(!broker.isSubscribed(1, "chat"));

    expect(broker.subscribe(1, "chat")?);
    expect(!broker.subscribe(1, "chat")?); // idempotent duplicate
    expect(broker.subscribe(2, "chat")?);
    expect(broker.subscribe(1, "alerts")?);

    expect_eq(broker.topicCount(), 2);
    expect_eq(broker.subscriberCount("chat"), 2);
    expect_eq(broker.subscriberCount("alerts"), 1);
    expect_eq(broker.total_subscriptions, 3);
    expect(broker.isSubscribed(1, "chat"));
    expect(broker.isSubscribed(2, "chat"));
    expect(!broker.isSubscribed(2, "alerts"));
});

test("ws pubsub: publish_to_self false vs true and server broadcast (sender=null)", fn()! {
    let broker = WsPubSubBroker.init(WsPubSubConfig{ publish_to_self: false });
    * = broker.subscribe(10, "room-1")?;
    * = broker.subscribe(20, "room-1")?;
    * = broker.subscribe(30, "room-1")?;

    let recipients: [8]u64 = [0; 8];

    // ws.publish(sender=20): skips self
    const r_ws = broker.collectRecipients("room-1", 20, recipients[0..]);
    expect(r_ws.topic_found);
    expect(r_ws.skipped_self);
    expect_eq(r_ws.delivered_count, 2);
    expect_eq(recipients[0], 10);
    expect_eq(recipients[1], 30);

    // server.publish(sender=null): delivers to all 3
    const r_srv = broker.collectRecipients("room-1", null, recipients[0..]);
    expect(r_srv.topic_found);
    expect(!r_srv.skipped_self);
    expect_eq(r_srv.delivered_count, 3);
    expect_eq(recipients[0], 10);
    expect_eq(recipients[1], 20);
    expect_eq(recipients[2], 30);

    // publish_to_self = true: ws.publish includes sender
    let echo_broker = WsPubSubBroker.init(WsPubSubConfig{ publish_to_self: true });
    * = echo_broker.subscribe(10, "room-1")?;
    * = echo_broker.subscribe(20, "room-1")?;
    const r_echo = echo_broker.collectRecipients("room-1", 10, recipients[0..]);
    expect(r_echo.topic_found);
    expect(!r_echo.skipped_self);
    expect_eq(r_echo.delivered_count, 2);
});

test("ws pubsub: unsubscribe reclaims empty topic buckets cleanly", fn()! {
    let broker = WsPubSubBroker.init(WsPubSubConfig{});
    * = broker.subscribe(1, "t1")?;
    * = broker.subscribe(1, "t2")?;
    * = broker.subscribe(2, "t2")?;
    expect_eq(broker.topicCount(), 2);

    // Unsubscribing sole subscriber of t1 reclaims t1
    expect(broker.unsubscribe(1, "t1"));
    expect(!broker.unsubscribe(1, "t1")); // already removed
    expect_eq(broker.topicCount(), 1);
    expect_eq(broker.subscriberCount("t1"), 0);

    // t2 still intact with 2 subscribers
    expect_eq(broker.subscriberCount("t2"), 2);
    expect(broker.unsubscribe(1, "t2"));
    expect_eq(broker.subscriberCount("t2"), 1);
    expect(broker.unsubscribe(2, "t2"));
    expect_eq(broker.topicCount(), 0);
    expect_eq(broker.total_subscriptions, 0);
});

test("ws pubsub: unsubscribeAll cleans up client across all topics on disconnect", fn()! {
    let broker = WsPubSubBroker.init(WsPubSubConfig{});
    * = broker.subscribe(100, "solo-a")?;
    * = broker.subscribe(100, "shared")?;
    * = broker.subscribe(200, "shared")?;
    * = broker.subscribe(100, "solo-b")?;
    expect_eq(broker.topicCount(), 3);

    const removed = broker.unsubscribeAll(100);
    expect_eq(removed, 3);
    expect_eq(broker.topicCount(), 1);
    expect_eq(broker.subscriberCount("shared"), 1);
    expect(broker.isSubscribed(200, "shared"));
    expect(!broker.isSubscribed(100, "shared"));
    expect_eq(broker.total_subscriptions, 1);

    // Calling again on already-disconnected client returns 0
    expect_eq(broker.unsubscribeAll(100), 0);
});

test("ws pubsub: listClientTopics enumerates subscriptions and respects buffer cap", fn()! {
    let broker = WsPubSubBroker.init(WsPubSubConfig{});
    * = broker.subscribe(7, "news")?;
    * = broker.subscribe(7, "sports")?;
    * = broker.subscribe(7, "tech")?;
    * = broker.subscribe(8, "other")?;

    let topics: [4][]const u8 = [""; 4];
    const n = broker.listClientTopics(7, topics[0..]);
    expect_eq(n, 3);
    expect(strEql(topics[0], "news"));
    expect(strEql(topics[1], "sports"));
    expect(strEql(topics[2], "tech"));

    let small: [2][]const u8 = [""; 2];
    const n_small = broker.listClientTopics(7, small[0..]);
    expect_eq(n_small, 2);
});

test("ws pubsub: boundary guards for EmptyTopic, InvalidClientId, SubscriberTableFull, unknown topic", fn()! {
    let broker = WsPubSubBroker.init(WsPubSubConfig{});
    let errs: usize = 0;

    if (_ := broker.subscribe(1, "")) {} else { errs += 1; }
    if (_ := broker.subscribe(0, "valid")) {} else { errs += 1; }

    // Fill all 64 subscribers on one topic
    let cid: u64 = 1;
    while (cid <= 64) {
        * = broker.subscribe(cid, "crowded")?;
        cid += 1;
    }
    expect_eq(broker.subscriberCount("crowded"), 64);

    // 65th subscriber must fail with SubscriberTableFull
    if (_ := broker.subscribe(65, "crowded")) {} else { errs += 1; }
    expect_eq(errs, 3);

    // Unknown topic publish returns topic_found=false, delivered_count=0
    let out: [4]u64 = [0; 4];
    const miss = broker.collectRecipients("nonexistent", null, out[0..]);
    expect(!miss.topic_found);
    expect_eq(miss.delivered_count, 0);
});
