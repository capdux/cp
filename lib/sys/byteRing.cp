// Origin: Pure-CP Stdlib (Zero-Heap Power-of-Two Masked Batch Ring Byte & Framed Message Queue) | Created: 2026-11-22
// Pure CP zero-heap-allocation power-of-two bitmask ring byte & length-prefixed frame queue
// for high-throughput I/O pipelines, audio streaming, and AI Agent event buses:
//   1. `ByteRingStats`
//   2. `ByteRingQueue`:
//      - Strict power-of-two capacity (`>= 2`) using zero-modulo bitmask indexing (`idx & mask`)
//      - Two-segment wraparound batch copy (`writeBytes`, `writeAllAtomic`, `readBytes`, `peekBytes`, `skipBytes`)
//      - Atomic 4-byte LE length-prefixed framing (`writeFrame`, `readFrame`) eliminating sticky packets
//      - High-watermark, dropped-write, and integer basis-point fill ratio telemetry

const std = @import("std");

/// Point-in-time and cumulative telemetry snapshot for `ByteRingQueue`.
pub struct ByteRingStats {
    capacity: usize = 0;
    used: usize = 0;
    free: usize = 0;
    high_watermark: usize = 0;
    total_written: u64 = 0;
    total_read: u64 = 0;
    dropped_writes: u64 = 0;
}

fn isPowerOfTwoAtLeast2(n: usize): bool {
    return n >= 2 && (n & (n - 1)) == 0;
}

fn encodeU32LE(v: u32, out4: *[4]u8) {
    out4[0] = (v & 0xFF) as u8;
    out4[1] = ((v >> 8) & 0xFF) as u8;
    out4[2] = ((v >> 16) & 0xFF) as u8;
    out4[3] = ((v >> 24) & 0xFF) as u8;
}

fn decodeU32LE(in4: [4]u8): u32 {
    return (in4[0] as u32) |
        ((in4[1] as u32) << 8) |
        ((in4[2] as u32) << 16) |
        ((in4[3] as u32) << 24);
}

/// Zero-heap-allocation power-of-two bitmask batch ring byte & length-prefixed frame queue.
pub struct ByteRingQueue {
    buf: []u8;
    mask: usize = 0;
    head: usize = 0;
    tail: usize = 0;
    high_watermark: usize = 0;
    total_written: u64 = 0;
    total_read: u64 = 0;
    dropped_writes: u64 = 0;

    /// Initializes a ring queue over caller-provided `storage`.
    /// Requires `storage.len >= 2` and a power of two (enabling zero-modulo bitmask indexing).
    /// Returns `null` if `storage.len` is invalid.
    static fn init(storage: *[]u8): ByteRingQueue? {
        if (!isPowerOfTwoAtLeast2(storage.len)) return null;
        return ByteRingQueue{
            buf: storage,
            mask: storage.len - 1,
            head: 0,
            tail: 0,
            high_watermark: 0,
            total_written: 0,
            total_read: 0,
            dropped_writes: 0,
        };
    }

    /// Returns the number of unread bytes currently stored in the ring.
    fn len(): usize {
        return _.tail -% _.head;
    }

    /// Returns the number of free bytes available for writing.
    fn available(): usize {
        return _.buf.len - (_.tail -% _.head);
    }

    /// Returns `true` if the ring contains zero unread bytes.
    fn isEmpty(): bool {
        return _.head == _.tail;
    }

    /// Returns `true` if the ring has zero free bytes remaining.
    fn isFull(): bool {
        return (_.tail -% _.head) == _.buf.len;
    }

    /// Returns current ring utilization in integer basis points (`0..=10000`, where `10000` = 100.00%).
    fn fillRatioBps(): u32 {
        const used: u64 = (_.tail -% _.head) as u64;
        const cap: u64 = _.buf.len as u64;
        return ((used * 10000) / cap) as u32;
    }

    /// Clears all unread bytes in O(1) by resetting ring cursors.
    fn clear(*_) {
        _.head = 0;
        _.tail = 0;
    }

    /// Writes up to `min(src.len, available())` bytes using two-segment wraparound copy.
    /// Returns the actual number of bytes written.
    fn writeBytes(*_, src: []const u8): usize {
        if (src.len == 0) return 0;
        const free: usize = _.available();
        if (free == 0) return 0;

        const to_write: usize = if (src.len < free) src.len else free;
        const w_pos: usize = _.tail & _.mask;
        const tail_room: usize = _.buf.len - w_pos;
        const first: usize = if (to_write < tail_room) to_write else tail_room;

        let i: usize = 0;
        while (i < first) : (i += 1) {
            _.buf[w_pos + i] = src[i];
        }
        const second: usize = to_write - first;
        let j: usize = 0;
        while (j < second) : (j += 1) {
            _.buf[j] = src[first + j];
        }

        _.tail = _.tail +% to_write;
        _.total_written += to_write as u64;
        const used_now: usize = _.tail -% _.head;
        if (used_now > _.high_watermark) {
            _.high_watermark = used_now;
        }
        return to_write;
    }

