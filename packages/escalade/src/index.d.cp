// Type definitions for CP

import { normalizePath } from "../../normalize-path/src/index.cp";
export class AncestorWalker {
    buf: [512]u8,
    len: usize,
    done: bool,
    fn new(start_path: string): AncestorWalker;
    fn isDone(): bool;
    fn next(_: auto, out: []u8): usize;
    fn remainingDepth(): usize;
}

export fn joinCandidate(dir: string, name: string, out: []u8): usize;
