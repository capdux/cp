// Type definitions for CP

export class MstrChunk {
    start: usize,
    end: usize,
    content: string,
    edited: bool,
}

export class MstrInsert {
    index: usize,
    content: string,
}

export fn mstrWriteStr(src: string, out: []u8, pos: usize): usize;
export fn mstrWriteLeftInserts(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize;
export fn mstrWriteRightInserts(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize;
export class MagicString {
    original: string,
    changed: bool,
    chunks: [64]MstrChunk,
    chunk_count: usize,
    global_intros: [16]string,
    global_intro_len: usize,
    global_outros: [16]string,
    global_outro_len: usize,
    left_inserts: [32]MstrInsert,
    left_insert_len: usize,
    right_inserts: [32]MstrInsert,
    right_insert_len: usize,
    fn new(original: string): MagicString;
    fn prepend(_: auto, s: string): bool;
    fn append(_: auto, s: string): bool;
    fn splitAt(_: auto, split_at: usize): bool;
    fn overwrite(_: auto, start: usize, end: usize, content: string): bool;
    fn remove(_: auto, start: usize, end: usize): bool;
    fn prependLeft(_: auto, index: usize, content: string): bool;
    fn appendRight(_: auto, index: usize, content: string): bool;
    fn hasChanged(): bool;
    fn writeTo(out: []u8): usize;
    fn toString(out: []u8): string;
    fn slice(start: usize, end: usize, out: []u8): usize;
}

