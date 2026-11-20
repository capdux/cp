// Source: /data/dev/bun/src/bun_alloc/memory.cp
// lib/mem/primitives.cp
// Memory alignment primitives & compiler-optimization-proof secure memory zeroing

/// Returns `true` if `n > 0` and `n` is a power of two.
pub fn isPowerOfTwo(n: usize): bool {
    return n > 0 && (n & (n - 1)) == 0;
}

/// Aligns `addr` upward to the nearest multiple of `alignment` (must be power of two).
pub fn alignForward(addr: usize, alignment: usize): usize {
    if (!isPowerOfTwo(alignment)) {
        return addr;
    }
    const mask = alignment - 1;
    return (addr + mask) & ~mask;
}

/// Aligns `addr` downward to the nearest multiple of `alignment` (must be power of two).
pub fn alignBackward(addr: usize, alignment: usize): usize {
    if (!isPowerOfTwo(alignment)) {
        return addr;
    }
    const mask = alignment - 1;
    return addr & ~mask;
}

/// Returns the padding bytes needed to advance `addr` to the next `alignment` boundary.
pub fn alignDelta(addr: usize, alignment: usize): usize {
    const aligned = alignForward(addr, alignment);
    return aligned - addr;
}

/// Zeroes `buf` using volatile writes so the compiler cannot dead-store-eliminate secret erasure.
pub fn secureZero(buf: []u8) {
    if (buf.len == 0) {
        return;
    }
    zig {
        const vptr: [*]volatile u8 = @ptrCast(buf.ptr);
        for (0..buf.len) |i| {
            vptr[i] = 0;
        }
    }
}
