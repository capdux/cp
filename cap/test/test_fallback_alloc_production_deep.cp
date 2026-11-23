import std from "std";
import { test, expect } from "cap:test";
import { StackBufferFallbackAllocator as BufferFallbackAllocator } from "cap:mem";

test("fallback_alloc: fast-path allocation stays inside inline buffer", () => {
    let stack_buf: [256]u8 = undefined;
    let fba = BufferFallbackAllocator.init(stack_buf[0..], std.testing.allocator);
    const alloc = fba.allocator();

    const a = alloc.alloc(u8, 64) catch unreachable;
    defer alloc.free(a);
    expect(fba.ownsBufferPtr(a.ptr)).toBeTrue();
    expect(fba.usedBytes()).toBe(64);
    expect(fba.remainingBytes()).toBe(192);
    expect(fba.capacity()).toBe(256);

    const b = alloc.alloc(u8, 32) catch unreachable;
    defer alloc.free(b);
    expect(fba.ownsBufferPtr(b.ptr)).toBeTrue();
    expect(fba.usedBytes()).toBe(96);
});

test("fallback_alloc: overflow transparently falls back to backing allocator", () => {
    let stack_buf: [64]u8 = undefined;
    let fba = BufferFallbackAllocator.init(stack_buf[0..], std.testing.allocator);
    const alloc = fba.allocator();

    const small = alloc.alloc(u8, 48) catch unreachable;
    defer alloc.free(small);
    expect(fba.ownsBufferPtr(small.ptr)).toBeTrue();

    // 128 > remaining 16 bytes => routed to std.testing.allocator (zero-leak checked!)
    let big = alloc.alloc(u8, 128) catch unreachable;
    defer alloc.free(big);
    expect(fba.ownsBufferPtr(big.ptr)).toBeFalse();
    big[0] = 0xAA;
    big[127] = 0x55;
    expect(big[0]).toBe(0xAA);
    expect(big[127]).toBe(0x55);
});

test("fallback_alloc: reset reclaims inline buffer for reuse", () => {
    let stack_buf: [128]u8 = undefined;
    let fba = BufferFallbackAllocator.init(stack_buf[0..], std.testing.allocator);
    const alloc = fba.allocator();

    const s1 = alloc.alloc(u8, 100) catch unreachable;
    expect(fba.ownsBufferPtr(s1.ptr)).toBeTrue();
    expect(fba.usedBytes()).toBe(100);

    fba.reset();
    expect(fba.usedBytes()).toBe(0);
    expect(fba.remainingBytes()).toBe(128);

    const s2 = alloc.alloc(u8, 100) catch unreachable;
    expect(fba.ownsBufferPtr(s2.ptr)).toBeTrue();
    expect(fba.usedBytes()).toBe(100);
});

test("fallback_alloc: resize and remap work across inline and fallback regions", () => {
    let stack_buf: [64]u8 = undefined;
    let fba = BufferFallbackAllocator.init(stack_buf[0..], std.testing.allocator);
    const alloc = fba.allocator();

    let list = std.ArrayList(u32).empty;
    defer list.deinit(alloc);

    let i: u32 = 0;
    while (i < 64) : (i += 1) {
        list.append(alloc, i * 3) catch unreachable;
    }
    expect(list.items.len).toBe(64);
    expect(list.items[0]).toBe(0);
    expect(list.items[63]).toBe(189);
});
