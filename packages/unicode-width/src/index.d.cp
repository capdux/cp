// Type definitions for CP

export fn uwNextCodepoint(s: &[]u8, pos: usize, next_pos: &mut usize): u32;
export fn uwIsZeroWidth(cp: u32): bool;
export fn uwIsWide(cp: u32): bool;
export fn uwIsAmbiguous(cp: u32): bool;
export fn charWidth(cp: u32): usize;
export fn charWidthCjk(cp: u32): usize;
export fn strWidth(s: string): usize;
export fn strWidthCjk(s: string): usize;
export fn truncateToWidth(s: string, max_cols: usize): usize;
