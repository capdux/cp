// Source: Absorbed from /data/dev/bun/src/bun_alloc/BufferFallbackAllocator.cp | Synced: 2026-11-22T12:43:45Z
// lib/mem/fallback_allocator.cp
// BufferFallbackAllocator<N>, MaybeOwned<T>, and Linux MemFdBuffer

const std = @import("std");
import { alignForward } from "./primitives.cp";

pub err FallbackAllocError {
    OutOfMemory,
}

/// Stack-first allocator with N-byte inline fixed buffer and seamless heap fallback.
pub struct BufferFallbackAllocator<const N: usize> {
    buffer: [N]u8 = [0; N],
    stack_used: usize = 0,
    fallback: std.mem.Allocator = std.heap.page_allocator,
    stack_allocs: usize = 0,
    stack_bytes: usize = 0,
    heap_allocs: usize = 0,
    heap_bytes: usize = 0,

    pub static fn init(fallback: std.mem.Allocator): BufferFallbackAllocator<N> {
        return BufferFallbackAllocator<N>{
            buffer: [0; N],
            stack_used: 0,
            fallback: fallback,
            stack_allocs: 0,
            stack_bytes: 0,
            heap_allocs: 0,
            heap_bytes: 0,
        };
    }

    pub static fn initDefault(): BufferFallbackAllocator<N> {
        return BufferFallbackAllocator<N>.init(std.heap.page_allocator);
    }

    pub fn ownsSlice(*_, slice: []const u8): bool {
        if (N == 0 || slice.len == 0) {
            return false;
        }
        let buf_start: usize = 0;
        let sl_start: usize = 0;
        unsafe {
            buf_start = @intFromPtr(&_.buffer[0]);
            sl_start = @intFromPtr(slice.ptr);
        }
        return sl_start >= buf_start && (sl_start + slice.len) <= (buf_start + N);
    }

    pub fn allocStack(*_, len: usize): ([]u8)? {
        if (_.stack_used + len > N) {
            return null;
        }
        const start = _.stack_used;
        _.stack_used += len;
        _.stack_allocs += 1;
        _.stack_bytes += len;
        return _.buffer[start .. start + len];
    }

    pub fn allocAligned(*_, len: usize, align_pow2: usize): ([]u8)! {
        if (align_pow2 == 0 || (align_pow2 & (align_pow2 - 1)) != 0) {
            return FallbackAllocError.OutOfMemory;
        }
        if (len == 0) {
            let empty_out: []u8 = [];
            return empty_out;
        }
        if (N > 0) {
            const base_addr = unsafe { @intFromPtr(&_.buffer[0]) };
            const cur_addr = base_addr + _.stack_used;
            const aligned_addr = alignForward(cur_addr, align_pow2);
            const pad = aligned_addr - cur_addr;
            if (_.stack_used + pad + len <= N) {
                const start = _.stack_used + pad;
                _.stack_used = start + len;
                _.stack_allocs += 1;
                _.stack_bytes += len;
                return _.buffer[start .. start + len];
            }
        }
        const heap_buf = _.fallback.alloc(u8, len) catch return FallbackAllocError.OutOfMemory;
        _.heap_allocs += 1;
        _.heap_bytes += len;
        return heap_buf;
    }

    /// Allocates `len` bytes from the N-byte stack buffer if possible, seamlessly falling back to heap.
    pub fn allocBytes(*_, len: usize): ([]u8)! {
        if (len == 0) {
            let empty_out: []u8 = [];
            return empty_out;
        }
        if (stack_mem := _.allocStack(len)) {
            return stack_mem;
        }
        const heap_buf = _.fallback.alloc(u8, len) catch return FallbackAllocError.OutOfMemory;
        _.heap_allocs += 1;
        _.heap_bytes += len;
        return heap_buf;
    }

    pub fn freeBytes(*_, slice: []u8) {
        if (slice.len == 0) {
            return;
        }
        if (_.ownsSlice(slice)) {
            const buf_start = unsafe { @intFromPtr(&_.buffer[0]) };
            const sl_start = unsafe { @intFromPtr(slice.ptr) };
            if (sl_start + slice.len == buf_start + _.stack_used) {
                _.stack_used = sl_start - buf_start;
            }
            return;
        }
        _.fallback.free(slice);
    }

    pub fn stackCapacity(): usize {
        return N;
    }

    pub fn stackUsed(): usize {
        return _.stack_used;
    }

    pub fn stackRemaining(): usize {
        return N - _.stack_used;
    }

    pub fn stackAllocs(): usize {
        return _.stack_allocs;
    }

    pub fn heapAllocs(): usize {
        return _.heap_allocs;
    }

