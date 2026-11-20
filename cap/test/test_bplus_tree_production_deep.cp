// Source: cp/cap/test/test_bplus_tree_production_deep.cp (Deep production tests for cap:bplus_tree)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BPlusTree,
    BPlusError,
    SlottedPage,
    encodeSlottedPage,
    decodeSlottedPage,
    PAGE_SIZE,
    MAX_PAYLOAD_SIZE,
} from "cap:bplus_tree";

test("bplus_tree: 500 disordered inserts, multi-level splits, overwrite updates, invariants") {
    const alloc = std.testing.allocator;
    let tree = try BPlusTree.init(alloc, 4);
    defer tree.deinit();

    let k_buf: [16]u8 = std.mem.zeroes([16]u8);
    let v_buf: [24]u8 = std.mem.zeroes([24]u8);

    let i: usize = 0;
    while (i < 500) {
        const idx = (i * 197 + 31) % 500;
        const k = try std.fmt.bufPrint(&k_buf, "k_{d:0>4}", .{idx});
        const v = try std.fmt.bufPrint(&v_buf, "v_{d:0>4}", .{idx});
        try tree.insert(k, v);
        i += 1;
    }

    try expect_eq(tree.len(), 500);
    try std.testing.expect(tree.height() >= 4);
    try tree.validateInvariants();

    // Verify all 500 keys and overwrite 50 of them
    let j: usize = 0;
    while (j < 50) {
        const k = try std.fmt.bufPrint(&k_buf, "k_{d:0>4}", .{j * 10});
        const v = try std.fmt.bufPrint(&v_buf, "upd_{d:0>4}", .{j * 10});
        try tree.insert(k, v);
        j += 1;
    }

    try expect_eq(tree.len(), 500);
    try tree.validateInvariants();

    const got = tree.get("k_0120").?;
    try std.testing.expectEqualStrings("upd_0120", got);
    const untouched = tree.get("k_0123").?;
    try std.testing.expectEqualStrings("v_0123", untouched);
}

test("bplus_tree: random and reverse removes trigger borrow, merge, and root shrink") {
    const alloc = std.testing.allocator;
    let tree = try BPlusTree.init(alloc, 4);
    defer tree.deinit();

    let k_buf: [16]u8 = std.mem.zeroes([16]u8);
    let v_buf: [24]u8 = std.mem.zeroes([24]u8);

    let i: usize = 0;
    while (i < 300) {
        const k = try std.fmt.bufPrint(&k_buf, "item_{d:0>4}", .{i});
        const v = try std.fmt.bufPrint(&v_buf, "val_{d:0>4}", .{i});
        try tree.insert(k, v);
        i += 1;
    }
    try tree.validateInvariants();
    const initial_height = tree.height();
    try std.testing.expect(initial_height >= 3);

    // Remove 150 keys in pseudo-random order
    let removed_flags: [300]bool = std.mem.zeroes([300]bool);
    let step: usize = 0;
    while (step < 150) {
        const target = (step * 73 + 19) % 300;
        if (!removed_flags[target]) {
            const k = try std.fmt.bufPrint(&k_buf, "item_{d:0>4}", .{target});
            try std.testing.expect(tree.remove(k));
            removed_flags[target] = true;
        }
        if (step % 15 == 0) {
            try tree.validateInvariants();
        }
        step += 1;
    }
    try tree.validateInvariants();

    // Remove remaining keys in reverse order until tree is empty
    let rev: usize = 300;
    while (rev > 0) {
        rev -= 1;
        if (!removed_flags[rev]) {
            const k = try std.fmt.bufPrint(&k_buf, "item_{d:0>4}", .{rev});
            try std.testing.expect(tree.remove(k));
        }
        if (rev % 25 == 0) {
            try tree.validateInvariants();
        }
    }

    try expect_eq(tree.len(), 0);
    try expect_eq(tree.height(), 0);
    try std.testing.expect(tree.first() == null);
    try std.testing.expect(tree.last() == null);
    try tree.validateInvariants();
}