    /// Atomically writes all of `src` if `src.len <= available()`.
    /// If insufficient free space exists, performs zero partial writes, increments `dropped_writes`,
    /// and returns `false`.
    fn writeAllAtomic(*_, src: []const u8): bool {
        if (src.len > _.available()) {
            _.dropped_writes += 1;
            return false;
        }
        * = _.writeBytes(src);
        return true;
    }

    /// Copies up to `min(dst.len, len())` unread bytes into `dst` without advancing the read cursor.
    /// Returns the number of bytes copied.
    fn peekBytes(dst: *[]u8): usize {
        if (dst.len == 0) return 0;
        const used: usize = _.len();
        if (used == 0) return 0;

        const to_peek: usize = if (dst.len < used) dst.len else used;
        const r_pos: usize = _.head & _.mask;
        const tail_avail: usize = _.buf.len - r_pos;
        const first: usize = if (to_peek < tail_avail) to_peek else tail_avail;

        let i: usize = 0;
        while (i < first) : (i += 1) {
            dst[i] = _.buf[r_pos + i];
        }
        const second: usize = to_peek - first;
        let j: usize = 0;
        while (j < second) : (j += 1) {
            dst[first + j] = _.buf[j];
        }
        return to_peek;
    }

    /// Reads up to `min(dst.len, len())` unread bytes into `dst`, advancing the read cursor.
    /// Returns the number of bytes read.
    fn readBytes(*_, dst: *[]u8): usize {
        const n: usize = _.peekBytes(dst);
        if (n > 0) {
            _.head = _.head +% n;
            _.total_read += n as u64;
        }
        return n;
    }

    /// Advances the read cursor by `min(count, len())` bytes without copying data.
    /// Returns the number of bytes skipped.
    fn skipBytes(*_, count: usize): usize {
        const used: usize = _.len();
        const n: usize = if (count < used) count else used;
        if (n > 0) {
            _.head = _.head +% n;
            _.total_read += n as u64;
        }
        return n;
    }

    /// Atomically enqueues a 4-byte little-endian length-prefixed frame (`[u32 LE len][payload]`).
    /// Returns `false` (and increments `dropped_writes`) if free space is less than `4 + payload.len`.
    fn writeFrame(*_, payload: []const u8): bool {
        if (payload.len > (std.math.maxInt(u32) as usize)) {
            _.dropped_writes += 1;
            return false;
        }
        if (payload.len > std.math.maxInt(usize) - 4 || (4 + payload.len) > _.available()) {
            _.dropped_writes += 1;
            return false;
        }
        let hdr: [4]u8 = [0; 4];
        encodeU32LE(payload.len as u32, &hdr);
        * = _.writeBytes(hdr[0..]);
        if (payload.len > 0) {
            * = _.writeBytes(payload);
        }
        return true;
    }

    /// Atomically dequeues one 4-byte little-endian length-prefixed frame into `out_buf`.
    /// Returns the payload byte length written into `out_buf`, or `null` (leaving the ring intact)
    /// if no complete frame is available or `out_buf.len < frame_len`.
    fn readFrame(*_, out_buf: *[]u8): usize? {
        const used: usize = _.len();
        if (used < 4) return null;

        let hdr: [4]u8 = [0; 4];
        let hdr_s: []u8 = hdr[0..];
        const peeked: usize = _.peekBytes(hdr_s);
        if (peeked < 4) return null;

        const frame_len: usize = decodeU32LE(hdr) as usize;
        if (frame_len > used - 4) return null;
        if (out_buf.len < frame_len) return null;

        * = _.skipBytes(4);
        if (frame_len > 0) {
            let target_s: []u8 = out_buf[0..frame_len];
            * = _.readBytes(target_s);
        }
        return frame_len;
    }

    /// Returns a point-in-time and cumulative telemetry snapshot.
    fn stats(): ByteRingStats {
        const used: usize = _.tail -% _.head;
        return ByteRingStats{
            capacity: _.buf.len,
            used: used,
            free: _.buf.len - used,
            high_watermark: _.high_watermark,
            total_written: _.total_written,
            total_read: _.total_read,
            dropped_writes: _.dropped_writes,
        };
    }
}
