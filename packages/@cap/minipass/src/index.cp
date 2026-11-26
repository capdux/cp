// packages/@cap/minipass/src/index.cp
// Clean-room zero-allocation ring-buffer byte stream for CP.
// Parity with npm:minipass (Isaac Schlueter).

pub const MPASS_CAP: usize = 4096;

fn mpassMin(a: usize, b: usize): usize {
    return if (a < b) a else b;
}

pub struct MinipassStream {
    buf: [4096]u8,
    head: usize,
    len: usize,
    flowing: bool,
    ended: bool,
    destroyed: bool,

    static fn new(): MinipassStream {
        let empty: [4096]u8 = undefined;
        let s = MinipassStream{
            buf: empty,
            head: 0,
            len: 0,
            flowing: false,
            ended: false,
            destroyed: false,
        };
        return s;
    }

    fn bufferedBytes(): usize {
        return _.len;
    }

    fn freeSpace(): usize {
        return MPASS_CAP - _.len;
    }

    fn isPaused(): bool {
        return !_.flowing;
    }

    fn isEnded(): bool {
        return _.ended;
    }

    fn isDrained(): bool {
        return _.ended && _.len == 0;
    }

    fn pause(*_) {
        _.flowing = false;
    }

    fn @"resume"(*_) {
        _.flowing = true;
    }

    fn end(*_) {
        _.ended = true;
    }

    fn destroy(*_) {
        _.destroyed = true;
        _.ended = true;
        _.head = 0;
        _.len = 0;
    }

    fn write(*_, data: []const u8): bool {
        if (_.ended || _.destroyed) {
            return false;
        }
        if (data.len > MPASS_CAP - _.len) {
            return false;
        }
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            const idx = (_.head + _.len + i) % MPASS_CAP;
            _.buf[idx] = data[i];
        }
        _.len += data.len;
        return true;
    }

    fn read(*_, out: []u8): usize {
        if (_.destroyed || _.len == 0 || out.len == 0) {
            return 0;
        }
        const n = mpassMin(out.len, _.len);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out[i] = _.buf[(_.head + i) % MPASS_CAP];
        }
        _.head = (_.head + n) % MPASS_CAP;
        _.len -= n;
        return n;
    }

    fn readExact(*_, n: usize, out: []u8): usize {
        if (_.destroyed || n == 0 || n > _.len || n > out.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out[i] = _.buf[(_.head + i) % MPASS_CAP];
        }
        _.head = (_.head + n) % MPASS_CAP;
        _.len -= n;
        return n;
    }

    fn peek(out: []u8): usize {
        if (_.destroyed || _.len == 0 || out.len == 0) {
            return 0;
        }
        const n = mpassMin(out.len, _.len);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out[i] = _.buf[(_.head + i) % MPASS_CAP];
        }
        return n;
    }

    fn pipeTo(*_, dest: *MinipassStream): usize {
        if (_.destroyed || dest.ended || dest.destroyed) {
            return 0;
        }
        const dest_free = MPASS_CAP - dest.len;
        const n = mpassMin(_.len, dest_free);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const b = _.buf[(_.head + i) % MPASS_CAP];
            dest.buf[(dest.head + dest.len + i) % MPASS_CAP] = b;
        }
        _.head = (_.head + n) % MPASS_CAP;
        _.len -= n;
        dest.len += n;
        if (_.ended && _.len == 0) {
            dest.ended = true;
        }
        return n;
    }
}
