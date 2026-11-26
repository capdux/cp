// Type definitions for CP

export fn coidZeroArcs(): [16]u32;
export fn coidZeroDer(): [80]u8;
export fn coidEncodeBase128U64(val: u64, out: []u8, start_pos: usize): usize;
export fn coidEncodeArcsToDer(arcs: [16]u32, arc_len: usize, der_out: []u8): usize;
export fn coidWriteU32Dec(val: u32, out: []u8, start_pos: usize): usize;
export class OidParseResult {
    ok: bool,
    oid: ObjectIdentifier,
}

export class ObjectIdentifier {
    arcs: [16]u32,
    len: usize,
    der_buf: [80]u8,
    der_len: usize,
    fn empty(): ObjectIdentifier;
    fn fromStr(s: string): OidParseResult;
    fn fromDer(der: &[]u8): OidParseResult;
    fn fromDerTlv(tlv: &[]u8): OidParseResult;
    fn toDer(out: []u8): usize;
    fn toDerTlv(out: []u8): usize;
    fn toStr(out: []u8): usize;
    fn eq(other: ObjectIdentifier): bool;
    fn arcCount(): usize;
    fn arcAt(idx: usize): u32;
    fn derLen(): usize;
    fn isPrefixOf(other: ObjectIdentifier): bool;
}

