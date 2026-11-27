// Type definitions for CP

export class LineCol {
    line: usize,
    col: usize,
    valid: bool,
}

export fn offsetToLineCol(text: string, offset: usize): LineCol;
export fn lineColToOffset(text: string, line: usize, col: usize): i64;
export fn getLine(text: string, line_1based: usize): string;
export fn countLines(text: string): usize;
