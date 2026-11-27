// Type definitions for CP

import { bytesEq } from "cap:bin";
export fn rpIsSep(b: u8): bool;
export class RelPath {
    buf: [256]u8,
    length: usize,
    fn empty(): RelPath;
    fn fromStr(s: string): RelPath;
    fn len(): usize;
    fn isEmpty(): bool;
    fn equals(other: string): bool;
    fn copyTo(dst: []u8): usize;
    fn normalize(): RelPath;
    fn join(other: string): RelPath;
    fn joinNormalized(other: string): RelPath;
    fn parent(): RelPath?;
    fn fileNameInto(dst: []u8): usize?;
    fn extensionInto(dst: []u8): usize?;
    fn withExtension(ext: string): RelPath;
    fn hasPrefixPath(base: string): bool;
    fn stripPrefix(base: string): RelPath?;
    fn toLogicalPath(base: string): RelPath;
}

export fn normalizeRelPath(path: string): RelPath;
export fn joinRelPath(base: string, rel: string): RelPath;
export fn relativeTo(from_path: string, to_path: string): RelPath;
