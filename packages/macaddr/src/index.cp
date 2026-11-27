// @cap/macaddr — Zero-heap pure-CP IEEE 802 MAC-48/EUI-48 & EUI-64 address engine
// Clean-room MIT equivalent of crates.io `macaddr`.

import { hexVal } from "cap:ascii";
import { writeHexByte } from "cap:fmt";

fn macParseHexByte(hi_ch: u8, lo_ch: u8): u8? {
    let hi_opt = hexVal(hi_ch);
    if (hi_opt == null) {
        return null;
    }
    let lo_opt = hexVal(lo_ch);
    if (lo_opt == null) {
        return null;
    }
    let hi: u8 = hi_opt ?? 0;
    let lo: u8 = lo_opt ?? 0;
    return (hi << 4) | lo;
}

pub struct MacAddr6 {
    pub bytes: [6]u8,

    pub static fn new(b0: u8, b1: u8, b2: u8, b3: u8, b4: u8, b5: u8): MacAddr6 {
        let arr: [6]u8 = [b0, b1, b2, b3, b4, b5];
        return MacAddr6 { bytes: arr };
    }

    pub static fn fromArray(arr: [6]u8): MacAddr6 {
        return MacAddr6 { bytes: arr };
    }

    pub static fn nil(): MacAddr6 {
        let arr: [6]u8 = [0; 6];
        return MacAddr6 { bytes: arr };
    }

    pub static fn broadcast(): MacAddr6 {
        let arr: [6]u8 = [255; 6];
        return MacAddr6 { bytes: arr };
    }

    pub static fn parse(s: string): MacAddr6? {
        let out: [6]u8 = undefined;
        if (s.len == 17) {
            let sep: u8 = s[2];
            if (sep != 58 && sep != 45) { // ':' or '-'
                return null;
            }
            let i: usize = 0;
            while (i < 6) {
                let off: usize = i * 3;
                if (i < 5 && s[off + 2] != sep) {
                    return null;
                }
                let b_opt = macParseHexByte(s[off], s[off + 1]);
                if (b_opt == null) {
                    return null;
                }
                out[i] = b_opt ?? 0;
                i = i + 1;
            }
            return MacAddr6 { bytes: out };
        }
        if (s.len == 14) {
            // Cisco dot notation: xxxx.xxxx.xxxx
            if (s[4] != 46 || s[9] != 46) {
                return null;
            }
            let g: usize = 0;
            while (g < 3) {
                let off: usize = g * 5;
                let b0_opt = macParseHexByte(s[off], s[off + 1]);
                let b1_opt = macParseHexByte(s[off + 2], s[off + 3]);
                if (b0_opt == null || b1_opt == null) {
                    return null;
                }
                out[g * 2] = b0_opt ?? 0;
                out[g * 2 + 1] = b1_opt ?? 0;
                g = g + 1;
            }
            return MacAddr6 { bytes: out };
        }
        return null;
    }

