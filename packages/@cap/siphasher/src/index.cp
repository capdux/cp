fn sipU64(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn sipAdd64(a: u64, b: u64): u64 {
    let lo: u64 = (a & 4294967295) + (b & 4294967295);
    let carry: u64 = lo >> 32;
    let hi: u64 = ((a >> 32) + (b >> 32) + carry) & 4294967295;
    return (hi << 32) | (lo & 4294967295);
}

fn sipRotl64(x: u64, r: u64): u64 {
    return (x << r) | (x >> (64 - r));
}

fn sipLoadLe64(buf: []u8, off: usize): u64 {
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < 8) {
        acc = acc | ((buf[off + i] as u64) << ((i * 8) as u64));
        i = i + 1;
    }
    return acc;
}

struct SipState {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
}

fn sipRound(s: *SipState) {
    s.v0 = sipAdd64(s.v0, s.v1);
    s.v1 = sipRotl64(s.v1, 13);
    s.v1 = s.v1 ^ s.v0;
    s.v0 = sipRotl64(s.v0, 32);

    s.v2 = sipAdd64(s.v2, s.v3);
    s.v3 = sipRotl64(s.v3, 16);
    s.v3 = s.v3 ^ s.v2;

    s.v0 = sipAdd64(s.v0, s.v3);
    s.v3 = sipRotl64(s.v3, 21);
    s.v3 = s.v3 ^ s.v0;

    s.v2 = sipAdd64(s.v2, s.v1);
    s.v1 = sipRotl64(s.v1, 17);
    s.v1 = s.v1 ^ s.v2;
    s.v2 = sipRotl64(s.v2, 32);
}

fn sipHashCore(k0: u64, k1: u64, data: []u8, c_rounds: usize, d_rounds: usize): u64 {
    let st = SipState {
        v0: sipU64(1936682341, 1886610805) ^ k0, // 0x736f6d6570736575
        v1: sipU64(1685025377, 1852075885) ^ k1, // 0x646f72616e646f6d
        v2: sipU64(1819895653, 1852142177) ^ k0, // 0x6c7967656e657261
        v3: sipU64(1952801890, 2037671283) ^ k1, // 0x7465646279746573
    };

    let full_blocks = data.len / 8;
    let i: usize = 0;
    while (i < full_blocks) {
        let m = sipLoadLe64(data, i * 8);
        st.v3 = st.v3 ^ m;
        let r: usize = 0;
        while (r < c_rounds) { sipRound(*st); r = r + 1; }
        st.v0 = st.v0 ^ m;
        i = i + 1;
    }

    let rem_off = full_blocks * 8;
    let rem = data.len - rem_off;
    let b: u64 = ((data.len as u64) & 255) << 56;
    let j: usize = 0;
    while (j < rem) {
        b = b | ((data[rem_off + j] as u64) << ((j * 8) as u64));
        j = j + 1;
    }

    st.v3 = st.v3 ^ b;
    let r2: usize = 0;
    while (r2 < c_rounds) { sipRound(*st); r2 = r2 + 1; }
    st.v0 = st.v0 ^ b;

    st.v2 = st.v2 ^ 255;
    let r3: usize = 0;
    while (r3 < d_rounds) { sipRound(*st); r3 = r3 + 1; }
    return st.v0 ^ st.v1 ^ st.v2 ^ st.v3;
}

pub fn sipHash24(k0: u64, k1: u64, data: []u8): u64 {
    return sipHashCore(k0, k1, data, 2, 4);
}

pub fn sipHash13(k0: u64, k1: u64, data: []u8): u64 {
    return sipHashCore(k0, k1, data, 1, 3);
}

pub fn sipHash24Key16(key16: []u8, data: []u8): u64? {
    if (key16.len < 16) { return null; }
    let k0 = sipLoadLe64(key16, 0);
    let k1 = sipLoadLe64(key16, 8);
    return sipHash24(k0, k1, data);
}

pub fn sipHash24Str(k0: u64, k1: u64, s: string): u64 {
    let buf: [256]u8 = [0; 256];
    let n = if (s.len > 256) { 256 } else { s.len };
    let i: usize = 0;
    while (i < n) { buf[i] = s[i]; i = i + 1; }
    return sipHash24(k0, k1, buf[0..n]);
}
