// @cap/smart-buffer — Zero-heap cursor-based binary buffer
// Clean-room CP implementation inspired by npm:smart-buffer (JoshGlazebrook/smart-buffer)

import {
    readU16BE,
    readU16LE,
    readU32BE,
    readU32LE,
    writeU16BE,
    writeU16LE,
    writeU32BE,
    writeU32LE,
} from "cap:bin";

const SBUF_CAP: usize = 1024;

fn sbufMin(a: usize, b: usize): usize {
    return if (a < b) a else b;
}

fn sbufW16BE(dst: []u8, off: usize, v: u16): bool { return writeU16BE(dst, off, v); }
fn sbufW16LE(dst: []u8, off: usize, v: u16): bool { return writeU16LE(dst, off, v); }
fn sbufW32BE(dst: []u8, off: usize, v: u32): bool { return writeU32BE(dst, off, v); }
fn sbufW32LE(dst: []u8, off: usize, v: u32): bool { return writeU32LE(dst, off, v); }
fn sbufR16BE(src: []const u8, off: usize): u16? { return readU16BE(src, off); }
fn sbufR16LE(src: []const u8, off: usize): u16? { return readU16LE(src, off); }
fn sbufR32BE(src: []const u8, off: usize): u32? { return readU32BE(src, off); }
fn sbufR32LE(src: []const u8, off: usize): u32? { return readU32LE(src, off); }

pub struct SmartBuffer {
    buf: [1024]u8,
    read_offset: usize,
    write_offset: usize,

    static fn new(): SmartBuffer {
        return SmartBuffer{
            buf: undefined,
            read_offset: 0,
            write_offset: 0,
        };
    }

    static fn fromSlice(src: []const u8): SmartBuffer {
        let sb = SmartBuffer.new();
        const n = sbufMin(src.len, SBUF_CAP);
        if (n > 0) {
            @memcpy(sb.buf[0..n], src[0..n]);
        }
        sb.write_offset = n;
        return sb;
    }

    fn writeU8(*_, v: u8) {
        if (_.write_offset < SBUF_CAP) {
            _.buf[_.write_offset] = v;
            _.write_offset += 1;
        }
    }

    fn writeU16BE(*_, v: u16) {
        if (sbufW16BE(_.buf[0..SBUF_CAP], _.write_offset, v)) { _.write_offset += 2; }
    }

    fn writeU16LE(*_, v: u16) {
        if (sbufW16LE(_.buf[0..SBUF_CAP], _.write_offset, v)) { _.write_offset += 2; }
    }

    fn writeU32BE(*_, v: u32) {
        if (sbufW32BE(_.buf[0..SBUF_CAP], _.write_offset, v)) { _.write_offset += 4; }
    }

    fn writeU32LE(*_, v: u32) {
        if (sbufW32LE(_.buf[0..SBUF_CAP], _.write_offset, v)) { _.write_offset += 4; }
    }

    fn writeString(*_, s: string) {
        const n = sbufMin(s.len, SBUF_CAP - _.write_offset);
        if (n > 0) {
            @memcpy(_.buf[_.write_offset .. _.write_offset + n], s[0..n]);
            _.write_offset += n;
        }
    }

    fn writeStringNT(*_, s: string) {
        _.writeString(s);
        _.writeU8(0);
    }

    fn readU8(*_): u8 {
        if (_.read_offset < _.write_offset) {
            const v = _.buf[_.read_offset];
            _.read_offset += 1;
            return v;
        }
        return 0;
    }

    fn readU16BE(*_): u16 {
        if (v := sbufR16BE(_.buf[0.._.write_offset], _.read_offset)) { _.read_offset += 2; return v; }
        return 0;
    }

    fn readU16LE(*_): u16 {
        if (v := sbufR16LE(_.buf[0.._.write_offset], _.read_offset)) { _.read_offset += 2; return v; }
        return 0;
    }

    fn readU32BE(*_): u32 {
        if (v := sbufR32BE(_.buf[0.._.write_offset], _.read_offset)) { _.read_offset += 4; return v; }
        return 0;
    }

    fn readU32LE(*_): u32 {
        if (v := sbufR32LE(_.buf[0.._.write_offset], _.read_offset)) { _.read_offset += 4; return v; }
        return 0;
    }

    fn readString(*_, n: usize): string {
        const avail = if (_.write_offset > _.read_offset) _.write_offset - _.read_offset else 0;
        const take = sbufMin(n, avail);
        const start = _.read_offset;
        _.read_offset += take;
        return _.buf[start..start + take] as string;
    }

    fn remaining(): usize {
        return if (_.write_offset > _.read_offset) _.write_offset - _.read_offset else 0;
    }

    fn len(): usize {
        return _.write_offset;
    }

    fn rewind(*_) {
        _.read_offset = 0;
    }
}
