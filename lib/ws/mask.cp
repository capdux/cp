// Source: /data/dev/bun/src/http_jsc/websocket_client/
// ============================================================================
// Absorbed from Bun: bun/src/ws/mask.cp
// Upstream Sync: Refer to bun/src/ws/mask.zig for future upstream updates.
// ============================================================================
// lib/ws/mask.cp
// Fast SIMD-accelerated RFC 6455 4-byte XOR masking in CP

pub struct Mask {
    /// Applies RFC 6455 4-byte XOR mask in-place with 16-byte SIMD vectorization.
    pub static fn apply(data: []u8, key: [4]u8) {
        Mask.applyWithOffset(data, key, 0);
    }

    /// Applies RFC 6455 4-byte XOR mask in-place starting at `start_offset` within the payload.
    pub static fn applyWithOffset(data: []u8, key: [4]u8, start_offset: usize) {
        const len = data.len;
        if (len == 0) return;

        let i: usize = 0;
        const rot = start_offset % 4;
        const k0 = key[rot];
        const k1 = key[(rot + 1) % 4];
        const k2 = key[(rot + 2) % 4];
        const k3 = key[(rot + 3) % 4];

        if (len >= 16) {
            const key_vec: @Vector(16, u8) = [
                k0, k1, k2, k3,
                k0, k1, k2, k3,
                k0, k1, k2, k3,
                k0, k1, k2, k3,
            ];
            while (i + 16 <= len) {
                zig {
                    const ptr: *[16]u8 = @ptrCast(data[i..].ptr);
                    const chunk: @Vector(16, u8) = ptr.*;
                    ptr.* = chunk ^ key_vec;
                }
                i += 16;
            }
        }

        const local_key: [4]u8 = [k0, k1, k2, k3];
        while (i < len) : (i += 1) {
            data[i] ^= local_key[i % 4];
        }
    }
}
