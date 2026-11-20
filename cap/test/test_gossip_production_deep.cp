// Source: cp/cap/test/test_gossip_production_deep.cp (Deep tests for cap:gossip & cap.gossip)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GossipError,
    MemberState,
    GossipUpdate,
    SwimConfig,
    MessageKind,
    GossipMessage,
    OutboundMessage,
    SwimClusterNode,
    shouldSupersede,
    encodeMessage,
    decodeMessage,
    freeMessage,
    freeOutbound,
} from "cap:gossip";

fn deliverBatch(
    alloc: std.mem.Allocator,
    nodes: []*SwimClusterNode,
    ids: []const string,
    batch: []OutboundMessage,
    now_ms: u64,
    drop_id: string,
) -> GossipError!void {
    let queue = std.ArrayList(OutboundMessage).empty;
    defer {
        for (ob of queue.items) {
            alloc.free(ob.target_id);
            alloc.free(ob.target_addr);
            let m_mut = ob.msg;
            freeMessage(alloc, &m_mut);
        }
        queue.deinit(alloc);
    }

    for (ob of batch) {
        if (drop_id.len > 0 and std.mem.eql(u8, ob.target_id, drop_id)) {
            continue;
        }
        let target_idx: usize? = null;
        let i: usize = 0;
        while (i < ids.len) {
            if (std.mem.eql(u8, ids[i], ob.target_id)) {
                target_idx = i;
                break;
            }
            i += 1;
        }
        if (tidx := target_idx) {
            const resp = try nodes[tidx].handleMessage(ob.msg.sender_id, ob.msg, now_ms);
            for (r of resp) {
                queue.append(alloc, r) catch return GossipError.OutOfMemory;
            }
            alloc.free(resp);
        }
    }

    // Deliver second hop (e.g. Ack / Ping from PingReq)
    for (ob of queue.items) {
        if (drop_id.len > 0 and std.mem.eql(u8, ob.target_id, drop_id)) {
            continue;
        }
        let i: usize = 0;
        while (i < ids.len) {
            if (std.mem.eql(u8, ids[i], ob.target_id)) {
                const resp2 = try nodes[i].handleMessage(ob.msg.sender_id, ob.msg, now_ms);
                freeOutbound(alloc, resp2);
                break;
            }
            i += 1;
        }
    }
}

test("swim 4-node cluster joinSeed + tick + Ping/Ack + piggyback convergence") {
    const alloc = std.testing.allocator;
    const cfg = SwimConfig.default();

    let n1 = try SwimClusterNode.init(alloc, "n1", "10.0.0.1:7946", cfg);
    defer n1.deinit();
    let n2 = try SwimClusterNode.init(alloc, "n2", "10.0.0.2:7946", cfg);
    defer n2.deinit();
    let n3 = try SwimClusterNode.init(alloc, "n3", "10.0.0.3:7946", cfg);
    defer n3.deinit();
    let n4 = try SwimClusterNode.init(alloc, "n4", "10.0.0.4:7946", cfg);
    defer n4.deinit();

    let nodes = std.ArrayList(*SwimClusterNode).empty;
    defer nodes.deinit(alloc);
    try nodes.append(alloc, &n1);
    try nodes.append(alloc, &n2);
    try nodes.append(alloc, &n3);
    try nodes.append(alloc, &n4);

    let ids = std.ArrayList(string).empty;
    defer ids.deinit(alloc);
    try ids.append(alloc, "n1");
    try ids.append(alloc, "n2");
    try ids.append(alloc, "n3");
    try ids.append(alloc, "n4");

    // Join n2, n3, n4 to seed n1
    const j2 = try n2.joinSeed("n1", "10.0.0.1:7946", 100);
    try deliverBatch(alloc, nodes.items, ids.items, j2, 100, "");
    freeOutbound(alloc, j2);

    const j3 = try n3.joinSeed("n1", "10.0.0.1:7946", 110);
    try deliverBatch(alloc, nodes.items, ids.items, j3, 110, "");
    freeOutbound(alloc, j3);

    const j4 = try n4.joinSeed("n1", "10.0.0.1:7946", 120);
    try deliverBatch(alloc, nodes.items, ids.items, j4, 120, "");
    freeOutbound(alloc, j4);

    // Run several gossip rounds so piggyback converges across all 4 nodes
    let t: u64 = 1200;
    let round: usize = 0;
    while (round < 8) {
        let ni: usize = 0;
        while (ni < nodes.items.len) {
            const out = try nodes.items[ni].tick(t);
            try deliverBatch(alloc, nodes.items, ids.items, out, t, "");
            freeOutbound(alloc, out);
            ni += 1;
        }
        t += 1100;
        round += 1;
    }

    for (nd of nodes.items) {
        const active = try nd.activeMembers(alloc);
        defer alloc.free(active);
        expect_eq(active.len, 4);
    }
}

