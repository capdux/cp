import { test, expect, expect_eq } from "cap:test";
import {
    INDEX_STR_MAP_BUCKETS,
    INDEX_STR_MAP_MAX_LOAD,
    INDEX_STR_MAP_ARENA_CAP,
    IndexStringEntry,
    IndexStringMap,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("bundler index_string_map: basic put, get, overwrite & getOrPut", fn() {
    let map = IndexStringMap.init();
    expect(map.isEmpty());
    expect_eq(map.count(), 0);

    const ov1 = map.put(10, "dist/entry.js") catch {
        expect(false);
        return;
    };
    expect(!ov1);
    expect_eq(map.count(), 1);
    expect(map.contains(10));
    expect(!map.contains(99));

    if (v := map.get(10)) {
        expect(strEq(v, "dist/entry.js"));
    } else {
        expect(false);
    }

    const ov2 = map.put(10, "dist/entry-v2.js") catch {
        expect(false);
        return;
    };
    expect(ov2);
    expect_eq(map.count(), 1);

    const existing = map.getOrPut(10, "ignored.js") catch {
        expect(false);
        return;
    };
    expect(strEq(existing, "dist/entry-v2.js"));

    const inserted = map.getOrPut(20, "dist/shared.js") catch {
        expect(false);
        return;
    };
    expect(strEq(inserted, "dist/shared.js"));
    expect_eq(map.count(), 2);
});

test("bundler index_string_map: putCopy isolates ephemeral stack buffer via inline arena", fn() {
    let map = IndexStringMap.init();
    let scratch: [16]u8 = [0; 16];
    scratch[0] = 'c';
    scratch[1] = 'h';
    scratch[2] = 'u';
    scratch[3] = 'n';
    scratch[4] = 'k';
    scratch[5] = '.';
    scratch[6] = 'j';
    scratch[7] = 's';

    * = map.putCopy(7, scratch[0..8]) catch {
        expect(false);
        return;
    };
    expect_eq(map.arenaUsed(), 8);
    expect_eq(map.arenaRemaining(), INDEX_STR_MAP_ARENA_CAP - 8);

    // 立即覆写原临时缓冲区，验证 map 内数据完全独立无损
    scratch[0] = 'X';
    scratch[1] = 'Y';

    if (stored := map.get(7)) {
        expect(strEq(stored, "chunk.js"));
    } else {
        expect(false);
    }
});

test("bundler index_string_map: deterministic collectSortedEntries & reverse lookup", fn() {
    let map = IndexStringMap.init();
    * = map.put(30, "c.js") catch {
        expect(false);
        return;
    };
    * = map.putCopy(5, "a.js") catch {
        expect(false);
        return;
    };
    * = map.put(15, "b.js") catch {
        expect(false);
        return;
    };

    if (idx := map.findIndexByValue("a.js")) {
        expect_eq(idx, 5);
    } else {
        expect(false);
    }
    expect(map.findIndexByValue("missing.js") == null);

    let out: [4]IndexStringEntry = [IndexStringEntry{}; 4];
    const n = map.collectSortedEntries(out[0..]) catch {
        expect(false);
        return;
    };
    expect_eq(n, 3);
    expect_eq(out[0].index, 5);
    expect(strEq(out[0].value, "a.js"));
    expect_eq(out[1].index, 15);
    expect(strEq(out[1].value, "b.js"));
    expect_eq(out[2].index, 30);
    expect(strEq(out[2].value, "c.js"));

    // 导出缓冲不足防护
    let tiny: [2]IndexStringEntry = [IndexStringEntry{}; 2];
    let ok_buf = true;
    * = map.collectSortedEntries(tiny[0..]) catch {
        ok_buf = false;
    };
    expect(!ok_buf);
});

test("bundler index_string_map: tombstone-safe remove, re-insert & clear", fn() {
    let map = IndexStringMap.init();
    * = map.put(1, "one") catch {
        expect(false);
        return;
    };
    * = map.put(2, "two") catch {
        expect(false);
        return;
    };
    * = map.put(3, "three") catch {
        expect(false);
        return;
    };

    expect(map.remove(2));
    expect(!map.remove(2));
    expect_eq(map.count(), 2);
    expect(map.get(2) == null);

    if (v1 := map.get(1)) {
        expect(strEq(v1, "one"));
    } else {
        expect(false);
    }
    if (v3 := map.get(3)) {
        expect(strEq(v3, "three"));
    } else {
        expect(false);
    }

    * = map.putCopy(2, "two-new") catch {
        expect(false);
        return;
    };
    expect_eq(map.count(), 3);
    if (v2 := map.get(2)) {
        expect(strEq(v2, "two-new"));
    } else {
        expect(false);
    }

    map.clear();
    expect(map.isEmpty());
    expect_eq(map.arenaUsed(), 0);
    expect(map.get(1) == null);
});

test("bundler index_string_map: max-load capacity guard", fn() {
    expect_eq(INDEX_STR_MAP_BUCKETS, 256);
    expect_eq(INDEX_STR_MAP_MAX_LOAD, 192);
    expect_eq(INDEX_STR_MAP_ARENA_CAP, 8192);

    let map = IndexStringMap.init();
    let i: u32 = 0;
    while (i < INDEX_STR_MAP_MAX_LOAD) {
        * = map.put(i, "x") catch {
            expect(false);
            return;
        };
        i += 1;
    }
    expect_eq(map.count(), INDEX_STR_MAP_MAX_LOAD);

    let ok_full = true;
    * = map.put(9999, "overflow") catch {
        ok_full = false;
    };
    expect(!ok_full);
});

test("bundler index_string_map: inline arena capacity overflow guard", fn() {
    let map = IndexStringMap.init();
    let chunk: [1024]u8 = [0; 1024];
    let i: u32 = 0;
    while (i < 8) {
        * = map.putCopy(i, chunk[0..]) catch {
            expect(false);
            return;
        };
        i += 1;
    }
    expect_eq(map.arenaUsed(), INDEX_STR_MAP_ARENA_CAP);
    expect_eq(map.arenaRemaining(), 0);

    // 再拷贝 1 字节应触发 ArenaFull
    let ok_arena = true;
    * = map.putCopy(99, "z") catch {
        ok_arena = false;
    };
    expect(!ok_arena);
});
