// Source: Absorbed from /data/dev/bun/src/bun_alloc/allocation_scope.cp | Synced: 2026-11-24T05:07:24Z
// lib/mem/allocation_scope.cp
// Production-grade AllocationScope with live leak tracking, peak bytes, dangling/double-free detection & reporting

const std = @import("std");

pub err ScopeError {
    OutOfMemory,
    DoubleFree,
    InvalidFree,
    MemoryLeak,
    NotOwned,
    AlreadyOwned,
}

pub struct ScopeStats {
    pub live_allocations: usize = 0;
    pub live_bytes: usize = 0;
    pub total_allocations: usize = 0;
    pub total_allocated_bytes: usize = 0;
    pub peak_bytes: usize = 0;
    pub total_frees: usize = 0;
    pub double_frees: usize = 0;
    pub invalid_frees: usize = 0;
}

pub struct TrackedAllocation {
    pub addr: usize = 0;
    pub len: usize = 0;
    pub seq: usize = 0;
    pub tag: []const u8 = "";
}

struct FreedRecord {
    addr: usize = 0;
    len: usize = 0;
    seq: usize = 0;
}

struct ScopeState {
    parent: std.mem.Allocator = std.heap.page_allocator;
    live: std.AutoHashMapUnmanaged(usize, TrackedAllocation) = .empty;
    freed_ring: [1024]FreedRecord = undefined;
    freed_count: usize = 0;
    freed_cursor: usize = 0;
    next_seq: usize = 1;

    live_allocations: usize = 0;
    live_bytes: usize = 0;
    total_allocations: usize = 0;
    total_allocated_bytes: usize = 0;
    peak_bytes: usize = 0;
    total_frees: usize = 0;
    double_frees: usize = 0;
    invalid_frees: usize = 0;

    static fn create(parent: std.mem.Allocator): (*ScopeState)! {
        zig {
            const st_ptr = parent.create(ScopeState) catch return error.OutOfMemory;
            st_ptr.* = ScopeState{
                .parent = parent,
                .live = .empty,
                .freed_ring = undefined,
                .freed_count = 0,
                .freed_cursor = 0,
                .next_seq = 1,
                .live_allocations = 0,
                .live_bytes = 0,
                .total_allocations = 0,
                .total_allocated_bytes = 0,
                .peak_bytes = 0,
                .total_frees = 0,
                .double_frees = 0,
                .invalid_frees = 0,
            };
            return st_ptr;
        }
    }

    fn destroy(*_) {
        const p = _.parent;
        _.live.deinit(p);
        zig {
            p.destroy(self);
        }
    }

    fn recordAlloc(*_, addr: usize, len: usize, tag: []const u8): ! {
        const seq = _.next_seq;
        _.next_seq += 1;
        _.live.put(_.parent, addr, TrackedAllocation{
            addr: addr,
            len: len,
            seq: seq,
            tag: tag,
        }) catch return ScopeError.OutOfMemory;

        _.live_allocations += 1;
        _.live_bytes += len;
        _.total_allocations += 1;
        _.total_allocated_bytes += len;
        if (_.live_bytes > _.peak_bytes) {
            _.peak_bytes = _.live_bytes;
        }
    }

    fn wasPreviouslyFreed(addr: usize): bool {
        const limit = if (_.freed_count < 1024) _.freed_count else (1024 as usize);
        let i: usize = 0;
        while (i < limit) : (i += 1) {
            if (_.freed_ring[i].addr == addr) {
                return true;
            }
        }
        return false;
    }

    fn pushFreed(*_, addr: usize, len: usize, seq: usize) {
        _.freed_ring[_.freed_cursor] = FreedRecord{
            addr: addr,
            len: len,
            seq: seq,
        };
        _.freed_cursor = (_.freed_cursor + 1) % 1024;
        if (_.freed_count < 1024) {
            _.freed_count += 1;
        }
    }

    fn recordFree(*_, addr: usize): ! {
        let removed_opt: TrackedAllocation? = null;
        zig {
            if (self.live.fetchRemove(addr)) |kv| {
                removed_opt = kv.value;
            }
        }
        if (entry := removed_opt) {
            if (_.live_allocations > 0) {
                _.live_allocations -= 1;
            }
            if (_.live_bytes >= entry.len) {
                _.live_bytes -= entry.len;
            } else {
                _.live_bytes = 0;
            }
            _.total_frees += 1;
            _.pushFreed(addr, entry.len, entry.seq);
            return;
        }

        if (_.wasPreviouslyFreed(addr)) {
            _.double_frees += 1;
            return ScopeError.DoubleFree;
        }
        _.invalid_frees += 1;
        return ScopeError.InvalidFree;
    }
}

