// Source: cp/lib/geo/geohash.cp (Base32 Geohash Encoder, Decoder & 8-Neighbor Lookup)
// Pure CP Base32 Geohash encoder (1..12 precision), bounding-box decoder,
// and exact 8-direction neighbor grid lookup.

const std = @import("std");
import { GeoError, LatLng, BBox } from "./coord.cp";

const BASE32_ALPHABET: string = "0123456789bcdefghjkmnpqrstuvwxyz";

// Standard Niemeyer Geohash neighbor and border lookup tables indexed by parity (0 = even len, 1 = odd len).
const NEIGHBOR_N_EVEN: string = "p0r21436x8zb9dcf5h7kjnmqesgutwvy";
const NEIGHBOR_N_ODD: string = "bc01fg45238967deuvhjyznpkmstqrwx";
const NEIGHBOR_S_EVEN: string = "14365h7k9dcfesgujnmqp0r2twvyx8zb";
const NEIGHBOR_S_ODD: string = "238967debc01fg45kmstqrwxuvhjyznp";
const NEIGHBOR_E_EVEN: string = "bc01fg45238967deuvhjyznpkmstqrwx";
const NEIGHBOR_E_ODD: string = "p0r21436x8zb9dcf5h7kjnmqesgutwvy";
const NEIGHBOR_W_EVEN: string = "238967debc01fg45kmstqrwxuvhjyznp";
const NEIGHBOR_W_ODD: string = "14365h7k9dcfesgujnmqp0r2twvyx8zb";

const BORDER_N_EVEN: string = "prxz";
const BORDER_N_ODD: string = "bcfguvyz";
const BORDER_S_EVEN: string = "028b";
const BORDER_S_ODD: string = "0145hjnp";
const BORDER_E_EVEN: string = "bcfguvyz";
const BORDER_E_ODD: string = "prxz";
const BORDER_W_EVEN: string = "0145hjnp";
const BORDER_W_ODD: string = "028b";

enum CardinalDir {
    North,
    South,
    East,
    West,
}

fn base32Index(ch: u8): usize? {
    let i: usize = 0;
    while (i < BASE32_ALPHABET.len) : (i += 1) {
        if (BASE32_ALPHABET[i] == ch) return i;
    }
    return null;
}

fn lookupIndexIn(table: string, ch: u8): usize? {
    let i: usize = 0;
    while (i < table.len) : (i += 1) {
        if (table[i] == ch) return i;
    }
    return null;
}

fn containsChar(set: string, ch: u8): bool {
    return lookupIndexIn(set, ch) != null;
}

fn neighborTable(dir: CardinalDir, is_odd: bool): string {
    return switch (dir) {
        .North => if (is_odd) NEIGHBOR_N_ODD else NEIGHBOR_N_EVEN,
        .South => if (is_odd) NEIGHBOR_S_ODD else NEIGHBOR_S_EVEN,
        .East => if (is_odd) NEIGHBOR_E_ODD else NEIGHBOR_E_EVEN,
        .West => if (is_odd) NEIGHBOR_W_ODD else NEIGHBOR_W_EVEN,
    };
}

fn borderTable(dir: CardinalDir, is_odd: bool): string {
    return switch (dir) {
        .North => if (is_odd) BORDER_N_ODD else BORDER_N_EVEN,
        .South => if (is_odd) BORDER_S_ODD else BORDER_S_EVEN,
        .East => if (is_odd) BORDER_E_ODD else BORDER_E_EVEN,
        .West => if (is_odd) BORDER_W_ODD else BORDER_W_EVEN,
    };
}

