// Source: cp/cap/test/test_pubsub_production_deep.cp
// Deep Production Verification Suite for cap:pubsub & cap.pubsub
// Covers:
//   1. NATS (* / >) & MQTT (+ / #) wildcard matching and malformed pattern / wildcard publish rejection
//   2. Multi-subscriber fan-out, per-subscriber queues, and unsubscribe isolation
//   3. Bounded backpressure DropOldest vs DropNewest policies and dropped statistics
//   4. publishRetained immediate replay on new subscription and empty-payload clear semantics
//   5. `import cap from "cap"; cap.pubsub` automatic dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TopicStyle,
    TopicMatcher,
    PubSubError,
    DropPolicy,
    BrokerMessage,
    BrokerStats,
    PubSubBroker,
} from "cap:sys";

fn testNatsAndMqttWildcardMatching(): !void {
    // 1. NATS matching: `.` separator, `*` single-level, `>` tail multi-level (1+ levels)
    expect_eq(TopicMatcher.matches(.Nats, "agent.1.status", "agent.1.status"), true);
    expect_eq(TopicMatcher.matches(.Nats, "agent.*.status", "agent.1.status"), true);
    expect_eq(TopicMatcher.matches(.Nats, "agent.*.status", "agent.1.2.status"), false);
    expect_eq(TopicMatcher.matches(.Nats, "agent.*.>", "agent.1.cpu.load"), true);
    expect_eq(TopicMatcher.matches(.Nats, "agent.>", "agent"), false);
    expect_eq(TopicMatcher.matches(.Nats, "agent.>", "agent.1"), true);
    expect_eq(TopicMatcher.matches(.Nats, "agent.>", "agent.1.2.3"), true);
    expect_eq(TopicMatcher.matches(.Nats, ">", "heartbeat"), true);
    expect_eq(TopicMatcher.matches(.Nats, ">", "a.b.c"), true);
    expect_eq(TopicMatcher.matches(.Nats, "*", "heartbeat"), true);
    expect_eq(TopicMatcher.matches(.Nats, "*", "a.b"), false);

    // 2. MQTT matching: `/` separator, `+` single-level, `#` tail multi-level (0+ levels, parent inclusive)
    expect_eq(TopicMatcher.matches(.Mqtt, "sensors/room1/temp", "sensors/room1/temp"), true);
    expect_eq(TopicMatcher.matches(.Mqtt, "sensors/+/temp", "sensors/room1/temp"), true);
    expect_eq(TopicMatcher.matches(.Mqtt, "sensors/+/temp", "sensors/room1/floor2/temp"), false);
    expect_eq(TopicMatcher.matches(.Mqtt, "+/+/+", "a/b/c"), true);
    expect_eq(TopicMatcher.matches(.Mqtt, "sensors/#", "sensors"), true);
    expect_eq(TopicMatcher.matches(.Mqtt, "sensors/#", "sensors/room1"), true);
    expect_eq(TopicMatcher.matches(.Mqtt, "sensors/#", "sensors/room1/temp"), true);
    expect_eq(TopicMatcher.matches(.Mqtt, "#", "any/topic/here"), true);
    expect_eq(TopicMatcher.matches(.Mqtt, "sensors/+", "sensors"), false);

    // 3. Invalid patterns (empty, empty segments, partial wildcards, non-trailing `>` / `#`)
    const bad_nats_patterns: [9]string = [
        "",
        ".",
        ".agent",
        "agent.",
        "agent..status",
        "agent.>.status",
        "agent.st*",
        "agent.>x",
        "agent.**",
    ];
    for (idx of 0..bad_nats_patterns.len) {
        expect_eq(TopicMatcher.isValidPattern(.Nats, bad_nats_patterns[idx]), false);
        expect_eq(TopicMatcher.matches(.Nats, bad_nats_patterns[idx], "agent.1.status"), false);
    }

    const bad_mqtt_patterns: [9]string = [
        "",
        "/",
        "/sensors",
        "sensors/",
        "sensors//temp",
        "sensors/#/temp",
        "sensors/+a",
        "sensors/a#",
        "sensors/++",
    ];
    for (idx of 0..bad_mqtt_patterns.len) {
        expect_eq(TopicMatcher.isValidPattern(.Mqtt, bad_mqtt_patterns[idx]), false);
        expect_eq(TopicMatcher.matches(.Mqtt, bad_mqtt_patterns[idx], "sensors/room1/temp"), false);
    }

    // 4. Invalid publish topics (wildcards or empty segments forbidden)
    const bad_nats_topics: [6]string = ["", ".a", "a.", "a..b", "a.*", "a.>"];
    for (idx of 0..bad_nats_topics.len) {
        expect_eq(TopicMatcher.isValidPublishTopic(.Nats, bad_nats_topics[idx]), false);
        expect_eq(TopicMatcher.matches(.Nats, ">", bad_nats_topics[idx]), false);
    }

    const bad_mqtt_topics: [6]string = ["", "/a", "a/", "a//b", "a/+", "a/#"];
    for (idx of 0..bad_mqtt_topics.len) {
        expect_eq(TopicMatcher.isValidPublishTopic(.Mqtt, bad_mqtt_topics[idx]), false);
        expect_eq(TopicMatcher.matches(.Mqtt, "#", bad_mqtt_topics[idx]), false);
    }

    // 5. Broker rejects invalid pattern on subscribe and wildcard topic on publish
    const alloc = std.heap.page_allocator;
    let broker = PubSubBroker.init(alloc, .Nats);
    defer broker.deinit();

    let hit_invalid_pat = false;
    * = broker.subscribe("agent.>.bad", 8, .DropOldest) catch |e| {
        if (e == PubSubError.InvalidPattern) hit_invalid_pat = true;
    };
    expect_eq(hit_invalid_pat, true);

    let hit_invalid_pub = false;
    * = broker.publish("agent.*.bad", "x") catch |e| {
        if (e == PubSubError.InvalidTopic) hit_invalid_pub = true;
    };
    expect_eq(hit_invalid_pub, true);
}

