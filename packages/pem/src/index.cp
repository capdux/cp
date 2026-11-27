// @cap/pem — Zero-heap RFC 7468 PEM Armor Parser & Encoder
// Clean-room MIT implementation in pure CP (replaces Rust `pem` [55M] & npm `pem-jwk`/`sshpk`).
// Accelerated with comptime 256-entry RFC 7468 LUT, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

import { findByte } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
import { writeStr } from "cap:fmt";

type PemVec16 = @Vector(16, u8);

const PEM_FLAG_LABEL: u8 = 1;
const PEM_FLAG_WS: u8 = 2;

fn pemBuildFlagsLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if ((c >= ('A' as usize) && c <= ('Z' as usize))
            || (c >= ('0' as usize) && c <= ('9' as usize))
            || c == (' ' as usize)
            || c == ('-' as usize))
        {
            f = f | PEM_FLAG_LABEL;
        }
        if (c == (' ' as usize) || c == ('\t' as usize) || c == ('\r' as usize) || c == ('\n' as usize)) {
            f = f | PEM_FLAG_WS;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const PEM_FLAGS_LUT: [256]u8 = pemBuildFlagsLut();

fn pemHasWs16(v: PemVec16) -> bool {
    const v_sp: PemVec16 = @splat(' ' as u8);
    const v_tab: PemVec16 = @splat('\t' as u8);
    const v_cr: PemVec16 = @splat('\r' as u8);
    const v_lf: PemVec16 = @splat('\n' as u8);
    return @reduce(.Or, v == v_sp)
        || @reduce(.Or, v == v_tab)
        || @reduce(.Or, v == v_cr)
        || @reduce(.Or, v == v_lf);
}

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

fn isValidLabel(label: string) -> bool {
    if (label.len == 0 || label.len > 64) { return false; }
    let i: usize = 0;
    while (i + 4 <= label.len) {
        if ((PEM_FLAGS_LUT[label[i] as usize]
            & PEM_FLAGS_LUT[label[i + 1] as usize]
            & PEM_FLAGS_LUT[label[i + 2] as usize]
            & PEM_FLAGS_LUT[label[i + 3] as usize]
            & PEM_FLAG_LABEL) == 0)
        {
            return false;
        }
        i += 4;
    }
    while (i < label.len) {
        if ((PEM_FLAGS_LUT[label[i] as usize] & PEM_FLAG_LABEL) == 0) { return false; }
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

    const bOpt = indexOfBytes(input, beginPrefix, from_offset);
    if (bOpt == null) { return invalid; }
    const bPos: usize = bOpt ?? (0 as usize);

    const labelStart: usize = bPos + beginPrefix.len;
    const bEndOpt = indexOfBytes(input, dashes, labelStart);
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
        const eOpt = indexOfBytes(input, endPrefix, searchPos);
        if (eOpt == null) { return invalid; }
        const ePos: usize = eOpt ?? (0 as usize);
        const eLabelStart: usize = ePos + endPrefix.len;
        const eEndOpt = indexOfBytes(input, dashes, eLabelStart);
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
        const lineEnd: usize = findByte(body, '\n', i) ?? body.len;
        const line: string = body[i..lineEnd];
        i = if (lineEnd < body.len) lineEnd + 1 else body.len;

        // Skip RFC 1421 header lines containing ':'
        if (findByte(line, ':', 0) != null) { continue; }

        let k: usize = 0;
        while (k + 16 <= line.len && pos + 16 <= out.len) {
            const v: PemVec16 = line[k .. k + 16][0..16].*;
            if (!pemHasWs16(v)) {
                @memcpy(out[pos .. pos + 16], line[k .. k + 16]);
                pos += 16;
                k += 16;
                continue;
            }
            let j: usize = 0;
            while (j < 16) {
                const c: u8 = line[k + j];
                if ((PEM_FLAGS_LUT[c as usize] & PEM_FLAG_WS) == 0) {
                    if (pos < out.len) {
                        out[pos] = c;
                        pos += 1;
                    }
                }
                j += 1;
            }
            k += 16;
        }
        while (k < line.len) {
            const c: u8 = line[k];
            if ((PEM_FLAGS_LUT[c as usize] & PEM_FLAG_WS) == 0) {
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

/// Encode a label and Base64 payload into RFC 7468 PEM format with 64-character line wrapping.
pub fn encodePem(label: string, base64_payload: string, out: []u8) -> string {
    let pos: usize = 0;
    pos = writeStr("-----BEGIN ", out, pos);
    pos = writeStr(label, out, pos);
    pos = writeStr("-----\n", out, pos);

    let col: usize = 0;
    let i: usize = 0;
    while (i < base64_payload.len) {
        if (i + 16 <= base64_payload.len && col + 16 <= 64 && pos + 16 <= out.len) {
            const v: PemVec16 = base64_payload[i .. i + 16][0..16].*;
            if (!pemHasWs16(v)) {
                @memcpy(out[pos .. pos + 16], base64_payload[i .. i + 16]);
                pos += 16;
                col += 16;
                i += 16;
                if (col == 64) {
                    pos = writeStr("\n", out, pos);
                    col = 0;
                }
                continue;
            }
        }
        const c: u8 = base64_payload[i];
        if ((PEM_FLAGS_LUT[c as usize] & PEM_FLAG_WS) == 0) {
            if (pos < out.len) {
                out[pos] = c;
                pos += 1;
            }
            col += 1;
            if (col == 64) {
                pos = writeStr("\n", out, pos);
                col = 0;
            }
        }
        i += 1;
    }
    if (col > 0) {
        pos = writeStr("\n", out, pos);
    }

    pos = writeStr("-----END ", out, pos);
    pos = writeStr(label, out, pos);
    pos = writeStr("-----\n", out, pos);
    return out[0..pos];
}