test("bplus_tree: rangeScan closed, half-open, full scan, first and last") {
    const alloc = std.testing.allocator;
    let tree = try BPlusTree.init(alloc, 6);
    defer tree.deinit();

    let k_buf: [16]u8 = std.mem.zeroes([16]u8);
    let v_buf: [16]u8 = std.mem.zeroes([16]u8);

    let i: usize = 0;
    while (i < 100) {
        const k = try std.fmt.bufPrint(&k_buf, "key_{d:0>3}", .{i});
        const v = try std.fmt.bufPrint(&v_buf, "v_{d:0>3}", .{i});
        try tree.insert(k, v);
        i += 1;
    }

    try std.testing.expectEqualStrings("key_000", tree.first().?.key);
    try std.testing.expectEqualStrings("key_099", tree.last().?.key);

    // Closed range [key_020, key_035] -> 16 items
    const closed = try tree.rangeScan(alloc, "key_020", "key_035");
    defer alloc.free(closed);
    try expect_eq(closed.len, 16);
    try std.testing.expectEqualStrings("key_020", closed[0].key);
    try std.testing.expectEqualStrings("key_035", closed[15].key);

    // Half-open range [key_020, key_035) -> 15 items
    const half_open = try tree.rangeScanEx(alloc, "key_020", "key_035", false);
    defer alloc.free(half_open);
    try expect_eq(half_open.len, 15);
    try std.testing.expectEqualStrings("key_034", half_open[14].key);

    // Full scan
    const full = try tree.rangeScan(alloc, null, null);
    defer alloc.free(full);
    try expect_eq(full.len, 100);
    let idx: usize = 0;
    while (idx < full.len) {
        const expected = try std.fmt.bufPrint(&k_buf, "key_{d:0>3}", .{idx});
        try std.testing.expectEqualStrings(expected, full[idx].key);
        idx += 1;
    }
}

test("slotted_page: 4KB insert, get, delete, compact, encode/decode and CRC32C tamper") {
    let page = SlottedPage.init();
    const s0 = try page.insertCell("alpha-record-0001");
    const s1 = try page.insertCell("beta-record-0002-longer-payload");
    const s2 = try page.insertCell("gamma-record-0003");
    try expect_eq(s0, 0);
    try expect_eq(s1, 1);
    try expect_eq(s2, 2);
    try expect_eq(page.activeCount(), 3);

    try std.testing.expectEqualStrings("beta-record-0002-longer-payload", try page.getCell(s1));

    // Delete middle cell and verify SlotDeleted
    try page.deleteCell(s1);
    try expect_eq(page.activeCount(), 2);
    try std.testing.expectError(BPlusError.SlotDeleted, page.getCell(s1));

    const space_before = page.freeSpace();
    page.compact();
    const space_after = page.freeSpace();
    try std.testing.expect(space_after > space_before);
    try std.testing.expectEqualStrings("alpha-record-0001", try page.getCell(s0));
    try std.testing.expectEqualStrings("gamma-record-0003", try page.getCell(s2));

    // Encode & decode roundtrip
    let raw: [PAGE_SIZE]u8 = std.mem.zeroes([PAGE_SIZE]u8);
    encodeSlottedPage(&page, &raw);
    const decoded = try decodeSlottedPage(&raw);
    try std.testing.expectEqualStrings("gamma-record-0003", try decoded.getCell(s2));

    // Tamper payload -> CorruptedChecksum
    raw[100] ^= 0xFF;
    try std.testing.expectError(BPlusError.CorruptedChecksum, decodeSlottedPage(&raw));

    // Tamper magic -> InvalidMagic
    raw[0] = 0x00;
    try std.testing.expectError(BPlusError.InvalidMagic, decodeSlottedPage(&raw));

    // PageFull check
    let full_page = SlottedPage.init();
    let big_buf: [2048]u8 = std.mem.zeroes([2048]u8);
    * = try full_page.insertCell(&big_buf);
    try std.testing.expectError(BPlusError.PageFull, full_page.insertCell(&big_buf));
    try std.testing.expect(MAX_PAYLOAD_SIZE > 4000);
}

test("bplus_tree: cap.bplus_tree dynamic mount and zero-leak check") {
    const alloc = std.testing.allocator;
    let tree = try cap.bplus_tree.BPlusTree.init(alloc, 64);
    defer tree.deinit();

    let k_buf: [16]u8 = std.mem.zeroes([16]u8);
    let v_buf: [16]u8 = std.mem.zeroes([16]u8);

    let i: usize = 0;
    while (i < 200) {
        const k = try std.fmt.bufPrint(&k_buf, "dyn_{d:0>4}", .{i});
        const v = try std.fmt.bufPrint(&v_buf, "val_{d:0>4}", .{i});
        try tree.insert(k, v);
        i += 1;
    }

    let j: usize = 0;
    while (j < 100) {
        const k = try std.fmt.bufPrint(&k_buf, "dyn_{d:0>4}", .{j * 2});
        try std.testing.expect(tree.remove(k));
        j += 1;
    }

    try expect_eq(tree.len(), 100);
    try tree.validateInvariants();
    try std.testing.expectEqualStrings("dyn_0001", tree.first().?.key);
    try std.testing.expectEqualStrings("dyn_0199", tree.last().?.key);
}
