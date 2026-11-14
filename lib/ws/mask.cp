// lib/ws/mask.cp
// Fast RFC 6455 4-byte XOR masking in CP

pub const Mask = struct {
    pub fn apply(data: []u8, key: [4]u8) {
        let i: usize = 0;
        const len = data.len;
        while (i < len) : (i += 1) {
            data[i] ^= key[i % 4];
        }
    }
};
