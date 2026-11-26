// Type definitions for CP

import { isValidUtf8 } from "../../utf8-valid/src/index.cp";
export class CamSpan {
    start: usize,
    end: usize,
    found: bool,
}

export class CamCompStep {
    start: usize,
    end: usize,
    next_pos: usize,
    is_root: bool,
    found: bool,
}

export fn camTrimTrailingSlashes(data: &[]u8, len: usize): usize;
export fn camSliceEq(a: &[]u8, a_start: usize, a_end: usize, b: &[]u8, b_start: usize, b_end: usize): bool;
export fn camIsDot(data: &[]u8, start: usize, end: usize): bool;
export fn camIsDotDot(data: &[]u8, start: usize, end: usize): bool;
export fn camFileNameSpan(data: &[]u8, len: usize): CamSpan;
export fn camFindLastDot(data: &[]u8, start: usize, end: usize): usize?;
export fn camCopyToDst(data: &[]u8, start: usize, end: usize, dst: []u8): usize?;
export fn camNextComponent(data: &[]u8, len: usize, cursor: usize): CamCompStep;
export fn camParentLength(data: &[]u8, len: usize): usize?;
export class Utf8PathBuf {
    buf: [512]u8,
    length: usize,
    fn fromStr(s: string): Utf8PathBuf;
    fn fromBytes(b: []u8): Utf8PathBuf?;
    fn isAbsolute(): bool;
    fn isRelative(): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn equals(other: string): bool;
    fn push(_: auto, path: string);
    fn pop(_: auto): bool;
    fn parent(): Utf8PathBuf?;
    fn fileNameInto(dst: []u8): usize?;
    fn fileStemInto(dst: []u8): usize?;
    fn extensionInto(dst: []u8): usize?;
    fn setFileName(_: auto, name: string);
    fn setExtension(_: auto, ext: string): bool;
    fn componentCount(): usize;
    fn hasPrefixPath(base: string): bool;
    fn stripPrefix(base: string): Utf8PathBuf?;
}

