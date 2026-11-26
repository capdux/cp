// Type definitions for CP

export class InspectOptions {
    quote_style_double: bool,
    max_string_len: usize,
}

export fn defaultInspectOptions(): InspectOptions;
export fn oinWriteStr(s: string, out: []u8, pos: usize): usize;
export fn oinHex(n: u8): u8;
export fn inspectString(s: string, opts: InspectOptions, out: []u8): usize;
export fn inspectBool(v: bool, out: []u8): usize;
export fn inspectInt(val: i64, out: []u8): usize;
export fn inspectBytes(data: &[]u8, max_bytes: usize, out: []u8): usize;
export fn inspectStringArray(items: &[]string, opts: InspectOptions, out: []u8): usize;
