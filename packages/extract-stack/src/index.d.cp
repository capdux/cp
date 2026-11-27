// Type definitions for CP

export fn exsIsHspace(c: u8): bool;
export fn exsIsFrameLine(line: string): bool;
export fn exsFindFirstFrameOffset(stack: string): usize;
export fn exsTrimLine(line: string): string;
export fn extractStack(stack: string): string;
export fn extractErrorHeader(stack: string): string;
export fn extractStackLines(stack: string, out: []string): usize;
