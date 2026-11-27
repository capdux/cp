// Type definitions for CP

import { EnvInfo, isUnicodeSupported } from "../../is-unicode-supported/src/index.cp";
export enum FigureKind {
    Tick,
    Cross,
    Star,
    Square,
    Circle,
    Info,
    Warning,
    Pointer,
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    Ellipsis,
    Bullet,
    Heart,
    RadioOn,
    RadioOff,
    CheckboxOn,
    CheckboxOff,
    Line
}

export fn figStartsAt(input: string, pos: usize, needle: string): bool;
export fn figWriteStr(out: []u8, pos: usize, s: string): usize;
export fn figureFor(kind: FigureKind, unicode_ok: bool): string;
export fn figureForEnv(kind: FigureKind, env: EnvInfo): string;
export fn replaceSymbols(input: string, unicode_ok: bool, out: []u8): usize;
