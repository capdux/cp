// Type definitions for CP

import { isDigit, strEq } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
import { WKT_POINT, WKT_LINESTRING, WKT_POLYGON, WKT_MAX_COORDS, Coord2D, WktGeometry, WktBbox, parseWkt, formatWkt, bboxWkt } from "../../wkt/src/index.cp";
export class GeoJsonGeometry {
    kind: u8,
    coords: [32]Coord2D,
    coord_count: usize,
    fn fromWkt(g: WktGeometry): GeoJsonGeometry;
    fn toWkt(): WktGeometry;
}

export class GeoJsonFeature {
    id: string,
    geom: WktGeometry,
    has_bbox: bool,
    bbox: WktBbox,
    fn init(id: string, geom: WktGeometry, include_bbox: bool): GeoJsonFeature;
}

export fn gjZeroCoord(): Coord2D;
export fn gjEmptyGeom(kind: u8): WktGeometry;
export fn gjIsSpace(c: u8): bool;
export fn gjSkipWs(s: string, pos: &mut usize);
export fn gjConsumeChar(s: string, pos: &mut usize, ch: u8): bool;
export fn gjWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn gjWriteJsonEscaped(out: []u8, pos: usize, s: string): usize;
export fn gjWriteF64(out: []u8, pos: usize, val: f64): usize;
export fn gjWriteCoord(out: []u8, pos: usize, c: Coord2D): usize;
export fn gjParseNumber(s: string, pos: &mut usize, out: &mut f64): bool;
export fn gjParseStringSlice(s: string, pos: &mut usize, start_out: &mut usize, end_out: &mut usize): bool;
export fn gjSkipValue(s: string, pos: &mut usize, depth: u32): bool;
export fn gjParsePosition(s: string, pos: &mut usize, out: &mut Coord2D): bool;
export fn gjParsePosList(s: string, pos: &mut usize, geom: &mut WktGeometry): bool;
export fn formatGeoJsonGeometry(g: WktGeometry, out: []u8): usize;
export fn formatGeoJsonFeature(f: GeoJsonFeature, out: []u8): usize;
export fn parseGeoJsonGeometry(json: string): WktGeometry?;
export fn parseGeoJsonPoint(json: string): WktGeometry?;
export fn wktToGeoJson(wkt_str: string, out: []u8): usize;
export fn geoJsonToWkt(json: string, out: []u8): usize;
