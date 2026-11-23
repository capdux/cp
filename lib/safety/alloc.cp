// Source: Absorbed from /data/dev/bun/src/safety/alloc.cp | Synced: 2026-11-23T01:34:00Z
import std from "std";

const alloc_safety_enabled: bool = true;

/// Asserts that two allocators are identical.
pub fn assertEq(alloc1: std.mem.Allocator, alloc2: std.mem.Allocator) {
    const vt1: *const anyopaque = unsafe { @ptrCast(alloc1.vtable) };
    const vt2: *const anyopaque = unsafe { @ptrCast(alloc2.vtable) };
    if (vt1 != vt2) {
        @panic("allocator mismatch: vtables differ");
    }
    if (alloc1.ptr != alloc2.ptr) {
        @panic("allocator mismatch: pointers differ");
    }
}

pub fn assertAllocatorEq(alloc1: std.mem.Allocator, alloc2: std.mem.Allocator) {
    assertEq(alloc1, alloc2);
}

/// Asserts that two allocators are equal with a custom message.
pub fn assertEqFmt(alloc1: std.mem.Allocator, alloc2: std.mem.Allocator, comptime msg: []const u8) {
    * = msg;
    assertEq(alloc1, alloc2);
}

/// A safety wrapper around unmanaged collections to ensure multiple distinct allocators
/// are not inadvertently mixed on the same collection instance.
pub struct CheckedAllocator {
    vtable: (*const anyopaque)? = null,
    ptr: (*anyopaque)? = null,

    pub static fn init(allocator: std.mem.Allocator): CheckedAllocator {
        return CheckedAllocator{
            vtable: unsafe { @ptrCast(allocator.vtable) },
            ptr: allocator.ptr,
        };
    }

    pub fn set(*_, allocator: std.mem.Allocator) {
        if (_.vtable == null) {
            _.vtable = unsafe { @ptrCast(allocator.vtable) };
            _.ptr = allocator.ptr;
        } else {
            _.assertEq(allocator);
        }
    }

    pub fn assertEq(allocator: std.mem.Allocator) {
        if (vt := _.vtable) {
            const cur_vt: *const anyopaque = unsafe { @ptrCast(allocator.vtable) };
            if (vt != cur_vt) {
                @panic("allocator mismatch: container vtable != incoming");
            }
            if (p := _.ptr) {
                if (p != allocator.ptr) {
                    @panic("allocator mismatch: container ptr != incoming");
                }
            }
        }
    }

    pub fn isSet(): bool {
        return _.vtable != null;
    }

    pub fn reset(*_) {
        _.vtable = null;
        _.ptr = null;
    }
}
