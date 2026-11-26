// Type definitions for CP

import { charWidth } from "../../unicode-width/src/index.cp";
export enum BorderStyle {
    Single,
    Double,
    Round,
    Bold,
    Classic,
    SingleDouble
}

export enum TextAlign {
    Left,
    Center,
    Right
}

export class BoxOptions {
    style: BorderStyle,
    text_align: TextAlign,
    padding_x: u8,
    padding_y: u8,
    margin_left: u8,
    title: string,
    title_align: TextAlign,
    fn defaults(): BoxOptions;
}

export class BorderChars {
    tl: string,
    top: string,
    tr: string,
    left: string,
    right: string,
    bl: string,
    bottom: string,
    br: string,
}

export fn borderForStyle(style: BorderStyle): BorderChars;
export class Utf8Step {
    cp: u32,
    width: usize,
}

export fn decodeUtf8At(s: string, i: usize): Utf8Step;
export fn codepointWidth(cp: u32): usize;
export fn displayWidth(s: string): usize;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushStr(out: []u8, pos: usize, s: string): usize;
export fn pushRepeat(out: []u8, pos: usize, s: string, count: usize): usize;
export fn pushSpaces(out: []u8, pos: usize, count: usize): usize;
export fn renderBox(text: string, opts: BoxOptions, out: []u8): string;
export fn box(text: string, out: []u8): string;
export fn titledBox(title: string, text: string, style: BorderStyle, out: []u8): string;
