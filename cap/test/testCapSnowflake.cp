// cap/test/testCapSnowflake.cp
// Comprehensive unit tests for @cap/snowflake (Twitter/Discord 64-bit Snowflake)

import { test, expect_eq } from "cap:test";
import {
    TWITTER_EPOCH_MS,
    DISCORD_EPOCH_MS,
    SnowflakeParts,
    SnowflakeGenerator,
    compose,
    decompose,
    initGenerator,
    nextId,
    formatId,
    parseId,
} from "../../packages/@sapphire/snowflake/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/snowflake — compose & decompose lossless field round-trip", () => {
        const ts: u64 = TWITTER_EPOCH_MS + 1000000;
        const id: u64 = compose(TWITTER_EPOCH_MS, ts, 17, 29, 3456);
        expect_eq(id > 0, true);

        const p: SnowflakeParts = decompose(id, TWITTER_EPOCH_MS);
        expect_eq(p.timestamp_ms, ts);
        expect_eq(p.datacenter_id, 17 as u8);
        expect_eq(p.worker_id, 29 as u8);
        expect_eq(p.sequence, 3456 as u16);

        // Out-of-bounds fields return 0
        expect_eq(compose(TWITTER_EPOCH_MS, TWITTER_EPOCH_MS - 1, 0, 0, 0), 0 as u64);
        expect_eq(compose(TWITTER_EPOCH_MS, ts, 32, 0, 0), 0 as u64);
        expect_eq(compose(TWITTER_EPOCH_MS, ts, 0, 32, 0), 0 as u64);
        expect_eq(compose(TWITTER_EPOCH_MS, ts, 0, 0, 4096), 0 as u64);
    });

    test("2. @cap/snowflake — nextId increments sequence in same ms & resets across ms", () => {
        let gen: SnowflakeGenerator = initGenerator(TWITTER_EPOCH_MS, 3, 7);
        const t1: u64 = TWITTER_EPOCH_MS + 50000;

        const id0: i64 = nextId(&gen, t1);
        const id1: i64 = nextId(&gen, t1);
        const id2: i64 = nextId(&gen, t1 + 1);

        expect_eq(id0 > 0 && id1 > id0 && id2 > id1, true);
        expect_eq(decompose(id0 as u64, TWITTER_EPOCH_MS).sequence, 0 as u16);
        expect_eq(decompose(id1 as u64, TWITTER_EPOCH_MS).sequence, 1 as u16);
        expect_eq(decompose(id2 as u64, TWITTER_EPOCH_MS).sequence, 0 as u16);
        expect_eq(decompose(id2 as u64, TWITTER_EPOCH_MS).timestamp_ms, t1 + 1);
    });

    test("3. @cap/snowflake — clock rollback returns -1", () => {
        let gen: SnowflakeGenerator = initGenerator(TWITTER_EPOCH_MS, 1, 1);
        const t: u64 = TWITTER_EPOCH_MS + 10000;
        expect_eq(nextId(&gen, t) > 0, true);
        expect_eq(nextId(&gen, t - 1), -1 as i64);
        expect_eq(nextId(&gen, TWITTER_EPOCH_MS - 5), -1 as i64);
    });

    test("4. @cap/snowflake — same-ms sequence exhaustion at 4096 returns -2", () => {
        let gen: SnowflakeGenerator = initGenerator(TWITTER_EPOCH_MS, 0, 1);
        const t: u64 = TWITTER_EPOCH_MS + 12345;
        let i: usize = 0;
        while (i < 4096) {
            expect_eq(nextId(&gen, t) >= 0, true);
            i += 1;
        }
        // 4097th call in the same millisecond signals overflow (-2)
        expect_eq(nextId(&gen, t), -2 as i64);
        // Advancing 1ms immediately recovers
        expect_eq(nextId(&gen, t + 1) >= 0, true);
    });

    test("5. @cap/snowflake — classic Discord Snowflake ID decomposition", () => {
        // Official Discord docs example: 175928847299117063 -> 2016-04-30T11:18:25.796Z (1462015105796 ms)
        const discord_id: i64 = parseId("175928847299117063");
        expect_eq(discord_id > 0, true);
        const p: SnowflakeParts = decompose(discord_id as u64, DISCORD_EPOCH_MS);
        expect_eq(p.timestamp_ms, 1462015105796 as u64);
        expect_eq(p.datacenter_id, 1 as u8);
        expect_eq(p.worker_id, 0 as u8);
        expect_eq(p.sequence, 7 as u16);
    });

    test("6. @cap/snowflake — formatId & parseId round-trip & tiny buffer protection", () => {
        const raw_str = "175928847299117063";
        const parsed: i64 = parseId(raw_str);
        expect_eq(parsed > 0, true);

        let buf: [24]u8 = undefined;
        const n: usize = formatId(parsed as u64, buf[0..24]);
        expect_eq(sliceEq(buf[0..n], raw_str), true);

        expect_eq(parseId(""), -1 as i64);
        expect_eq(parseId("123abc"), -1 as i64);
        expect_eq(parseId("9999999999999999999"), -1 as i64);

        let tiny: [5]u8 = undefined;
        expect_eq(formatId(parsed as u64, tiny[0..5]), 0 as usize);
    });
}
