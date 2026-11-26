// Type definitions for CP

export fn isContinuationByte(b: u8): bool;
export fn floorCharBoundary(s: string, max_bytes: usize): usize;
export fn truncateBytes(s: string, max_bytes: usize): string;
export fn countCodepoints(s: string): usize;
export fn truncateChars(s: string, max_chars: usize): string;
export fn truncateWithEllipsis(s: string, max_bytes: usize, ellipsis: string, out: []u8): usize;
