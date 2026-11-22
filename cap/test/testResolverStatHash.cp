import {
    STAT_HASH_SEED,
    STAT_HTTP_DATE_LEN,
    STAT_STRONG_ETAG_LEN,
    STAT_WEAK_ETAG_LEN,
    STAT_MODE_LNK,
    StatHashError,
    FileStatMeta,
    formatHttpDate,
    FileStatHash,
} from "cap:resolver";
import { expect, test } from "cap:test";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("resolver statHash: formatHttpDate RFC 9110 IMF-fixdate accuracy & leap day", fn () {
    let buf: [32]u8 = [0; 32];

    // Unix epoch 0 -> Thu, 01 Jan 1970 00:00:00 GMT
    const n0 = formatHttpDate(0, buf[0..]) catch 0;
    expect(n0).toBe(STAT_HTTP_DATE_LEN);
    expect(sliceEql(buf[0..n0], "Thu, 01 Jan 1970 00:00:00 GMT")).toBe(true);

    // Classic RFC 7231 example: 784111777s -> Sun, 06 Nov 1994 08:49:37 GMT
    const n1 = formatHttpDate(784111777000, buf[0..]) catch 0;
    expect(n1).toBe(STAT_HTTP_DATE_LEN);
    expect(sliceEql(buf[0..n1], "Sun, 06 Nov 1994 08:49:37 GMT")).toBe(true);

    // Leap day 2024-02-29 23:59:59 UTC -> 1709251199s -> Thu, 29 Feb 2024 23:59:59 GMT
    const n2 = formatHttpDate(1709251199000, buf[0..]) catch 0;
    expect(n2).toBe(STAT_HTTP_DATE_LEN);
    expect(sliceEql(buf[0..n2], "Thu, 29 Feb 2024 23:59:59 GMT")).toBe(true);
});

test("resolver statHash: FileStatHash update change detection & Last-Modified cache", fn () {
    expect(STAT_HASH_SEED).toBe(42);
    let sh = FileStatHash.init();
    expect(sh.hasLastModified()).toBe(false);

    const m1 = FileStatMeta.regular(2048, 784111777, 500000000, 9001);
    expect(m1.isRegularFile()).toBe(true);
    expect(m1.mtimeMsUnsigned()).toBe(784111777500);

    // First update always changes from initial 0
    const c1 = sh.update(m1, "/app/src/index.ts");
    expect(c1).toBe(true);
    expect(sh.value != 0).toBe(true);
    expect(sh.hasLastModified()).toBe(true);
    expect(sh.last_modified_u64).toBe(784111777500);

    let lm_ok = false;
    if (lm := sh.lastModified()) {
        lm_ok = sliceEql(lm, "Sun, 06 Nov 1994 08:49:37 GMT");
    }
    expect(lm_ok).toBe(true);

    // Identical stat + path returns changed=false and preserves cached Last-Modified
    const c2 = sh.update(m1, "/app/src/index.ts");
    expect(c2).toBe(false);
    expect(sh.hasLastModified()).toBe(true);

    // Changing size or path invalidates fingerprint
    const prev_val = sh.value;
    const c3 = sh.update(m1, "/app/src/other.ts");
    expect(c3).toBe(true);
    expect(sh.value != prev_val).toBe(true);
});

test("resolver statHash: non-regular files and zero/negative mtime clear Last-Modified", fn () {
    let sh = FileStatHash.init();
    const reg = FileStatMeta.regular(128, 1709251199, 0, 77);
    * = sh.update(reg, "/app/file.js");
    expect(sh.hasLastModified()).toBe(true);

    // Directory clears cached Last-Modified
    const dir = FileStatMeta.directory(1709251199, 78);
    expect(dir.isDirectory()).toBe(true);
    expect(dir.isRegularFile()).toBe(false);
    * = sh.update(dir, "/app/dir");
    expect(sh.hasLastModified()).toBe(false);
    expect(sh.last_modified_u64).toBe(0);

    // Symlink also clears Last-Modified
    const lnk = FileStatMeta{
        size: 16,
        mode: STAT_MODE_LNK | 0o777,
        mtime_sec: 1709251199,
        mtime_nsec: 0,
        ino: 79,
    };
    expect(lnk.isSymlink()).toBe(true);
    * = sh.update(lnk, "/app/link");
    expect(sh.hasLastModified()).toBe(false);

    // Regular file with negative/zero mtime clears Last-Modified
    const zero_mtime = FileStatMeta.regular(64, -10, 0, 80);
    expect(zero_mtime.mtimeMsUnsigned()).toBe(0);
    * = sh.update(zero_mtime, "/app/virtual.js");
    expect(sh.hasLastModified()).toBe(false);
});

test("resolver statHash: strong and weak HTTP ETag formatting", fn () {
    let sh = FileStatHash.init();
    const meta = FileStatMeta.regular(512, 1709251199, 0, 42);
    * = sh.update(meta, "/app/app.css");

    let sbuf: [32]u8 = [0; 32];
    const sn = sh.formatStrongEtag(sbuf[0..]) catch 0;
    expect(sn).toBe(STAT_STRONG_ETAG_LEN);
    expect(sbuf[0]).toBe('"');
    expect(sbuf[17]).toBe('"');

    let wbuf: [32]u8 = [0; 32];
    const wn = sh.formatWeakEtag(wbuf[0..]) catch 0;
    expect(wn).toBe(STAT_WEAK_ETAG_LEN);
    expect(wbuf[0]).toBe('W');
    expect(wbuf[1]).toBe('/');
    expect(wbuf[2]).toBe('"');
    expect(wbuf[19]).toBe('"');
    // Hex payload matches between strong and weak ETags
    expect(sliceEql(sbuf[1..17], wbuf[3..19])).toBe(true);
});

test("resolver statHash: determinism and equality across independent instances", fn () {
    let a = FileStatHash.init();
    let b = FileStatHash.init();
    const meta = FileStatMeta.regular(1024, 1700000000, 123456789, 555);
    * = a.update(meta, "/pkg/mod.cp");
    * = b.update(meta, "/pkg/mod.cp");
    expect(a.eql(b)).toBe(true);

    // Perturbing inode alone changes fingerprint
    const meta2 = FileStatMeta.regular(1024, 1700000000, 123456789, 556);
    * = b.update(meta2, "/pkg/mod.cp");
    expect(a.eql(b)).toBe(false);
});

test("resolver statHash: buffer too small and year out-of-range guards", fn () {
    let tiny: [16]u8 = [0; 16];

    let date_small = false;
    * = formatHttpDate(784111777000, tiny[0..]) catch |e| {
        date_small = e == StatHashError.BufferTooSmall;
    };
    expect(date_small).toBe(true);

    // Year > 9999 (~315576000000s = ~10000 years)
    let big: [32]u8 = [0; 32];
    let year_oob = false;
    * = formatHttpDate(315576000000000, big[0..]) catch |e| {
        year_oob = e == StatHashError.YearOutOfRange;
    };
    expect(year_oob).toBe(true);

    let sh = FileStatHash.init();
    let strong_small = false;
    * = sh.formatStrongEtag(tiny[0..]) catch |e| {
        strong_small = e == StatHashError.BufferTooSmall;
    };
    expect(strong_small).toBe(true);

    let weak_small = false;
    * = sh.formatWeakEtag(tiny[0..]) catch |e| {
        weak_small = e == StatHashError.BufferTooSmall;
    };
    expect(weak_small).toBe(true);
});
