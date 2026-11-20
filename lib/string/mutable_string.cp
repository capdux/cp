// Source: /data/dev/bun/src/string/MutableString.cp
/**
 * CP MutableString (cap:string)
 * 对应 Bun src/string/MutableString.zig 的可增长字符串缓冲
 */

const std = @import("std");

pub struct MutableString {
    allocator: std.mem.Allocator,
    list: std.ArrayListUnmanaged(u8) = .empty,

    pub static fn init(allocator: std.mem.Allocator, init_cap: usize): !MutableString {
        let list: std.ArrayListUnmanaged(u8) = .empty;
        if (init_cap > 0) {
            list = try std.ArrayListUnmanaged(u8).initCapacity(allocator, init_cap);
        }
        return MutableString{
            allocator: allocator,
            list: list,
        };
    }

    pub static fn initEmpty(allocator: std.mem.Allocator): MutableString {
        return MutableString{
            allocator: allocator,
            list: .empty,
        };
    }

    pub static fn init2048(allocator: std.mem.Allocator): !MutableString {
        return MutableString.init(allocator, 2048);
    }

    pub static fn initCopy(allocator: std.mem.Allocator, str: []const u8): !MutableString {
        let ms = try MutableString.init(allocator, str.len);
        errdefer ms.deinit();
        try ms.append(str);
        return ms;
    }

    pub fn deinit(*_) {
        _.list.deinit(_.allocator);
    }

    pub fn clone(): !MutableString {
        return MutableString.initCopy(_.allocator, _.list.items);
    }

    pub fn isEmpty(): bool {
        return _.list.items.len == 0;
    }

    pub fn len(): usize {
        return _.list.items.len;
    }

    pub fn capacity(): usize {
        return _.list.capacity;
    }

    pub fn growIfNeeded(*_, amount: usize): !void {
        try _.list.ensureUnusedCapacity(_.allocator, amount);
    }

    pub fn ensureUnusedCapacity(*_, amount: usize): !void {
        try _.list.ensureUnusedCapacity(_.allocator, amount);
    }

    pub fn writableNBytesAssumeCapacity(*_, amount: usize): []u8 {
        const old_len = _.list.items.len;
        _.list.items.len = old_len + amount;
        return _.list.items[old_len .. old_len + amount];
    }

    pub fn writableNBytes(*_, amount: usize): ![]u8 {
        try _.growIfNeeded(amount);
        return _.writableNBytesAssumeCapacity(amount);
    }

    pub fn append(*_, bytes: []const u8): !void {
        if (bytes.len == 0) return;
        try _.list.appendSlice(_.allocator, bytes);
    }

    pub fn appendSlice(*_, bytes: []const u8): !void {
        try _.append(bytes);
    }

    pub fn appendAssumeCapacity(*_, bytes: []const u8) {
        _.list.appendSliceAssumeCapacity(bytes);
    }

    pub fn appendChar(*_, ch: u8): !void {
        try _.list.append(_.allocator, ch);
    }

    pub fn appendCharAssumeCapacity(*_, ch: u8) {
        _.list.appendAssumeCapacity(ch);
    }

    pub fn appendCharNTimes(*_, ch: u8, n: usize): !void {
        if (n == 0) return;
        const dst = try _.writableNBytes(n);
        @memset(dst, ch);
    }

    pub fn appendInt(*_, val: u64): !void {
        let buf: [32]u8 = undefined;
        const formatted = std.fmt.bufPrint(&buf, "{d}", .{val}) catch unreachable;
        try _.append(formatted);
    }

    pub fn appendSignedInt(*_, val: i64): !void {
        let buf: [32]u8 = undefined;
        const formatted = std.fmt.bufPrint(&buf, "{d}", .{val}) catch unreachable;
        try _.append(formatted);
    }

    pub fn appendFmt(*_, comptime fmt_str: []const u8, args: auto): !void {
        const out = try std.fmt.allocPrint(_.allocator, fmt_str, args);
        defer _.allocator.free(out);
        try _.append(out);
    }

    pub fn write(*_, bytes: []const u8): !usize {
        try _.append(bytes);
        return bytes.len;
    }

    pub fn writeAll(*_, bytes: []const u8): !void {
        try _.append(bytes);
    }

    pub fn reset(*_) {
        _.list.clearRetainingCapacity();
    }

    pub fn resetTo(*_, new_len: usize) {
        if (new_len < _.list.items.len) {
            _.list.items.len = new_len;
        }
    }

    pub fn slice(): []const u8 {
        return _.list.items;
    }

    pub fn mutSlice(*_): []u8 {
        return _.list.items;
    }

    pub fn toOwnedSlice(*_): ![]u8 {
        return try _.list.toOwnedSlice(_.allocator);
    }

    pub fn toOwnedSliceZ(*_): ![:0]u8 {
        return try _.list.toOwnedSliceSentinel(_.allocator, 0);
    }

    pub fn containsChar(ch: u8): bool {
        return _.indexOfChar(ch) != null;
    }

    pub fn indexOfChar(ch: u8): usize? {
        return std.mem.indexOfScalar(u8, _.list.items, ch);
    }

    pub fn lastIndexOfChar(ch: u8): usize? {
        return std.mem.lastIndexOfScalar(u8, _.list.items, ch);
    }

    pub fn eql(other: []const u8): bool {
        return std.mem.eql(u8, _.list.items, other);
    }
}
