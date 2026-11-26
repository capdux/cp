// Type definitions for CP

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
export fn wktIsSpace(c: u8): bool;
export fn wktIsAlpha(c: u8): bool;
export fn wktIsDigit(c: u8): bool;
export fn wktToLower(c: u8): u8;
export fn wktSkipWs(s: string, pos: &mut usize);
export fn wktWordEq(buf: &[]u8, len: usize, lit: string): bool;
export fn wktReadWord(s: string, pos: &mut usize, out: []u8): usize;
export fn wktParseNumber(s: string, pos: &mut usize, out: &mut f64): bool;
export fn wktParseCoord(s: string, pos: &mut usize, out: &mut Coord2D): bool;
export fn wktConsumeChar(s: string, pos: &mut usize, ch: u8): bool;
export fn wktWriteStr(out: []u8, pos: usize, s: string): usize;
export fn wktWriteU64(out: []u8, pos: usize, val: u64): usize;
export fn wktWriteF64(out: []u8, pos: usize, val: f64): usize;
export fn wktWriteCoord(out: []u8, pos: usize, c: Coord2D): usize;
export fn parseWkt(input: string): WktGeometry?;
export fn formatWkt(g: WktGeometry, out: []u8): usize;
export fn bboxWkt(g: WktGeometry): WktBbox;
