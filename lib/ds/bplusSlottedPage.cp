// Source: cp/lib/bplus_tree/slotted_page.cp (4KB Slotted Page binary codec with CRC32C)

const std = @import("std");
import { BPlusError } from "./bplusTypes.cp";

pub const PAGE_SIZE: usize = 4096;
pub const PAGE_MAGIC: u32 = 0x43504250;
pub const PAGE_HEADER_SIZE: usize = 16;
pub const PAGE_FOOTER_SIZE: usize = 4;
pub const SLOT_SIZE: usize = 4;
pub const DELETED_OFFSET: u16 = 0xFFFF;
pub const MAX_PAYLOAD_SIZE: usize = PAGE_SIZE - PAGE_HEADER_SIZE - PAGE_FOOTER_SIZE - SLOT_SIZE;

pub struct SlottedPage {
    pub data: [PAGE_SIZE]u8,

    pub fn init(): SlottedPage {
        let page = SlottedPage{
            .data = std.mem.zeroes([PAGE_SIZE]u8),
        };
        page.writeU32(0, PAGE_MAGIC);
        page.writeU16(4, 0);
        page.writeU16(6, @intCast(PAGE_HEADER_SIZE));
        page.writeU16(8, @intCast(PAGE_SIZE - PAGE_FOOTER_SIZE));
        page.writeU16(10, 0);
        page.writeU16(12, 0);
        page.writeU16(14, 0);
        page.updateChecksum();
        return page;
    }

    fn readU16(offset: usize): u16 {
        return std.mem.readInt(u16, _.data[offset..][0..2], .little);
    }

    fn writeU16(*_, offset: usize, val: u16) {
        std.mem.writeInt(u16, _.data[offset..][0..2], val, .little);
    }

    fn readU32(offset: usize): u32 {
        return std.mem.readInt(u32, _.data[offset..][0..4], .little);
    }

    fn writeU32(*_, offset: usize, val: u32) {
        std.mem.writeInt(u32, _.data[offset..][0..4], val, .little);
    }

    pub fn slotCount(): u16 {
        return _.readU16(4);
    }

    pub fn activeCount(): u16 {
        return _.readU16(10);
    }

    fn freeStart(): u16 {
        return _.readU16(6);
    }

    fn freeEnd(): u16 {
        return _.readU16(8);
    }

    pub fn freeSpace(): usize {
        const fs: usize = @intCast(_.freeStart());
        const fe: usize = @intCast(_.freeEnd());
        if (fe <= fs) {
            return 0;
        }
        return fe - fs;
    }

    fn slotOffset(slot_id: u16): usize {
        return PAGE_HEADER_SIZE + @as(usize, @intCast(slot_id)) * SLOT_SIZE;
    }

    pub fn insertCell(*_, payload: []const u8): BPlusError!u16 {
        if (payload.len > MAX_PAYLOAD_SIZE) {
            return BPlusError.CellTooLarge;
        }
        const needed = SLOT_SIZE + payload.len;
        if (_.freeSpace() < needed) {
            return BPlusError.PageFull;
        }

        const slot_id = _.slotCount();
        const new_free_end: u16 = _.freeEnd() - @as(u16, @intCast(payload.len));
        const new_free_start: u16 = _.freeStart() + @as(u16, @intCast(SLOT_SIZE));

        if (payload.len > 0) {
            const dst_start: usize = @intCast(new_free_end);
            @memcpy(_.data[dst_start .. dst_start + payload.len], payload);
        }

        const s_off = slotOffset(slot_id);
        _.writeU16(s_off, new_free_end);
        _.writeU16(s_off + 2, @intCast(payload.len));

        _.writeU16(4, slot_id + 1);
        _.writeU16(6, new_free_start);
        _.writeU16(8, new_free_end);
        _.writeU16(10, _.activeCount() + 1);
        _.updateChecksum();
        return slot_id;
    }

