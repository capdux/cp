// Source: cp/cap/test/test_static_hashmap_production_deep.cp (Production deep tests for cap:static_hashmap)

import { test, expect_eq } from "cap:test";
import {
    StaticHashMap,
    KeyValue,
    StringContext,
    AsciiIgnoreCaseContext,
    IntContext,
    EMPTY_HASH,
    fnv1a64,
    fnv1a64AsciiLower,
    splitmix64,
} from "cap:static_hashmap";

const COMPTIME_METHOD_MAP = StaticHashMap<string, u16, StringContext, 16>.fromPairs([
    KeyValue<string, u16>{ key: "GET", value: 1 },
    KeyValue<string, u16>{ key: "POST", value: 2 },
    KeyValue<string, u16>{ key: "PUT", value: 3 },
    KeyValue<string, u16>{ key: "DELETE", value: 4 },
    KeyValue<string, u16>{ key: "PATCH", value: 5 },
    KeyValue<string, u16>{ key: "HEAD", value: 6 },
]);

struct CollidingStringContext {
    fn hash(k: string): u64 {
        * = k;
        return 0x12345678;
    }

    fn eql(a: string, b: string): bool {
        return std.mem.eql(u8, a, b);
    }
}

test("Comptime static string hashmap O(1) get/has and slot statistics") {
    expect_eq(COMPTIME_METHOD_MAP.count(), @as(usize, 6));
    expect_eq(COMPTIME_METHOD_MAP.capacity(), @as(usize, 16));
    expect_eq(COMPTIME_METHOD_MAP.loadFactorPermille(), @as(u32, 375));

    expect_eq(COMPTIME_METHOD_MAP.has("GET"), true);
    expect_eq(COMPTIME_METHOD_MAP.get("GET").?, @as(u16, 1));
    expect_eq(COMPTIME_METHOD_MAP.get("POST").?, @as(u16, 2));
    expect_eq(COMPTIME_METHOD_MAP.get("DELETE").?, @as(u16, 4));
    expect_eq(COMPTIME_METHOD_MAP.get("HEAD").?, @as(u16, 6));
    expect_eq(COMPTIME_METHOD_MAP.has("OPTIONS"), false);
    expect_eq(COMPTIME_METHOD_MAP.get("get") == null, true);
}

test("ASCII case-insensitive string map and integer key static map") {
    const headers = StaticHashMap<string, u32, AsciiIgnoreCaseContext, 16>.fromPairs([
        KeyValue<string, u32>{ key: "Content-Type", value: 101 },
        KeyValue<string, u32>{ key: "Authorization", value: 102 },
        KeyValue<string, u32>{ key: "X-Request-Id", value: 103 },
    ]);

    expect_eq(headers.get("content-type").?, @as(u32, 101));
    expect_eq(headers.get("CONTENT-TYPE").?, @as(u32, 101));
    expect_eq(headers.get("CoNtEnT-tYpE").?, @as(u32, 101));
    expect_eq(headers.get("authorization").?, @as(u32, 102));
    expect_eq(headers.get("x-request-id").?, @as(u32, 103));
    expect_eq(headers.get("Accept") == null, true);

    let int_map = StaticHashMap<u32, u64, IntContext<u32>, 32>.init();
    let k: u32 = 1;
    while (k <= 20) {
        expect_eq(int_map.put(k * 100, (k as u64) * 999), true);
        k += 1;
    }
    expect_eq(int_map.count(), @as(usize, 20));
    expect_eq(int_map.get(100).?, @as(u64, 999));
    expect_eq(int_map.get(2000).?, @as(u64, 19980));
    expect_eq(int_map.get(999999) == null, true);
}

