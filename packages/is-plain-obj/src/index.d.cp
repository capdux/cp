// Type definitions for CP

export class ValueMeta {
    proto_kind: u8,
    has_to_string_tag: bool,
    is_iterator: bool,
    is_null: bool,
    fn plain(): ValueMeta;
    fn nullProto(): ValueMeta;
}

export fn ipoIsSpace(ch: u8): bool;
export fn ipoSliceEq(s: string, start: usize, end: usize, target: string): bool;
export fn ipoIsDangerousSlice(s: string, start: usize, end: usize): bool;
export fn isPlainObjectMeta(m: ValueMeta): bool;
export fn isPlainJsonObject(json_text: string): bool;
export fn isPlainObjectKey(key: string): bool;
export fn hasDangerousProtoKey(json_text: string): bool;
