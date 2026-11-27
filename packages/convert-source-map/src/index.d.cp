// Type definitions for CP

export enum SourceMapCommentKind {
    None,
    InlineBase64,
    ExternalFile
}

export class SourceMapComment {
    kind: SourceMapCommentKind,
    comment_start: usize,
    comment_end: usize,
    payload: string,
}

export fn csmStartsWith(s: string, pos: usize, lit: string): bool;
export fn csmPush(out: []u8, pos: usize, s: string): usize;
export fn csmB64Val(b: u8): i32;
export fn csmB64Encode(data: &[]u8, out: []u8, start_pos: usize): usize;
export fn csmB64Decode(b64: string, out: []u8): i64;
export fn findSourceMapComment(src: string): SourceMapComment;
export fn fromSource(src: string, out: []u8): i64;
export fn toComment(json_map: string, multiline: bool, out: []u8): usize;
export fn removeComments(src: string, out: []u8): usize;
