// @cap/smart-buffer — Zero-heap cursor-based binary buffer
// Clean-room CP implementation inspired by npm:smart-buffer (JoshGlazebrook/smart-buffer)

const SBUF_CAP: usize = 1024;

fn sbufMin(a: usize, b: usize): usize {
    if (a < b) {
        return a;
    }
    return b;
}

pub struct SmartBuffer {
    buf: [1024]u8,
    read_offset: usize,
    write_offset: usize,

    static fn new(): SmartBuffer {
        return SmartBuffer{
            buf: [0; 1024],
            read_offset: 0,
            write_offset: 0,
        };
    }

    static fn fromSlice(src: []const u8): SmartBuffer {
        let sb = SmartBuffer.new();
        const n = sbufMin(src.len, SBUF_CAP);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            sb.buf[i] = src[i];
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
        if (_.write_offset + 2 <= SBUF_CAP) {
            _.buf[_.write_offset] = u8((v >> 8) & 0xFF);
            _.buf[_.write_offset + 1] = u8(v & 0xFF);
            _.write_offset += 2;
        }
    }

    fn writeU16LE(*_, v: u16) {
        if (_.write_offset + 2 <= SBUF_CAP) {
            _.buf[_.write_offset] = u8(v & 0xFF);
            _.buf[_.write_offset + 1] = u8((v >> 8) & 0xFF);
            _.write_offset += 2;
        }
    }

    fn writeU32BE(*_, v: u32) {
        if (_.write_offset + 4 <= SBUF_CAP) {
            _.buf[_.write_offset] = u8((v >> 24) & 0xFF);
            _.buf[_.write_offset + 1] = u8((v >> 16) & 0xFF);
            _.buf[_.write_offset + 2] = u8((v >> 8) & 0xFF);
            _.buf[_.write_offset + 3] = u8(v & 0xFF);
            _.write_offset += 4;
        }
    }

    fn writeU32LE(*_, v: u32) {
        if (_.write_offset + 4 <= SBUF_CAP) {
            _.buf[_.write_offset] = u8(v & 0xFF);
            _.buf[_.write_offset + 1] = u8((v >> 8) & 0xFF);
            _.buf[_.write_offset + 2] = u8((v >> 16) & 0xFF);
            _.buf[_.write_offset + 3] = u8((v >> 24) & 0xFF);
            _.write_offset += 4;
        }
    }

    fn writeString(*_, s: string) {
        const avail = SBUF_CAP - _.write_offset;
        const n = sbufMin(s.len, avail);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            _.buf[_.write_offset + i] = s[i];
        }
        _.write_offset += n;
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
        if (_.read_offset + 2 <= _.write_offset) {
            const b0 = u16(_.buf[_.read_offset]);
            const b1 = u16(_.buf[_.read_offset + 1]);
            _.read_offset += 2;
            return (b0 << 8) | b1;
        }
        return 0;
    }

    fn readU16LE(*_): u16 {
        if (_.read_offset + 2 <= _.write_offset) {
            const b0 = u16(_.buf[_.read_offset]);
            const b1 = u16(_.buf[_.read_offset + 1]);
            _.read_offset += 2;
            return b0 | (b1 << 8);
        }
        return 0;
    }

    fn readU32BE(*_): u32 {
        if (_.read_offset + 4 <= _.write_offset) {
            const b0 = u32(_.buf[_.read_offset]);
            const b1 = u32(_.buf[_.read_offset + 1]);
            const b2 = u32(_.buf[_.read_offset + 2]);
            const b3 = u32(_.buf[_.read_offset + 3]);
            _.read_offset += 4;
            return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        }
        return 0;
    }

    fn readU32LE(*_): u32 {
        if (_.read_offset + 4 <= _.write_offset) {
            const b0 = u32(_.buf[_.read_offset]);
            const b1 = u32(_.buf[_.read_offset + 1]);
            const b2 = u32(_.buf[_.read_offset + 2]);
            const b3 = u32(_.buf[_.read_offset + 3]);
            _.read_offset += 4;
            return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
        }
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
        if (_.write_offset > _.read_offset) {
            return _.write_offset - _.read_offset;
        }
        return 0;
    }

    fn len(): usize {
        return _.write_offset;
    }

    fn rewind(*_) {
        _.read_offset = 0;
    }
}
