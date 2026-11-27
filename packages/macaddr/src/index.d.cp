// Type definitions for CP

import { hexVal } from "cap:ascii";
import { writeHexByte } from "cap:fmt";
export fn macParseHexByte(hi_ch: u8, lo_ch: u8): u8?;
export class MacAddr6 {
    bytes: [6]u8,
    fn new(b0: u8, b1: u8, b2: u8, b3: u8, b4: u8, b5: u8): MacAddr6;
    fn fromArray(arr: [6]u8): MacAddr6;
    fn nil(): MacAddr6;
    fn broadcast(): MacAddr6;
    fn parse(s: string): MacAddr6?;
    fn isNil(): bool;
    fn isBroadcast(): bool;
    fn isMulticast(): bool;
    fn isUnicast(): bool;
    fn isLocal(): bool;
    fn isUniversal(): bool;
    fn oui(): [3]u8;
    fn formatColonInto(out: []u8): usize;
    fn formatHyphenInto(out: []u8): usize;
    fn formatDotInto(out: []u8): usize;
    fn equals(other: MacAddr6): bool;
}

export class MacAddr8 {
    bytes: [8]u8,
    fn fromArray(arr: [8]u8): MacAddr8;
    fn nil(): MacAddr8;
    fn broadcast(): MacAddr8;
    fn fromMac6Eui64(mac6: MacAddr6): MacAddr8;
    fn parse(s: string): MacAddr8?;
    fn isNil(): bool;
    fn isBroadcast(): bool;
    fn isMulticast(): bool;
    fn isUnicast(): bool;
    fn isLocal(): bool;
    fn isUniversal(): bool;
    fn formatColonInto(out: []u8): usize;
    fn formatHyphenInto(out: []u8): usize;
    fn formatDotInto(out: []u8): usize;
    fn equals(other: MacAddr8): bool;
}

