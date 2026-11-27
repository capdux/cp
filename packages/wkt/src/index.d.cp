// Type definitions for CP

import { isSpace, isAlpha, isDigit, toLowerByte, strEq } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export class Coord2D {
    x: f64,
    y: f64,
}

export class WktGeometry {
    kind: u8,
    is_empty: bool,
    coords: [32]Coord2D,
    coord_count: usize,
}

export class WktBbox {
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
    valid: bool,
}

export fn wktZeroCoord(): Coord2D;
export fn wktEmptyGeom(kind: u8): WktGeometry;
export fn wktSkipWs(s: string, pos: &mut usize);
export fn wktReadWord(s: string, pos: &mut usize, out: []u8): usize;
export fn wktParseNumber(s: string, pos: &mut usize, out: &mut f64): bool;
export fn wktParseCoord(s: string, pos: &mut usize, out: &mut Coord2D): bool;
export fn wktConsumeChar(s: string, pos: &mut usize, ch: u8): bool;
export fn wktWriteF64(out: []u8, pos: usize, val: f64): usize;
export fn wktWriteCoord(out: []u8, pos: usize, c: Coord2D): usize;
export fn parseWkt(input: string): WktGeometry?;
export fn formatWkt(g: WktGeometry, out: []u8): usize;
export fn bboxWkt(g: WktGeometry): WktBbox;
