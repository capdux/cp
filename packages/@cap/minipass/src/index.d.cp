// Type definitions for CP

export fn mpassMin(a: usize, b: usize): usize;
export class MinipassStream {
    buf: [4096]u8,
    head: usize,
    len: usize,
    flowing: bool,
    ended: bool,
    destroyed: bool,
    fn new(): MinipassStream;
    fn bufferedBytes(): usize;
    fn freeSpace(): usize;
    fn isPaused(): bool;
    fn isEnded(): bool;
    fn isDrained(): bool;
    fn pause(_: auto);
    fn @"resume"(_: auto);
    fn end(_: auto);
    fn destroy(_: auto);
    fn write(_: auto, data: &[]u8): bool;
    fn read(_: auto, out: []u8): usize;
    fn readExact(_: auto, n: usize, out: []u8): usize;
    fn peek(out: []u8): usize;
    fn pipeTo(_: auto, dest: &mut MinipassStream): usize;
}