fn writeAdjacentInPlace(dst: []u8, src: string, dir: CardinalDir): void ! GeoError {
    if (src.len == 0) return GeoError.InvalidGeohash;
    const last_idx = src.len - 1;
    const last_ch = src[last_idx];
    const is_odd = (src.len % 2) == 1;

    const border = borderTable(dir, is_odd);
    const n_table = neighborTable(dir, is_odd);

    if (src.len > 1 && containsChar(border, last_ch)) {
        writeAdjacentInPlace(dst[0..last_idx], src[0..last_idx], dir)?;
    } else {
        if (last_idx > 0) {
            @memcpy(dst[0..last_idx], src[0..last_idx]);
        }
    }

    const pos_opt = lookupIndexIn(n_table, last_ch);
    if (pos_opt == null) return GeoError.InvalidGeohash;
    const pos = pos_opt.?;
    dst[last_idx] = BASE32_ALPHABET[pos];
}

fn validateHash(hash: string): void ! GeoError {
    if (hash.len == 0) return GeoError.InvalidGeohash;
    for (ch of hash) {
        if (base32Index(ch) == null) {
            return GeoError.InvalidGeohash;
        }
    }
}

/// Decoded Geohash cell center, bounding box, and half-cell latitude/longitude error margins.
pub struct GeohashDecoded {
    pub center: LatLng = LatLng{},
    pub bbox: BBox = BBox{},
    pub lat_err: f64 = 0.0;
    pub lng_err: f64 = 0.0;
}

/// All 8 surrounding Base32 Geohash cells (`n`, `ne`, `e`, `se`, `s`, `sw`, `w`, `nw`).
/// Call `deinit(alloc)` to release the 8 owned cell strings.
pub struct GeohashNeighbors {
    pub n: []u8 = [],
    pub ne: []u8 = [],
    pub e: []u8 = [],
    pub se: []u8 = [],
    pub s: []u8 = [],
    pub sw: []u8 = [],
    pub w: []u8 = [],
    pub nw: []u8 = [],

    /// Releases all 8 neighbor geohash strings allocated by `Geohash.neighbors`.
    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.n.len > 0) alloc.free(_.n);
        if (_.ne.len > 0) alloc.free(_.ne);
        if (_.e.len > 0) alloc.free(_.e);
        if (_.se.len > 0) alloc.free(_.se);
        if (_.s.len > 0) alloc.free(_.s);
        if (_.sw.len > 0) alloc.free(_.sw);
        if (_.w.len > 0) alloc.free(_.w);
        if (_.nw.len > 0) alloc.free(_.nw);
        _.n = [];
        _.ne = [];
        _.e = [];
        _.se = [];
        _.s = [];
        _.sw = [];
        _.w = [];
        _.nw = [];
    }
}

/// Standard Base32 Geohash encoder, decoder, and 8-direction neighbor locator.
pub static struct Geohash {
    /// Encodes `pt` into a Base32 Geohash string of length `precision` (`1..12`).
    /// Caller owns the returned slice (`alloc.free(hash)`).
    pub static fn encode(alloc: std.mem.Allocator, pt: LatLng, precision: u8): ([]u8) ! GeoError {
        if (!pt.isValid()) return GeoError.InvalidCoordinate;
        if (precision < 1 || precision > 12) return GeoError.InvalidPrecision;

        const prec = precision as usize;
        let out = alloc.alloc(u8, prec) catch return GeoError.OutOfMemory;
        errdefer alloc.free(out);

        let min_lat: f64 = -90.0;
        let max_lat: f64 = 90.0;
        let min_lng: f64 = -180.0;
        let max_lng: f64 = 180.0;

        let is_lng = true;
        let bit_idx: u5 = 0;
        let ch_idx: usize = 0;
        let out_idx: usize = 0;

        while (out_idx < prec) {
            if (is_lng) {
                const mid = (min_lng + max_lng) * 0.5;
                if (pt.lng >= mid) {
                    ch_idx = (ch_idx << 1) | 1;
                    min_lng = mid;
                } else {
                    ch_idx = ch_idx << 1;
                    max_lng = mid;
                }
            } else {
                const mid = (min_lat + max_lat) * 0.5;
                if (pt.lat >= mid) {
                    ch_idx = (ch_idx << 1) | 1;
                    min_lat = mid;
                } else {
                    ch_idx = ch_idx << 1;
                    max_lat = mid;
                }
            }

            is_lng = !is_lng;
            bit_idx += 1;

            if (bit_idx == 5) {
                out[out_idx] = BASE32_ALPHABET[ch_idx];
                out_idx += 1;
                bit_idx = 0;
                ch_idx = 0;
            }
        }

        return out;
    }

