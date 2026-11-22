// Source: cp/lib/pem/pem.cp (Pure CP RFC 7468 PEM Certificate Chain & Key Block Codec)
// Zero-C-dependency PEM parser and encoder supporting multi-block chains and legacy RFC 1421 headers.

const std = @import("std");

/// Unified error set returned by `cap:pem` parsing and encoding operations.
pub err PemError {
    InvalidLabel,
    MismatchedEndLabel,
    MissingEndMarker,
    InvalidBase64,
    EmptyPayload,
    TooManyBlocks,
    OutOfMemory,
}

const DEFAULT_MAX_BLOCKS: usize = 64;
const PEM_LINE_WIDTH: usize = 64;
const BEGIN_PREFIX: []const u8 = "-----BEGIN";
const END_PREFIX: []const u8 = "-----END";
const MARKER_SUFFIX: []const u8 = "-----";
const BASE64_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/// Optional RFC 1421 encapsulated header field (e.g. `Proc-Type`, `DEK-Info`).
pub struct PemHeader {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// Decoded or encodable RFC 7468 PEM block containing label, optional headers, and raw DER/binary payload.
pub struct PemBlock {
    label: []const u8 = "",
    headers: []PemHeader = [],
    bytes: []const u8 = "",

    /// Releases owned `label`, `headers`, and `bytes` using `allocator`.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.label.len > 0) {
            allocator.free(_.label);
        }
        for (h of _.headers) {
            if (h.name.len > 0) {
                allocator.free(h.name);
            }
            if (h.value.len > 0) {
                allocator.free(h.value);
            }
        }
        if (_.headers.len > 0) {
            allocator.free(_.headers);
        }
        if (_.bytes.len > 0) {
            allocator.free(_.bytes);
        }
        _.label = "";
        _.headers = [];
        _.bytes = "";
    }

    /// Looks up a header value by case-insensitive `name`, returning `null` if absent.
    pub fn header(name: []const u8): ([]const u8)? {
        for (h of _.headers) {
            if (std.ascii.eqlIgnoreCase(h.name, name)) {
                return h.value;
            }
        }
        return null;
    }
}

