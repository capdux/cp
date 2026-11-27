// Type definitions for CP

import { strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
export enum SpinnerKind {
    Dots,
    Dots2,
    Line,
    Pipe,
    Arc,
    Circle,
    SquareCorners,
    Bounce,
    BouncingBar,
    Arrow,
    Clock,
    AsciiDots
}

export enum SymbolKind {
    Success,
    Error,
    Warning,
    Info
}

export fn intervalMs(kind: SpinnerKind): u32;
export fn frameCount(kind: SpinnerKind): usize;
export fn frameAt(kind: SpinnerKind, tick: usize): string;
export fn frameAtTime(kind: SpinnerKind, elapsed_ms: u64): string;
export fn findSpinnerByName(name: string): SpinnerKind?;
export fn logSymbol(kind: SymbolKind, ascii_only: bool): string;
export fn renderSpinnerLine(kind: SpinnerKind, tick: usize, text: string, out: []u8): string;
export fn renderStatusLine(sym: SymbolKind, ascii_only: bool, text: string, out: []u8): string;
