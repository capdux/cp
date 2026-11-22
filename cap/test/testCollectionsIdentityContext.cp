import {
    IDENTITY_TABLE_SLOTS,
    IdentityMapError,
    IdentityContextU64,
    IdentityContextU32,
    ArrayIdentityContext,
    ArrayIdentityContextU64,
    identityBucketIndexU64,
    identityBucketIndexU32,
    IdentitySetU64,
    IdentityMapU64,
} from "../../lib/collections/index.cp";
import { expect, test } from "cap:test";

test("collections identityContext: hash/eql identity contexts and bucket indexing", fn () {
    const h64: u64 = 0xDEADBEEFCAFEBABE;
    expect(IdentityContextU64.hash(h64)).toBe(h64);
    expect(IdentityContextU64.eql(h64, h64)).toBe(true);
    expect(IdentityContextU64.eql(h64, 0)).toBe(false);

    const h32: u32 = 0x12345678;
    expect(IdentityContextU32.hash(h32)).toBe(@as(u64, h32));
    expect(IdentityContextU32.eql(h32, h32)).toBe(true);

    expect(ArrayIdentityContext.hash(h32)).toBe(h32);
    expect(ArrayIdentityContext.eql(h32, h32, 0)).toBe(true);
    expect(ArrayIdentityContext.eql(h32, 99, 1)).toBe(false);

    // ArrayIdentityContextU64 truncates to lower 32 bits
    expect(ArrayIdentityContextU64.hash(h64)).toBe(0xCAFEBABE);
    expect(ArrayIdentityContextU64.eql(h64, h64, 3)).toBe(true);

    // Bucket index masking (capacity 64 -> mask 63)
    expect(identityBucketIndexU64(0xCAFEBABE, 63)).toBe(0x3E);
    expect(identityBucketIndexU32(130, 63)).toBe(2);
});

test("collections identityContext: IdentitySetU64 basic ops and zero-key boundary", fn () {
    let set = IdentitySetU64.init();
    expect(set.isEmpty()).toBe(true);
    expect(set.len()).toBe(0);
    expect(set.capacity()).toBe(IDENTITY_TABLE_SLOTS);

    // Key 0 must be fully supported and distinct from empty slots
    expect(set.contains(0)).toBe(false);
    expect(set.insert(0) catch false).toBe(true);
    expect(set.contains(0)).toBe(true);
    expect(set.insert(0) catch true).toBe(false); // duplicate returns false
    expect(set.len()).toBe(1);

    expect(set.insert(0x9E3779B97F4A7C15) catch false).toBe(true);
    expect(set.contains(0x9E3779B97F4A7C15)).toBe(true);
    expect(set.len()).toBe(2);

    expect(set.remove(0)).toBe(true);
    expect(set.contains(0)).toBe(false);
    expect(set.remove(0)).toBe(false);
    expect(set.len()).toBe(1);
});

test("collections identityContext: linear probing collision chain & tombstone recycling", fn () {
    let set = IdentitySetU64.init();
    // All three keys collide on bucket 5 (5, 5+64, 5+128)
    const k1: u64 = 5;
    const k2: u64 = 5 + 64;
    const k3: u64 = 5 + 128;

    expect(set.insert(k1) catch false).toBe(true);
    expect(set.insert(k2) catch false).toBe(true);
    expect(set.insert(k3) catch false).toBe(true);
    expect(set.len()).toBe(3);

    // Removing k2 leaves a tombstone; k3 must still be reachable across tombstone
    expect(set.remove(k2)).toBe(true);
    expect(set.tombstones).toBe(1);
    expect(set.contains(k1)).toBe(true);
    expect(set.contains(k2)).toBe(false);
    expect(set.contains(k3)).toBe(true);

    // Re-inserting k4 colliding on bucket 5 recycles the tombstone slot
    const k4: u64 = 5 + 192;
    expect(set.insert(k4) catch false).toBe(true);
    expect(set.tombstones).toBe(0);
    expect(set.contains(k4)).toBe(true);
    expect(set.contains(k3)).toBe(true);
    expect(set.len()).toBe(3);
});

test("collections identityContext: IdentitySetU64 full-table guard and clear", fn () {
    let set = IdentitySetU64.init();
    let i: u64 = 0;
    while (i < @as(u64, @intCast(IDENTITY_TABLE_SLOTS))) {
        expect(set.insert(i * 131 + 1) catch false).toBe(true);
        i += 1;
    }
    expect(set.len()).toBe(IDENTITY_TABLE_SLOTS);

    // 65th distinct key returns TableFull
    let got_full = false;
    * = set.insert(0xFFFFFFFFFFFF) catch |e| {
        got_full = e == IdentityMapError.TableFull;
    };
    expect(got_full).toBe(true);

    // Existing key still returns false (not TableFull)
    expect(set.insert(1) catch true).toBe(false);

    set.clear();
    expect(set.isEmpty()).toBe(true);
    expect(set.tombstones).toBe(0);
    expect(set.contains(1)).toBe(false);
});

test("collections identityContext: IdentityMapU64 put, update, get and remove", fn () {
    let map = IdentityMapU64.init();
    expect(map.isEmpty()).toBe(true);
    expect(map.get(42) == null).toBe(true);

    expect(map.put(42, 1000) catch false).toBe(true);
    expect(map.put(42 + 64, 2000) catch false).toBe(true); // same bucket
    expect(map.len()).toBe(2);
    expect(map.get(42) ?? 0).toBe(1000);
    expect(map.get(42 + 64) ?? 0).toBe(2000);

    // Update existing key returns false and preserves len
    expect(map.put(42, 1500) catch true).toBe(false);
    expect(map.len()).toBe(2);
    expect(map.get(42) ?? 0).toBe(1500);

    // Remove first colliding key; second remains intact
    expect(map.remove(42) ?? 0).toBe(1500);
    expect(map.contains(42)).toBe(false);
    expect(map.get(42 + 64) ?? 0).toBe(2000);
    expect(map.len()).toBe(1);
    expect(map.remove(42) == null).toBe(true);
});

test("collections identityContext: IdentityMapU64 full-table guard and tombstone reuse", fn () {
    let map = IdentityMapU64.init();
    let i: u64 = 0;
    while (i < @as(u64, @intCast(IDENTITY_TABLE_SLOTS))) {
        * = map.put(i + 1000, i * 10) catch false;
        i += 1;
    }
    expect(map.len()).toBe(IDENTITY_TABLE_SLOTS);

    // Table full rejects new key
    let got_full = false;
    * = map.put(999999, 1) catch |e| {
        got_full = e == IdentityMapError.TableFull;
    };
    expect(got_full).toBe(true);

    // Removing one key frees a tombstone slot usable by a new key
    expect(map.remove(1000) ?? 999).toBe(0);
    expect(map.put(999999, 777) catch false).toBe(true);
    expect(map.get(999999) ?? 0).toBe(777);
    expect(map.len()).toBe(IDENTITY_TABLE_SLOTS);

    map.clear();
    expect(map.isEmpty()).toBe(true);
});
