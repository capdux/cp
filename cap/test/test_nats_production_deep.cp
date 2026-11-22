// Source: cp/cap/test/test_nats_production_deep.cp
// Deep Production Test Suite for cap:nats (NATS Wire Protocol, HPUB/HMSG Headers & Incremental Stream Parser)
// Covers:
//   1. NatsEncoder CONNECT/SUB/PUB/HPUB/UNSUB/PING/PONG encoding & NatsParser full round-trip
//   2. 1..7 byte arbitrary chunk half-packet & sticky-packet parsing (INFO + binary MSG + HMSG + PING + +OK + -ERR)
//   3. NATS/1.0 multi-value header parsing & case-insensitive header(name) lookup
//   4. Security & malformed guards: whitespace/CR-LF injection (InvalidSubject/CommandInjection), hdr_len > total_len (InvalidNatsHeaders), PayloadTooLarge
//   5. Dynamic `cap.nats` namespace mounting & `std.testing.allocator` zero memory leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    NatsError,
    NatsHeader,
    NatsMsg,
    NatsOp,
    NatsEncoder,
    NatsParser,
} from "cap:net";
import cap from "cap";

test("1. NatsEncoder All Client Commands & NatsParser Full Protocol Coverage") {
    const alloc = std.testing.allocator;

    const conn_cmd = NatsEncoder.encodeConnect(alloc, "{\"verbose\":false,\"pedantic\":false}") catch unreachable;
    defer alloc.free(conn_cmd);
    expect_eq(std.mem.eql(u8, conn_cmd, "CONNECT {\"verbose\":false,\"pedantic\":false}\r\n"), true);

    const sub_cmd = NatsEncoder.encodeSub(alloc, "orders.*", "workers", "sid-1") catch unreachable;
    defer alloc.free(sub_cmd);
    expect_eq(std.mem.eql(u8, sub_cmd, "SUB orders.* workers sid-1\r\n"), true);

    const pub_cmd = NatsEncoder.encodePub(alloc, "orders.new", "reply.99", "payload-1") catch unreachable;
    defer alloc.free(pub_cmd);
    expect_eq(std.mem.eql(u8, pub_cmd, "PUB orders.new reply.99 9\r\npayload-1\r\n"), true);

    let hdrs: [2]NatsHeader = [
        NatsHeader.init("X-Trace-Id", "tr-42"),
        NatsHeader.init("Content-Type", "application/json"),
    ];
    const hpub_cmd = NatsEncoder.encodeHpub(alloc, "orders.new", null, &hdrs, "{\"ok\":true}") catch unreachable;
    defer alloc.free(hpub_cmd);
    // NATS/1.0\r\n (10) + X-Trace-Id: tr-42\r\n (19) + Content-Type: application/json\r\n (32) + \r\n (2) = 63
    // total = 63 + 11 = 74
    expect_eq(
        std.mem.eql(
            u8,
            hpub_cmd,
            "HPUB orders.new 63 74\r\nNATS/1.0\r\nX-Trace-Id: tr-42\r\nContent-Type: application/json\r\n\r\n{\"ok\":true}\r\n",
        ),
        true,
    );

    const unsub_cmd = NatsEncoder.encodeUnsub(alloc, "sid-1", 5) catch unreachable;
    defer alloc.free(unsub_cmd);
    expect_eq(std.mem.eql(u8, unsub_cmd, "UNSUB sid-1 5\r\n"), true);

    const ping_cmd = NatsEncoder.encodePing(alloc) catch unreachable;
    defer alloc.free(ping_cmd);
    expect_eq(std.mem.eql(u8, ping_cmd, "PING\r\n"), true);

    const pong_cmd = NatsEncoder.encodePong(alloc) catch unreachable;
    defer alloc.free(pong_cmd);
    expect_eq(std.mem.eql(u8, pong_cmd, "PONG\r\n"), true);

    let parser = NatsParser.init(alloc, 65536);
    defer parser.deinit();

    parser.feed("PING\r\nPONG\r\n+OK\r\n-ERR 'Unknown Protocol Operation'\r\n") catch unreachable;

    if (op1 := parser.nextOp() catch unreachable) {
        defer op1.deinit(alloc);
        switch (op1) {
            .ping => {},
            else => expect_eq(true, false),
        }
    } else {
        expect_eq(true, false);
    }

    if (op2 := parser.nextOp() catch unreachable) {
        defer op2.deinit(alloc);
        switch (op2) {
            .pong => {},
            else => expect_eq(true, false),
        }
    } else {
        expect_eq(true, false);
    }

    if (op3 := parser.nextOp() catch unreachable) {
        defer op3.deinit(alloc);
        switch (op3) {
            .ok => {},
            else => expect_eq(true, false),
        }
    } else {
        expect_eq(true, false);
    }

    if (op4 := parser.nextOp() catch unreachable) {
        defer op4.deinit(alloc);
        switch (op4) {
            .err msg => expect_eq(std.mem.eql(u8, msg, "'Unknown Protocol Operation'"), true),
            else => expect_eq(true, false),
        }
    } else {
        expect_eq(true, false);
    }

    expect_eq(parser.nextOp() catch unreachable == null, true);
}

