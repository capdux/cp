// Type definitions for CP

import { cursorHide, cursorShow, eraseLine, cursorLeft } from "../../ansi-escapes/src/index.cp";
export fn oraWriteSlice(out: []u8, pos: usize, s: string): usize;
export fn oraFrameCount(style: u8): usize;
export fn oraFrameAt(style: u8, index: usize): string;
export fn oraRenderStatus(symbol: string, msg: string, fallback: string, out: []u8): usize;
export class Spinner {
    style: u8,
    text: string,
    frame_index: usize,
    fn new(style: u8, text: string): Spinner;
    fn frameCount(): usize;
    fn currentFrame(): string;
    fn tick(_: auto): string;
    fn setText(_: auto, text: string);
    fn renderFrame(with_ansi: bool, out: []u8): usize;
    fn renderSucceed(msg: string, out: []u8): usize;
    fn renderFail(msg: string, out: []u8): usize;
    fn renderWarn(msg: string, out: []u8): usize;
    fn renderInfo(msg: string, out: []u8): usize;
    fn hideCursorSeq(): string;
    fn showCursorSeq(): string;
}