fn scopeVtableAlloc(ctx: *!, len: usize, alignment: std.mem.Alignment, ret_addr: usize): [*]u8?! {
    unsafe {
        const st = ctx as *ScopeState!;
        if (ptr := st.parent.rawAlloc(len, alignment, ret_addr)) {
            const addr = @intFromPtr(ptr);
            st.recordAlloc(addr, len, "std.mem.Allocator") catch {
                st.parent.rawFree(ptr[0..len], alignment, ret_addr);
                return null;
            };
            return ptr;
        }
        return null;
    }
}

fn scopeVtableResize(ctx: *!, memory: []u8, alignment: std.mem.Alignment, new_len: usize, ret_addr: usize): bool {
    unsafe {
        const st = ctx as *ScopeState!;
        if (!st.parent.rawResize(memory, alignment, new_len, ret_addr)) {
            return false;
        }
        const addr = @intFromPtr(memory.ptr);
        if (entry := st.live.getPtr(addr)) {
            const old_len = entry.len;
            entry.len = new_len;
            if (new_len >= old_len) {
                const diff = new_len - old_len;
                st.live_bytes += diff;
                st.total_allocated_bytes += diff;
                if (st.live_bytes > st.peak_bytes) {
                    st.peak_bytes = st.live_bytes;
                }
            } else {
                const diff = old_len - new_len;
                st.live_bytes = if (st.live_bytes >= diff) (st.live_bytes - diff) else 0;
            }
        }
        return true;
    }
}

fn scopeVtableRemap(ctx: *!, memory: []u8, alignment: std.mem.Alignment, new_len: usize, ret_addr: usize): [*]u8?! {
    unsafe {
        const st = ctx as *ScopeState!;
        const old_addr = @intFromPtr(memory.ptr);
        if (new_ptr := st.parent.rawRemap(memory, alignment, new_len, ret_addr)) {
            const new_addr = @intFromPtr(new_ptr);
            st.recordFree(old_addr) catch {};
            st.recordAlloc(new_addr, new_len, "remap") catch {};
            return new_ptr;
        }
        return null;
    }
}

fn scopeVtableFree(ctx: *!, buf: []u8, alignment: std.mem.Alignment, ret_addr: usize) {
    unsafe {
        const st = ctx as *ScopeState!;
        if (buf.len == 0) {
            return;
        }
        const addr = @intFromPtr(buf.ptr);
        st.recordFree(addr) catch {
            // Do NOT forward invalid or double free to parent allocator to keep process safe.
            return;
        };
        st.parent.rawFree(buf, alignment, ret_addr);
    }
}

const SCOPE_VTABLE = std.mem.Allocator.VTable{
    alloc: &scopeVtableAlloc,
    resize: &scopeVtableResize,
    remap: &scopeVtableRemap,
    free: &scopeVtableFree,
};

pub struct AllocationScope {
    state: (*ScopeState)? = null;

    pub static fn init(parent: std.mem.Allocator): AllocationScope! {
        const st = ScopeState.create(parent)?;
        return AllocationScope{ state: st };
    }

    pub static fn initDefault(): AllocationScope! {
        return AllocationScope.init(std.heap.page_allocator);
    }

    pub fn deinit(*_) {
        if (st := _.state) {
            st.destroy();
            _.state = null;
        }
    }

    ~init() {
        _.deinit();
    }

    pub fn allocator(): std.mem.Allocator {
        if (st := _.state) {
            zig {
                return std.mem.Allocator{
                    .ptr = st,
                    .vtable = &SCOPE_VTABLE,
                };
            }
        }
        return std.heap.page_allocator;
    }

    pub fn stats(): ScopeStats {
        if (st := _.state) {
            return ScopeStats{
                live_allocations: st.live_allocations,
                live_bytes: st.live_bytes,
                total_allocations: st.total_allocations,
                total_allocated_bytes: st.total_allocated_bytes,
                peak_bytes: st.peak_bytes,
                total_frees: st.total_frees,
                double_frees: st.double_frees,
                invalid_frees: st.invalid_frees,
            };
        }
        return ScopeStats{};
    }

    pub fn liveAllocations(): usize {
        return _.stats().live_allocations;
    }

    pub fn liveBytes(): usize {
        return _.stats().live_bytes;
    }

    pub fn totalAllocatedBytes(): usize {
        return _.stats().total_allocated_bytes;
    }

    pub fn peakBytes(): usize {
        return _.stats().peak_bytes;
    }