    /// Decodes a Base32 Geohash string into its center coordinate, bounding box, and error margins.
    pub static fn decode(hash: string): GeohashDecoded ! GeoError {
        if (hash.len == 0) return GeoError.InvalidGeohash;

        let min_lat: f64 = -90.0;
        let max_lat: f64 = 90.0;
        let min_lng: f64 = -180.0;
        let max_lng: f64 = 180.0;
        let is_lng = true;

        for (ch of hash) {
            const val_opt = base32Index(ch);
            if (val_opt == null) return GeoError.InvalidGeohash;
            const val = val_opt.?;
            let shift: u3 = 4;
            while (true) {
                const bit = (val >> shift) & 1;
                if (is_lng) {
                    const mid = (min_lng + max_lng) * 0.5;
                    if (bit == 1) {
                        min_lng = mid;
                    } else {
                        max_lng = mid;
                    }
                } else {
                    const mid = (min_lat + max_lat) * 0.5;
                    if (bit == 1) {
                        min_lat = mid;
                    } else {
                        max_lat = mid;
                    }
                }
                is_lng = !is_lng;
                if (shift == 0) break;
                shift -= 1;
            }
        }

        const lat_err = (max_lat - min_lat) * 0.5;
        const lng_err = (max_lng - min_lng) * 0.5;
        const bbox = BBox {
            min_lat: min_lat,
            max_lat: max_lat,
            min_lng: min_lng,
            max_lng: max_lng,
        };
        return GeohashDecoded {
            center: bbox.center(),
            bbox: bbox,
            lat_err: lat_err,
            lng_err: lng_err,
        };
    }

    /// Computes all 8 neighboring Geohash cell codes around `hash` at the same precision.
    /// Caller owns the returned struct and must call `neighbors.deinit(alloc)`.
    pub static fn neighbors(alloc: std.mem.Allocator, hash: string): GeohashNeighbors ! GeoError {
        validateHash(hash)?;
        const len = hash.len;

        let n = alloc.alloc(u8, len) catch return GeoError.OutOfMemory;
        errdefer alloc.free(n);
        writeAdjacentInPlace(n, hash, .North)?;

        let s = alloc.alloc(u8, len) catch return GeoError.OutOfMemory;
        errdefer alloc.free(s);
        writeAdjacentInPlace(s, hash, .South)?;

        let e = alloc.alloc(u8, len) catch return GeoError.OutOfMemory;
        errdefer alloc.free(e);
        writeAdjacentInPlace(e, hash, .East)?;

        let w = alloc.alloc(u8, len) catch return GeoError.OutOfMemory;
        errdefer alloc.free(w);
        writeAdjacentInPlace(w, hash, .West)?;

        let ne = alloc.alloc(u8, len) catch return GeoError.OutOfMemory;
        errdefer alloc.free(ne);
        writeAdjacentInPlace(ne, n, .East)?;

        let nw = alloc.alloc(u8, len) catch return GeoError.OutOfMemory;
        errdefer alloc.free(nw);
        writeAdjacentInPlace(nw, n, .West)?;

        let se = alloc.alloc(u8, len) catch return GeoError.OutOfMemory;
        errdefer alloc.free(se);
        writeAdjacentInPlace(se, s, .East)?;

        let sw = alloc.alloc(u8, len) catch return GeoError.OutOfMemory;
        errdefer alloc.free(sw);
        writeAdjacentInPlace(sw, s, .West)?;

        return GeohashNeighbors {
            n: n,
            ne: ne,
            e: e,
            se: se,
            s: s,
            sw: sw,
            w: w,
            nw: nw,
        };
    }
}
