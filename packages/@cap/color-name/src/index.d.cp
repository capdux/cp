// Type definitions for CP

export class RgbColor {
    r: u8,
    g: u8,
    b: u8,
    found: bool,
}

export class ColorEntry {
    name: string,
    r: u8,
    g: u8,
    b: u8,
}

export fn cnToLower(b: u8): u8;
export fn cnCmpIgnoreCase(a: string, b: string): i8;
export fn colorAt(index: usize): ColorEntry;
export fn lookupColor(name: string): RgbColor;
export fn lookupColorHex(name: string): i32;
export fn reverseLookupColor(r: u8, g: u8, b: u8): string;
export fn isNamedColor(name: string): bool;