    pub fn isNil(): bool {
        let i: usize = 0;
        while (i < 6) {
            if (_.bytes[i] != 0) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn isBroadcast(): bool {
        let i: usize = 0;
        while (i < 6) {
            if (_.bytes[i] != 255) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn isMulticast(): bool {
        return (_.bytes[0] & 1) != 0;
    }

    pub fn isUnicast(): bool {
        return (_.bytes[0] & 1) == 0;
    }

    pub fn isLocal(): bool {
        return (_.bytes[0] & 2) != 0;
    }

    pub fn isUniversal(): bool {
        return (_.bytes[0] & 2) == 0;
    }

    pub fn oui(): [3]u8 {
        let prefix: [3]u8 = [_.bytes[0], _.bytes[1], _.bytes[2]];
        return prefix;
    }

    pub fn formatColonInto(out: []u8): usize {
        let p: usize = 0;
        let i: usize = 0;
        while (i < 6) {
            if (i > 0 && p < out.len) {
                out[p] = 58; // ':'
                p = p + 1;
            }
            p = writeHexByte(_.bytes[i], false, out, p);
            i = i + 1;
        }
        return p;
    }

    pub fn formatHyphenInto(out: []u8): usize {
        let p: usize = 0;
        let i: usize = 0;
        while (i < 6) {
            if (i > 0 && p < out.len) {
                out[p] = 45; // '-'
                p = p + 1;
            }
            p = writeHexByte(_.bytes[i], false, out, p);
            i = i + 1;
        }
        return p;
    }

    pub fn formatDotInto(out: []u8): usize {
        let p: usize = 0;
        let g: usize = 0;
        while (g < 3) {
            if (g > 0 && p < out.len) {
                out[p] = 46; // '.'
                p = p + 1;
            }
            p = writeHexByte(_.bytes[g * 2], false, out, p);
            p = writeHexByte(_.bytes[g * 2 + 1], false, out, p);
            g = g + 1;
        }
        return p;
    }

    pub fn equals(other: MacAddr6): bool {
        let i: usize = 0;
        while (i < 6) {
            if (_.bytes[i] != other.bytes[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }
}

pub struct MacAddr8 {
    pub bytes: [8]u8,

    pub static fn fromArray(arr: [8]u8): MacAddr8 {
        return MacAddr8 { bytes: arr };
    }

    pub static fn nil(): MacAddr8 {
        let arr: [8]u8 = [0; 8];
        return MacAddr8 { bytes: arr };
    }

    pub static fn broadcast(): MacAddr8 {
        let arr: [8]u8 = [255; 8];
        return MacAddr8 { bytes: arr };
    }

    pub static fn fromMac6Eui64(mac6: MacAddr6): MacAddr8 {
        let arr: [8]u8 = [
            mac6.bytes[0] ^ 2,
            mac6.bytes[1],
            mac6.bytes[2],
            255,
            254,
            mac6.bytes[3],
            mac6.bytes[4],
            mac6.bytes[5]
        ];
        return MacAddr8 { bytes: arr };
    }

    pub static fn parse(s: string): MacAddr8? {
        let out: [8]u8 = undefined;
        if (s.len == 23) {
            let sep: u8 = s[2];
            if (sep != 58 && sep != 45) { // ':' or '-'
                return null;
            }
            let i: usize = 0;
            while (i < 8) {
                let off: usize = i * 3;
                if (i < 7 && s[off + 2] != sep) {
                    return null;
                }
                let b_opt = macParseHexByte(s[off], s[off + 1]);
                if (b_opt == null) {
                    return null;
                }
                out[i] = b_opt ?? 0;
                i = i + 1;
            }
            return MacAddr8 { bytes: out };
        }
        if (s.len == 19) {
            // Cisco dot notation: xxxx.xxxx.xxxx.xxxx
            if (s[4] != 46 || s[9] != 46 || s[14] != 46) {
                return null;
            }
            let g: usize = 0;
            while (g < 4) {
                let off: usize = g * 5;
                let b0_opt = macParseHexByte(s[off], s[off + 1]);
                let b1_opt = macParseHexByte(s[off + 2], s[off + 3]);
                if (b0_opt == null || b1_opt == null) {
                    return null;
                }
                out[g * 2] = b0_opt ?? 0;
                out[g * 2 + 1] = b1_opt ?? 0;
                g = g + 1;
            }
            return MacAddr8 { bytes: out };
        }
        return null;
    }

    pub fn isNil(): bool {
        let i: usize = 0;
        while (i < 8) {
            if (_.bytes[i] != 0) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn isBroadcast(): bool {
        let i: usize = 0;
        while (i < 8) {
            if (_.bytes[i] != 255) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    pub fn isMulticast(): bool {
        return (_.bytes[0] & 1) != 0;
    }

    pub fn isUnicast(): bool {
        return (_.bytes[0] & 1) == 0;
    }

    pub fn isLocal(): bool {
        return (_.bytes[0] & 2) != 0;
    }

    pub fn isUniversal(): bool {
        return (_.bytes[0] & 2) == 0;
    }

    pub fn formatColonInto(out: []u8): usize {
        let p: usize = 0;
        let i: usize = 0;
        while (i < 8) {
            if (i > 0 && p < out.len) {
                out[p] = 58; // ':'
                p = p + 1;
            }
            p = writeHexByte(_.bytes[i], false, out, p);
            i = i + 1;
        }
        return p;
    }

    pub fn formatHyphenInto(out: []u8): usize {
        let p: usize = 0;
        let i: usize = 0;
        while (i < 8) {
            if (i > 0 && p < out.len) {
                out[p] = 45; // '-'
                p = p + 1;
            }
            p = writeHexByte(_.bytes[i], false, out, p);
            i = i + 1;
        }
        return p;
    }

    pub fn formatDotInto(out: []u8): usize {
        let p: usize = 0;
        let g: usize = 0;
        while (g < 4) {
            if (g > 0 && p < out.len) {
                out[p] = 46; // '.'
                p = p + 1;
            }
            p = writeHexByte(_.bytes[g * 2], false, out, p);
            p = writeHexByte(_.bytes[g * 2 + 1], false, out, p);
            g = g + 1;
        }
        return p;
    }

    pub fn equals(other: MacAddr8): bool {
        let i: usize = 0;
        while (i < 8) {
            if (_.bytes[i] != other.bytes[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }
}
