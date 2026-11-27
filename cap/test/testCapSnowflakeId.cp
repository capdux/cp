import { test, expect_eq } from "cap:test";
import {
    SnowflakeGenerator, encodeSnowflake, decodeSnowflake,
    compose, decompose, timestamp, datacenterId, workerId, sequence,
    minIdForTimestamp, maxIdForTimestamp, isValidSnowflakeString,
    formatSnowflake, parseSnowflake, parse,
    TWITTER_EPOCH_MS, DISCORD_EPOCH_MS, SF_MAX_SEQUENCE
} from "../../packages/snowflake/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/snowflake-id — encodeSnowflake & decodeSnowflake round-trip", () => {
        let ts: u64 = TWITTER_EPOCH_MS + 100000;
        let id = encodeSnowflake(TWITTER_EPOCH_MS, ts, 12, 25, 333);
        let p = decodeSnowflake(id, TWITTER_EPOCH_MS);
        expect_eq(p.timestamp_ms, ts);
        expect_eq(p.datacenter_id, 12);
        expect_eq(p.worker_id, 25);
        expect_eq(p.sequence, 333);
    });

    test("2. @cap/snowflake-id — Monotonic sequence increment within same millisecond", () => {
        let gen = SnowflakeGenerator.new(DISCORD_EPOCH_MS, 3, 7);
        let t: u64 = DISCORD_EPOCH_MS + 5000;
        let id0 = gen.nextId(t) ?? 0;
        let id1 = gen.nextId(t) ?? 0;
        let id2 = gen.nextId(t) ?? 0;
        expect_eq(id1 > id0, true);
        expect_eq(id2 > id1, true);
        expect_eq(decodeSnowflake(id0, DISCORD_EPOCH_MS).sequence, 0);
        expect_eq(decodeSnowflake(id1, DISCORD_EPOCH_MS).sequence, 1);
        expect_eq(decodeSnowflake(id2, DISCORD_EPOCH_MS).sequence, 2);
    });

    test("3. @cap/snowflake-id — Sequence resets on next millisecond", () => {
        let gen = SnowflakeGenerator.new(DISCORD_EPOCH_MS, 1, 2);
        let t1: u64 = DISCORD_EPOCH_MS + 100;
        let t2: u64 = DISCORD_EPOCH_MS + 101;
        gen.nextId(t1);
        gen.nextId(t1);
        let id_next = gen.nextId(t2) ?? 0;
        let p = decodeSnowflake(id_next, DISCORD_EPOCH_MS);
        expect_eq(p.timestamp_ms, t2);
        expect_eq(p.sequence, 0);
    });

    test("4. @cap/snowflake-id — Clock rollback & pre-epoch return null", () => {
        let gen = SnowflakeGenerator.new(DISCORD_EPOCH_MS, 0, 0);
        expect_eq(gen.nextId(DISCORD_EPOCH_MS - 1) == null, true);
        expect_eq(gen.nextId(DISCORD_EPOCH_MS + 500) != null, true);
        expect_eq(gen.nextId(DISCORD_EPOCH_MS + 499) == null, true);
    });

    test("5. @cap/snowflake-id — Sequence overflow in same ms returns null cleanly", () => {
        let gen = SnowflakeGenerator.new(TWITTER_EPOCH_MS, 0, 0);
        gen.last_ts_ms = TWITTER_EPOCH_MS + 10;
        gen.sequence = SF_MAX_SEQUENCE;
        expect_eq(gen.nextId(TWITTER_EPOCH_MS + 10) == null, true);
        expect_eq(gen.nextId(TWITTER_EPOCH_MS + 11) != null, true);
    });

    test("6. @cap/snowflake-id — formatSnowflake & parseSnowflake round-trip via @cap/itoa", () => {
        let id = encodeSnowflake(TWITTER_EPOCH_MS, TWITTER_EPOCH_MS + 987654321, 5, 9, 42);
        let out: [32]u8 = undefined;
        let n = formatSnowflake(id, out[0..32]);
        let parsed = parseSnowflake(out[0..n] as string) ?? 0;
        expect_eq(parsed, id);
        expect_eq(parseSnowflake("not-a-num") == null, true);
        expect_eq(parseSnowflake("") == null, true);
    });

    test("7. @cap/snowflake-id — compose, decompose, field extractors & parse", () => {
        let custom_epoch: u64 = 1600000000000;
        let ts: u64 = custom_epoch + 54321;
        let id = compose(custom_epoch, ts, 19, 27, 1023);
        let p = decompose(id, custom_epoch);
        expect_eq(p.timestamp_ms, ts);
        expect_eq(p.datacenter_id, 19);
        expect_eq(p.worker_id, 27);
        expect_eq(p.sequence, 1023);
        expect_eq(timestamp(id, custom_epoch), ts);
        expect_eq(datacenterId(id), 19);
        expect_eq(workerId(id), 27);
        expect_eq(sequence(id), 1023);
        expect_eq(id >= minIdForTimestamp(custom_epoch, ts), true);
        expect_eq(id <= maxIdForTimestamp(custom_epoch, ts), true);
        expect_eq(parse("18446744073709551615") != null, true);
        expect_eq(parse("18446744073709551616") == null, true);
        expect_eq(isValidSnowflakeString("123456789"), true);
        expect_eq(isValidSnowflakeString("123a"), false);
    });

    test("8. @cap/snowflake-id — withTwitterEpoch, withDiscordEpoch, nextBatch, compose/decompose/reset", () => {
        let tw = SnowflakeGenerator.withTwitterEpoch(2, 4);
        let dc = SnowflakeGenerator.withDiscordEpoch(6, 8);
        let batch: [4]u64 = [0, 0, 0, 0];
        let count = tw.nextBatch(TWITTER_EPOCH_MS + 1000, batch[0..4]);
        expect_eq(count, 4);
        expect_eq(tw.timestamp(batch[0]), TWITTER_EPOCH_MS + 1000);
        expect_eq(tw.decompose(batch[3]).sequence, 3);
        let manual = dc.compose(DISCORD_EPOCH_MS + 2000, 77);
        expect_eq(dc.decompose(manual).worker_id, 8);
        expect_eq(dc.decompose(manual).sequence, 77);
        tw.reset();
        expect_eq(tw.last_ts_ms, 0);
        expect_eq(tw.sequence, 0);
    });
}