    pub fn heapBytes(): usize {
        return _.heap_bytes;
    }

    pub fn reset(*_) {
        _.stack_used = 0;
    }

    static fn vtableAlloc(ctx: *!, len: usize, alignment: std.mem.Alignment, ret_addr: usize): [*]u8?! {
        zig {
            const inst: *@This() = @ptrCast(@alignCast(ctx));
            const align_bytes = alignment.toByteUnits();
            if (N > 0 and len > 0) {
                const base_addr = @intFromPtr(&inst.buffer[0]);
                const cur_addr = base_addr + inst.stack_used;
                const aligned_addr = alignForward(cur_addr, align_bytes);
                const pad = aligned_addr - cur_addr;
                if (inst.stack_used + pad + len <= N) {
                    const start = inst.stack_used + pad;
                    inst.stack_used = start + len;
                    inst.stack_allocs += 1;
                    inst.stack_bytes += len;
                    return @as([*]u8, @ptrCast(&inst.buffer[start]));
                }
            }
            if (inst.fallback.rawAlloc(len, alignment, ret_addr)) |ptr| {
                inst.heap_allocs += 1;
                inst.heap_bytes += len;
                return ptr;
            }
            return null;
        }
    }

    static fn vtableResize(ctx: *!, memory: []u8, alignment: std.mem.Alignment, new_len: usize, ret_addr: usize): bool {
        zig {
            const inst: *@This() = @ptrCast(@alignCast(ctx));
            if (inst.ownsSlice(memory)) {
                const buf_start = @intFromPtr(&inst.buffer[0]);
                const sl_start = @intFromPtr(memory.ptr);
                const off = sl_start - buf_start;
                if (off + memory.len == inst.stack_used and off + new_len <= N) {
                    inst.stack_used = off + new_len;
                    return true;
                }
                return false;
            }
            return inst.fallback.rawResize(memory, alignment, new_len, ret_addr);
        }
    }

    static fn vtableRemap(ctx: *!, memory: []u8, alignment: std.mem.Alignment, new_len: usize, ret_addr: usize): [*]u8?! {
        zig {
            const inst: *@This() = @ptrCast(@alignCast(ctx));
            if (inst.ownsSlice(memory)) {
                if (vtableResize(ctx, memory, alignment, new_len, ret_addr)) {
                    return memory.ptr;
                }
                return null;
            }
            return inst.fallback.rawRemap(memory, alignment, new_len, ret_addr);
        }
    }

    static fn vtableFree(ctx: *!, buf: []u8, alignment: std.mem.Alignment, ret_addr: usize) {
        zig {
            const inst: *@This() = @ptrCast(@alignCast(ctx));
            if (buf.len == 0) {
                return;
            }
            if (inst.ownsSlice(buf)) {
                const buf_start = @intFromPtr(&inst.buffer[0]);
                const sl_start = @intFromPtr(buf.ptr);
                if (sl_start + buf.len == buf_start + inst.stack_used) {
                    inst.stack_used = sl_start - buf_start;
                }
                return;
            }
            inst.fallback.rawFree(buf, alignment, ret_addr);
        }
    }

    pub fn allocator(*_): std.mem.Allocator {
        zig {
            const VTABLE = std.mem.Allocator.VTable{
                .alloc = &vtableAlloc,
                .resize = &vtableResize,
                .remap = &vtableRemap,
                .free = &vtableFree,
            };
            return std.mem.Allocator{
                .ptr = self,
                .vtable = &VTABLE,
            };
        }
    }
}

/// Zero-cost abstraction over borrowed slices (`[]const T`) and heap-owned slices (`[]T`).
pub struct MaybeOwned<T> {
    slice_data: []const T = [],
    owned_slice: []T = [],
    owned: bool = false,
    alloc: std.mem.Allocator = std.heap.page_allocator,

    pub static fn initBorrowed(data: []const T): MaybeOwned<T> {
        return MaybeOwned<T>{
            slice_data: data,
            owned_slice: [],
            owned: false,
            alloc: std.heap.page_allocator,
        };
    }

    pub static fn initOwned(data: []T, alloc: std.mem.Allocator): MaybeOwned<T> {
        return MaybeOwned<T>{
            slice_data: data,
            owned_slice: data,
            owned: true,
            alloc: alloc,
        };
    }

    pub static fn initOwnedDefault(data: []T): MaybeOwned<T> {
        return MaybeOwned<T>.initOwned(data, std.heap.page_allocator);
    }