    pub fn getCell(slot_id: u16): BPlusError![]const u8 {
        if (slot_id >= _.slotCount()) {
            return BPlusError.InvalidSlot;
        }
        const s_off = slotOffset(slot_id);
        const cell_off = _.readU16(s_off);
        const cell_len = _.readU16(s_off + 2);
        if (cell_off == DELETED_OFFSET) {
            return BPlusError.SlotDeleted;
        }
        const start: usize = @intCast(cell_off);
        const end: usize = start + @as(usize, @intCast(cell_len));
        if (end > PAGE_SIZE - PAGE_FOOTER_SIZE) {
            return BPlusError.InvalidSlot;
        }
        return _.data[start..end];
    }

    pub fn deleteCell(*_, slot_id: u16): BPlusError!void {
        if (slot_id >= _.slotCount()) {
            return BPlusError.InvalidSlot;
        }
        const s_off = slotOffset(slot_id);
        const cell_off = _.readU16(s_off);
        if (cell_off == DELETED_OFFSET) {
            return BPlusError.SlotDeleted;
        }
        _.writeU16(s_off, DELETED_OFFSET);
        _.writeU16(s_off + 2, 0);
        const cur_active = _.activeCount();
        if (cur_active > 0) {
            _.writeU16(10, cur_active - 1);
        }
        _.updateChecksum();
    }

    pub fn compact(*_) {
        let scratch: [PAGE_SIZE]u8 = std.mem.zeroes([PAGE_SIZE]u8);
        let write_end: usize = PAGE_SIZE - PAGE_FOOTER_SIZE;
        const count = _.slotCount();

        let i: u16 = 0;
        while (i < count) {
            const s_off = slotOffset(i);
            const cell_off = _.readU16(s_off);
            const cell_len = _.readU16(s_off + 2);
            if (cell_off != DELETED_OFFSET) {
                const len_usize: usize = @intCast(cell_len);
                const src_start: usize = @intCast(cell_off);
                write_end -= len_usize;
                if (len_usize > 0) {
                    @memcpy(scratch[write_end .. write_end + len_usize], _.data[src_start .. src_start + len_usize]);
                }
                _.writeU16(s_off, @intCast(write_end));
            }
            i += 1;
        }

        const data_area_len = (PAGE_SIZE - PAGE_FOOTER_SIZE) - write_end;
        if (data_area_len > 0) {
            @memcpy(_.data[write_end .. PAGE_SIZE - PAGE_FOOTER_SIZE], scratch[write_end .. PAGE_SIZE - PAGE_FOOTER_SIZE]);
        }
        const fs: usize = @intCast(_.freeStart());
        if (write_end > fs) {
            @memset(_.data[fs..write_end], 0);
        }
        _.writeU16(8, @intCast(write_end));
        _.updateChecksum();
    }

    pub fn computeChecksum(): u32 {
        return std.hash.crc.Crc32Iscsi.hash(_.data[0 .. PAGE_SIZE - PAGE_FOOTER_SIZE]);
    }

    pub fn updateChecksum(*_) {
        const crc = _.computeChecksum();
        _.writeU32(PAGE_SIZE - PAGE_FOOTER_SIZE, crc);
    }

    pub fn verifyChecksum(): bool {
        const stored = _.readU32(PAGE_SIZE - PAGE_FOOTER_SIZE);
        return stored == _.computeChecksum();
    }
}

pub fn encodeSlottedPage(page: *SlottedPage, out: *[PAGE_SIZE]u8) {
    page.updateChecksum();
    @memcpy(out, &page.data);
}

pub fn decodeSlottedPage(raw: []const u8): BPlusError!SlottedPage {
    if (raw.len != PAGE_SIZE) {
        return BPlusError.InvalidPageSize;
    }
    let page = SlottedPage{
        .data = std.mem.zeroes([PAGE_SIZE]u8),
    };
    @memcpy(&page.data, raw);

    if (page.readU32(0) != PAGE_MAGIC) {
        return BPlusError.InvalidMagic;
    }
    if (!page.verifyChecksum()) {
        return BPlusError.CorruptedChecksum;
    }
    const fs: usize = @intCast(page.freeStart());
    const fe: usize = @intCast(page.freeEnd());
    const expected_fs = PAGE_HEADER_SIZE + @as(usize, @intCast(page.slotCount())) * SLOT_SIZE;
    if (fs != expected_fs or fs > fe or fe > PAGE_SIZE - PAGE_FOOTER_SIZE) {
        return BPlusError.InvalidPageSize;
    }
    return page;
}