fn testMultiSubscriberFanoutAndUnsubscribe(): !void {
    const alloc = std.heap.page_allocator;
    let broker = PubSubBroker.init(alloc, .Nats);
    defer broker.deinit();

    const sub_exact = broker.subscribe("orders.created", 16, .DropOldest)?;
    const sub_single = broker.subscribe("orders.*", 16, .DropOldest)?;
    const sub_all = broker.subscribe(">", 16, .DropOldest)?;
    const sub_other = broker.subscribe("payments.>", 16, .DropOldest)?;

    expect_eq(broker.stats().active_subscriptions, 4 as usize);

    const delivered_1 = broker.publish("orders.created", "ord_101")?;
    expect_eq(delivered_1, 3 as usize);
    expect_eq(broker.pendingCount(sub_exact), 1 as usize);
    expect_eq(broker.pendingCount(sub_single), 1 as usize);
    expect_eq(broker.pendingCount(sub_all), 1 as usize);
    expect_eq(broker.pendingCount(sub_other), 0 as usize);

    if (m1 := broker.poll(sub_exact)) {
        let owned = m1;
        defer owned.deinit();
        expect_eq(owned.seq, 1 as u64);
        expect_eq(std.mem.eql(u8, owned.topic, "orders.created"), true);
        expect_eq(std.mem.eql(u8, owned.payload, "ord_101"), true);
        expect_eq(owned.retained, false);
    } else {
        expect_eq(true, false);
    }
    expect_eq(broker.pendingCount(sub_exact), 0 as usize);
    expect_eq(broker.poll(sub_exact) == null, true);

    // Unsubscribe `sub_single` while leaving pending message inside to verify cleanup
    expect_eq(broker.unsubscribe(sub_single), true);
    expect_eq(broker.unsubscribe(sub_single), false);
    expect_eq(broker.pendingCount(sub_single), 0 as usize);
    expect_eq(broker.poll(sub_single) == null, true);
    expect_eq(broker.stats().active_subscriptions, 3 as usize);

    // Next publish fans out to remaining matching subs only (`sub_exact` + `sub_all`)
    const delivered_2 = broker.publish("orders.created", "ord_102")?;
    expect_eq(delivered_2, 2 as usize);
    expect_eq(broker.pendingCount(sub_exact), 1 as usize);
    expect_eq(broker.pendingCount(sub_all), 2 as usize);

    // Verify `sub_all` preserves FIFO order across both publishes
    if (a1 := broker.poll(sub_all)) {
        let owned = a1;
        defer owned.deinit();
        expect_eq(std.mem.eql(u8, owned.payload, "ord_101"), true);
    } else {
        expect_eq(true, false);
    }
    if (a2 := broker.poll(sub_all)) {
        let owned = a2;
        defer owned.deinit();
        expect_eq(std.mem.eql(u8, owned.payload, "ord_102"), true);
    } else {
        expect_eq(true, false);
    }
}

