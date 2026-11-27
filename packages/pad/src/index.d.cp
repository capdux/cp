// Type definitions for CP

export fn countChars(s: string): usize;
export fn copyBytes(dst: []u8, pos: usize, src: string);
export fn fillByte(dst: []u8, pos: usize, count: usize, ch: u8);
export fn padStart(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;
export fn padEnd(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;
export fn padCenter(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;
export fn repeatStr(s: string, count: usize, out: []u8): usize;
export class PadOptions {
    pad_char: u8,
    pad_str: string,
    truncate: bool,
    left: bool,
    fn defaults(): PadOptions;
}

export fn truncateChars(s: string, max_chars: usize): string;
export fn calcPadBytes(pad_chars: usize, pad_char: u8, pad_str: string): usize;
export fn writePadPattern(dst: []u8, pos: usize, pad_chars: usize, pad_char: u8, pad_str: string): usize;
export fn padLeft(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;
export fn padRight(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;
export fn pad(s: string, target_chars: usize, opts: PadOptions, out: []u8): usize;
export fn padCenterWith(s: string, target_chars: usize, opts: PadOptions, out: []u8): usize;
