// Source: cp/cap/test/test_ar_production_deep.cp (Production deep tests for cap:ar)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    AR_MAGIC,
    AR_HEADER_SIZE,
    AR_END_MAGIC,
    ArMemberKind,
    ArMember,
    isAr,
    ArIter,
    findArMember,
    countArMembers,
} from "cap:ar";

fn putPadded(dst: []u8, off: usize, width: usize, text: []const u8) {
    let i: usize = 0;
    while (i < width) {
        dst[off + i] = if (i < text.len) text[i] else ' ';
        i += 1;
    }
}

fn writeU64DecStr(dst: []u8, val: u64): usize {
    if (val == 0) {
        dst[0] = '0';
        return 1;
    }
    let tmp: [20]u8 = undefined;
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = ('0' + ((v % 10) as u8)) as u8;
        v = v / 10;
        n += 1;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = tmp[n - 1 - i];
        i += 1;
    }
    return n;
}

fn appendMember(
    dst: []u8,
    off: usize,
    raw_name: []const u8,
    mtime_str: []const u8,
    uid_str: []const u8,
    gid_str: []const u8,
    mode_str: []const u8,
    payload: []const u8,
): usize {
    let size_buf: [20]u8 = undefined;
    const size_len = writeU64DecStr(size_buf[0..20], payload.len as u64);

    putPadded(dst, off + 0, 16, raw_name);
    putPadded(dst, off + 16, 12, mtime_str);
    putPadded(dst, off + 28, 6, uid_str);
    putPadded(dst, off + 34, 6, gid_str);
    putPadded(dst, off + 40, 8, mode_str);
    putPadded(dst, off + 48, 10, size_buf[0..size_len]);
    dst[off + 58] = '`';
    dst[off + 59] = '\n';

    let p = off + 60;
    let i: usize = 0;
    while (i < payload.len) {
        dst[p + i] = payload[i];
        i += 1;
    }
    p += payload.len;

    // 2-byte even alignment pad for odd payload sizes
    if ((payload.len & 1) == 1) {
        dst[p] = '\n';
        p += 1;
    }
    return p - off;
}

fn copyMagic(dst: []u8): usize {
    let i: usize = 0;
    while (i < 8) {
        dst[i] = AR_MAGIC[i];
        i += 1;
    }
    return 8;
}

