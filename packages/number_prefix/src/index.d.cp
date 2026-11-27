// Type definitions for CP

export enum PrefixKind {
    Standalone,
    Kilo,
    Mega,
    Giga,
    Tera,
    Peta,
    Exa,
    Kibi,
    Mebi,
    Gibi,
    Tebi,
    Pebi,
    Exbi
}

export class PrefixedNumber {
    value: f64,
    prefix: PrefixKind,
    symbol: string,
    is_standalone: bool,
}

export fn decimalPrefix(amount: f64): PrefixedNumber;
export fn binaryPrefix(amount: f64): PrefixedNumber;
export fn npfWriteStr(s: string, out: []u8, pos: usize): usize;
export fn npfWriteU64(val: u64, out: []u8, pos: usize): usize;
export fn formatPrefixed(pn: PrefixedNumber, decimals: u8, unit: string, out: []u8): usize;
