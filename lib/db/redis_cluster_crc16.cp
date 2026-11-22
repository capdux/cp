// Source: cp/lib/redis_cluster/crc16.cp (Redis Cluster CRC16-CCITT & Hash Tag)

const std = @import("std");

pub const REDIS_CLUSTER_SLOTS: u16 = 16384;

// CRC16-CCITT (XMODEM, poly 0x1021, init 0x0000, no reflection, no final XOR)
// Official Redis Cluster test vector: crc16Ccitt("123456789") == 0x31C3
pub fn crc16Ccitt(data: []const u8): u16 {
    let crc: u16 = 0;
    for (byte of data) {
        crc = crc ^ (@as(u16, byte) << 8);
        let i: u8 = 0;
        while (i < 8) {
            if ((crc & 0x8000) != 0) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
            i += 1;
        }
    }
    return crc;
}

// Extract Redis Cluster Hash Tag from key:
// 1. Find the first '{'
// 2. Find the first '}' after that '{'
// 3. If both exist and there is at least 1 byte between them, return that slice
// 4. Otherwise return the entire key
pub fn extractHashTag(key: []const u8): []const u8 {
    let open_idx: usize? = null;
    let idx: usize = 0;
    while (idx < key.len) {
        if (key[idx] == '{') {
            open_idx = idx;
            break;
        }
        idx += 1;
    }
    if (open_idx == null) {
        return key;
    }
    const start = open_idx.?;
    let i: usize = start + 1;
    while (i < key.len) {
        if (key[i] == '}') {
            if (i == start + 1) {
                return key;
            }
            return key[start + 1 .. i];
        }
        i += 1;
    }
    return key;
}

// Compute the Redis Cluster slot [0, 16383] for a key
pub fn keySlot(key: []const u8): u16 {
    const tag = extractHashTag(key);
    return crc16Ccitt(tag) % REDIS_CLUSTER_SLOTS;
}

// Verify all keys map to the same Redis Cluster slot
pub fn ensureSameSlot(keys: []const []const u8): !u16 {
    if (keys.len == 0) {
        return 0;
    }
    const first_slot = keySlot(keys[0]);
    let i: usize = 1;
    while (i < keys.len) {
        if (keySlot(keys[i]) != first_slot) {
            return error.CrossSlot;
        }
        i += 1;
    }
    return first_slot;
}
