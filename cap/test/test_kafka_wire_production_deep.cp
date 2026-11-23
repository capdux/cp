// Source: cp/cap/test/test_kafka_wire_production_deep.cp (Deep production tests for cap:kafka_wire)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KafkaError,
    KafkaReader,
    KafkaWriter,
    ApiKey,
    RequestHeader,
    ResponseHeader,
    ApiVersionRange,
    ApiVersionsReq,
    ApiVersionsResp,
    MetadataReq,
    BrokerMeta,
    PartitionMeta,
    TopicMeta,
    MetadataResp,
    KafkaStreamFramer,
    RecordHeader,
    Record,
    RecordBatch,
    encodeRecordBatch,
    decodeRecordBatch,
} from "cap:net";

fn expectCrcMismatch(alloc: std.mem.Allocator, data: []const u8): bool {
    let r = decodeRecordBatch(alloc, data) catch |e| {
        return e == KafkaError.CrcMismatch;
    };
    r.deinit(alloc);
    return false;
}

fn expectFrameTooLarge(framer: *KafkaStreamFramer): bool {
    * = framer.nextFrame() catch |e| {
        return e == KafkaError.FrameTooLarge;
    };
    return false;
}

test("kafka_wire primitives: zigzag varint, compact string/array & tagged fields") {
    const alloc = std.testing.allocator;
    let w = KafkaWriter.init(alloc);
    defer w.deinit();

    w.writeI32(-42)?;
    w.writeI64(9876543210)?;
    w.writeUVarint(300)?;
    w.writeVarint(-1)?;
    w.writeVarint(123456)?;
    w.writeVarlong(-987654321)?;
    w.writeNullableString(null)?;
    w.writeNullableString("client-a")?;
    w.writeCompactString(null)?;
    w.writeCompactString("compact-hello")?;
    w.writeCompactArrayLen(null)?;
    w.writeCompactArrayLen(3)?;
    w.writeUVarint(1)?;
    w.writeUVarint(7)?;
    w.writeUVarint(2)?;
    w.writeBytes("ok")?;

    const bytes = w.items();
    let r = KafkaReader.init(bytes);

    expect_eq(r.readI32()?, -42);
    expect_eq(r.readI64()?, 9876543210);
    expect_eq(r.readUVarint()?, 300);
    expect_eq(r.readVarint()?, -1);
    expect_eq(r.readVarint()?, 123456);
    expect_eq(r.readVarlong()?, -987654321);
    expect_eq(r.readNullableString()? == null, true);
    expect_eq(std.mem.eql(u8, (r.readNullableString()?) orelse "", "client-a"), true);
    expect_eq(r.readCompactString()? == null, true);
    expect_eq(std.mem.eql(u8, (r.readCompactString()?) orelse "", "compact-hello"), true);
    expect_eq(r.readCompactArrayLen()? == null, true);
    expect_eq((r.readCompactArrayLen()?) orelse 0, 3);
    r.skipTaggedFields()?;
    expect_eq(r.remaining(), 0);
}