    pub static fn initOwnedCopy(data: []const T, alloc: std.mem.Allocator): MaybeOwned<T>! {
        if (data.len == 0) {
            return MaybeOwned<T>{
                slice_data: [],
                owned_slice: [],
                owned: true,
                alloc: alloc,
            };
        }
        const duped = alloc.dupe(T, data) catch return FallbackAllocError.OutOfMemory;
        return MaybeOwned<T>{
            slice_data: duped,
            owned_slice: duped,
            owned: true,
            alloc: alloc,
        };
    }

    pub static fn initOwnedCopyDefault(data: []const T): MaybeOwned<T>! {
        return MaybeOwned<T>.initOwnedCopy(data, std.heap.page_allocator);
    }

    pub fn isOwned(): bool {
        return _.owned;
    }

    pub fn len(): usize {
        return _.slice_data.len;
    }

    pub fn asSlice(): []const T {
        return _.slice_data;
    }

    /// Promotes a borrowed slice to a heap-owned mutable slice (cloning once if borrowed).
    pub fn toOwned(*_): ([]T)! {
        if (_.owned) {
            return _.owned_slice;
        }
        if (_.slice_data.len == 0) {
            _.owned = true;
            _.owned_slice = [];
            return _.owned_slice;
        }
        const duped = _.alloc.dupe(T, _.slice_data) catch return FallbackAllocError.OutOfMemory;
        _.owned_slice = duped;
        _.slice_data = duped;
        _.owned = true;
        return duped;
    }

    pub fn toOwnedWith(*_, alloc: std.mem.Allocator): ([]T)! {
        if (!_.owned) {
            _.alloc = alloc;
        }
        return _.toOwned();
    }

    pub fn deinit(*_) {
        if (_.owned && _.owned_slice.len > 0) {
            _.alloc.free(_.owned_slice);
        }
        _.slice_data = [];
        _.owned_slice = [];
        _.owned = false;
    }

    ~init() {
        _.deinit();
    }
}

pub err MemFdError {
    CreateFailed,
    TruncateFailed,
    WriteFailed,
    MmapFailed,
    InvalidFd,
}

/// Linux `memfd_create` anonymous memory-backed file buffer with shared mmap view & FD transfer.
pub struct MemFdBuffer {
    fd_raw: i32 = -1;
    mapped_ptr: [*]u8! = undefined;
    mapped_len: usize = 0;
    data_len: usize = 0;

    static fn syscallIsErr(rc: usize): bool {
        return rc > (std.math.maxInt(usize) - 4096);
    }

    pub static fn create(bytes: []const u8, label: [:0]const u8 = "cp-memfd"): MemFdBuffer! {
        zig {
            const flags: u32 = std.os.linux.MFD.CLOEXEC;
            const fd_rc = std.os.linux.memfd_create(label.ptr, flags);
            if (fd_rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.CreateFailed;
            }
            const fd_val: i32 = @intCast(fd_rc);
            errdefer _ = std.os.linux.close(fd_val);

            const target_len: usize = if (bytes.len == 0) 4096 else bytes.len;
            const trunc_rc = std.os.linux.ftruncate(fd_val, @intCast(target_len));
            if (trunc_rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.TruncateFailed;
            }

            var off: usize = 0;
            while (off < bytes.len) {
                const w_rc = std.os.linux.pwrite(fd_val, bytes[off..].ptr, bytes.len - off, @intCast(off));
                if (w_rc > std.math.maxInt(usize) - 4096) {
                    const e = std.os.linux.errno(w_rc);
                    if (e == .AGAIN or e == .INTR) continue;
                    return MemFdError.WriteFailed;
                }
                if (w_rc == 0) {
                    return MemFdError.WriteFailed;
                }
                off += w_rc;
            }

            const page_sz = std.heap.pageSize();
            const map_len = std.mem.alignForward(usize, target_len, page_sz);
            const prot = std.os.linux.PROT{ .READ = true, .WRITE = true };
            const mflags = std.os.linux.MAP{ .TYPE = .SHARED };
            const m_rc = std.os.linux.mmap(null, map_len, prot, mflags, fd_val, 0);
            if (m_rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.MmapFailed;
            }
            const map_ptr: [*]u8 = @ptrFromInt(m_rc);
            return MemFdBuffer{
                .fd_raw = fd_val,
                .mapped_ptr = map_ptr,
                .mapped_len = map_len,
                .data_len = bytes.len,
            };
        }
    }

