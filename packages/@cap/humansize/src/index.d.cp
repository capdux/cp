// Type definitions for CP

export enum BaseUnit {
    Decimal,
    Binary,
    Windows
}

export class FormatSizeOptions {
    base: BaseUnit,
    decimal_places: u8,
    space_after_value: bool,
    long_units: bool,
}

export fn hszWriteSlice(out: []u8, pos: usize, s: string): usize;
export fn hszWriteU64(out: []u8, pos: usize, val: u64): usize;
export fn hszWriteFixedFrac(out: []u8, pos: usize, frac: u64, places: u8): usize;
export fn hszPow10(places: u8): u64;
export fn hszUnitStr(base: BaseUnit, idx: usize, long_units: bool, is_one: bool): string;
export fn formatSize(bytes: u64, opts: FormatSizeOptions, out: []u8): usize;
export fn formatDecimal(bytes: u64, out: []u8): usize;
export fn formatBinary(bytes: u64, out: []u8): usize;