test("kafka_wire protocol: RequestHeader/ResponseHeader, ApiVersions v3 & Metadata v9") {
    const alloc = std.testing.allocator;

    let w1 = KafkaWriter.init(alloc);
    defer w1.deinit();
    const req_hdr = RequestHeader{
        .api_key = @intFromEnum(ApiKey.ApiVersions),
        .api_version = 3,
        .correlation_id = 1001,
        .client_id = "cp-kafka-client",
        .header_version = 2,
    };
    const api_req = ApiVersionsReq{
        .client_software_name = "cp-wire",
        .client_software_version = "1.0.0",
    };
    req_hdr.encode(&w1)?;
    api_req.encode(&w1, 3)?;

    let r1 = KafkaReader.init(w1.items());
    const dec_req_hdr = RequestHeader.decode(&r1, 2)?;
    const dec_api_req = ApiVersionsReq.decode(&r1, 3)?;
    expect_eq(dec_req_hdr.api_key, 18);
    expect_eq(dec_req_hdr.correlation_id, 1001);
    expect_eq(std.mem.eql(u8, dec_req_hdr.client_id orelse "", "cp-kafka-client"), true);
    expect_eq(std.mem.eql(u8, dec_api_req.client_software_name orelse "", "cp-wire"), true);
    expect_eq(std.mem.eql(u8, dec_api_req.client_software_version orelse "", "1.0.0"), true);

    let w2 = KafkaWriter.init(alloc);
    defer w2.deinit();
    const resp_hdr = ResponseHeader{
        .correlation_id = 1001,
        .header_version = 1,
    };
    const ranges: [3]ApiVersionRange = [
        ApiVersionRange{ .api_key = 0, .min_version = 0, .max_version = 9 },
        ApiVersionRange{ .api_key = 3, .min_version = 0, .max_version = 12 },
        ApiVersionRange{ .api_key = 18, .min_version = 0, .max_version = 3 },
    ];
    const api_resp = ApiVersionsResp{
        .error_code = 0,
        .api_keys = &ranges,
        .throttle_time_ms = 15,
    };
    resp_hdr.encode(&w2)?;
    api_resp.encode(&w2, 3)?;

    let r2 = KafkaReader.init(w2.items());
    const dec_resp_hdr = ResponseHeader.decode(&r2, 1)?;
    let dec_api_resp = ApiVersionsResp.decode(alloc, &r2, 3)?;
    defer dec_api_resp.deinit(alloc);
    expect_eq(dec_resp_hdr.correlation_id, 1001);
    expect_eq(dec_api_resp.error_code, 0);
    expect_eq(dec_api_resp.throttle_time_ms, 15);
    expect_eq(dec_api_resp.api_keys.len, 3);
    expect_eq(dec_api_resp.api_keys[1].max_version, 12);

    let w3 = KafkaWriter.init(alloc);
    defer w3.deinit();
    const req_topics: [2]string = ["orders", "payments"];
    const m_req = MetadataReq{
        .topics = &req_topics,
        .allow_auto_topic_creation = false,
    };
    m_req.encode(&w3, 9)?;

    let r3 = KafkaReader.init(w3.items());
    let dec_m_req = MetadataReq.decode(alloc, &r3, 9)?;
    defer dec_m_req.deinit(alloc);
    expect_eq(dec_m_req.allow_auto_topic_creation, false);
    if (ts := dec_m_req.topics) {
        expect_eq(ts.len, 2);
    } else {
        expect_eq(true, false);
    }

    let w4 = KafkaWriter.init(alloc);
    defer w4.deinit();
    const brokers: [1]BrokerMeta = [
        BrokerMeta{ .node_id = 1, .host = "broker-1.local", .port = 9092, .rack = "az-1" },
    ];
    const reps: [2]i32 = [1, 2];
    const isrs: [1]i32 = [1];
    const parts: [1]PartitionMeta = [
        PartitionMeta{
            .error_code = 0,
            .partition_index = 0,
            .leader_id = 1,
            .replica_nodes = &reps,
            .isr_nodes = &isrs,
        },
    ];
    const topics: [1]TopicMeta = [
        TopicMeta{
            .error_code = 0,
            .name = "orders",
            .is_internal = false,
            .partitions = &parts,
        },
    ];
    const m_resp = MetadataResp{
        .throttle_time_ms = 5,
        .brokers = &brokers,
        .cluster_id = "cluster-prod-1",
        .controller_id = 1,
        .topics = &topics,
    };
    m_resp.encode(&w4, 9)?;

    let r4 = KafkaReader.init(w4.items());
    let dec_m_resp = MetadataResp.decode(alloc, &r4, 9)?;
    defer dec_m_resp.deinit(alloc);
    expect_eq(dec_m_resp.controller_id, 1);
    expect_eq(std.mem.eql(u8, dec_m_resp.cluster_id orelse "", "cluster-prod-1"), true);
    expect_eq(dec_m_resp.brokers.len, 1);
    expect_eq(std.mem.eql(u8, dec_m_resp.brokers[0].host, "broker-1.local"), true);
    expect_eq(dec_m_resp.topics.len, 1);
    expect_eq(dec_m_resp.topics[0].partitions[0].leader_id, 1);
}

