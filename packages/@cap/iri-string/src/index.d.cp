// Type definitions for CP

export class IriParts {
    scheme_start: usize,
    scheme_len: usize,
    auth_start: usize,
    auth_len: usize,
    path_start: usize,
    path_len: usize,
    query_start: usize,
    query_len: usize,
    frag_start: usize,
    frag_len: usize,
    has_scheme: bool,
    has_auth: bool,
    has_query: bool,
    has_frag: bool,
    valid: bool,
}

export fn invalidIri(): IriParts;
export fn isAsciiAlpha(c: u8): bool;
export fn isSchemeChar(c: u8): bool;
export fn parseIri(s: string): IriParts;
export class RemCursor {
    pos: usize,
    prepend_slash: bool,
}

export fn remLen(path: string, cur: RemCursor): usize;
export fn remAt(path: string, cur: RemCursor, idx: usize): u8;
export fn remAdvance(cur: &mut RemCursor, n: usize);
export fn popLastSegment(out: []u8, out_len: usize): usize;
export fn removeDotSegments(path: string, out: []u8): usize;
export class OutBuf {
    pos: usize,
    overflow: bool,
}

export fn obPush(ob: &mut OutBuf, out: []u8, ch: u8);
export fn obWrite(ob: &mut OutBuf, out: []u8, src: string);
export fn resolveReference(base: string, rel: string, out: []u8): usize;