test("Missing keys, empty key, long key and value overwrite boundary guards") {
    let m = StaticHashMap<string, i32, StringContext, 16>.init();

    // 空字符串键合法存储与查询
    expect_eq(m.get("") == null, true);
    expect_eq(m.put("", 42), true);
    expect_eq(m.get("").?, @as(i32, 42));
    expect_eq(m.put("", 99), true);
    expect_eq(m.count(), @as(usize, 1));
    expect_eq(m.get("").?, @as(i32, 99));

    // 超长 512 字节键查询与插入
    let long_buf: [512]u8 = ['x'; 512];
    const long_key: string = long_buf[0..512];
    expect_eq(m.has(long_key), false);
    expect_eq(m.put(long_key, 777), true);
    expect_eq(m.get(long_key).?, @as(i32, 777));

    // 仅差 1 字节的超长键不能误命中
    let long_buf2: [512]u8 = ['x'; 512];
    long_buf2[511] = 'y';
    expect_eq(m.get(long_buf2[0..512]) == null, true);
}

test("Hash collision chain probing and backward-shift deletion integrity") {
    let m = StaticHashMap<string, u32, CollidingStringContext, 16>.init();
    const keys: [8]string = ["k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7"];

    let i: usize = 0;
    while (i < 8) {
        expect_eq(m.put(keys[i], (i as u32) + 10), true);
        expect_eq(m.probeCount(keys[i]), i + 1);
        i += 1;
    }
    expect_eq(m.count(), @as(usize, 8));

    // 删除探测链中间元素 k2、k0、k5，验证后续碰撞键仍 100% 可达
    expect_eq(m.delete("k2").?, @as(u32, 12));
    expect_eq(m.delete("k0").?, @as(u32, 10));
    expect_eq(m.delete("k5").?, @as(u32, 15));
    expect_eq(m.delete("k2") == null, true);
    expect_eq(m.count(), @as(usize, 5));

    expect_eq(m.get("k0") == null, true);
    expect_eq(m.get("k1").?, @as(u32, 11));
    expect_eq(m.get("k2") == null, true);
    expect_eq(m.get("k3").?, @as(u32, 13));
    expect_eq(m.get("k4").?, @as(u32, 14));
    expect_eq(m.get("k5") == null, true);
    expect_eq(m.get("k6").?, @as(u32, 16));
    expect_eq(m.get("k7").?, @as(u32, 17));
}

test("Full table capacity guard, MapFull error, delete reuse and clear") {
    let m = StaticHashMap<string, u8, StringContext, 4>.init();
    expect_eq(m.put("a", 1), true);
    expect_eq(m.put("b", 2), true);
    expect_eq(m.put("c", 3), true);
    expect_eq(m.put("d", 4), true);
    expect_eq(m.count(), @as(usize, 4));
    expect_eq(m.loadFactorPermille(), @as(u32, 1000));

    // 满表时更新已有键成功
    expect_eq(m.put("b", 22), true);
    expect_eq(m.get("b").?, @as(u8, 22));

    // 满表时插入新键必须被拦截
    expect_eq(m.put("e", 5), false);
    const full_err = switch (m.putChecked("e", 5)) {
        err.MapFull => true,
        else => false,
    };
    expect_eq(full_err, true);

    // 删除一个后可重新插入
    expect_eq(m.delete("c").?, @as(u8, 3));
    expect_eq(m.put("e", 5), true);
    expect_eq(m.get("e").?, @as(u8, 5));

    // clear 重置
    m.clear();
    expect_eq(m.count(), @as(usize, 0));
    expect_eq(m.get("a") == null, true);
    expect_eq(m.get("e") == null, true);
}

test("Hash functions invariants and avalanche properties") {
    expect_eq(fnv1a64("") != EMPTY_HASH, true);
    expect_eq(fnv1a64("hello") != fnv1a64("world"), true);
    expect_eq(fnv1a64AsciiLower("Content-Type"), fnv1a64AsciiLower("content-type"));
    expect_eq(fnv1a64AsciiLower("HTTP/2"), fnv1a64AsciiLower("http/2"));
    expect_eq(splitmix64(0) != splitmix64(1), true);
    expect_eq(splitmix64(0) != EMPTY_HASH, true);
}
