// @cap/ssri — Zero-allocation W3C Subresource Integrity (SRI) parser, validator & matcher for CP
// Clean-room MIT implementation inspired by npm `ssri` (85M/wk).

const MAX_HASHES: usize = 16;

pub enum SriAlgo {
    Sha256,
    Sha384,
    Sha512,
    Blake3,
    Unknown,
}

pub struct SriHash {
    pub valid: bool,
    pub algo: SriAlgo,
    pub algo_name: string,
    pub digest: string,
    pub options: string,
}

pub struct SriIntegrity {
    pub hashes: [MAX_HASHES]SriHash,
    pub count: usize,

    pub fn get(idx: usize) -> SriHash? {
        if (idx >= _.count) {
            return null;
        }
        return _.hashes[idx];
    }

    pub fn isEmpty() -> bool {
        return _.count == 0;
    }
}

fn emptyHash() -> SriHash {
    return SriHash{
        valid: false,
        algo: SriAlgo.Unknown,
        algo_name: "",
        digest: "",
        options: "",
    };
}

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn strEqIgnoreCase(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isAlnum(c: u8) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn isValidAlgoName(name: string) -> bool {
    if (name.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < name.len) {
        if (!isAlnum(name[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseAlgo(name: string) -> SriAlgo {
    if (strEqIgnoreCase(name, "sha512")) { return SriAlgo.Sha512; }
    if (strEqIgnoreCase(name, "sha384")) { return SriAlgo.Sha384; }
    if (strEqIgnoreCase(name, "blake3")) { return SriAlgo.Blake3; }
    if (strEqIgnoreCase(name, "sha256")) { return SriAlgo.Sha256; }
    return SriAlgo.Unknown;
}

pub fn algoName(algo: SriAlgo) -> string {
    switch (algo) {
        SriAlgo.Sha256 => { return "sha256"; },
        SriAlgo.Sha384 => { return "sha384"; },
        SriAlgo.Sha512 => { return "sha512"; },
        SriAlgo.Blake3 => { return "blake3"; },
        SriAlgo.Unknown => { return ""; },
    }
}

pub fn algoStrength(algo: SriAlgo) -> u8 {
    switch (algo) {
        SriAlgo.Sha512 => { return 4 as u8; },
        SriAlgo.Sha384 => { return 3 as u8; },
        SriAlgo.Blake3 => { return 2 as u8; },
        SriAlgo.Sha256 => { return 1 as u8; },
        SriAlgo.Unknown => { return 0 as u8; },
    }
}

fn isBase64DataChar(c: u8) -> bool {
    if (c >= 'A' && c <= 'Z') { return true; }
    if (c >= 'a' && c <= 'z') { return true; }
    if (c >= '0' && c <= '9') { return true; }
    return c == '+' || c == '/' || c == '-' || c == '_';
}

fn isValidBase64(s: string) -> bool {
    if (s.len == 0) {
        return false;
    }
    if ((s.len % (4 as usize)) == (1 as usize)) {
        return false;
    }
    let padCount: usize = 0;
    let seenPad = false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '=') {
            seenPad = true;
            padCount += 1;
            if (padCount > 2) {
                return false;
            }
        } else {
            if (seenPad || !isBase64DataChar(c)) {
                return false;
            }
        }
        i += 1;
    }
    if (padCount > 0 && (s.len % (4 as usize)) != (0 as usize)) {
        return false;
    }
    return true;
}

fn isValidOptions(opts: string) -> bool {
    let i: usize = 0;
    while (i < opts.len) {
        const c = opts[i];
        if (c < 0x21 || c > 0x7E) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn constantTimeEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff = diff | (a[i] ^ b[i]);
        i += 1;
    }
    return diff == (0 as u8);
}

fn parseSingleToken(tok: string) -> SriHash {
    let dashIdx: usize = 0;
    let foundDash = false;
    let i: usize = 0;
    while (i < tok.len) {
        if (tok[i] == '-') {
            dashIdx = i;
            foundDash = true;
            break;
        }
        i += 1;
    }
    if (!foundDash || dashIdx == 0 || dashIdx + 1 >= tok.len) {
        return emptyHash();
    }

    const rawAlgo = tok[0..dashIdx];
    if (!isValidAlgoName(rawAlgo)) {
        return emptyHash();
    }

    const rest = tok[(dashIdx + 1)..tok.len];
    let qIdx: usize = rest.len;
    let hasQ = false;
    let j: usize = 0;
    while (j < rest.len) {
        if (rest[j] == '?') {
            qIdx = j;
            hasQ = true;
            break;
        }
        j += 1;
    }

    const digest = rest[0..qIdx];
    const opts = if (hasQ) { rest[(qIdx + 1)..rest.len] } else { "" };

    if (!isValidBase64(digest)) {
        return emptyHash();
    }
    if (hasQ && (opts.len == 0 || !isValidOptions(opts))) {
        return emptyHash();
    }

    const algo = parseAlgo(rawAlgo);
    const canonicalName = if (algo != SriAlgo.Unknown) { algoName(algo) } else { rawAlgo };
    return SriHash{
        valid: true,
        algo: algo,
        algo_name: canonicalName,
        digest: digest,
        options: opts,
    };
}

pub fn parseIntegrity(raw: string) -> SriIntegrity {
    let arr: [MAX_HASHES]SriHash = undefined;
    let k: usize = 0;
    while (k < MAX_HASHES) {
        arr[k] = emptyHash();
        k += 1;
    }

    let count: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        while (i < raw.len && isSpace(raw[i])) {
            i += 1;
        }
        if (i >= raw.len) {
            break;
        }
        const start = i;
        while (i < raw.len && !isSpace(raw[i])) {
            i += 1;
        }
        const tok = raw[start..i];
        const parsed = parseSingleToken(tok);
        if (parsed.valid && count < MAX_HASHES) {
            arr[count] = parsed;
            count += 1;
        }
    }

    return SriIntegrity{
        hashes: arr,
        count: count,
    };
}

pub fn strongestHash(sri: SriIntegrity) -> SriHash? {
    let bestIdx: usize = 0;
    let bestStrength: u8 = 0;
    let found = false;
    let i: usize = 0;
    while (i < sri.count) {
        const h = sri.hashes[i];
        if (h.valid) {
            const st = algoStrength(h.algo);
            if (st > bestStrength) {
                bestStrength = st;
                bestIdx = i;
                found = true;
            }
        }
        i += 1;
    }
    if (!found) {
        return null;
    }
    return sri.hashes[bestIdx];
}

pub fn pickAlgorithm(sri: SriIntegrity, algo: SriAlgo) -> SriHash? {
    let i: usize = 0;
    while (i < sri.count) {
        const h = sri.hashes[i];
        if (h.valid && h.algo == algo) {
            return h;
        }
        i += 1;
    }
    return null;
}

pub fn matchesIntegrity(expected: SriIntegrity, actual: SriIntegrity) -> bool {
    if (expected.count == 0 || actual.count == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < expected.count) {
        const e = expected.hashes[i];
        if (e.valid && e.algo != SriAlgo.Unknown) {
            let j: usize = 0;
            while (j < actual.count) {
                const a = actual.hashes[j];
                if (a.valid && a.algo == e.algo && constantTimeEq(e.digest, a.digest)) {
                    return true;
                }
                j += 1;
            }
        }
        i += 1;
    }
    return false;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

pub fn formatHash(algo: SriAlgo, base64_digest: string, out: []u8) -> string {
    if (algo == SriAlgo.Unknown || !isValidBase64(base64_digest)) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = appendStr(out, pos, algoName(algo));
    pos = appendByte(out, pos, '-');
    pos = appendStr(out, pos, base64_digest);
    return out[0..pos];
}

pub fn formatIntegrity(sri: SriIntegrity, out: []u8) -> string {
    let pos: usize = 0;
    let written: usize = 0;
    let i: usize = 0;
    while (i < sri.count) {
        const h = sri.hashes[i];
        if (h.valid && h.algo_name.len > 0 && h.digest.len > 0) {
            if (written > 0) {
                pos = appendByte(out, pos, ' ');
            }
            pos = appendStr(out, pos, h.algo_name);
            pos = appendByte(out, pos, '-');
            pos = appendStr(out, pos, h.digest);
            if (h.options.len > 0) {
                pos = appendByte(out, pos, '?');
                pos = appendStr(out, pos, h.options);
            }
            written += 1;
        }
        i += 1;
    }
    return out[0..pos];
}

export {
    SriAlgo,
    SriHash,
    SriIntegrity,
    algoName,
    algoStrength,
    parseIntegrity,
    strongestHash,
    pickAlgorithm,
    matchesIntegrity,
    formatHash,
    formatIntegrity,
};