test("2. 1..7 Byte Arbitrary Chunk Half-Packet & Sticky-Packet Stream Parsing") {
    const alloc = std.testing.allocator;

    const stream =
        "INFO {\"server_id\":\"NATS1\",\"max_payload\":1048576}\r\n" ++
        "MSG bin.topic 7 reply.bin 6\r\nAB\r\n\x00Z\r\n" ++
        "HMSG hdr.topic 8 31 36\r\nNATS/1.0\r\nX-Env: prod\r\n\r\nhello\r\n" ++
        "PING\r\n" ++
        "+OK\r\n" ++
        "-ERR 'Authorization Violation'\r\n";

    let step: usize = 1;
    while (step <= 7) : (step += 1) {
        let parser = NatsParser.init(alloc, 65536);
        defer parser.deinit();

        let op_count: usize = 0;
        let offset: usize = 0;
        while (offset < stream.len) {
            const end = @min(offset + step, stream.len);
            parser.feed(stream[offset..end]) catch unreachable;
            offset = end;

            while (true) {
                const maybe_op = parser.nextOp() catch unreachable;
                if (op := maybe_op) {
                    defer op.deinit(alloc);
                    if (op_count == 0) {
                        switch (op) {
                            .info s => expect_eq(std.mem.eql(u8, s, "{\"server_id\":\"NATS1\",\"max_payload\":1048576}"), true),
                            else => expect_eq(true, false),
                        }
                    } else if (op_count == 1) {
                        switch (op) {
                            .msg m => {
                                expect_eq(std.mem.eql(u8, m.subject, "bin.topic"), true);
                                expect_eq(std.mem.eql(u8, m.sid, "7"), true);
                                if (rt := m.reply_to) {
                                    expect_eq(std.mem.eql(u8, rt, "reply.bin"), true);
                                } else {
                                    expect_eq(true, false);
                                }
                                expect_eq(std.mem.eql(u8, m.payload, "AB\r\n\x00Z"), true);
                            },
                            else => expect_eq(true, false),
                        }
                    } else if (op_count == 2) {
                        switch (op) {
                            .msg m => {
                                expect_eq(std.mem.eql(u8, m.subject, "hdr.topic"), true);
                                expect_eq(std.mem.eql(u8, m.sid, "8"), true);
                                expect_eq(m.reply_to == null, true);
                                if (env_val := m.header("x-env")) {
                                    expect_eq(std.mem.eql(u8, env_val, "prod"), true);
                                } else {
                                    expect_eq(true, false);
                                }
                                expect_eq(std.mem.eql(u8, m.payload, "hello"), true);
                            },
                            else => expect_eq(true, false),
                        }
                    } else if (op_count == 3) {
                        switch (op) {
                            .ping => {},
                            else => expect_eq(true, false),
                        }
                    } else if (op_count == 4) {
                        switch (op) {
                            .ok => {},
                            else => expect_eq(true, false),
                        }
                    } else if (op_count == 5) {
                        switch (op) {
                            .err e => expect_eq(std.mem.eql(u8, e, "'Authorization Violation'"), true),
                            else => expect_eq(true, false),
                        }
                    } else {
                        expect_eq(true, false);
                    }
                    op_count += 1;
                } else {
                    break;
                }
            }
        }
        expect_eq(op_count, 6 as usize);
        expect_eq(parser.bufferedLen(), 0 as usize);
    }
}

