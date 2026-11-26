// @cap/cidr-tools — Zero-allocation IPv4 CIDR parser, formatter, merge & exclude engine for CP
// Clean-room MIT implementation inspired by npm `cidr-tools` and `ip-cidr`.

const MAX_MERGE_RANGES: usize = 128;

pub struct Cidr4 {
    pub network: u32,
    pub prefix: u8,
    pub valid: bool,
}

fn invalidCidr() -> Cidr4 {
    return Cidr4{
        network: 0 as u32,
        prefix: 0 as u8,
        valid: false,
    };
}

fn pow2_64(n: u8) -> u64 {
    let v: u64 = 1;
    let i: u8 = 0;
    while (i < n) {
        v = v * (2 as u64);
        i += 1 as u8;
    }
    return v;
}

fn prefixMask(prefix: u8) -> u32 {
    if (prefix == (0 as u8)) {
        return 0 as u32;
    }
    if (prefix >= (32 as u8)) {
        return 0xFFFFFFFF as u32;
    }
    const hostBits = (32 as u8) - prefix;
    const size = pow2_64(hostBits);
    return ((0x100000000 as u64) - size) as u32;
}

pub fn parseIpv4(s: []const u8) -> i64 {
    if (s.len < (7 as usize) || s.len > (15 as usize)) {
        return -1 as i64;
    }
    let ip: u64 = 0;
    let partCount: usize = 0;
    let i: usize = 0;

    while (i < s.len) {
        if (partCount >= (4 as usize)) {
            return -1 as i64;
        }
        const segStart = i;
        let val: u32 = 0;
        while (i < s.len && s[i] != '.') {
            const c = s[i];
            if (c < '0' || c > '9') {
                return -1 as i64;
            }
            // Reject leading zero on multi-digit octet
            if (i == segStart + (1 as usize) && s[segStart] == '0') {
                return -1 as i64;
            }
            val = (val * (10 as u32)) + ((c - '0') as u32);
            if (val > (255 as u32)) {
                return -1 as i64;
            }
            i += 1;
        }
        if (i == segStart) {
            return -1 as i64;
        }
        ip = (ip * (256 as u64)) | (val as u64);
        partCount += 1;

        if (i < s.len) {
            // Must be '.' and not trailing
            i += 1;
            if (i >= s.len) {
                return -1 as i64;
            }
        }
    }

    if (partCount != (4 as usize)) {
        return -1 as i64;
    }
    return ip as i64;
}

pub fn parseCidr4(s: []const u8) -> Cidr4 {
    if (s.len == (0 as usize)) {
        return invalidCidr();
    }
    let slashIdx: usize = s.len;
    let hasSlash = false;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') {
            if (hasSlash) {
                return invalidCidr();
            }
            slashIdx = i;
            hasSlash = true;
        }
        i += 1;
    }

    const ipParsed = parseIpv4(s[0..slashIdx]);
    if (ipParsed < (0 as i64)) {
        return invalidCidr();
    }

    let prefix: u8 = 32;
    if (hasSlash) {
        const pStart = slashIdx + (1 as usize);
        if (pStart >= s.len || (s.len - pStart) > (2 as usize)) {
            return invalidCidr();
        }
        if ((s.len - pStart) == (2 as usize) && s[pStart] == '0') {
            return invalidCidr();
        }
        let pVal: u32 = 0;
        let k = pStart;
        while (k < s.len) {
            const c = s[k];
            if (c < '0' || c > '9') {
                return invalidCidr();
            }
            pVal = (pVal * (10 as u32)) + ((c - '0') as u32);
            k += 1;
        }
        if (pVal > (32 as u32)) {
            return invalidCidr();
        }
        prefix = pVal as u8;
    }

    const rawIp = ipParsed as u32;
    const net = rawIp & prefixMask(prefix);
    return Cidr4{
        network: net,
        prefix: prefix,
        valid: true,
    };
}

