// Type definitions for CP

export class MagnetLink {
    hash_kind: u8,
    info_hash: string,
    display_name: string,
    exact_length: i64,
    trackers: [8]string,
    tracker_count: usize,
    valid: bool,
}

export fn toLower(c: u8): u8;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn startsWithIgnoreCase(s: string, prefix: string): bool;
export fn isHexStr(s: string): bool;
export fn isBase32Str(s: string): bool;
export fn parsePositiveI64(s: string): i64;
export class TopicMatch {
    kind: u8,
    hash: string,
}

export fn classifyExactTopic(xt: string): TopicMatch;
export fn parseMagnet(uri: string): MagnetLink;
export fn appendBytes(dst: []u8, pos: usize, src: string): usize;
export fn formatMagnetV1(info_hash: string, display_name: string, trackers: &[]string, out: []u8): usize;
