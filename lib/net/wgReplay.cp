// Source: cp/lib/wireguard/replay.cp (TAI64N timestamp & RFC 6479 2048-bit replay window)

const std = @import("std");

pub enum WgError {
    InvalidLength,
    InvalidMessageType,
    InvalidMac1,
    InvalidMac2,
    ReplayDetected,
    WindowOutOfRange,
    StaleTimestamp,
    DecryptFailed,
    InvalidKey,
    InvalidState,
}

const TAI64_EPOCH_OFFSET: i128 = 0x400000000000000a;
const WINDOW_WORDS: usize = 32;
const WINDOW_LIMIT: u64 = (WINDOW_WORDS - 1) * 64;

pub struct Tai64n {
    pub sec: u64,
    pub nsec: u32,

    pub static fn fromUnixNs(unix_sec: i64, unix_nsec: u32) -> Tai64n {
        const s: u64 = @intCast(TAI64_EPOCH_OFFSET + @as(i128, unix_sec));
        return Tai64n {
            .sec = s,
            .nsec = unix_nsec,
        };
    }

    pub fn encode() -> [12]u8 {
        let out: [12]u8 = undefined;
        std.mem.writeInt(u64, out[0..8], _.sec, .big);
        std.mem.writeInt(u32, out[8..12], _.nsec, .big);
        return out;
    }

    pub static fn decode(data: []const u8) -> !Tai64n {
        if (data.len < 12) {
            return WgError.InvalidLength;
        }
        const s = std.mem.readInt(u64, data[0..8], .big);
        const ns = std.mem.readInt(u32, data[8..12], .big);
        return Tai64n {
            .sec = s,
            .nsec = ns,
        };
    }

    pub fn compare(other: Tai64n) -> i32 {
        if (_.sec > other.sec) {
            return 1;
        }
        if (_.sec < other.sec) {
            return -1;
        }
        if (_.nsec > other.nsec) {
            return 1;
        }
        if (_.nsec < other.nsec) {
            return -1;
        }
        return 0;
    }
}

pub struct ReplayWindow {
    pub last_counter: u64,
    pub bitmap: [32]u64,
    pub has_packet: bool,

    pub fn init() -> ReplayWindow {
        return ReplayWindow {
            .last_counter = 0,
            .bitmap = [0; 32],
            .has_packet = false,
        };
    }

    pub fn checkAndUpdate(*_, counter: u64) -> !void {
        if (!_.has_packet) {
            _.has_packet = true;
            _.last_counter = counter;
            const w_idx: usize = @intCast((counter >> 6) & 31);
            const b_idx: u6 = @intCast(counter & 63);
            _.bitmap[w_idx] = @as(u64, 1) << b_idx;
            return;
        }

        if (counter > _.last_counter) {
            const curr_word: u64 = counter >> 6;
            const last_word: u64 = _.last_counter >> 6;
            const diff_words: u64 = curr_word - last_word;

            if (diff_words >= WINDOW_WORDS) {
                let i: usize = 0;
                while (i < WINDOW_WORDS) {
                    _.bitmap[i] = 0;
                    i += 1;
                }
            } else if (diff_words > 0) {
                let step: u64 = 1;
                while (step <= diff_words) {
                    const clear_idx: usize = @intCast((last_word + step) & 31);
                    _.bitmap[clear_idx] = 0;
                    step += 1;
                }
            }

            const w_idx: usize = @intCast(curr_word & 31);
            const b_idx: u6 = @intCast(counter & 63);
            _.bitmap[w_idx] |= (@as(u64, 1) << b_idx);
            _.last_counter = counter;
            return;
        }

        const diff: u64 = _.last_counter - counter;
        if (diff >= WINDOW_LIMIT) {
            return WgError.WindowOutOfRange;
        }

        const w_idx: usize = @intCast((counter >> 6) & 31);
        const b_idx: u6 = @intCast(counter & 63);
        const mask: u64 = @as(u64, 1) << b_idx;
        if ((_.bitmap[w_idx] & mask) != 0) {
            return WgError.ReplayDetected;
        }
        _.bitmap[w_idx] |= mask;
    }
}
