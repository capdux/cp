// Source: /data/dev/bun/src/http_jsc/websocket_client/
// ============================================================================
// Absorbed from Bun: bun/src/ws/mask.cp
// Upstream Sync: Refer to bun/src/ws/mask.zig for future upstream updates.
// ============================================================================
// lib/ws/mask.cp
// Fast RFC 6455 4-byte XOR masking in CP

pub struct Mask {
    pub static fn apply(data: []u8, key: [4]u8) {
        let i: usize = 0;
        const len = data.len;
        while (i < len) : (i += 1) {
            data[i] ^= key[i % 4];
        }
    }
}