fn appendU8Dec(dst: []u8, pos: usize, v_in: u8) -> usize {
    let tmp: [4]u8 = undefined;
    let n: usize = 0;
    let v = v_in;
    if (v == (0 as u8)) {
        tmp[0] = '0';
        n = 1 as usize;
    } else {
        while (v > (0 as u8)) {
            tmp[n] = (v % (10 as u8)) + '0';
            v = v / (10 as u8);
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > (0 as usize) && p < dst.len) {
        k -= 1;
        dst[p] = tmp[k];
        p += 1;
    }
    return p;
}

pub fn formatCidr4(c: Cidr4, dst: []u8) -> usize {
    if (!c.valid || c.prefix > (32 as u8)) {
        return 0 as usize;
    }
    const net = c.network & prefixMask(c.prefix);
    const o0 = ((net / (16777216 as u32)) & (0xFF as u32)) as u8;
    const o1 = ((net / (65536 as u32)) & (0xFF as u32)) as u8;
    const o2 = ((net / (256 as u32)) & (0xFF as u32)) as u8;
    const o3 = (net & (0xFF as u32)) as u8;

    let pos: usize = 0;
    pos = appendU8Dec(dst, pos, o0);
    if (pos < dst.len) { dst[pos] = '.'; pos += 1; }
    pos = appendU8Dec(dst, pos, o1);
    if (pos < dst.len) { dst[pos] = '.'; pos += 1; }
    pos = appendU8Dec(dst, pos, o2);
    if (pos < dst.len) { dst[pos] = '.'; pos += 1; }
    pos = appendU8Dec(dst, pos, o3);
    if (pos < dst.len) { dst[pos] = '/'; pos += 1; }
    pos = appendU8Dec(dst, pos, c.prefix);
    return pos;
}

pub fn cidrHostCount(c: Cidr4) -> u64 {
    if (!c.valid || c.prefix > (32 as u8)) {
        return 0 as u64;
    }
    return pow2_64((32 as u8) - c.prefix);
}

pub fn cidrStart(c: Cidr4) -> u32 {
    if (!c.valid || c.prefix > (32 as u8)) {
        return 0 as u32;
    }
    return c.network & prefixMask(c.prefix);
}

pub fn cidrEnd(c: Cidr4) -> u32 {
    if (!c.valid || c.prefix > (32 as u8)) {
        return 0 as u32;
    }
    const start64 = cidrStart(c) as u64;
    const count = cidrHostCount(c);
    return (start64 + count - (1 as u64)) as u32;
}

pub fn containsIp(c: Cidr4, ip: u32) -> bool {
    if (!c.valid || c.prefix > (32 as u8)) {
        return false;
    }
    const s = cidrStart(c);
    const e = cidrEnd(c);
    return ip >= s && ip <= e;
}

pub fn containsCidr(outer: Cidr4, inner: Cidr4) -> bool {
    if (!outer.valid || !inner.valid || outer.prefix > (32 as u8) || inner.prefix > (32 as u8)) {
        return false;
    }
    return cidrStart(inner) >= cidrStart(outer) && cidrEnd(inner) <= cidrEnd(outer);
}

pub fn overlaps(a: Cidr4, b: Cidr4) -> bool {
    if (!a.valid || !b.valid || a.prefix > (32 as u8) || b.prefix > (32 as u8)) {
        return false;
    }
    return cidrStart(a) <= cidrEnd(b) && cidrStart(b) <= cidrEnd(a);
}

struct Range64 {
    lo: u64,
    hi: u64,
}

fn emitRangeCidrs(lo_in: u64, hi: u64, out: []Cidr4, pos_in: usize) -> usize {
    let lo = lo_in;
    let pos = pos_in;
    while (lo <= hi) {
        let chosenPrefix: u8 = 32;
        let chosenSize: u64 = 1;
        let p: u8 = 0;
        while (p <= (32 as u8)) {
            const sz = pow2_64((32 as u8) - p);
            if ((lo % sz) == (0 as u64) && (lo + sz - (1 as u64)) <= hi) {
                chosenPrefix = p;
                chosenSize = sz;
                break;
            }
            p += 1 as u8;
        }
        if (pos < out.len) {
            out[pos] = Cidr4{
                network: lo as u32,
                prefix: chosenPrefix,
                valid: true,
            };
        }
        pos += 1;
        lo += chosenSize;
    }
    return pos;
}

pub fn mergeCidrs(inputs: []const Cidr4, out: []Cidr4) -> usize {
    let ranges: [MAX_MERGE_RANGES]Range64 = undefined;
    let rCount: usize = 0;
    let i: usize = 0;
    while (i < inputs.len && rCount < MAX_MERGE_RANGES) {
        const c = inputs[i];
        if (c.valid && c.prefix <= (32 as u8)) {
            ranges[rCount] = Range64{
                lo: cidrStart(c) as u64,
                hi: cidrEnd(c) as u64,
            };
            rCount += 1;
        }
        i += 1;
    }
    if (rCount == (0 as usize)) {
        return 0 as usize;
    }

    // Insertion sort by lo ascending
    let k: usize = 1;
    while (k < rCount) {
        const key = ranges[k];
        let j = k;
        while (j > (0 as usize) && ranges[j - (1 as usize)].lo > key.lo) {
            ranges[j] = ranges[j - (1 as usize)];
            j -= 1;
        }
        ranges[j] = key;
        k += 1;
    }

    // Merge overlapping and adjacent ranges, then emit minimal canonical CIDRs
    let outPos: usize = 0;
    let curLo = ranges[0].lo;
    let curHi = ranges[0].hi;
    let m: usize = 1;
    while (m < rCount) {
        if (ranges[m].lo <= (curHi + (1 as u64))) {
            if (ranges[m].hi > curHi) {
                curHi = ranges[m].hi;
            }
        } else {
            outPos = emitRangeCidrs(curLo, curHi, out, outPos);
            curLo = ranges[m].lo;
            curHi = ranges[m].hi;
        }
        m += 1;
    }
    outPos = emitRangeCidrs(curLo, curHi, out, outPos);
    return outPos;
}

pub fn excludeCidr(base: Cidr4, remove: Cidr4, out: []Cidr4) -> usize {
    if (!base.valid || base.prefix > (32 as u8)) {
        return 0 as usize;
    }
    if (!remove.valid || !overlaps(base, remove)) {
        if (out.len > (0 as usize)) {
            out[0] = Cidr4{
                network: cidrStart(base),
                prefix: base.prefix,
                valid: true,
            };
        }
        return 1 as usize;
    }

    const bLo = cidrStart(base) as u64;
    const bHi = cidrEnd(base) as u64;
    const rLo = cidrStart(remove) as u64;
    const rHi = cidrEnd(remove) as u64;

    let pos: usize = 0;
    if (rLo > bLo) {
        pos = emitRangeCidrs(bLo, rLo - (1 as u64), out, pos);
    }
    if (rHi < bHi) {
        pos = emitRangeCidrs(rHi + (1 as u64), bHi, out, pos);
    }
    return pos;
}

export {
    Cidr4,
    parseIpv4,
    parseCidr4,
    formatCidr4,
    cidrStart,
    cidrEnd,
    cidrHostCount,
    containsIp,
    containsCidr,
    overlaps,
    mergeCidrs,
    excludeCidr,
};