test("kafka_wire RecordBatch v2 multi-record roundtrip & CRC32C tamper detection") {
    const alloc = std.testing.allocator;
    const h1: [2]RecordHeader = [
        RecordHeader{ .key = "trace", .value = "t-001" },
        RecordHeader{ .key = "empty", .value = null },
    ];
    const h2: [0]RecordHeader = [];
    const recs: [2]Record = [
        Record{
            .attributes = 0,
            .timestamp_delta = 0,
            .offset_delta = 0,
            .key = "user-1",
            .value = "payload-alpha",
            .headers = &h1,
        },
        Record{
            .attributes = 0,
            .timestamp_delta = 25,
            .offset_delta = 1,
            .key = null,
            .value = "payload-beta",
            .headers = &h2,
        },
    ];
    const batch = RecordBatch{
        .base_offset = 500,
        .partition_leader_epoch = 3,
        .attributes = 0,
        .last_offset_delta = 1,
        .base_timestamp = 1710000000000,
        .max_timestamp = 1710000000025,
        .producer_id = 42,
        .producer_epoch = 1,
        .base_sequence = 7,
        .records = &recs,
    };

    let encoded = encodeRecordBatch(alloc, &batch)?;
    defer alloc.free(encoded);

    let decoded = decodeRecordBatch(alloc, encoded)?;
    defer decoded.deinit(alloc);
    expect_eq(decoded.base_offset, 500);
    expect_eq(decoded.producer_id, 42);
    expect_eq(decoded.records.len, 2);
    expect_eq(std.mem.eql(u8, decoded.records[0].key orelse "", "user-1"), true);
    expect_eq(std.mem.eql(u8, decoded.records[0].value orelse "", "payload-alpha"), true);
    expect_eq(decoded.records[0].headers.len, 2);
    expect_eq(std.mem.eql(u8, decoded.records[0].headers[0].value orelse "", "t-001"), true);
    expect_eq(decoded.records[0].headers[1].value == null, true);
    expect_eq(decoded.records[1].key == null, true);
    expect_eq(decoded.records[1].offset_delta, 1);

    encoded[encoded.len - 1] ^= 0xff;
    expect_eq(expectCrcMismatch(alloc, encoded), true);
}

test("kafka_wire KafkaStreamFramer 1..7 byte chunking & max_frame_bytes guard") {
    const alloc = std.testing.allocator;
    const f1 = KafkaStreamFramer.framePayload(alloc, "frame-one-payload")?;
    defer alloc.free(f1);
    const f2 = KafkaStreamFramer.framePayload(alloc, "second")?;
    defer alloc.free(f2);

    let combined = std.ArrayList(u8).empty;
    defer combined.deinit(alloc);
    combined.appendSlice(alloc, f1) catch return KafkaError.OutOfMemory;
    combined.appendSlice(alloc, f2) catch return KafkaError.OutOfMemory;

    let framer = KafkaStreamFramer.init(alloc, 1024);
    defer framer.deinit();

    let got_count: usize = 0;
    let pos: usize = 0;
    let step: usize = 1;
    while (pos < combined.items.len) {
        const end = @min(pos + step, combined.items.len);
        framer.push(combined.items[pos..end])?;
        while (true) {
            if (frame := framer.nextFrame()?) {
                defer alloc.free(frame);
                if (got_count == 0) {
                    expect_eq(std.mem.eql(u8, frame, "frame-one-payload"), true);
                } else if (got_count == 1) {
                    expect_eq(std.mem.eql(u8, frame, "second"), true);
                }
                got_count += 1;
            } else {
                break;
            }
        }
        pos = end;
        step = (step % 7) + 1;
    }
    expect_eq(got_count, 2);

    let small_framer = KafkaStreamFramer.init(alloc, 8);
    defer small_framer.deinit();
    small_framer.push(f1)?;
    expect_eq(expectFrameTooLarge(&small_framer), true);
}

test("kafka_wire cap.kafka_wire dynamic mount & zero-leak allocator") {
    const alloc = std.testing.allocator;
    let w = cap.net.KafkaWriter.init(alloc);
    defer w.deinit();

    w.writeCompactString("cap-mount-ok")?;
    let r = cap.net.KafkaReader.init(w.items());
    const s = r.readCompactString()?;
    expect_eq(std.mem.eql(u8, s orelse "", "cap-mount-ok"), true);
}
