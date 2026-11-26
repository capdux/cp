// Type definitions for CP

export fn skipAnsiAt(input: string, pos: usize): usize;
export fn hasAnsi(input: string): bool;
export fn strippedLen(input: string): usize;
export fn stripAnsi(input: string, out: []u8): usize;
export fn visibleAsciiCount(input: string): usize;