test("swim failure detection: Ping timeout -> PingReq -> Suspect -> Offline") {
    const alloc = std.testing.allocator;
    const cfg = SwimConfig{
        .protocol_period_ms = 1000,
        .ack_timeout_ms = 500,
        .suspect_timeout_ms = 2000,
        .indirect_ping_count = 2,
        .max_piggyback = 8,
        .retransmit_mult = 4,
    };

    let n1 = try SwimClusterNode.init(alloc, "n1", "10.0.0.1:7946", cfg);
    defer n1.deinit();
    let n2 = try SwimClusterNode.init(alloc, "n2", "10.0.0.2:7946", cfg);
    defer n2.deinit();
    let n3 = try SwimClusterNode.init(alloc, "n3", "10.0.0.3:7946", cfg);
    defer n3.deinit();

    let nodes = std.ArrayList(*SwimClusterNode).empty;
    defer nodes.deinit(alloc);
    try nodes.append(alloc, &n1);
    try nodes.append(alloc, &n2);
    try nodes.append(alloc, &n3);

    let ids = std.ArrayList(string).empty;
    defer ids.deinit(alloc);
    try ids.append(alloc, "n1");
    try ids.append(alloc, "n2");
    try ids.append(alloc, "n3");

    const j2 = try n2.joinSeed("n1", "10.0.0.1:7946", 10);
    try deliverBatch(alloc, nodes.items, ids.items, j2, 10, "");
    freeOutbound(alloc, j2);

    const j3 = try n3.joinSeed("n1", "10.0.0.1:7946", 20);
    try deliverBatch(alloc, nodes.items, ids.items, j3, 20, "");
    freeOutbound(alloc, j3);

    // n3 goes unreachable; probe until n1 probes n3
    let now: u64 = 1000;
    let saw_pingreq = false;
    let saw_suspect = false;

    let step: usize = 0;
    while (step < 12) {
        const out = try n1.tick(now);
        for (ob of out) {
            if (ob.msg.kind == MessageKind.PingReq and std.mem.eql(u8, ob.msg.target_id, "n3")) {
                saw_pingreq = true;
            }
        }
        try deliverBatch(alloc, nodes.items, ids.items, out, now, "n3");
        freeOutbound(alloc, out);

        if (m := n1.getMember("n3")) {
            if (m.state == MemberState.Suspect) {
                saw_suspect = true;
                break;
            }
        }
        now += 600;
        step += 1;
    }

    expect_eq(saw_pingreq, true);
    expect_eq(saw_suspect, true);

    // Advance past suspect_timeout_ms (2000ms) -> Offline
    now += 2500;
    const out_off = try n1.tick(now);
    freeOutbound(alloc, out_off);

    const m3 = n1.getMember("n3").?;
    expect_eq(m3.state, MemberState.Offline);
}

test("swim refutation: Suspect notice triggers incarnation bump and Active broadcast") {
    const alloc = std.testing.allocator;
    const cfg = SwimConfig.default();

    let n1 = try SwimClusterNode.init(alloc, "n1", "10.0.0.1:7946", cfg);
    defer n1.deinit();
    let n2 = try SwimClusterNode.init(alloc, "n2", "10.0.0.2:7946", cfg);
    defer n2.deinit();

    const j2 = try n2.joinSeed("n1", "10.0.0.1:7946", 10);
    for (ob of j2) {
        const r = try n1.handleMessage(ob.msg.sender_id, ob.msg, 10);
        freeOutbound(alloc, r);
    }
    freeOutbound(alloc, j2);

    // Someone falsely suspects n2 at incarnation 2
    let suspect_upd = std.ArrayList(GossipUpdate).empty;
    defer suspect_upd.deinit(alloc);
    try suspect_upd.append(alloc, GossipUpdate{
        .node_id = "n2",
        .addr = "10.0.0.2:7946",
        .state = MemberState.Suspect,
        .incarnation = 2,
    });

    const false_msg = GossipMessage{
        .kind = MessageKind.Ping,
        .seq = 99,
        .sender_id = "n1",
        .sender_addr = "10.0.0.1:7946",
        .target_id = "n2",
        .target_addr = "10.0.0.2:7946",
        .updates = suspect_upd.items,
    };

    // n1 also records suspect inc=2 locally
    const r1 = try n1.handleMessage("n3", GossipMessage{
        .kind = MessageKind.Sync,
        .seq = 98,
        .sender_id = "n1",
        .sender_addr = "10.0.0.1:7946",
        .target_id = "n1",
        .target_addr = "10.0.0.1:7946",
        .updates = suspect_upd.items,
    }, 50);
    freeOutbound(alloc, r1);
    expect_eq(n1.getMember("n2").?.state, MemberState.Suspect);

    // n2 receives the false suspect -> bumps incarnation to 3 and replies with Active inc=3
    const r2 = try n2.handleMessage("n1", false_msg, 60);
    defer freeOutbound(alloc, r2);
    expect_eq(n2.selfIncarnation(), 3);

    for (ob of r2) {
        const r3 = try n1.handleMessage(ob.msg.sender_id, ob.msg, 70);
        freeOutbound(alloc, r3);
    }

    const m2_on_n1 = n1.getMember("n2").?;
    expect_eq(m2_on_n1.state, MemberState.Active);
    expect_eq(m2_on_n1.incarnation, 3);
}

