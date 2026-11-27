// Type definitions for CP

export fn isBareKey(k: string): bool;
export class TomlWriter {
    out: []u8,
    pos: usize,
    overflow: bool,
    fn init(out: []u8): TomlWriter;
    fn asSlice(): string;
    fn putByte(_: auto, b: u8);
    fn putRaw(_: auto, s: string);
    fn writeQuotedString(_: auto, s: string);
    fn writeKey(_: auto, k: string);
    fn writeI64(_: auto, val: i64);
    fn writeBool(_: auto, b: bool);
    fn writeComment(_: auto, text: string);
    fn writeTableHeader(_: auto, name: string);
    fn writeArrayTableHeader(_: auto, name: string);
    fn writeKvStr(_: auto, key: string, val: string);
    fn writeKvInt(_: auto, key: string, val: i64);
    fn writeKvBool(_: auto, key: string, val: bool);
    fn writeKvStrArray(_: auto, key: string, items: &[]string);
}

