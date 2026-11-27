import { test, expect_eq } from "cap:test";
import {
    FEC_CAPACITY,
    FileCacheEntry,
    FileEntryCache,
} from "../../packages/file-entry-cache/src/index.cp";

fn main() {
    test("1. @cap/file-entry-cache — set and get round-trip with metadata", () => {
        let cache = FileEntryCache.new();
        expect_eq(cache.len(), 0 as usize);
        expect_eq(cache.set("src/main.cp", 1700000001, 512, 0x11223344, "ok"), true);
        expect_eq(cache.len(), 1 as usize);

        if (e := cache.get("src/main.cp")) {
            expect_eq(e.path, "src/main.cp");
            expect_eq(e.mtime_ms, 1700000001 as i64);
            expect_eq(e.size, 512 as u64);
            expect_eq(e.content_hash, 0x11223344 as u64);
            expect_eq(e.payload, "ok");
            expect_eq(e.valid, true);
        } else {
            expect_eq(true, false);
        }
    });

    test("2. @cap/file-entry-cache — hasChanged detects uncached, mtime, size, and hash diffs", () => {
        let cache = FileEntryCache.new();
        expect_eq(cache.hasChanged("src/a.cp", 100, 200, 300), true);

        * = cache.set("src/a.cp", 100, 200, 300, "lint:clean");
        expect_eq(cache.hasChanged("src/a.cp", 100, 200, 300), false);
        expect_eq(cache.hasChanged("src/a.cp", 101, 200, 300), true);
        expect_eq(cache.hasChanged("src/a.cp", 100, 201, 300), true);
        expect_eq(cache.hasChanged("src/a.cp", 100, 200, 301), true);
    });

    test("3. @cap/file-entry-cache — set updates existing path in place without growing len", () => {
        let cache = FileEntryCache.new();
        * = cache.set("src/app.cp", 1000, 100, 111, "v1");
        * = cache.set("src/app.cp", 2000, 150, 222, "v2");
        expect_eq(cache.len(), 1 as usize);
        expect_eq(cache.hasChanged("src/app.cp", 2000, 150, 222), false);

        if (e := cache.get("src/app.cp")) {
            expect_eq(e.payload, "v2");
            expect_eq(e.size, 150 as u64);
        } else {
            expect_eq(true, false);
        }
    });

    test("4. @cap/file-entry-cache — remove deletes entry and frees slot for reuse", () => {
        let cache = FileEntryCache.new();
        * = cache.set("a.cp", 1, 10, 100, "A");
        * = cache.set("b.cp", 2, 20, 200, "B");
        expect_eq(cache.remove("missing.cp"), false);
        expect_eq(cache.remove("a.cp"), true);
        expect_eq(cache.len(), 1 as usize);
        expect_eq(cache.get("a.cp") == null, true);
        expect_eq(cache.hasChanged("a.cp", 1, 10, 100), true);
        expect_eq(cache.set("c.cp", 3, 30, 300, "C"), true);
        expect_eq(cache.len(), 2 as usize);
    });

    test("5. @cap/file-entry-cache — reconcile purges deleted files and returns removed count", () => {
        let cache = FileEntryCache.new();
        * = cache.set("src/a.cp", 1, 10, 1, "a");
        * = cache.set("src/b.cp", 2, 20, 2, "b");
        * = cache.set("src/c.cp", 3, 30, 3, "c");
        * = cache.set("src/d.cp", 4, 40, 4, "d");

        const existing: [2]string = ["src/b.cp", "src/d.cp"];
        const purged = cache.reconcile(existing[0..2]);
        expect_eq(purged, 2 as usize);
        expect_eq(cache.len(), 2 as usize);
        expect_eq(cache.get("src/a.cp") == null, true);
        expect_eq(cache.get("src/c.cp") == null, true);
        expect_eq(cache.hasChanged("src/b.cp", 2, 20, 2), false);
        expect_eq(cache.hasChanged("src/d.cp", 4, 40, 4), false);
    });

    test("6. @cap/file-entry-cache — enforces 64-slot capacity and rejects overflow", () => {
        let cache = FileEntryCache.new();
        const names: [64]string = [
            "f00", "f01", "f02", "f03", "f04", "f05", "f06", "f07",
            "f08", "f09", "f10", "f11", "f12", "f13", "f14", "f15",
            "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
            "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
            "f32", "f33", "f34", "f35", "f36", "f37", "f38", "f39",
            "f40", "f41", "f42", "f43", "f44", "f45", "f46", "f47",
            "f48", "f49", "f50", "f51", "f52", "f53", "f54", "f55",
            "f56", "f57", "f58", "f59", "f60", "f61", "f62", "f63",
        ];
        let i: usize = 0;
        while (i < FEC_CAPACITY) : (i += 1) {
            expect_eq(cache.set(names[i], i as i64, i as u64, i as u64, "ok"), true);
        }
        expect_eq(cache.len(), 64 as usize);
        expect_eq(cache.set("f64", 64, 64, 64, "overflow"), false);
        expect_eq(cache.set("f00", 99, 99, 99, "updated"), true);
        expect_eq(cache.len(), 64 as usize);
    });
}