test("swim wire encodeMessage/decodeMessage roundtrip and malformed/truncated rejection") {
    const alloc = std.testing.allocator;

    let upds = std.ArrayList(GossipUpdate).empty;
    defer upds.deinit(alloc);
    try upds.append(alloc, GossipUpdate{
        .node_id = "node-a",
        .addr = "127.0.0.1:7001",
        .state = MemberState.Active,
        .incarnation = 42,
    });
    try upds.append(alloc, GossipUpdate{
        .node_id = "node-b",
        .addr = "127.0.0.1:7002",
        .state = MemberState.Suspect,
        .incarnation = 7,
    });

    const orig = GossipMessage{
        .kind = MessageKind.PingReq,
        .seq = 123456789,
        .sender_id = "node-x",
        .sender_addr = "127.0.0.1:7000",
        .target_id = "node-b",
        .target_addr = "127.0.0.1:7002",
        .updates = upds.items,
    };

    const bytes = try encodeMessage(alloc, orig);
    defer alloc.free(bytes);

    let decoded = try decodeMessage(alloc, bytes);
    defer freeMessage(alloc, &decoded);

    expect_eq(decoded.kind, MessageKind.PingReq);
    expect_eq(decoded.seq, 123456789);
    expect_eq(std.mem.eql(u8, decoded.sender_id, "node-x"), true);
    expect_eq(decoded.updates.len, 2);
    expect_eq(decoded.updates[0].incarnation, 42);
    expect_eq(decoded.updates[1].state, MemberState.Suspect);

    // Truncated rejection
    const trunc_res = decodeMessage(alloc, bytes[0 .. bytes.len - 3]);
    expect_eq(trunc_res == GossipError.TruncatedMessage, true);

    // Bad magic rejection
    let bad_magic = try alloc.dupe(u8, bytes);
    defer alloc.free(bad_magic);
    bad_magic[0] = 0x00;
    const magic_res = decodeMessage(alloc, bad_magic);
    expect_eq(magic_res == GossipError.InvalidMessage, true);

    // Unknown kind tag rejection
    let bad_tag = try alloc.dupe(u8, bytes);
    defer alloc.free(bad_tag);
    bad_tag[3] = 99;
    const tag_res = decodeMessage(alloc, bad_tag);
    expect_eq(tag_res == GossipError.UnknownTag, true);
}

test("cap.gossip dynamic mount and std.testing.allocator leak-free") {
    const alloc = std.testing.allocator;
    const g = cap.gossip;

    expect_eq(g.shouldSupersede(g.MemberState.Active, 1, g.MemberState.Suspect, 1), true);
    expect_eq(g.shouldSupersede(g.MemberState.Suspect, 1, g.MemberState.Active, 1), false);
    expect_eq(g.shouldSupersede(g.MemberState.Offline, 1, g.MemberState.Active, 2), true);

    let node = try g.SwimClusterNode.init(alloc, "dyn-1", "127.0.0.1:9001", g.SwimConfig.default());
    defer node.deinit();

    const out = try node.joinSeed("dyn-0", "127.0.0.1:9000", 500);
    defer g.freeOutbound(alloc, out);

    expect_eq(node.memberCount(), 2);
    expect_eq(out.len, 1);

    const encoded = try g.encodeMessage(alloc, out[0].msg);
    defer alloc.free(encoded);
    let decoded = try g.decodeMessage(alloc, encoded);
    defer g.freeMessage(alloc, &decoded);

    expect_eq(decoded.kind, g.MessageKind.Sync);
}