fn testBoundedBackpressurePolicies(): !void {
    const alloc = std.heap.page_allocator;
    let broker = PubSubBroker.init(alloc, .Mqtt);
    defer broker.deinit();

    const sub_old = broker.subscribe("telemetry/+", 2, .DropOldest)?;
    const sub_new = broker.subscribe("telemetry/+", 2, .DropNewest)?;
    const sub_zero = broker.subscribe("telemetry/+", 0, .DropOldest)?;

    // Publish 4 messages to capacity-2 queues (and capacity-0 queue)
    // m1: sub_old=[m1], sub_new=[m1], sub_zero=drop -> 2 delivered, 1 dropped
    // m2: sub_old=[m1,m2], sub_new=[m1,m2], sub_zero=drop -> 2 delivered, 1 dropped
    // m3: sub_old=[m2,m3] (drop m1), sub_new=[m1,m2] (drop m3), sub_zero=drop -> 1 delivered, 3 dropped
    // m4: sub_old=[m3,m4] (drop m2), sub_new=[m1,m2] (drop m4), sub_zero=drop -> 1 delivered, 3 dropped
    expect_eq(broker.publish("telemetry/gpu0", "m1")?, 2 as usize);
    expect_eq(broker.publish("telemetry/gpu0", "m2")?, 2 as usize);
    expect_eq(broker.publish("telemetry/gpu0", "m3")?, 1 as usize);
    expect_eq(broker.publish("telemetry/gpu0", "m4")?, 1 as usize);

    expect_eq(broker.pendingCount(sub_old), 2 as usize);
    expect_eq(broker.pendingCount(sub_new), 2 as usize);
    expect_eq(broker.pendingCount(sub_zero), 0 as usize);

    const st: BrokerStats = broker.stats();
    expect_eq(st.published, 4 as u64);
    expect_eq(st.delivered, 6 as u64);
    expect_eq(st.dropped, 8 as u64);

    // DropOldest retained newest two: m3, m4
    if (o1 := broker.poll(sub_old)) {
        let owned = o1;
        defer owned.deinit();
        expect_eq(std.mem.eql(u8, owned.payload, "m3"), true);
    } else {
        expect_eq(true, false);
    }
    if (o2 := broker.poll(sub_old)) {
        let owned = o2;
        defer owned.deinit();
        expect_eq(std.mem.eql(u8, owned.payload, "m4"), true);
    } else {
        expect_eq(true, false);
    }

    // DropNewest retained oldest two: m1, m2
    if (n1 := broker.poll(sub_new)) {
        let owned = n1;
        defer owned.deinit();
        expect_eq(std.mem.eql(u8, owned.payload, "m1"), true);
    } else {
        expect_eq(true, false);
    }
    if (n2 := broker.poll(sub_new)) {
        let owned = n2;
        defer owned.deinit();
        expect_eq(std.mem.eql(u8, owned.payload, "m2"), true);
    } else {
        expect_eq(true, false);
    }
}

