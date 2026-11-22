const inet = struct {
    pub const LEN = 4;
    pub const v4 = struct {
        pub const ADDR_LEN = 4;
    };
};

pub union(enum) Addr {
    v4: [inet.LEN]u8,
    none: void,

    pub fn copyTo(buf: *[inet.LEN]u8): usize {
        switch (_) {
            .v4 => |src| {
                let i: usize = 0;
                while (i < inet.LEN) {
                    buf[i] = src[i];
                    i += 1;
                }
                return inet.LEN;
            },
            .none => return 0,
        }
    }

    pub fn copyNested(buf: *[inet.v4.ADDR_LEN]u8): usize {
        switch (_) {
            .v4 => |src| {
                buf[0] = src[0];
                buf[3] = src[3];
                return inet.v4.ADDR_LEN;
            },
            .none => return 0,
        }
    }

    pub fn copyWithExtra(buf: *[inet.LEN + 2]u8): usize {
        buf[0] = 9;
        buf[5] = 42;
        return inet.LEN + 2;
    }

    pub fn sumConst(buf: *const [inet.LEN]u8): u32 {
        return (buf[0] as u32) + (buf[1] as u32) + (buf[2] as u32) + (buf[3] as u32);
    }
}

fn fillTop(buf: *[inet.LEN]u8) {
    buf[0] = 127;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 1;
}

fn main() {
    let raw: [inet.LEN]u8 = undefined;
    fillTop(&raw);
    if (raw[0] != 127 || raw[3] != 1) {
        @panic("fillTop failed");
    }

    const addr = Addr { .v4 = raw };
    let out: [inet.LEN]u8 = undefined;
    const n = addr.copyTo(&out);
    if (n != 4 || out[0] != 127 || out[3] != 1) {
        @panic("copyTo failed");
    }

    let nested: [inet.v4.ADDR_LEN]u8 = undefined;
    const n2 = addr.copyNested(&nested);
    if (n2 != 4 || nested[0] != 127 || nested[3] != 1) {
        @panic("copyNested failed");
    }

    let extra: [inet.LEN + 2]u8 = undefined;
    const n3 = Addr.copyWithExtra(&extra);
    if (n3 != 6 || extra[0] != 9 || extra[5] != 42) {
        @panic("copyWithExtra failed");
    }

    const s = Addr.sumConst(&out);
    if (s != 128) {
        @panic("sumConst failed");
    }
}
