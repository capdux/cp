// Type definitions for CP

export fn h3AllLow63(): u64;
export fn h3DigitShift(r: u8): u6;
export fn h3IsPentagonBaseCell(bc: u8): bool;
export fn h3HexDigitVal(c: u8): u8;
export fn h3HexChar(nibble: u8): u8;
export fn h3ClearBits(val: u64, mask: u64): u64;
export class H3Cell {
    bits: u64,
    fn fromParts(res: u8, base_cell: u8, digits: [15]u8): H3Cell?;
    fn fromU64(raw: u64): H3Cell?;
    fn resolution(): u8;
    fn baseCell(): u8;
    fn digitAt(res: u8): u8;
    fn isPentagon(): bool;
    fn parent(target_res: u8): H3Cell?;
    fn childAt(digit: u8): H3Cell?;
    fn toHex(out: []u8): usize;
}

export fn parseH3Hex(hex: string): H3Cell?;
export fn avgHexAreaKm2(res: u8): f64;
