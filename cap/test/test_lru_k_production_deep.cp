// Source: native-cp-stdlib (Scan-Resistant LRU-K Cache)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { LruKCache, LruKError } from "cap:db";
import cap from "cap";

test("lru_k: scan resistance protects hot keys from cold scan pollution", () => {
    let cache = try LruKCache(u32, u32).init(std.testing.allocator, 2, 3, 2);
    defer cache.deinit();

    // Hot keys 1 and 2 are accessed twice -> promoted to cache queue
    try cache.put(1, 100);
    expect_eq(cache.len(), @as(usize, 0));
    expect_eq(cache.historyLen(), @as(usize, 1));
    expect_eq(cache.get(1).?, @as(u32, 100));
    expect_eq(cache.len(), @as(usize, 1));
    expect_eq(cache.historyLen(), @as(usize, 0));

    try cache.put(2, 200);
    try cache.put(2, 202);
    expect_eq(cache.len(), @as(usize, 2));
    expect_eq(cache.historyLen(), @as(usize, 0));

    // Sequential cold scan of 10 keys (each accessed only once)
    // Should evict only within history queue without evicting hot keys 1 and 2
    let scan_key: u32 = 100;
    while (scan_key < 110) {
        try cache.put(scan_key, scan_key * 10);
        scan_key += 1;
    }

    expect_eq(cache.len(), @as(usize, 2));
    expect_eq(cache.historyLen(), @as(usize, 3));
    expect_eq(cache.peek(1).?, @as(u32, 100));
    expect_eq(cache.peek(2).?, @as(u32, 202));
    expect_eq(cache.peek(100) == null, true);
    expect_eq(cache.peek(109).?, @as(u32, 1090));
});

test("lru_k: put overwrite, get/peek hit/miss stats and cache eviction", () => {
    let cache = try LruKCache(u32, u32).init(std.testing.allocator, 2, 4, 2);
    defer cache.deinit();

    // Promote 1 and 2 into cache queue (1 is LRU, 2 is MRU)
    try cache.put(1, 10);
    * = cache.get(1);
    try cache.put(2, 20);
    * = cache.get(2);

    // Miss check
    expect_eq(cache.get(999) == null, true);
    expect_eq(cache.hits(), @as(u64, 2));
    expect_eq(cache.misses(), @as(u64, 1));

    // Peek does not alter stats or LRU order
    expect_eq(cache.peek(1).?, @as(u32, 10));
    expect_eq(cache.hits(), @as(u64, 2));
    expect_eq(cache.misses(), @as(u64, 1));

    // Overwrite 1 -> promotes 1 to MRU, making 2 the LRU
    try cache.put(1, 11);
    expect_eq(cache.peek(1).?, @as(u32, 11));

    // Promote 3 -> evicts 2 from cache queue
    try cache.put(3, 30);
    * = cache.get(3);
    expect_eq(cache.len(), @as(usize, 2));
    expect_eq(cache.peek(2) == null, true);
    expect_eq(cache.peek(1).?, @as(u32, 11));
    expect_eq(cache.peek(3).?, @as(u32, 30));
});

test("lru_k: remove and clear reset queues and stats", () => {
    let cache = try LruKCache(u32, u32).init(std.testing.allocator, 3, 3, 2);
    defer cache.deinit();

    try cache.put(10, 100);
    * = cache.get(10); // in cache
    try cache.put(20, 200); // in history

    expect_eq(cache.len(), @as(usize, 1));
    expect_eq(cache.historyLen(), @as(usize, 1));

    const removed_hot = cache.remove(10);
    expect_eq(removed_hot.?, @as(u32, 100));
    expect_eq(cache.len(), @as(usize, 0));
    expect_eq(cache.peek(10) == null, true);

    const removed_hist = cache.remove(20);
    expect_eq(removed_hist.?, @as(u32, 200));
    expect_eq(cache.historyLen(), @as(usize, 0));
    expect_eq(cache.remove(999) == null, true);

    // Re-populate and clear
    try cache.put(30, 300);
    * = cache.get(30);
    * = cache.get(999);
    cache.clear();
    expect_eq(cache.len(), @as(usize, 0));
    expect_eq(cache.historyLen(), @as(usize, 0));
    expect_eq(cache.hits(), @as(u64, 0));
    expect_eq(cache.misses(), @as(u64, 0));
    expect_eq(cache.peek(30) == null, true);
});

test("lru_k: invalid capacity and invalid k parameter interception", () => {
    if (c := LruKCache(u32, u32).init(std.testing.allocator, 0, 4, 2)) {
        let mut_c = c;
        mut_c.deinit();
        return error.TestUnexpectedResult;
    } else (err) {
        expect_eq(err == LruKError.InvalidCapacity, true);
    }

    if (c := LruKCache(u32, u32).init(std.testing.allocator, 4, 0, 2)) {
        let mut_c = c;
        mut_c.deinit();
        return error.TestUnexpectedResult;
    } else (err) {
        expect_eq(err == LruKError.InvalidCapacity, true);
    }

    if (c := LruKCache(u32, u32).init(std.testing.allocator, 4, 4, 0)) {
        let mut_c = c;
        mut_c.deinit();
        return error.TestUnexpectedResult;
    } else (err) {
        expect_eq(err == LruKError.InvalidK, true);
    }
});

test("lru_k: cap.lru_k dynamic mounting and k=1 direct caching", () => {
    let cache = try cap.db.LruKCache(u64, u64).init(std.testing.allocator, 2, 2, 1);
    defer cache.deinit();

    try cache.put(1, 111);
    try cache.put(2, 222);
    expect_eq(cache.len(), @as(usize, 2));
    expect_eq(cache.historyLen(), @as(usize, 0));

    // Accessing 1 makes 2 the LRU
    expect_eq(cache.get(1).?, @as(u64, 111));
    try cache.put(3, 333);
    expect_eq(cache.peek(2) == null, true);
    expect_eq(cache.peek(1).?, @as(u64, 111));
    expect_eq(cache.peek(3).?, @as(u64, 333));
});