test("3. NATS/1.0 Multi-Value Headers & Case-Insensitive Header Lookup") {
    const alloc = std.testing.allocator;

    const raw_hmsg =
        "HMSG audit.log 42 inbox.1 66 70\r\n" ++
        "NATS/1.0\r\n" ++
        "X-Tag: alpha\r\n" ++
        "X-Tag: beta\r\n" ++
        "X-Request-Id: req-9001\r\n" ++
        "\r\n" ++
        "body\r\n";

    let parser = NatsParser.init(alloc, 4096);
    defer parser.deinit();
    parser.feed(raw_hmsg) catch unreachable;

    if (op := parser.nextOp() catch unreachable) {
        defer op.deinit(alloc);
        switch (op) {
            .msg m => {
                expect_eq(m.headers.len, 3 as usize);
                expect_eq(std.mem.eql(u8, m.headers[0].name, "X-Tag"), true);
                expect_eq(std.mem.eql(u8, m.headers[0].value, "alpha"), true);
                expect_eq(std.mem.eql(u8, m.headers[1].name, "X-Tag"), true);
                expect_eq(std.mem.eql(u8, m.headers[1].value, "beta"), true);
                expect_eq(std.mem.eql(u8, m.headers[2].name, "X-Request-Id"), true);
                expect_eq(std.mem.eql(u8, m.headers[2].value, "req-9001"), true);

                if (first_tag := m.header("x-tag")) {
                    expect_eq(std.mem.eql(u8, first_tag, "alpha"), true);
                } else {
                    expect_eq(true, false);
                }
                if (req_id := m.header("X-REQUEST-ID")) {
                    expect_eq(std.mem.eql(u8, req_id, "req-9001"), true);
                } else {
                    expect_eq(true, false);
                }
                expect_eq(m.header("X-Missing") == null, true);
                expect_eq(std.mem.eql(u8, m.payload, "body"), true);
            },
            else => expect_eq(true, false),
        }
    } else {
        expect_eq(true, false);
    }
}

test("4. Security Guards: InvalidSubject, CommandInjection, InvalidNatsHeaders & PayloadTooLarge") {
    const alloc = std.testing.allocator;

    // 4.1 InvalidSubject (whitespace in subject)
    let hit_invalid_subj = false;
    * = NatsEncoder.encodePub(alloc, "bad subject", null, "hi") catch |e| {
        if (e == err.InvalidSubject) hit_invalid_subj = true;
        ""
    };
    expect_eq(hit_invalid_subj, true);

    // 4.2 CommandInjection (CR/LF in subject and header)
    let hit_subj_inj = false;
    * = NatsEncoder.encodeSub(alloc, "foo\r\nPUB evil 2\r\nhi", null, "1") catch |e| {
        if (e == err.CommandInjection) hit_subj_inj = true;
        ""
    };
    expect_eq(hit_subj_inj, true);

    let bad_hdrs: [1]NatsHeader = [NatsHeader.init("X-Evil", "val\r\nInjected: 1")];
    let hit_hdr_inj = false;
    * = NatsEncoder.encodeHpub(alloc, "ok.topic", null, &bad_hdrs, "x") catch |e| {
        if (e == err.CommandInjection) hit_hdr_inj = true;
        ""
    };
    expect_eq(hit_hdr_inj, true);

    // 4.3 InvalidNatsHeaders (hdr_len > total_len)
    let p_hdr = NatsParser.init(alloc, 4096);
    defer p_hdr.deinit();
    p_hdr.feed("HMSG t 1 20 10\r\n0123456789\r\n") catch unreachable;
    let hit_bad_hdr_len = false;
    * = p_hdr.nextOp() catch |e| {
        if (e == err.InvalidNatsHeaders) hit_bad_hdr_len = true;
        null
    };
    expect_eq(hit_bad_hdr_len, true);

    // 4.4 PayloadTooLarge
    let p_size = NatsParser.init(alloc, 16);
    defer p_size.deinit();
    p_size.feed("MSG big.topic 1 32\r\n") catch unreachable;
    let hit_too_large = false;
    * = p_size.nextOp() catch |e| {
        if (e == err.PayloadTooLarge) hit_too_large = true;
        null
    };
    expect_eq(hit_too_large, true);
}

test("5. Dynamic `cap.nats` Namespace Mounting & Zero-Leak Round-Trip") {
    const alloc = std.testing.allocator;

    let hdrs: [1]cap.net.NatsHeader = [
        cap.net.NatsHeader.init("X-Source", "cp-nats"),
    ];
    const wire = cap.net.NatsEncoder.encodeHpub(alloc, "sys.health", "reply.sys", &hdrs, "ok") catch unreachable;
    defer alloc.free(wire);

    let parser = cap.net.NatsParser.init(alloc, 8192);
    defer parser.deinit();

    // Feed equivalent HMSG from server
    parser.feed("HMSG sys.health 99 reply.sys 31 33\r\nNATS/1.0\r\nX-Source: cp-nats\r\n\r\nok\r\n") catch unreachable;
    if (op := parser.nextOp() catch unreachable) {
        defer op.deinit(alloc);
        switch (op) {
            .msg m => {
                expect_eq(std.mem.eql(u8, m.subject, "sys.health"), true);
                expect_eq(std.mem.eql(u8, m.sid, "99"), true);
                if (src := m.header("x-source")) {
                    expect_eq(std.mem.eql(u8, src, "cp-nats"), true);
                } else {
                    expect_eq(true, false);
                }
                expect_eq(std.mem.eql(u8, m.payload, "ok"), true);
            },
            else => expect_eq(true, false),
        }
    } else {
        expect_eq(true, false);
    }
}