fn isHorizWs(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimHorizWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isHorizWs(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && (isHorizWs(s[end - 1]) || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

/// Validates an RFC 7468 PEM label (non-empty printable ASCII without control chars, newlines, or leading/trailing spaces).
fn isValidLabel(label: []const u8): bool {
    if (label.len == 0) {
        return false;
    }
    if (label[0] == ' ' || label[label.len - 1] == ' ') {
        return false;
    }
    if (std.mem.indexOf(u8, label, "-----") != null) {
        return false;
    }
    for (c of label) {
        if (c < 0x20 || c > 0x7E) {
            return false;
        }
    }
    return true;
}

fn base64CharVal(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
    if (c >= '0' && c <= '9') return (c - '0') + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 0xFF;
}

fn isBase64Ws(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn decodeBase64Strict(allocator: std.mem.Allocator, raw_body: []const u8): ([]u8) ! PemError {
    // First strip ASCII whitespace into a clean buffer so we can validate length & padding strictly.
    let clean: std.ArrayList(u8) = .empty;
    defer clean.deinit(allocator);

    for (c of raw_body) {
        if (!isBase64Ws(c)) {
            clean.append(allocator, c) catch return PemError.OutOfMemory;
        }
    }

    const len = clean.items.len;
    if (len == 0) {
        return PemError.EmptyPayload;
    }
    if (len % 4 != 0) {
        return PemError.InvalidBase64;
    }

    let pad: usize = 0;
    if (clean.items[len - 1] == '=') {
        pad += 1;
        if (clean.items[len - 2] == '=') {
            pad += 1;
        }
    }

    const out_len = (len / 4) * 3 - pad;
    let out = allocator.alloc(u8, out_len) catch return PemError.OutOfMemory;
    errdefer allocator.free(out);

    let in_idx: usize = 0;
    let out_idx: usize = 0;
    while (in_idx < len) : (in_idx += 4) {
        const is_last = (in_idx + 4 == len);
        const c0 = clean.items[in_idx];
        const c1 = clean.items[in_idx + 1];
        const c2 = clean.items[in_idx + 2];
        const c3 = clean.items[in_idx + 3];

        const v0 = base64CharVal(c0);
        const v1 = base64CharVal(c1);
        if (v0 == 0xFF || v1 == 0xFF) {
            return PemError.InvalidBase64;
        }

        if (c2 == '=') {
            if (!is_last || c3 != '=') {
                return PemError.InvalidBase64;
            }
            const triple: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12);
            out[out_idx] = ((triple >> 16) & 0xFF) as u8;
            out_idx += 1;
        } else if (c3 == '=') {
            if (!is_last) {
                return PemError.InvalidBase64;
            }
            const v2 = base64CharVal(c2);
            if (v2 == 0xFF) {
                return PemError.InvalidBase64;
            }
            const triple: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12) | ((v2 as u32) << 6);
            out[out_idx] = ((triple >> 16) & 0xFF) as u8;
            out[out_idx + 1] = ((triple >> 8) & 0xFF) as u8;
            out_idx += 2;
        } else {
            const v2 = base64CharVal(c2);
            const v3 = base64CharVal(c3);
            if (v2 == 0xFF || v3 == 0xFF) {
                return PemError.InvalidBase64;
            }
            const triple: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12) | ((v2 as u32) << 6) | (v3 as u32);
            out[out_idx] = ((triple >> 16) & 0xFF) as u8;
            out[out_idx + 1] = ((triple >> 8) & 0xFF) as u8;
            out[out_idx + 2] = (triple & 0xFF) as u8;
            out_idx += 3;
        }
    }

    return out;
}

fn dupBytes(allocator: std.mem.Allocator, s: []const u8): ([]u8) ! PemError {
    return allocator.dupe(u8, s) catch return PemError.OutOfMemory;
}

fn freeHeaderList(allocator: std.mem.Allocator, list: *std.ArrayList(PemHeader)) {
    for (h of list.items) {
        if (h.name.len > 0) allocator.free(h.name);
        if (h.value.len > 0) allocator.free(h.value);
    }
    list.deinit(allocator);
}

fn extractBoundaryLabel(trimmed_line: []const u8, prefix: []const u8): ([]const u8) ! PemError {
    if (!std.mem.endsWith(u8, trimmed_line, MARKER_SUFFIX)) {
        return PemError.InvalidLabel;
    }
    // Must have a single space after prefix (`-----BEGIN <LABEL>-----` or `-----END <LABEL>-----`).
    if (trimmed_line.len < prefix.len + MARKER_SUFFIX.len + 1) {
        return PemError.InvalidLabel;
    }
    if (trimmed_line[prefix.len] != ' ') {
        return PemError.InvalidLabel;
    }
    const label_start = prefix.len + 1;
    const label_end = trimmed_line.len - MARKER_SUFFIX.len;
    if (label_end <= label_start) {
        return PemError.InvalidLabel;
    }
    const label = trimmed_line[label_start..label_end];
    if (!isValidLabel(label)) {
        return PemError.InvalidLabel;
    }
    return label;
}

fn parseBlocksInternal(
    allocator: std.mem.Allocator,
    pem_text: []const u8,
    max_blocks: usize,
    stop_after_first: bool,
): ([]PemBlock) ! PemError {
    let blocks: std.ArrayList(PemBlock) = .empty;
    errdefer {
        let bi: usize = 0;
        while (bi < blocks.items.len) : (bi += 1) {
            blocks.items[bi].deinit(allocator);
        }
        blocks.deinit(allocator);
    }

    let pos: usize = 0;
    while (pos < pem_text.len) {
        // Scan line-by-line until a `-----BEGIN` marker is found.
        let begin_label: ([]const u8)? = null;
        while (pos < pem_text.len) {
            const nl_rel = std.mem.indexOfScalar(u8, pem_text[pos..], '\n');
            const line_end = if (nl_rel != null) pos + nl_rel.? else pem_text.len;
            const raw_line = pem_text[pos..line_end];
            pos = if (nl_rel != null) line_end + 1 else pem_text.len;

            const trimmed = trimHorizWs(raw_line);
            if (std.mem.startsWith(u8, trimmed, BEGIN_PREFIX)) {
                begin_label = extractBoundaryLabel(trimmed, BEGIN_PREFIX)?;
                break;
            }
        }

        const label_slice = begin_label orelse break;

        if (blocks.items.len >= max_blocks) {
            return PemError.TooManyBlocks;
        }

        let headers_list: std.ArrayList(PemHeader) = .empty;
        errdefer freeHeaderList(allocator, &headers_list);

        let body_buf: std.ArrayList(u8) = .empty;
        defer body_buf.deinit(allocator);

        let in_body = false;
        let found_end = false;

        while (pos < pem_text.len) {
            const nl_rel = std.mem.indexOfScalar(u8, pem_text[pos..], '\n');
            const line_end = if (nl_rel != null) pos + nl_rel.? else pem_text.len;
            const raw_line = pem_text[pos..line_end];
            pos = if (nl_rel != null) line_end + 1 else pem_text.len;

            const trimmed = trimHorizWs(raw_line);
            if (std.mem.startsWith(u8, trimmed, END_PREFIX)) {
                const end_label = extractBoundaryLabel(trimmed, END_PREFIX) catch {
                    return PemError.MismatchedEndLabel;
                };
                if (!std.mem.eql(u8, label_slice, end_label)) {
                    return PemError.MismatchedEndLabel;
                }
                found_end = true;
                break;
            }

            if (!in_body) {
                if (trimmed.len == 0) {
                    continue;
                }
                const colon_opt = std.mem.indexOfScalar(u8, trimmed, ':');
                if (colon_opt != null) {
                    const colon_idx = colon_opt.?;
                    const h_name = trimHorizWs(trimmed[0..colon_idx]);
                    const h_val = trimHorizWs(trimmed[colon_idx + 1..]);
                    const owned_name = dupBytes(allocator, h_name)?;
                    const owned_val = dupBytes(allocator, h_val) catch {
                        allocator.free(owned_name);
                        return PemError.OutOfMemory;
                    };
                    headers_list.append(allocator, PemHeader{
                        name: owned_name,
                        value: owned_val,
                    }) catch {
                        allocator.free(owned_name);
                        allocator.free(owned_val);
                        return PemError.OutOfMemory;
                    };
                    continue;
                }
                in_body = true;
            }

            body_buf.appendSlice(allocator, trimmed) catch return PemError.OutOfMemory;
        }

        if (!found_end) {
            return PemError.MissingEndMarker;
        }

        const decoded_bytes = decodeBase64Strict(allocator, body_buf.items)?;
        errdefer allocator.free(decoded_bytes);

        const owned_label = dupBytes(allocator, label_slice)?;
        errdefer allocator.free(owned_label);

        const owned_headers = headers_list.toOwnedSlice(allocator) catch return PemError.OutOfMemory;
        blocks.append(allocator, PemBlock{
            label: owned_label,
            headers: owned_headers,
            bytes: decoded_bytes,
        }) catch {
            allocator.free(owned_headers);
            return PemError.OutOfMemory;
        };

        if (stop_after_first) {
            break;
        }
    }

    return blocks.toOwnedSlice(allocator) catch return PemError.OutOfMemory;
}

/// Parses all PEM blocks in `pem_text` up to a default limit of 64 blocks.
pub fn parse(allocator: std.mem.Allocator, pem_text: []const u8): ([]PemBlock) ! PemError {
    return parseWithMaxBlocks(allocator, pem_text, DEFAULT_MAX_BLOCKS);
}

/// Parses all PEM blocks in `pem_text`, enforcing `max_blocks` as a resource exhaustion guard.
pub fn parseWithMaxBlocks(allocator: std.mem.Allocator, pem_text: []const u8, max_blocks: usize): ([]PemBlock) ! PemError {
    return parseBlocksInternal(allocator, pem_text, max_blocks, false);
}

/// Parses only the first PEM block in `pem_text`, or returns `null` if no block is present.
pub fn parseFirst(allocator: std.mem.Allocator, pem_text: []const u8): (PemBlock?) ! PemError {
    const blocks = parseBlocksInternal(allocator, pem_text, 1, true)?;
    if (blocks.len == 0) {
        allocator.free(blocks);
        return null;
    }
    const first = blocks[0];
    allocator.free(blocks);
    return first;
}

/// Releases every `PemBlock` in `blocks` and frees the outer `blocks` slice.
pub fn freeBlocks(allocator: std.mem.Allocator, blocks: []PemBlock) {
    let i: usize = 0;
    while (i < blocks.len) : (i += 1) {
        blocks[i].deinit(allocator);
    }
    allocator.free(blocks);
}

fn appendBase64Wrapped(out: *std.ArrayList(u8), allocator: std.mem.Allocator, bytes: []const u8): void ! PemError {
    if (bytes.len == 0) {
        return;
    }

    let col: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) : (i += 3) {
        const rem = bytes.len - i;
        const b0: u32 = bytes[i] as u32;
        const b1: u32 = if (rem > 1) (bytes[i + 1] as u32) else 0;
        const b2: u32 = if (rem > 2) (bytes[i + 2] as u32) else 0;
        const triple: u32 = (b0 << 16) | (b1 << 8) | b2;

        let quad: [4]u8 = [
            BASE64_ALPHABET[((triple >> 18) & 0x3F) as usize],
            BASE64_ALPHABET[((triple >> 12) & 0x3F) as usize],
            if (rem > 1) BASE64_ALPHABET[((triple >> 6) & 0x3F) as usize] else '=',
            if (rem > 2) BASE64_ALPHABET[(triple & 0x3F) as usize] else '=',
        ];

        for (ch of quad) {
            out.append(allocator, ch) catch return PemError.OutOfMemory;
            col += 1;
            if (col == PEM_LINE_WIDTH) {
                out.append(allocator, '\n') catch return PemError.OutOfMemory;
                col = 0;
            }
        }
    }

    if (col != 0) {
        out.append(allocator, '\n') catch return PemError.OutOfMemory;
    }
}