    pub fn totalFrees(): usize {
        return _.stats().total_frees;
    }

    pub fn doubleFrees(): usize {
        return _.stats().double_frees;
    }

    pub fn invalidFrees(): usize {
        return _.stats().invalid_frees;
    }

    pub fn hasLeaks(): bool {
        return _.liveAllocations() > 0;
    }

    pub fn allocBytes(*_, len: usize, tag: []const u8 = "bytes"): ([]u8)! {
        const st = _.state ?? return ScopeError.OutOfMemory;
        if (len == 0) {
            let empty_slice: []u8 = [];
            return empty_slice;
        }
        const buf = st.parent.alloc(u8, len) catch return ScopeError.OutOfMemory;
        const addr = unsafe { @intFromPtr(buf.ptr) };
        st.recordAlloc(addr, len, tag) catch {
            st.parent.free(buf);
            return ScopeError.OutOfMemory;
        };
        return buf;
    }

    pub fn freeBytes(*_, slice: []u8): ! {
        const st = _.state ?? return ScopeError.InvalidFree;
        if (slice.len == 0) {
            return;
        }
        const addr = unsafe { @intFromPtr(slice.ptr) };
        st.recordFree(addr)?;
        st.parent.free(slice);
    }

    pub fn trackExternalAlloc(*_, slice: []const u8, tag: []const u8 = "external"): ! {
        const st = _.state ?? return ScopeError.OutOfMemory;
        if (slice.len == 0) return;
        const addr = unsafe { @intFromPtr(slice.ptr) };
        st.recordAlloc(addr, slice.len, tag)?;
    }

    pub fn trackExternalFree(*_, slice: []const u8): ! {
        const st = _.state ?? return ScopeError.InvalidFree;
        if (slice.len == 0) return;
        const addr = unsafe { @intFromPtr(slice.ptr) };
        st.recordFree(addr)?;
    }

    pub fn isOwned(slice: []const u8): bool {
        const st = _.state ?? return false;
        if (slice.len == 0) return false;
        const addr = unsafe { @intFromPtr(slice.ptr) };
        return st.live.contains(addr);
    }

    pub fn assertOwned(slice: []const u8): ! {
        if (!_.isOwned(slice)) {
            return ScopeError.NotOwned;
        }
    }

    pub fn assertUnowned(slice: []const u8): ! {
        if (_.isOwned(slice)) {
            return ScopeError.AlreadyOwned;
        }
    }

    pub fn assertNoLeaks(): ! {
        if (_.hasLeaks()) {
            return ScopeError.MemoryLeak;
        }
    }

    pub fn assertNoLeaksOrPanic() {
        if (_.hasLeaks()) {
            let buf: [1024]u8 = undefined;
            const rep = _.formatReport(&buf);
            std.debug.panic("{s}", .{rep});
        }
    }

    pub fn formatReport(out_buf: []u8): []const u8 {
        const s = _.stats();
        let pos: usize = 0;
        const header = std.fmt.bufPrint(
            out_buf[pos..],
            "AllocationScope Report:\n  live_allocations: {d}\n  live_bytes: {d}\n  total_allocations: {d}\n  total_allocated_bytes: {d}\n  peak_bytes: {d}\n  total_frees: {d}\n  double_frees: {d}\n  invalid_frees: {d}\n",
            .{
                s.live_allocations,
                s.live_bytes,
                s.total_allocations,
                s.total_allocated_bytes,
                s.peak_bytes,
                s.total_frees,
                s.double_frees,
                s.invalid_frees,
            },
        ) catch return out_buf[0..pos];
        pos += header.len;

        if (st := _.state) {
            if (s.live_allocations > 0 && pos < out_buf.len) {
                const lhdr = std.fmt.bufPrint(out_buf[pos..], "  leaks:\n", .{}) catch return out_buf[0..pos];
                pos += lhdr.len;
                let it = st.live.iterator();
                let shown: usize = 0;
                while (entry := it.next()) {
                    if (shown >= 8) break;
                    const item = entry.value_ptr.*;
                    const line = std.fmt.bufPrint(
                        out_buf[pos..],
                        "    - #{d} addr=0x{x} len={d} tag={s}\n",
                        .{ item.seq, item.addr, item.len, item.tag },
                    ) catch break;
                    pos += line.len;
                    shown += 1;
                }
            }
        }
        return out_buf[0..pos];
    }

    pub fn formatReportAlloc(alloc: std.mem.Allocator): ([]u8)! {
        let tmp: [4096]u8 = undefined;
        const rep = _.formatReport(&tmp);
        return alloc.dupe(u8, rep);
    }
}
