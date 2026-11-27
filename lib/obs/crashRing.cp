// Source: cp/lib/crash_handler/ring.cp (Zero-heap fixed crash writer & breadcrumb ring buffer)

const std = @import("std");
import { SignalKind, StackFrame, Breadcrumb, signalName, signalDescription } from "./crashTypes.cp";

fn isSensitiveKey(key: []const u8): bool {
    const patterns: [6][]const u8 = [ "KEY", "TOKEN", "SECRET", "PASS", "AUTH", "COOKIE" ];
    let pi: usize = 0;
    while (pi < patterns.len) {
        let pat = patterns[pi];
        if (key.len >= pat.len) {
            let i: usize = 0;
            while (i + pat.len <= key.len) {
                let matched = true;
                let j: usize = 0;
                while (j < pat.len) {
                    let c = key[i + j];
                    if (c >= 'a' && c <= 'z') {
                        c = c - 32;
                    }
                    if (c != pat[j]) {
                        matched = false;
                        break;
                    }
                    j += 1;
                }
                if (matched) {
                    return true;
                }
                i += 1;
            }
        }
        pi += 1;
    }
    return false;
}

pub fn FixedCrashWriter(comptime N: usize): type {
    return struct {
        const Self = @This();

        buf: [N]u8,
        pos: usize,
        truncated: bool,

        pub static fn init(): Self {
            return Self{
                .buf = std.mem.zeroes([N]u8),
                .pos = 0,
                .truncated = false,
            };
        }

        pub fn reset(*_) {
            _.pos = 0;
            _.truncated = false;
        }

        pub fn len(): usize {
            return _.pos;
        }

        pub fn capacity(): usize {
            return N;
        }

        pub fn isTruncated(): bool {
            return _.truncated;
        }

        pub fn slice(): []const u8 {
            return _.buf[0.._.pos];
        }

        pub fn writeByte(*_, b: u8) {
            if (_.pos < N) {
                _.buf[_.pos] = b;
                _.pos += 1;
            } else {
                _.truncated = true;
            }
        }

        pub fn writeBytes(*_, data: []const u8) {
            if (data.len == 0) {
                return;
            }
            let rem = N - _.pos;
            if (rem == 0) {
                _.truncated = true;
                return;
            }
            let copy_len = if (data.len <= rem) data.len else rem;
            let i: usize = 0;
            while (i < copy_len) {
                _.buf[_.pos + i] = data[i];
                i += 1;
            }
            _.pos += copy_len;
            if (copy_len < data.len) {
                _.truncated = true;
            }
        }

        pub fn writeHex(*_, val: usize) {
            _.writeBytes("0x");
            if (val == 0) {
                _.writeByte('0');
                return;
            }
            const hex_digits = "0123456789abcdef";
            let tmp: [32]u8 = undefined;
            let count: usize = 0;
            let v = val;
            while (v > 0) {
                tmp[count] = hex_digits[v & 0xf];
                v = v >> 4;
                count += 1;
            }
            while (count > 0) {
                count -= 1;
                _.writeByte(tmp[count]);
            }
        }

        pub fn writeUint(*_, val: u64) {
            if (val == 0) {
                _.writeByte('0');
                return;
            }
            let tmp: [32]u8 = undefined;
            let count: usize = 0;
            let v = val;
            while (v > 0) {
                let d: u8 = @intCast(v % 10);
                tmp[count] = @as(u8, 48) + d;
                v = v / 10;
                count += 1;
            }
            while (count > 0) {
                count -= 1;
                _.writeByte(tmp[count]);
            }
        }

        pub fn writeInt(*_, val: i64) {
            if (val < 0) {
                _.writeByte('-');
                let abs_val: u64 = @intCast(-val);
                _.writeUint(abs_val);
            } else {
                let pos_val: u64 = @intCast(val);
                _.writeUint(pos_val);
            }
        }

        pub fn writeSignalSummary(*_, sig: SignalKind, fault_addr: usize) {
            _.writeBytes(signalName(sig));
            _.writeBytes(": ");
            _.writeBytes(signalDescription(sig));
            if (fault_addr != 0) {
                _.writeBytes(" at ");
                _.writeHex(fault_addr);
            }
            _.writeByte('\n');
        }

        pub fn writeFrame(*_, index: usize, frame: StackFrame) {
            _.writeBytes("  #");
            _.writeUint(@intCast(index));
            _.writeBytes(" ");
            _.writeHex(frame.addr);
            if (frame.symbol.len > 0) {
                _.writeBytes(" in ");
                _.writeBytes(frame.symbol);
                if (frame.offset > 0) {
                    _.writeBytes("+");
                    _.writeHex(frame.offset);
                }
            }
            if (frame.file.len > 0) {
                _.writeBytes(" (");
                _.writeBytes(frame.file);
                if (frame.line > 0) {
                    _.writeByte(':');
                    _.writeUint(@intCast(frame.line));
                    if (frame.col > 0) {
                        _.writeByte(':');
                        _.writeUint(@intCast(frame.col));
                    }
                }
                _.writeByte(')');
            }
            _.writeByte('\n');
        }

        pub fn writeRedactedEnv(*_, key: []const u8, value: []const u8) {
            _.writeBytes(key);
            _.writeBytes("=");
            if (isSensitiveKey(key)) {
                _.writeBytes("[REDACTED]");
            } else {
                _.writeBytes(value);
            }
            _.writeByte('\n');
        }
    };
}

pub fn BreadcrumbIter(comptime CAP: usize): type {
    return struct {
        const Self = @This();

        ring: *const BreadcrumbRing(CAP),
        index: usize,

        pub fn next(*_): Breadcrumb? {
            if (_.index >= _.ring.count) {
                return null;
            }
            let item = _.ring.getFromOldest(_.index);
            _.index += 1;
            return item;
        }
    };
}

pub fn BreadcrumbRing(comptime CAP: usize): type {
    return struct {
        const Self = @This();

        items: [CAP]Breadcrumb,
        head: usize,
        count: usize,
        total_pushed: u64,

        pub static fn init(): Self {
            return Self{
                .items = std.mem.zeroes([CAP]Breadcrumb),
                .head = 0,
                .count = 0,
                .total_pushed = 0,
            };
        }

        pub fn push(*_, item: Breadcrumb) {
            if (CAP == 0) {
                return;
            }
            _.items[_.head] = item;
            _.head = (_.head + 1) % CAP;
            if (_.count < CAP) {
                _.count += 1;
            }
            _.total_pushed += 1;
        }

        pub fn len(): usize {
            return _.count;
        }

        pub fn capacity(): usize {
            return CAP;
        }

        pub fn clear(*_) {
            _.head = 0;
            _.count = 0;
        }

        pub fn getFromOldest(idx: usize): Breadcrumb? {
            if (idx >= _.count || CAP == 0) {
                return null;
            }
            let start = if (_.count < CAP) 0 else _.head;
            let pos = (start + idx) % CAP;
            return _.items[pos];
        }

        pub fn iterFromOldest(): BreadcrumbIter(CAP) {
            const Iter = BreadcrumbIter(CAP);
            return Iter{
                .ring = _,
                .index = 0,
            };
        }
    };
}
