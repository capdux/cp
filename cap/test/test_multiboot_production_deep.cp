// Source: cp/cap/test/test_multiboot_production_deep.cp (Production deep tests for cap:multiboot)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MB1_HEADER_MAGIC,
    MB1_BOOTLOADER_MAGIC,
    MB2_HEADER_MAGIC,
    MB2_BOOTLOADER_MAGIC,
    MB2_ARCH_I386,
    MB2_ARCH_MIPS32,
    TAG_END,
    TAG_CMDLINE,
    TAG_BOOT_LOADER_NAME,
    TAG_MODULE,
    TAG_BASIC_MEMINFO,
    TAG_MMAP,
    TAG_FRAMEBUFFER,
    MMAP_AVAILABLE,
    MMAP_RESERVED,
    MMAP_ACPI_RECLAIMABLE,
    MMAP_NVS,
    MMAP_BADRAM,
    parseMb1Header,
    parseMb2Header,
    parseMb2BootInfo,
    parseMb2BasicMemInfo,
    iterMb2Mmap,
    parseMb2Framebuffer,
    parseMb2Module,
} from "cap:multiboot";

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64Le(buf: []u8, off: usize, v: u64) {
    writeU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    writeU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn copyInto(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn align8(n: usize): usize {
    const rem = n % 8;
    if (rem == 0) {
        return n;
    }
    return n + (8 - rem);
}

fn emitTag(buf: []u8, pos: usize, tag_type: u32, body: []const u8): usize {
    const sz: usize = (8 as usize) + body.len;
    writeU32Le(buf, pos, tag_type);
    writeU32Le(buf, pos + 4, sz as u32);
    copyInto(buf, pos + 8, body);
    return pos + align8(sz);
}

fn buildSampleBootInfo(buf: []u8): usize {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }
    let p: usize = 8; // skip total_size + reserved

    // 1. TAG_CMDLINE
    p = emitTag(buf, p, TAG_CMDLINE, "root=/dev/vda1 quiet\x00");

    // 2. TAG_BOOT_LOADER_NAME
    p = emitTag(buf, p, TAG_BOOT_LOADER_NAME, "GRUB 2.12\x00");

    // 3. TAG_BASIC_MEMINFO (lower=640KB, upper=523264KB)
    let mem_body: [8]u8 = [0; 8];
    writeU32Le(mem_body[0..], 0, 640);
    writeU32Le(mem_body[0..], 4, 523264);
    p = emitTag(buf, p, TAG_BASIC_MEMINFO, mem_body[0..]);

    // 4. TAG_MODULE (0x100000..0x200000, "initrd.img\0")
    let mod_body: [19]u8 = [0; 19];
    writeU32Le(mod_body[0..], 0, 0x00100000);
    writeU32Le(mod_body[0..], 4, 0x00200000);
    copyInto(mod_body[0..], 8, "initrd.img\x00");
    p = emitTag(buf, p, TAG_MODULE, mod_body[0..]);

    // 5. TAG_MMAP (entry_size=24, version=0, 3 entries: Available, Reserved, ACPI)
    let mmap_body: [80]u8 = [0; 80]; // 8 + 3*24
    writeU32Le(mmap_body[0..], 0, 24);
    writeU32Le(mmap_body[0..], 4, 0);
    // Entry 0: Available 0x0..0x9FC00
    writeU64Le(mmap_body[0..], 8, 0x0);
    writeU64Le(mmap_body[0..], 16, 0x9FC00);
    writeU32Le(mmap_body[0..], 24, MMAP_AVAILABLE);
    // Entry 1: Reserved 0xF0000..0x10000
    writeU64Le(mmap_body[0..], 32, 0xF0000);
    writeU64Le(mmap_body[0..], 40, 0x10000);
    writeU32Le(mmap_body[0..], 48, MMAP_RESERVED);
    // Entry 2: ACPI Reclaimable 0x7FE0000..0x20000
    writeU64Le(mmap_body[0..], 56, 0x7FE0000);
    writeU64Le(mmap_body[0..], 64, 0x20000);
    writeU32Le(mmap_body[0..], 72, MMAP_ACPI_RECLAIMABLE);
    p = emitTag(buf, p, TAG_MMAP, mmap_body[0..]);

    // 6. TAG_FRAMEBUFFER (addr=0xFD000000, pitch=7680, 1920x1080, bpp=32, type=1)
    let fb_body: [24]u8 = [0; 24];
    writeU64Le(fb_body[0..], 0, 0xFD000000);
    writeU32Le(fb_body[0..], 8, 7680);
    writeU32Le(fb_body[0..], 12, 1920);
    writeU32Le(fb_body[0..], 16, 1080);
    fb_body[20] = 32;
    fb_body[21] = 1;
    p = emitTag(buf, p, TAG_FRAMEBUFFER, fb_body[0..]);

    // 7. TAG_END (size = 8)
    writeU32Le(buf, p, TAG_END);
    writeU32Le(buf, p + 4, 8);
    p += 8;

    writeU32Le(buf, 0, p as u32);
    writeU32Le(buf, 4, 0); // reserved = 0
    return p;
}

test("1. Multiboot1 and Multiboot2 kernel headers 32-bit wrapping checksum validation", () => {
    expect_eq(MB1_HEADER_MAGIC, 0x1BADB002 as u32);
    expect_eq(MB1_BOOTLOADER_MAGIC, 0x2BADB002 as u32);
    expect_eq(MB2_HEADER_MAGIC, 0xE85250D6 as u32);
    expect_eq(MB2_BOOTLOADER_MAGIC, 0x36D76289 as u32);
    expect_eq(MB2_ARCH_I386, 0 as u32);
    expect_eq(MB2_ARCH_MIPS32, 4 as u32);

    // Valid Multiboot1 header: flags = 0x03, checksum = -(magic + flags) mod 2^32
    let mb1: [12]u8 = [0; 12];
    const mb1_flags: u32 = 0x00000003;
    const mb1_cksum: u32 = (((0x100000000 as u64) - (((MB1_HEADER_MAGIC as u64) + (mb1_flags as u64)) & 0xFFFFFFFF)) & 0xFFFFFFFF) as u32;
    writeU32Le(mb1[0..], 0, MB1_HEADER_MAGIC);
    writeU32Le(mb1[0..], 4, mb1_flags);
    writeU32Le(mb1[0..], 8, mb1_cksum);

    const h1 = parseMb1Header(mb1[0..]) ?? unreachable;
    expect_eq(h1.flags, mb1_flags);
    expect_eq(h1.checksum, mb1_cksum);

    // Valid Multiboot2 header: arch = i386, header_length = 24 (16B header + 8B end tag)
    let mb2: [24]u8 = [0; 24];
    const mb2_len: u32 = 24;
    const mb2_sum: u64 = ((MB2_HEADER_MAGIC as u64) + (MB2_ARCH_I386 as u64) + (mb2_len as u64)) & 0xFFFFFFFF;
    const mb2_cksum: u32 = (((0x100000000 as u64) - mb2_sum) & 0xFFFFFFFF) as u32;
    writeU32Le(mb2[0..], 0, MB2_HEADER_MAGIC);
    writeU32Le(mb2[0..], 4, MB2_ARCH_I386);
    writeU32Le(mb2[0..], 8, mb2_len);
    writeU32Le(mb2[0..], 12, mb2_cksum);
    writeU32Le(mb2[0..], 16, 0); // end tag
    writeU32Le(mb2[0..], 20, 8);

    const h2 = parseMb2Header(mb2[0..]) ?? unreachable;
    expect_eq(h2.arch, MB2_ARCH_I386);
    expect_eq(h2.header_length, 24 as u32);
    expect_eq(h2.tags_bytes.len, 8);
});

test("2. Multiboot2 Boot Info 8-byte aligned multi-tag iteration, cmdline, and bootloaderName", () => {
    let buf: [512]u8 = [0; 512];
    const total = buildSampleBootInfo(buf[0..]);

    const info = parseMb2BootInfo(buf[0..total]) ?? unreachable;
    expect_eq(info.total_size, total as u32);
    expect_eq(std.mem.eql(u8, info.cmdline() ?? unreachable, "root=/dev/vda1 quiet"), true);
    expect_eq(std.mem.eql(u8, info.bootloaderName() ?? unreachable, "GRUB 2.12"), true);

    let iter = info.tags();
    let count: usize = 0;
    while (tag := iter.next()) {
        * = tag;
        count += 1;
    }
    expect_eq(count, 6);
    expect_eq(iter.hasError(), false);
});

test("3. Strongly typed Multiboot2 BasicMemInfo, Module, and Framebuffer tag decoding", () => {
    let buf: [512]u8 = [0; 512];
    const total = buildSampleBootInfo(buf[0..]);
    const info = parseMb2BootInfo(buf[0..total]) ?? unreachable;

    // BasicMemInfo
    const mem_tag = info.findTag(TAG_BASIC_MEMINFO) ?? unreachable;
    const mem = parseMb2BasicMemInfo(mem_tag) ?? unreachable;
    expect_eq(mem.mem_lower_kb, 640 as u32);
    expect_eq(mem.mem_upper_kb, 523264 as u32);

    // Module
    const mod_tag = info.findTag(TAG_MODULE) ?? unreachable;
    const mod = parseMb2Module(mod_tag) ?? unreachable;
    expect_eq(mod.mod_start, 0x00100000 as u32);
    expect_eq(mod.mod_end, 0x00200000 as u32);
    expect_eq(std.mem.eql(u8, mod.cmdline, "initrd.img"), true);

    // Framebuffer
    const fb_tag = info.findTag(TAG_FRAMEBUFFER) ?? unreachable;
    const fb = parseMb2Framebuffer(fb_tag) ?? unreachable;
    expect_eq(fb.addr, 0xFD000000 as u64);
    expect_eq(fb.pitch, 7680 as u32);
    expect_eq(fb.width, 1920 as u32);
    expect_eq(fb.height, 1080 as u32);
    expect_eq(fb.bpp, 32 as u8);
    expect_eq(fb.fb_type, 1 as u8);
});

test("4. Mb2MmapIter traversal over Available, Reserved, and ACPI Reclaimable memory regions", () => {
    let buf: [512]u8 = [0; 512];
    const total = buildSampleBootInfo(buf[0..]);
    const info = parseMb2BootInfo(buf[0..total]) ?? unreachable;

    const mmap_tag = info.findTag(TAG_MMAP) ?? unreachable;
    let iter = iterMb2Mmap(mmap_tag) ?? unreachable;

    const e0 = iter.next() ?? unreachable;
    expect_eq(e0.base_addr, 0x0 as u64);
    expect_eq(e0.length, 0x9FC00 as u64);
    expect_eq(e0.entry_type, MMAP_AVAILABLE);
    expect_eq(e0.isAvailable(), true);

    const e1 = iter.next() ?? unreachable;
    expect_eq(e1.base_addr, 0xF0000 as u64);
    expect_eq(e1.length, 0x10000 as u64);
    expect_eq(e1.entry_type, MMAP_RESERVED);
    expect_eq(e1.isAvailable(), false);

    const e2 = iter.next() ?? unreachable;
    expect_eq(e2.base_addr, 0x7FE0000 as u64);
    expect_eq(e2.length, 0x20000 as u64);
    expect_eq(e2.entry_type, MMAP_ACPI_RECLAIMABLE);
    expect_eq(e2.isAvailable(), false);

    expect_eq(iter.next() == null, true);

    // Verify remaining mmap constants
    expect_eq(MMAP_NVS, 4 as u32);
    expect_eq(MMAP_BADRAM, 5 as u32);
});

test("5. Malformed input rejection: bad checksums, non-zero reserved, tag.size < 8, truncation", () => {
    // Truncated headers
    expect_eq(parseMb1Header("short") == null, true);
    expect_eq(parseMb2Header("short") == null, true);
    expect_eq(parseMb2BootInfo("short") == null, true);

    // MB1 bad checksum
    let mb1: [12]u8 = [0; 12];
    writeU32Le(mb1[0..], 0, MB1_HEADER_MAGIC);
    writeU32Le(mb1[0..], 4, 0);
    writeU32Le(mb1[0..], 8, 12345);
    expect_eq(parseMb1Header(mb1[0..]) == null, true);

    // MB2 bad checksum & header_length > data.len
    let mb2: [16]u8 = [0; 16];
    writeU32Le(mb2[0..], 0, MB2_HEADER_MAGIC);
    writeU32Le(mb2[0..], 4, 0);
    writeU32Le(mb2[0..], 8, 16);
    writeU32Le(mb2[0..], 12, 0);
    expect_eq(parseMb2Header(mb2[0..]) == null, true);

    // BootInfo non-zero reserved
    let mbi: [32]u8 = [0; 32];
    writeU32Le(mbi[0..], 0, 16);
    writeU32Le(mbi[0..], 4, 1); // reserved != 0
    expect_eq(parseMb2BootInfo(mbi[0..]) == null, true);

    // BootInfo total_size > data.len
    writeU32Le(mbi[0..], 0, 64);
    writeU32Le(mbi[0..], 4, 0);
    expect_eq(parseMb2BootInfo(mbi[0..]) == null, true);

    // Malformed tag with size < 8 sets iter.hasError()
    writeU32Le(mbi[0..], 0, 16);
    writeU32Le(mbi[0..], 4, 0);
    writeU32Le(mbi[0..], 8, TAG_CMDLINE);
    writeU32Le(mbi[0..], 12, 4); // invalid size < 8
    const info_bad = parseMb2BootInfo(mbi[0..16]) ?? unreachable;
    let it_bad = info_bad.tags();
    expect_eq(it_bad.next() == null, true);
    expect_eq(it_bad.hasError(), true);
});
