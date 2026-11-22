// Source: cp/cap/test/test_fdt_production_deep.cp (Production deep tests for cap:fdt)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FDT_MAGIC,
    FDT_HEADER_SIZE,
    FDT_VERSION,
    FDT_BEGIN_NODE,
    FDT_END_NODE,
    FDT_PROP,
    FDT_NOP,
    FDT_END,
    FdtTokenKind,
    isFdt,
    parseFdt,
} from "cap:sys";

fn writeU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeU64Be(buf: []u8, off: usize, v: u64) {
    writeU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    writeU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn copyInto(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn align4(n: usize): usize {
    const rem = n % 4;
    if (rem == 0) {
        return n;
    }
    return n + (4 - rem);
}

// Helper to append a NUL-terminated node name (padded to 4B) into struct block
fn emitBeginNode(buf: []u8, pos: usize, name: []const u8): usize {
    writeU32Be(buf, pos, FDT_BEGIN_NODE);
    copyInto(buf, pos + 4, name);
    buf[pos + 4 + name.len] = 0;
    return pos + 4 + align4(name.len + 1);
}

fn emitEndNode(buf: []u8, pos: usize): usize {
    writeU32Be(buf, pos, FDT_END_NODE);
    return pos + 4;
}

fn emitNop(buf: []u8, pos: usize): usize {
    writeU32Be(buf, pos, FDT_NOP);
    return pos + 4;
}

fn emitEnd(buf: []u8, pos: usize): usize {
    writeU32Be(buf, pos, FDT_END);
    return pos + 4;
}

fn emitProp(buf: []u8, pos: usize, nameoff: u32, val: []const u8): usize {
    writeU32Be(buf, pos, FDT_PROP);
    writeU32Be(buf, pos + 4, val.len as u32);
    writeU32Be(buf, pos + 8, nameoff);
    copyInto(buf, pos + 12, val);
    return pos + 12 + align4(val.len);
}

// Builds a complete synthetic DTB:
//   mem_rsvmap: (0x80000000, 0x10000), (0x90000000, 0x2000), (0, 0)
//   tree:
//     / {
//       compatible = "cp,board-v1\0";
//       #address-cells = <2>;
//       (NOP)
//       cpus {
//         cpu@0 {
//           reg = <0x00000000 0x00000000>;
//         };
//       };
//       memory@80000000 {
//         reg = <0x0000000080000000>; (u64)
//       };
//     };
fn buildSampleDtb(buf: []u8): usize {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i += 1;
    }

    // 1. Memory reservation block at offset 40 (3 * 16 = 48 bytes -> ends at 88)
    const off_rsv: usize = 40;
    writeU64Be(buf, 40, 0x80000000);
    writeU64Be(buf, 48, 0x10000);
    writeU64Be(buf, 56, 0x90000000);
    writeU64Be(buf, 64, 0x2000);
    writeU64Be(buf, 72, 0);
    writeU64Be(buf, 80, 0);

    // 2. Strings block content:
    //   0: "compatible\0" (11 bytes)
    //   11: "#address-cells\0" (15 bytes)
    //   26: "reg\0" (4 bytes)
    //   Total: 30 bytes
    const off_struct: usize = 88;
    let p: usize = off_struct;

    p = emitBeginNode(buf, p, ""); // root "/"
    p = emitProp(buf, p, 0, "cp,board-v1\x00");

    let cells_val: [4]u8 = [0; 4];
    writeU32Be(cells_val[0..], 0, 2);
    p = emitProp(buf, p, 11, cells_val[0..]);

    p = emitNop(buf, p); // verify NOP is skipped seamlessly

    p = emitBeginNode(buf, p, "cpus");
    p = emitBeginNode(buf, p, "cpu@0");
    let cpu_reg: [4]u8 = [0; 4];
    writeU32Be(cpu_reg[0..], 0, 0);
    p = emitProp(buf, p, 26, cpu_reg[0..]);
    p = emitEndNode(buf, p); // end cpu@0
    p = emitEndNode(buf, p); // end cpus

    p = emitBeginNode(buf, p, "memory@80000000");
    let mem_reg: [8]u8 = [0; 8];
    writeU64Be(mem_reg[0..], 0, 0x0000000080000000);
    p = emitProp(buf, p, 26, mem_reg[0..]);
    p = emitEndNode(buf, p); // end memory@80000000

    p = emitEndNode(buf, p); // end root
    p = emitEnd(buf, p);

    const size_struct: usize = p - off_struct;
    const off_strings: usize = p;
    copyInto(buf, off_strings, "compatible\x00#address-cells\x00reg\x00");
    const size_strings: usize = 30;
    const total_size: usize = off_strings + size_strings;

    // 3. Write 40B FDT Header
    writeU32Be(buf, 0, FDT_MAGIC);
    writeU32Be(buf, 4, total_size as u32);
    writeU32Be(buf, 8, off_struct as u32);
    writeU32Be(buf, 12, off_strings as u32);
    writeU32Be(buf, 16, off_rsv as u32);
    writeU32Be(buf, 20, FDT_VERSION);
    writeU32Be(buf, 24, 16); // last_comp_version
    writeU32Be(buf, 28, 0);  // boot_cpuid_phys
    writeU32Be(buf, 32, size_strings as u32);
    writeU32Be(buf, 36, size_struct as u32);

    return total_size;
}

test("1. 40B FDT Header validation, strings lookup, and Memory Reservation Map iteration", () => {
    expect_eq(FDT_MAGIC, 0xD00DFEED as u32);
    expect_eq(FDT_HEADER_SIZE, 40);
    expect_eq(FDT_VERSION, 17 as u32);

    let buf: [512]u8 = [0; 512];
    const total = buildSampleDtb(buf[0..]);

    expect_eq(isFdt(buf[0..total]), true);
    const blob = parseFdt(buf[0..total]) ?? unreachable;
    expect_eq(blob.header.totalsize, total as u32);
    expect_eq(blob.header.version, 17 as u32);
    expect_eq(blob.header.last_comp_version, 16 as u32);
    expect_eq(blob.header.boot_cpuid_phys, 0 as u32);

    // Direct strings block lookup
    expect_eq(std.mem.eql(u8, blob.stringAt(0) ?? unreachable, "compatible"), true);
    expect_eq(std.mem.eql(u8, blob.stringAt(11) ?? unreachable, "#address-cells"), true);
    expect_eq(std.mem.eql(u8, blob.stringAt(26) ?? unreachable, "reg"), true);
    expect_eq(blob.stringAt(999) == null, true);

    // Memory reservation map iteration stops at (0, 0) sentinel
    let rsv_iter = blob.memReservations();
    const r0 = rsv_iter.next() ?? unreachable;
    expect_eq(r0.address, 0x80000000 as u64);
    expect_eq(r0.size, 0x10000 as u64);

    const r1 = rsv_iter.next() ?? unreachable;
    expect_eq(r1.address, 0x90000000 as u64);
    expect_eq(r1.size, 0x2000 as u64);

    expect_eq(rsv_iter.next() == null, true);
});

test("2. Nested Structure Block traversal, NOP skipping, 4B alignment, and depth tracking", () => {
    let buf: [512]u8 = [0; 512];
    const total = buildSampleDtb(buf[0..]);
    const blob = parseFdt(buf[0..total]) ?? unreachable;

    let iter = blob.tokens();

    // 1. BEGIN_NODE "" (depth 0)
    const t0 = iter.next() ?? unreachable;
    expect_eq(t0.kind == FdtTokenKind.begin_node, true);
    expect_eq(t0.depth, 0 as u32);
    expect_eq(t0.name.len, 0);

    // 2. PROP "compatible" = "cp,board-v1" (depth 1)
    const t1 = iter.next() ?? unreachable;
    expect_eq(t1.kind == FdtTokenKind.prop, true);
    expect_eq(t1.depth, 1 as u32);
    expect_eq(std.mem.eql(u8, t1.name, "compatible"), true);
    expect_eq(std.mem.eql(u8, t1.asString() ?? unreachable, "cp,board-v1"), true);

    // 3. PROP "#address-cells" = 2 (depth 1)
    const t2 = iter.next() ?? unreachable;
    expect_eq(t2.kind == FdtTokenKind.prop, true);
    expect_eq(std.mem.eql(u8, t2.name, "#address-cells"), true);
    expect_eq(t2.asU32() ?? 0, 2 as u32);
    expect_eq(t2.asU64() == null, true); // 4 bytes != 8 bytes

    // (FDT_NOP was skipped automatically!)
    // 4. BEGIN_NODE "cpus" (depth 1)
    const t3 = iter.next() ?? unreachable;
    expect_eq(t3.kind == FdtTokenKind.begin_node, true);
    expect_eq(t3.depth, 1 as u32);
    expect_eq(std.mem.eql(u8, t3.name, "cpus"), true);

    // 5. BEGIN_NODE "cpu@0" (depth 2)
    const t4 = iter.next() ?? unreachable;
    expect_eq(t4.kind == FdtTokenKind.begin_node, true);
    expect_eq(t4.depth, 2 as u32);
    expect_eq(std.mem.eql(u8, t4.name, "cpu@0"), true);

    // 6. PROP "reg" = 0 (u32, depth 3)
    const t5 = iter.next() ?? unreachable;
    expect_eq(t5.kind == FdtTokenKind.prop, true);
    expect_eq(t5.depth, 3 as u32);
    expect_eq(t5.asU32() ?? 99, 0 as u32);

    // 7. END_NODE cpu@0 (depth 2) & END_NODE cpus (depth 1)
    const t6 = iter.next() ?? unreachable;
    expect_eq(t6.kind == FdtTokenKind.end_node, true);
    expect_eq(t6.depth, 2 as u32);
    const t7 = iter.next() ?? unreachable;
    expect_eq(t7.kind == FdtTokenKind.end_node, true);
    expect_eq(t7.depth, 1 as u32);

    // 8. BEGIN_NODE "memory@80000000" (depth 1)
    const t8 = iter.next() ?? unreachable;
    expect_eq(t8.kind == FdtTokenKind.begin_node, true);
    expect_eq(std.mem.eql(u8, t8.name, "memory@80000000"), true);

    // 9. PROP "reg" = 0x80000000 (u64, depth 2)
    const t9 = iter.next() ?? unreachable;
    expect_eq(t9.kind == FdtTokenKind.prop, true);
    expect_eq(t9.asU64() ?? 0, 0x0000000080000000 as u64);
    expect_eq(t9.asU32() == null, true); // 8 bytes != 4 bytes

    // 10. END_NODE memory, END_NODE root, FDT_END
    expect_eq((iter.next() ?? unreachable).kind == FdtTokenKind.end_node, true);
    expect_eq((iter.next() ?? unreachable).kind == FdtTokenKind.end_node, true);
    const t_end = iter.next() ?? unreachable;
    expect_eq(t_end.kind == FdtTokenKind.end, true);
    expect_eq(iter.hasError(), false);
    expect_eq(iter.next() == null, true);
});

test("3. Header rejection: bad magic, truncated totalsize, unsupported version, unaligned/out-of-bounds sections", () => {
    expect_eq(isFdt("nope"), false);
    expect_eq(parseFdt("short") == null, true);

    let buf: [512]u8 = [0; 512];
    const total = buildSampleDtb(buf[0..]);

    // Bad magic
    writeU32Be(buf[0..], 0, 0x12345678);
    expect_eq(isFdt(buf[0..total]), false);
    expect_eq(parseFdt(buf[0..total]) == null, true);

    // totalsize > data.len
    * = buildSampleDtb(buf[0..]);
    writeU32Be(buf[0..], 4, (total + 16) as u32);
    expect_eq(parseFdt(buf[0..total]) == null, true);

    // version < 17
    * = buildSampleDtb(buf[0..]);
    writeU32Be(buf[0..], 20, 16);
    expect_eq(parseFdt(buf[0..total]) == null, true);

    // last_comp_version > 17
    * = buildSampleDtb(buf[0..]);
    writeU32Be(buf[0..], 24, 18);
    expect_eq(parseFdt(buf[0..total]) == null, true);

    // Unaligned off_dt_struct
    * = buildSampleDtb(buf[0..]);
    writeU32Be(buf[0..], 8, 89);
    expect_eq(parseFdt(buf[0..total]) == null, true);

    // Out-of-bounds strings block
    * = buildSampleDtb(buf[0..]);
    writeU32Be(buf[0..], 32, 9999);
    expect_eq(parseFdt(buf[0..total]) == null, true);
});

test("4. Token iterator error detection: out-of-bounds strings offset and unterminated string", () => {
    let buf: [512]u8 = [0; 512];
    const total = buildSampleDtb(buf[0..]);

    // Corrupt first property's nameoff (at off_struct + 8 + 8 = 88 + 16 = 104) to point past strings block
    writeU32Be(buf[0..], 88 + 8 + 8, 9999);

    const blob = parseFdt(buf[0..total]) ?? unreachable;
    let iter = blob.tokens();
    // Root begin_node succeeds
    expect_eq((iter.next() ?? unreachable).kind == FdtTokenKind.begin_node, true);
    // Corrupted property fails and sets hasError()
    expect_eq(iter.next() == null, true);
    expect_eq(iter.hasError(), true);

    // Unterminated string in strings block
    const total2 = buildSampleDtb(buf[0..]);
    const str_off = (buf[12] as usize << 24) | (buf[13] as usize << 16) | (buf[14] as usize << 8) | (buf[15] as usize);
    // Replace all NULs in strings block with 'X'
    let k: usize = str_off;
    while (k < total2) {
        if (buf[k] == 0) {
            buf[k] = 0x58;
        }
        k += 1;
    }
    const blob2 = parseFdt(buf[0..total2]) ?? unreachable;
    expect_eq(blob2.stringAt(0) == null, true);
});

test("5. Token iterator error detection: unknown tag and unterminated node name", () => {
    // Build minimal DTB with an invalid structure tag (0x000000FF)
    let buf: [128]u8 = [0; 128];
    writeU32Be(buf[0..], 0, FDT_MAGIC);
    writeU32Be(buf[0..], 4, 72);  // totalsize
    writeU32Be(buf[0..], 8, 56);  // off_dt_struct
    writeU32Be(buf[0..], 12, 68); // off_dt_strings
    writeU32Be(buf[0..], 16, 40); // off_mem_rsvmap (16B sentinel at 40..56)
    writeU32Be(buf[0..], 20, 17); // version
    writeU32Be(buf[0..], 24, 16); // last_comp_version
    writeU32Be(buf[0..], 28, 0);
    writeU32Be(buf[0..], 32, 4);  // size_dt_strings
    writeU32Be(buf[0..], 36, 12); // size_dt_struct

    writeU32Be(buf[0..], 56, 0x000000FF); // invalid tag
    const blob = parseFdt(buf[0..72]) ?? unreachable;
    let iter = blob.tokens();
    expect_eq(iter.next() == null, true);
    expect_eq(iter.hasError(), true);

    // Unterminated node name (BEGIN_NODE followed by non-zero bytes until end of struct block)
    writeU32Be(buf[0..], 56, FDT_BEGIN_NODE);
    let m: usize = 60;
    while (m < 68) {
        buf[m] = 0x41; // 'A'
        m += 1;
    }
    const blob2 = parseFdt(buf[0..72]) ?? unreachable;
    let iter2 = blob2.tokens();
    expect_eq(iter2.next() == null, true);
    expect_eq(iter2.hasError(), true);
});
