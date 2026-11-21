// Source: cp/cap/test/test_romfs_production_deep.cp (Production deep tests for cap:romfs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ROMFS_MAGIC,
    ROMFS_ALIGN,
    ROMFS_MAX_CHECKSUM_LEN,
    ROMFS_TYPE_MASK,
    ROMFS_EXEC_BIT,
    ROMFS_NEXT_MASK,
    TYPE_HARD_LINK,
    TYPE_DIRECTORY,
    TYPE_REGULAR,
    TYPE_SYMLINK,
    TYPE_BLOCK_DEV,
    TYPE_CHAR_DEV,
    TYPE_SOCKET,
    TYPE_FIFO,
    romfsChecksum,
    RomfsSuperblock,
    RomfsEntry,
    RomfsDirIter,
    RomfsImage,
    isRomfs,
    parseRomfs,
} from "cap:romfs";

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn eqStr(a: []const u8, b: []const u8): bool {
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

fn align16(v: usize): usize {
    return ((v + 15) / 16) * 16;
}

// Writes a 16B-aligned romfs entry at `off` and returns the next 16B-aligned offset.
fn writeEntry(
    buf: []u8,
    off: usize,
    next_off: u32,
    ftype: u8,
    exec: bool,
    spec_info: u32,
    name: []const u8,
    payload: []const u8,
): usize {
    let nextfh = (next_off & ROMFS_NEXT_MASK) | ((ftype as u32) & ROMFS_TYPE_MASK);
    if (exec) {
        nextfh = nextfh | ROMFS_EXEC_BIT;
    }
    putU32Be(buf, off, nextfh);
    putU32Be(buf, off + 4, spec_info);
    putU32Be(buf, off + 8, payload.len as u32);
    putU32Be(buf, off + 12, 0); // checksum placeholder
    copyBytes(buf, off + 16, name);
    buf[off + 16 + name.len] = 0;
    const hdr_end = align16(off + 16 + name.len + 1);
    const partial = romfsChecksum(buf[off..hdr_end]);
    const fix = ((0x100000000 as u64 - (partial as u64)) & 0xFFFFFFFF) as u32;
    putU32Be(buf, off + 12, fix);

    if (payload.len > 0) {
        copyBytes(buf, hdr_end, payload);
        return align16(hdr_end + payload.len);
    }
    return hdr_end;
}

fn sealSuperblock(buf: []u8, vol_name: []const u8, full_size: usize) {
    copyBytes(buf, 0, ROMFS_MAGIC);
    putU32Be(buf, 8, full_size as u32);
    putU32Be(buf, 12, 0);
    copyBytes(buf, 16, vol_name);
    buf[16 + vol_name.len] = 0;
    let check_len = full_size;
    if (check_len > ROMFS_MAX_CHECKSUM_LEN) {
        check_len = ROMFS_MAX_CHECKSUM_LEN;
    }
    const partial = romfsChecksum(buf[0..check_len]);
    const fix = ((0x100000000 as u64 - (partial as u64)) & 0xFFFFFFFF) as u32;
    putU32Be(buf, 12, fix);
}

test("1. romfs superblock parsing, 32-bit checksum & 16B-aligned volume_name", () => {
    expect_eq(ROMFS_ALIGN, 16 as usize);
    expect_eq(ROMFS_MAX_CHECKSUM_LEN, 512 as usize);
    expect_eq(TYPE_HARD_LINK, 0 as u8);
    expect_eq(TYPE_BLOCK_DEV, 4 as u8);
    expect_eq(TYPE_CHAR_DEV, 5 as u8);
    expect_eq(TYPE_SOCKET, 6 as u8);
    expect_eq(TYPE_FIFO, 7 as u8);

    // Superblock: 16B hdr + "initrd" (6 + 1 NUL -> aligned to 32B) => first_file_offset = 32
    // Single root file at 32: "hello.txt" with content "Hello romfs!"
    let img: [128]u8 = std.mem.zeroes([128]u8);
    const end_off = writeEntry(img[0..], 32, 0, TYPE_REGULAR, false, 0, "hello.txt", "Hello romfs!");
    sealSuperblock(img[0..], "initrd", end_off);

    expect_eq(isRomfs(img[0..end_off]), true);
    expect_eq(romfsChecksum(img[0..end_off]), 0 as u32);

    const fs = parseRomfs(img[0..end_off]) ?? unreachable;
    expect_eq(fs.superblock.full_size, end_off as u32);
    expect_eq(eqStr(fs.superblock.volume_name, "initrd"), true);
    expect_eq(fs.superblock.first_file_offset, 32 as usize);
});

test("2. Root & subdirectory multi-entry traversal (Directory, Regular, Executable, Symlink) & zero-copy data", () => {
    // Layout:
    // 0..32   : Superblock ("cp-rom") -> first_file_offset = 32
    // 32..64  : Root entry 0: dir "bin" (next=64, spec_info=144 -> first child at 144)
    // 64..112 : Root entry 1: regular "motd" (next=112, content="Welcome to CP!")
    // 112..144: Root entry 2: symlink "sh" (next=0, target="/bin/cp-sh")
    // 144..192: Child of "bin": executable regular "cp-sh" (next=0, content="#!/bin/cp")
    let img: [256]u8 = std.mem.zeroes([256]u8);
    const off1 = writeEntry(img[0..], 32, 64, TYPE_DIRECTORY, true, 144, "bin", "");
    expect_eq(off1, 64 as usize);
    const off2 = writeEntry(img[0..], 64, 112, TYPE_REGULAR, false, 0, "motd", "Welcome to CP!");
    expect_eq(off2, 112 as usize);
    const off3 = writeEntry(img[0..], 112, 0, TYPE_SYMLINK, false, 0, "sh", "/bin/cp-sh");
    expect_eq(off3, 160 as usize);
    // Re-write "bin" with spec_info = 160 so child starts at 160 cleanly
    * = writeEntry(img[0..], 32, 64, TYPE_DIRECTORY, true, 160, "bin", "");
    const end_off = writeEntry(img[0..], 160, 0, TYPE_REGULAR, true, 0, "cp-sh", "#!/bin/cp");
    sealSuperblock(img[0..], "cp-rom", end_off);

    const fs = parseRomfs(img[0..end_off]) ?? unreachable;
    let root_it = fs.rootEntries();

    const r0 = root_it.next() ?? unreachable;
    expect_eq(r0.isDir(), true);
    expect_eq(r0.is_executable, true);
    expect_eq(eqStr(r0.name, "bin"), true);
    expect_eq(r0.firstChildOffset() ?? unreachable, 160 as u32);

    const r1 = root_it.next() ?? unreachable;
    expect_eq(r1.isRegular(), true);
    expect_eq(r1.is_executable, false);
    expect_eq(eqStr(r1.name, "motd"), true);
    expect_eq(eqStr(r1.data, "Welcome to CP!"), true);

    const r2 = root_it.next() ?? unreachable;
    expect_eq(r2.isSymlink(), true);
    expect_eq(eqStr(r2.name, "sh"), true);
    expect_eq(eqStr(r2.data, "/bin/cp-sh"), true);

    expect_eq(root_it.next() == null, true);

    // Traverse subdirectory "bin"
    let bin_it = fs.dirEntries(r0);
    const c0 = bin_it.next() ?? unreachable;
    expect_eq(c0.isRegular(), true);
    expect_eq(c0.is_executable, true);
    expect_eq(eqStr(c0.name, "cp-sh"), true);
    expect_eq(eqStr(c0.data, "#!/bin/cp"), true);
    expect_eq(bin_it.next() == null, true);
});

test("3. findInDir & entryAt lookup in root and nested directory", () => {
    let img: [256]u8 = std.mem.zeroes([256]u8);
    * = writeEntry(img[0..], 32, 64, TYPE_DIRECTORY, false, 112, "etc", "");
    * = writeEntry(img[0..], 64, 0, TYPE_REGULAR, false, 0, "README", "romfs-doc");
    const end_off = writeEntry(img[0..], 112, 0, TYPE_REGULAR, false, 0, "hosts", "127.0.0.1");
    sealSuperblock(img[0..], "root", end_off);

    const fs = parseRomfs(img[0..end_off]) ?? unreachable;

    const readme = fs.findInDir(fs.superblock.first_file_offset, "README") ?? unreachable;
    expect_eq(eqStr(readme.data, "romfs-doc"), true);

    const etc_dir = fs.findInDir(fs.superblock.first_file_offset, "etc") ?? unreachable;
    const etc_child_off = (etc_dir.firstChildOffset() ?? unreachable) as usize;
    const hosts = fs.findInDir(etc_child_off, "hosts") ?? unreachable;
    expect_eq(eqStr(hosts.data, "127.0.0.1"), true);

    // Non-existent lookup returns null
    expect_eq(fs.findInDir(etc_child_off, "missing") == null, true);

    // Direct entryAt
    const direct = fs.entryAt(64) ?? unreachable;
    expect_eq(eqStr(direct.name, "README"), true);
    expect_eq(fs.entryAt(65) == null, true); // unaligned offset rejected
});

test("4. Cyclic next_offset loop guard terminates safely without infinite loop", () => {
    // Entry at 32 has next_offset = 64; Entry at 64 has next_offset = 32 (backward cycle!)
    let img: [128]u8 = std.mem.zeroes([128]u8);
    * = writeEntry(img[0..], 32, 64, TYPE_REGULAR, false, 0, "a", "");
    const end_off = writeEntry(img[0..], 64, 32, TYPE_REGULAR, false, 0, "b", "");
    sealSuperblock(img[0..], "cyc", end_off);

    const fs = parseRomfs(img[0..end_off]) ?? unreachable;
    let it = fs.rootEntries();

    const e0 = it.next() ?? unreachable;
    expect_eq(eqStr(e0.name, "a"), true);
    const e1 = it.next() ?? unreachable;
    expect_eq(eqStr(e1.name, "b"), true);
    // Cycle to offset 32 (<= 64) is detected and terminated
    expect_eq(it.next() == null, true);
});

test("5. Malformed romfs rejection (bad magic, bad checksum, full_size overflow, truncated)", () => {
    // 1. Truncated
    let short_buf: [16]u8 = std.mem.zeroes([16]u8);
    copyBytes(short_buf[0..], 0, ROMFS_MAGIC);
    expect_eq(isRomfs(short_buf[0..]), true);
    expect_eq(parseRomfs(short_buf[0..]) == null, true);

    // 2. Bad magic
    let img: [128]u8 = std.mem.zeroes([128]u8);
    const end_off = writeEntry(img[0..], 32, 0, TYPE_REGULAR, false, 0, "f", "ok");
    sealSuperblock(img[0..], "vol", end_off);

    img[0] = 0x58; // 'X'
    expect_eq(isRomfs(img[0..end_off]), false);
    expect_eq(parseRomfs(img[0..end_off]) == null, true);

    // 3. Bad superblock checksum
    sealSuperblock(img[0..], "vol", end_off);
    img[15] = img[15] ^ 0x01;
    expect_eq(parseRomfs(img[0..end_off]) == null, true);

    // 4. full_size > data.len
    sealSuperblock(img[0..], "vol", end_off);
    expect_eq(parseRomfs(img[0..(end_off - 16)]) == null, true);

    // 5. Corrupted file header checksum is rejected by entryAt
    let img2: [128]u8 = std.mem.zeroes([128]u8);
    const end2 = writeEntry(img2[0..], 32, 0, TYPE_REGULAR, false, 0, "f", "ok");
    img2[47] = img2[47] ^ 0xFF; // corrupt file entry checksum
    sealSuperblock(img2[0..], "vol", end2);
    const fs2 = parseRomfs(img2[0..end2]) ?? unreachable;
    expect_eq(fs2.entryAt(32) == null, true);
});
