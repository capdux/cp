// @cap/pem — Zero-heap RFC 7468 PEM Armor Parser & Encoder
// Clean-room MIT implementation in pure CP (replaces Rust `pem` [55M] & npm `pem-jwk`/`sshpk`).

pub struct PemBlock {
    pub valid: bool,
    pub label: string,
    pub raw_body: string,
    pub start_offset: usize,
    pub end_offset: usize,
}

pub struct PemBundle {
    pub blocks: [16]PemBlock,
    pub count: usize,
}

fn findSubstr(haystack: string, start: usize, needle: string) -> usize? {
    if (needle.len == 0) { return start; }
    if (start + needle.len > haystack.len) { return null; }
    let i: usize = start;
    while (i + needle.len <= haystack.len) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) { return i; }
        i += 1;
    }
    return null;
}

fn isValidLabel(label: string) -> bool {
    if (label.len == 0 || label.len > 64) { return false; }
    let i: usize = 0;
    while (i < label.len) {
        const c: u8 = label[i];
        const ok: bool = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-';
        if (!ok) { return false; }
        i += 1;
    }
    return true;
}

/// Parse the first RFC 7468 PEM block starting at or after `from_offset`.
pub fn parsePemFrom(input: string, from_offset: usize) -> PemBlock {
    const invalid: PemBlock = PemBlock {
        valid: false,
        label: "",
        raw_body: "",
        start_offset: 0,
        end_offset: 0,
    };

    const beginPrefix: string = "-----BEGIN ";
    const dashes: string = "-----";

    const bOpt = findSubstr(input, from_offset, beginPrefix);
    if (bOpt == null) { return invalid; }
    const bPos: usize = bOpt ?? (0 as usize);

    const labelStart: usize = bPos + beginPrefix.len;
    const bEndOpt = findSubstr(input, labelStart, dashes);
    if (bEndOpt == null) { return invalid; }
    const labelEnd: usize = bEndOpt ?? (0 as usize);
    const label: string = input[labelStart..labelEnd];
    if (!isValidLabel(label)) { return invalid; }

    // Advance past "-----" and optional CR/LF
    let bodyStart: usize = labelEnd + dashes.len;
    while (bodyStart < input.len && (input[bodyStart] == ' ' || input[bodyStart] == '\t' || input[bodyStart] == '\r')) {
        bodyStart += 1;
    }
    if (bodyStart < input.len && input[bodyStart] == '\n') {
        bodyStart += 1;
    }

    // Locate matching "-----END <label>-----"
    const endPrefix: string = "-----END ";
    let searchPos: usize = bodyStart;
    while (searchPos < input.len) {
        const eOpt = findSubstr(input, searchPos, endPrefix);
        if (eOpt == null) { return invalid; }
        const ePos: usize = eOpt ?? (0 as usize);
        const eLabelStart: usize = ePos + endPrefix.len;
        const eEndOpt = findSubstr(input, eLabelStart, dashes);
        if (eEndOpt == null) { return invalid; }
        const eLabelEnd: usize = eEndOpt ?? (0 as usize);
        const eLabel: string = input[eLabelStart..eLabelEnd];
        if (eLabel == label) {
            let bodyEnd: usize = ePos;
            while (bodyEnd > bodyStart && (input[bodyEnd - 1] == '\n' || input[bodyEnd - 1] == '\r')) {
                bodyEnd -= 1;
            }
            return PemBlock {
                valid: true,
                label: label,
                raw_body: input[bodyStart..bodyEnd],
                start_offset: bPos,
                end_offset: eLabelEnd + dashes.len,
            };
        }
        searchPos = eLabelEnd + dashes.len;
    }

    return invalid;
}

/// Parse the first PEM block in `input`.
pub fn parsePem(input: string) -> PemBlock {
    return parsePemFrom(input, 0);
}

/// Parse up to 16 PEM blocks from a certificate chain or key bundle.
pub fn parsePemBundle(input: string) -> PemBundle {
    let blocks: [16]PemBlock = undefined;
    let bundle: PemBundle = PemBundle {
        blocks: blocks,
        count: 0,
    };

    let cursor: usize = 0;
    while (cursor < input.len && bundle.count < 16) {
        const blk: PemBlock = parsePemFrom(input, cursor);
        if (!blk.valid) { break; }
        bundle.blocks[bundle.count] = blk;
        bundle.count += 1;
        cursor = blk.end_offset;
    }
    return bundle;
}

/// Strip whitespace and optional RFC 1421 `Key: Value` header lines from a PEM block's raw_body,
/// writing compact clean Base64 into `out`.
pub fn extractBase64(blk: PemBlock, out: []u8) -> string {
    if (!blk.valid) { return out[0..0]; }
    let pos: usize = 0;
    let i: usize = 0;
    const body: string = blk.raw_body;
    while (i < body.len) {
        // Read line
        let lineEnd: usize = i;
        while (lineEnd < body.len && body[lineEnd] != '\n') {
            lineEnd += 1;
        }
        const line: string = body[i..lineEnd];
        i = if (lineEnd < body.len) lineEnd + 1 else body.len;

        // Skip RFC 1421 header lines containing ':'
        let hasColon: bool = false;
        let k: usize = 0;
        while (k < line.len) {
            if (line[k] == ':') { hasColon = true; break; }
            k += 1;
        }
        if (hasColon) { continue; }

        k = 0;
        while (k < line.len) {
            const c: u8 = line[k];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                if (pos < out.len) {
                    out[pos] = c;
                    pos += 1;
                }
            }
            k += 1;
        }
    }
    return out[0..pos];
}

fn writeStr(out: []u8, pos: *usize, s: string) {
    let i: usize = 0;
    while (i < s.len && pos.* < out.len) {
        out[pos.*] = s[i];
        pos.* += 1;
        i += 1;
    }
}

/// Encode a label and Base64 payload into RFC 7468 PEM format with 64-character line wrapping.
pub fn encodePem(label: string, base64_payload: string, out: []u8) -> string {
    let pos: usize = 0;
    writeStr(out, &pos, "-----BEGIN ");
    writeStr(out, &pos, label);
    writeStr(out, &pos, "-----\n");

    let col: usize = 0;
    let i: usize = 0;
    while (i < base64_payload.len) {
        const c: u8 = base64_payload[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            if (pos < out.len) {
                out[pos] = c;
                pos += 1;
            }
            col += 1;
            if (col == 64) {
                writeStr(out, &pos, "\n");
                col = 0;
            }
        }
        i += 1;
    }
    if (col > 0) {
        writeStr(out, &pos, "\n");
    }

    writeStr(out, &pos, "-----END ");
    writeStr(out, &pos, label);
    writeStr(out, &pos, "-----\n");
    return out[0..pos];
}