    pub static fn createWithSize(size: usize, label: [:0]const u8 = "cp-memfd"): MemFdBuffer! {
        zig {
            const flags: u32 = std.os.linux.MFD.CLOEXEC;
            const fd_rc = std.os.linux.memfd_create(label.ptr, flags);
            if (fd_rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.CreateFailed;
            }
            const fd_val: i32 = @intCast(fd_rc);
            errdefer _ = std.os.linux.close(fd_val);

            const target_len: usize = if (size == 0) 4096 else size;
            const trunc_rc = std.os.linux.ftruncate(fd_val, @intCast(target_len));
            if (trunc_rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.TruncateFailed;
            }

            const page_sz = std.heap.pageSize();
            const map_len = std.mem.alignForward(usize, target_len, page_sz);
            const prot = std.os.linux.PROT{ .READ = true, .WRITE = true };
            const mflags = std.os.linux.MAP{ .TYPE = .SHARED };
            const m_rc = std.os.linux.mmap(null, map_len, prot, mflags, fd_val, 0);
            if (m_rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.MmapFailed;
            }
            const map_ptr: [*]u8 = @ptrFromInt(m_rc);
            return MemFdBuffer{
                .fd_raw = fd_val,
                .mapped_ptr = map_ptr,
                .mapped_len = map_len,
                .data_len = size,
            };
        }
    }

    pub fn fd(): i32 {
        return _.fd_raw;
    }

    pub fn isValid(): bool {
        return _.fd_raw >= 0 && _.mapped_len > 0;
    }

    pub fn len(): usize {
        return _.data_len;
    }

    pub fn capacity(): usize {
        return _.mapped_len;
    }

    pub fn asSlice(): []const u8 {
        if (!_.isValid() || _.data_len == 0) {
            return "";
        }
        let out: []const u8 = "";
        zig {
            out = self.mapped_ptr[0..self.data_len];
        }
        return out;
    }

    pub fn asMutSlice(): []u8 {
        if (!_.isValid() || _.data_len == 0) {
            let empty_mut: []u8 = [];
            return empty_mut;
        }
        let out: []u8 = [];
        zig {
            out = self.mapped_ptr[0..self.data_len];
        }
        return out;
    }

    /// Reads directly from the underlying anonymous memfd via `pread` (verifying kernel-backed storage).
    pub fn readFd(offset: usize, out_buf: []u8): usize! {
        if (_.fd_raw < 0) {
            return MemFdError.InvalidFd;
        }
        if (out_buf.len == 0) {
            return 0;
        }
        let got: usize = 0;
        zig {
            const rc = std.os.linux.pread(self.fd_raw, out_buf.ptr, out_buf.len, @intCast(offset));
            if (rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.WriteFailed;
            }
            got = rc;
        }
        return got;
    }

    /// Writes directly to the underlying anonymous memfd via `pwrite`.
    pub fn writeFd(*_, offset: usize, data: []const u8): usize! {
        if (_.fd_raw < 0) {
            return MemFdError.InvalidFd;
        }
        if (data.len == 0) {
            return 0;
        }
        let wrote: usize = 0;
        zig {
            const rc = std.os.linux.pwrite(self.fd_raw, data.ptr, data.len, @intCast(offset));
            if (rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.WriteFailed;
            }
            wrote = rc;
        }
        if (offset + wrote > _.data_len && offset + wrote <= _.mapped_len) {
            _.data_len = offset + wrote;
        }
        return wrote;
    }

    /// Creates a second shared mmap view backed by a duplicated fd (`F_DUPFD_CLOEXEC`).
    pub fn cloneView(): MemFdBuffer! {
        if (!_.isValid()) {
            return MemFdError.InvalidFd;
        }
        zig {
            const dup_rc = std.os.linux.fcntl(self.fd_raw, std.os.linux.F.DUPFD_CLOEXEC, 0);
            if (dup_rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.InvalidFd;
            }
            const new_fd: i32 = @intCast(dup_rc);
            errdefer _ = std.os.linux.close(new_fd);

            const new_map_len = self.mapped_len;
            const prot = std.os.linux.PROT{ .READ = true, .WRITE = true };
            const mflags = std.os.linux.MAP{ .TYPE = .SHARED };
            const m_rc = std.os.linux.mmap(null, new_map_len, prot, mflags, new_fd, 0);
            if (m_rc > std.math.maxInt(usize) - 4096) {
                return MemFdError.MmapFailed;
            }
            const new_ptr: [*]u8 = @ptrFromInt(m_rc);
            return MemFdBuffer{
                .fd_raw = new_fd,
                .mapped_ptr = new_ptr,
                .mapped_len = new_map_len,
                .data_len = self.data_len,
            };
        }
    }

    pub fn deinit(*_) {
        zig {
            if (self.mapped_len > 0) {
                _ = std.os.linux.munmap(self.mapped_ptr, self.mapped_len);
                self.mapped_len = 0;
            }
            if (self.fd_raw >= 0) {
                _ = std.os.linux.close(self.fd_raw);
                self.fd_raw = -1;
            }
            self.data_len = 0;
        }
    }

    ~init() {
        _.deinit();
    }
}