fn encodeBlockInto(
    out: *std.ArrayList(u8),
    allocator: std.mem.Allocator,
    label: []const u8,
    headers: []const PemHeader,
    bytes: []const u8,
): void ! PemError {
    if (!isValidLabel(label)) {
        return PemError.InvalidLabel;
    }

    out.appendSlice(allocator, "-----BEGIN ") catch return PemError.OutOfMemory;
    out.appendSlice(allocator, label) catch return PemError.OutOfMemory;
    out.appendSlice(allocator, "-----\n") catch return PemError.OutOfMemory;

    if (headers.len > 0) {
        for (h of headers) {
            out.appendSlice(allocator, h.name) catch return PemError.OutOfMemory;
            out.appendSlice(allocator, ": ") catch return PemError.OutOfMemory;
            out.appendSlice(allocator, h.value) catch return PemError.OutOfMemory;
            out.append(allocator, '\n') catch return PemError.OutOfMemory;
        }
        out.append(allocator, '\n') catch return PemError.OutOfMemory;
    }

    appendBase64Wrapped(out, allocator, bytes)?;

    out.appendSlice(allocator, "-----END ") catch return PemError.OutOfMemory;
    out.appendSlice(allocator, label) catch return PemError.OutOfMemory;
    out.appendSlice(allocator, "-----\n") catch return PemError.OutOfMemory;
}

/// Encodes `bytes` as an RFC 7468 PEM block with 64-character Base64 line wrapping.
pub fn encode(allocator: std.mem.Allocator, label: []const u8, bytes: []const u8): ([]u8) ! PemError {
    const empty_headers: [0]PemHeader = [];
    return encodeWithHeaders(allocator, label, &empty_headers, bytes);
}

/// Encodes `bytes` as a PEM block with optional RFC 1421 `Key: Value` headers and 64-character line wrapping.
pub fn encodeWithHeaders(
    allocator: std.mem.Allocator,
    label: []const u8,
    headers: []const PemHeader,
    bytes: []const u8,
): ([]u8) ! PemError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    encodeBlockInto(&out, allocator, label, headers, bytes)?;
    return out.toOwnedSlice(allocator) catch return PemError.OutOfMemory;
}

/// Encodes a slice of `PemBlock`s into a concatenated PEM certificate/key chain.
pub fn encodeChain(allocator: std.mem.Allocator, blocks: []const PemBlock): ([]u8) ! PemError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    for (blk of blocks) {
        encodeBlockInto(&out, allocator, blk.label, blk.headers, blk.bytes)?;
    }
    return out.toOwnedSlice(allocator) catch return PemError.OutOfMemory;
}