test("1. .deb style archive parses odd-sized members with 2-byte alignment and octal mode", () => {
    let buf: [256]u8 = undefined;
    const s = buf[0..256];
    let pos = copyMagic(s);
    // Odd length 5 ("2.0\n" is 4, let's use "2.0\r\n" len 5 to exercise odd padding)
    pos += appendMember(s, pos, "debian-binary", "1700000000", "0", "0", "100644", "2.0\r\n");
    pos += appendMember(s, pos, "control.tar.gz/", "1700000001", "1000", "1000", "100640", "CTRLDATA");

    expect_eq(isAr(s[0..pos]), true);
    expect_eq(countArMembers(s[0..pos]), 2);

    let it = ArIter.init(s[0..pos]);
    const m0 = it.next() ?? unreachable;
    expect_eq(m0.kind == ArMemberKind.normal, true);
    expect_eq(std.mem.eql(u8, m0.name, "debian-binary"), true);
    expect_eq(m0.mtime, 1700000000);
    expect_eq(m0.mode, 0o100644);
    expect_eq(m0.size, 5);
    expect_eq(std.mem.eql(u8, m0.data, "2.0\r\n"), true);

    const m1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m1.name, "control.tar.gz"), true);
    expect_eq(m1.uid, 1000);
    expect_eq(m1.gid, 1000);
    expect_eq(m1.mode, 0o100640);
    expect_eq(std.mem.eql(u8, m1.data, "CTRLDATA"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("2. GNU static library resolves '/' symtab, '//' string table, and '/<offset>' long filenames", () => {
    let buf: [512]u8 = undefined;
    const s = buf[0..512];
    let pos = copyMagic(s);

    pos += appendMember(s, pos, "/", "0", "0", "0", "0", "SYMTAB");
    const strtab = "very_long_object_file_one.o/\nsecond_super_long_module.o/\n";
    pos += appendMember(s, pos, "//", "", "", "", "", strtab);
    pos += appendMember(s, pos, "/0", "123", "0", "0", "100644", "OBJ1");
    pos += appendMember(s, pos, "/29", "124", "0", "0", "100644", "OBJ2!");

    let it = ArIter.init(s[0..pos]);

    const sym = it.next() ?? unreachable;
    expect_eq(sym.kind == ArMemberKind.gnu_symtab, true);

    const tbl = it.next() ?? unreachable;
    expect_eq(tbl.kind == ArMemberKind.gnu_strtab, true);

    const o1 = it.next() ?? unreachable;
    expect_eq(o1.kind == ArMemberKind.normal, true);
    expect_eq(std.mem.eql(u8, o1.name, "very_long_object_file_one.o"), true);
    expect_eq(std.mem.eql(u8, o1.data, "OBJ1"), true);

    const o2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, o2.name, "second_super_long_module.o"), true);
    expect_eq(std.mem.eql(u8, o2.data, "OBJ2!"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. BSD '#1/<len>' long filenames and '__.SYMDEF SORTED' strip embedded name bytes from payload", () => {
    let buf: [384]u8 = undefined;
    const s = buf[0..384];
    let pos = copyMagic(s);

    // BSD symtab via #1/20 ("__.SYMDEF SORTED\0\0\0\0" + "SYMDATA")
    const sym_payload = "__.SYMDEF SORTED\x00\x00\x00\x00SYMDATA";
    pos += appendMember(s, pos, "#1/20", "100", "501", "20", "100644", sym_payload);

    // BSD normal file via #1/24 ("my_long_bsd_archive.o\0\0\0" + "PAYLOAD")
    const obj_payload = "my_long_bsd_archive.o\x00\x00\x00PAYLOAD";
    pos += appendMember(s, pos, "#1/24", "101", "501", "20", "100644", obj_payload);

    let it = ArIter.init(s[0..pos]);
    const m0 = it.next() ?? unreachable;
    expect_eq(m0.kind == ArMemberKind.bsd_symtab, true);
    expect_eq(std.mem.eql(u8, m0.name, "__.SYMDEF SORTED"), true);
    expect_eq(std.mem.eql(u8, m0.data, "SYMDATA"), true);
    expect_eq(m0.size, 7);

    const m1 = it.next() ?? unreachable;
    expect_eq(m1.kind == ArMemberKind.normal, true);
    expect_eq(std.mem.eql(u8, m1.name, "my_long_bsd_archive.o"), true);
    expect_eq(std.mem.eql(u8, m1.data, "PAYLOAD"), true);
    expect_eq(m1.size, 7);

    expect_eq(it.next() == null, true);
});

test("4. findArMember and countArMembers work across GNU long names and missing entries", () => {
    let buf: [384]u8 = undefined;
    const s = buf[0..384];
    let pos = copyMagic(s);
    pos += appendMember(s, pos, "//", "", "", "", "", "long_target_entry.o/\n");
    pos += appendMember(s, pos, "short.o/", "0", "0", "0", "100644", "AA");
    pos += appendMember(s, pos, "/0", "0", "0", "0", "100644", "TARGET_OK");

    expect_eq(countArMembers(s[0..pos]), 3);
    const hit = findArMember(s[0..pos], "long_target_entry.o") ?? unreachable;
    expect_eq(std.mem.eql(u8, hit.data, "TARGET_OK"), true);
    expect_eq(findArMember(s[0..pos], "nonexistent.o") == null, true);
});

test("5. ArIter rejects bad magic, missing header end marker, truncated headers, and out-of-bounds size", () => {
    expect_eq(isAr("not-an-archive"), false);
    expect_eq(countArMembers("not-an-archive"), 0);

    let buf: [128]u8 = undefined;
    const s = buf[0..128];
    let pos = copyMagic(s);
    pos += appendMember(s, pos, "a.o", "0", "0", "0", "100644", "1234");

    // Corrupt end magic (`\n -> XX)
    let bad_end: [128]u8 = buf;
    bad_end[8 + 58] = 'X';
    let it1 = ArIter.init(bad_end[0..pos]);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.had_error, true);

    // Truncated payload (missing last 2 bytes)
    let it2 = ArIter.init(s[0 .. pos - 2]);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.had_error, true);

    // Truncated header (< 60 bytes after magic)
    let it3 = ArIter.init(s[0..40]);
    expect_eq(it3.next() == null, true);
    expect_eq(it3.had_error, true);
});