fn testRetainedMessagesAndEmptyPayloadClear(): !void {
    const alloc = std.heap.page_allocator;
    let broker = PubSubBroker.init(alloc, .Mqtt);
    defer broker.deinit();

    // 1. Publish retained message before any subscriber exists
    expect_eq(broker.publishRetained("cluster/leader", "node-alpha")?, 0 as usize);
    expect_eq(broker.publishRetained("cluster/health", "green")?, 0 as usize);
    expect_eq(broker.stats().retained_topics, 2 as usize);

    // 2. Late subscriber immediately receives matching retained message with `retained == true`
    const sub_leader = broker.subscribe("cluster/leader/#", 8, .DropOldest)?;
    expect_eq(broker.pendingCount(sub_leader), 1 as usize);
    if (m := broker.poll(sub_leader)) {
        let owned = m;
        defer owned.deinit();
        expect_eq(std.mem.eql(u8, owned.topic, "cluster/leader"), true);
        expect_eq(std.mem.eql(u8, owned.payload, "node-alpha"), true);
        expect_eq(owned.retained, true);
    } else {
        expect_eq(true, false);
    }

    // 3. Overwrite retained message: live sub receives live update (`retained == false`), new sub receives updated snapshot (`retained == true`)
    expect_eq(broker.publishRetained("cluster/leader", "node-beta")?, 1 as usize);
    expect_eq(broker.stats().retained_topics, 2 as usize);
    if (live := broker.poll(sub_leader)) {
        let owned = live;
        defer owned.deinit();
        expect_eq(std.mem.eql(u8, owned.payload, "node-beta"), true);
        expect_eq(owned.retained, false);
    } else {
        expect_eq(true, false);
    }

    const sub_all_late = broker.subscribe("cluster/#", 8, .DropOldest)?;
    expect_eq(broker.pendingCount(sub_all_late), 2 as usize);
    let saw_leader_beta = false;
    let saw_health_green = false;
    while (msg := broker.poll(sub_all_late)) {
        let owned = msg;
        defer owned.deinit();
        expect_eq(owned.retained, true);
        if (std.mem.eql(u8, owned.topic, "cluster/leader") && std.mem.eql(u8, owned.payload, "node-beta")) {
            saw_leader_beta = true;
        }
        if (std.mem.eql(u8, owned.topic, "cluster/health") && std.mem.eql(u8, owned.payload, "green")) {
            saw_health_green = true;
        }
    }
    expect_eq(saw_leader_beta, true);
    expect_eq(saw_health_green, true);

    // 4. Empty payload clears retained message per MQTT semantics while delivering live notification
    expect_eq(broker.publishRetained("cluster/leader", "")?, 2 as usize);
    expect_eq(broker.stats().retained_topics, 1 as usize);

    // Existing subscribers got the live empty-payload publish
    if (clr := broker.poll(sub_leader)) {
        let owned = clr;
        defer owned.deinit();
        expect_eq(owned.payload.len, 0 as usize);
        expect_eq(owned.retained, false);
    } else {
        expect_eq(true, false);
    }

    // Future subscriber to `cluster/leader` gets nothing (retained was cleared!)
    const sub_after_clear = broker.subscribe("cluster/leader", 8, .DropOldest)?;
    expect_eq(broker.pendingCount(sub_after_clear), 0 as usize);
    expect_eq(broker.poll(sub_after_clear) == null, true);
}

fn testCapPubsubDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    // 1. cap.sys.TopicMatcher & cap.sys.TopicStyle
    expect_eq(cap.sys.TopicMatcher.matches(cap.sys.TopicStyle.Nats, "ai.agent.*", "ai.agent.42"), true);
    expect_eq(cap.sys.TopicMatcher.isValidPublishTopic(cap.sys.TopicStyle.Nats, "ai.agent.*"), false);

    // 2. cap.sys.PubSubBroker + DropPolicy + BrokerMessage + BrokerStats + PubSubError
    let broker = cap.sys.PubSubBroker.init(alloc, cap.sys.TopicStyle.Nats);
    defer broker.deinit();

    * = broker.publishRetained("ai.boot.ready", "ok")?;
    const sid = broker.subscribe("ai.boot.>", 4, cap.sys.DropPolicy.DropOldest)?;
    if (msg := broker.poll(sid)) {
        let owned: cap.sys.BrokerMessage = msg;
        defer owned.deinit();
        expect_eq(owned.retained, true);
        expect_eq(std.mem.eql(u8, owned.payload, "ok"), true);
    } else {
        expect_eq(true, false);
    }

    let caught_err = false;
    * = broker.subscribe("ai..invalid", 4, cap.sys.DropPolicy.DropNewest) catch |e| {
        if (e == cap.sys.PubSubError.InvalidPattern) caught_err = true;
    };
    expect_eq(caught_err, true);

    const st: cap.sys.BrokerStats = broker.stats();
    expect_eq(st.active_subscriptions, 1 as usize);
    expect_eq(st.retained_topics, 1 as usize);
    expect_eq(broker.unsubscribe(sid), true);
}

fn main() {
    test("1. NATS (* / >) & MQTT (+ / #) wildcard matching and malformed input rejection", () => {
        testNatsAndMqttWildcardMatching()?;
    });
    test("2. Multi-subscriber fan-out, per-subscriber queues & unsubscribe isolation", () => {
        testMultiSubscriberFanoutAndUnsubscribe()?;
    });
    test("3. Bounded backpressure DropOldest vs DropNewest and dropped stats", () => {
        testBoundedBackpressurePolicies()?;
    });
    test("4. publishRetained replay on subscribe and empty-payload clear semantics", () => {
        testRetainedMessagesAndEmptyPayloadClear()?;
    });
    test("5. import cap from \"cap\"; cap.pubsub Automatic Dynamic Mounting", () => {
        testCapPubsubDynamicMount()?;
    });
}
