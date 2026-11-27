// Type definitions for CP

export class OidEntry {
    oid: string,
    short_name: string,
    description: string,
    category: string,
}

export fn oidrEntryAt(idx: usize): OidEntry;
export fn oidrStrEq(a: string, b: string): bool;
export fn oidEntryCount(): usize;
export fn lookupOid(dotted: string): OidEntry?;
export fn lookupShortName(sn: string): OidEntry?;
export fn oidrEncodeBase128(val: u64, out: []u8, pos: usize): usize?;
export fn oidrWriteU64Dec(val: u64, out: []u8, pos: usize): usize?;
export fn encodeOidDer(dotted: string, out: []u8): usize;
export class OidrDecodeStep {
    val: u64,
    next_pos: usize,
    ok: bool,
}

export fn oidrDecodeBase128(der: &[]u8, start: usize): OidrDecodeStep;
export fn decodeOidDer(der: &[]u8, out: []u8): usize;
