// Source: cp/cap/test/test_geohash_production_deep.cp
// Deep Production Verification Suite for cap:geohash & cap.geohash
// Covers:
//   1. Classic geographic coordinate vectors (Tokyo, San Francisco, Beijing, Origin, Jutland) encode/decode/decodeBounds roundtrip
//   2. 8-neighbor topology symmetry (e.g. west(east(h)) == h) & +-180 deg antimeridian wrap-around & pole clamping
//   3. 64-bit integer Z-order Morton Geohash (encodeInt/decodeInt) roundtrip, haversineMeters & precisionForRadiusMeters
//   4. Boundary & error guards: out-of-range lat/lon, NaN/Inf, invalid precision (0/13), empty string, non-Base32 chars ('a','i','l','o')
//   5. `import cap from "cap"; cap.geohash` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    EARTH_RADIUS_METERS,
    GeohashError,
    LatLon,
    BoundingBox,
    Direction,
    GeohashCode,
    Neighbors,
    encode,
    encodeAlloc,
    decode,
    decodeBounds,
    neighbor,
    neighbors,
    encodeInt,
    decodeInt,
    haversineMeters,
    precisionForRadiusMeters,
} from "cap:geohash";

fn approxEq(a: f64, b: f64, tol: f64): bool {
    return @abs(a - b) <= tol;
}

test("cap:geohash - classic geographic vectors encode/decode/decodeBounds roundtrip") {
    // 1. Jutland, Denmark canonical precision-11 vector: (57.64911, 10.40744) -> "u4pruydqqvj"
    let jutland = encode(57.64911, 10.40744, 11) catch unreachable;
    expect_eq(jutland.eqlStr("u4pruydqqvj"), true);
    const jutland_pt = decode(jutland.slice()) catch unreachable;
    const jutland_box = decodeBounds(jutland.slice()) catch unreachable;
    expect_eq(jutland_box.contains(57.64911, 10.40744), true);
    expect_eq(approxEq(jutland_pt.lat, 57.64911, jutland_box.lat_err), true);
    expect_eq(approxEq(jutland_pt.lon, 10.40744, jutland_box.lon_err), true);

    // 2. San Francisco (37.7749, -122.4194) -> "9q8yy" at precision 5
    let sf = encode(37.7749, -122.4194, 5) catch unreachable;
    expect_eq(sf.eqlStr("9q8yy"), true);
    const sf_box = decodeBounds("9q8yy") catch unreachable;
    expect_eq(sf_box.contains(37.7749, -122.4194), true);

    // 3. Beijing (39.9042, 116.4074) -> "wx4g0" at precision 5
    let bj = encode(39.9042, 116.4074, 5) catch unreachable;
    expect_eq(bj.eqlStr("wx4g0"), true);
    const bj_box = decodeBounds("wx4g0") catch unreachable;
    expect_eq(bj_box.contains(39.9042, 116.4074), true);

    // 4. Tokyo (35.6762, 139.6503) precision 1..12 roundtrip
    let prec: u8 = 1;
    while (prec <= 12) : (prec += 1) {
        let tk = encode(35.6762, 139.6503, prec) catch unreachable;
        expect_eq(tk.len, prec);
        const tk_box = decodeBounds(tk.slice()) catch unreachable;
        const tk_pt = decode(tk.slice()) catch unreachable;
        expect_eq(tk_box.contains(35.6762, 139.6503), true);
        expect_eq(approxEq(tk_pt.lat, tk_box.center_lat, 1e-12), true);
        expect_eq(approxEq(tk_pt.lon, tk_box.center_lon, 1e-12), true);
    }

    // 5. Origin (0.0, 0.0) -> "s0000" at precision 5
    let origin = encode(0.0, 0.0, 5) catch unreachable;
    expect_eq(origin.eqlStr("s0000"), true);
    const origin_box = decodeBounds(origin.slice()) catch unreachable;
    expect_eq(origin_box.contains(0.0, 0.0), true);
}

test("cap:geohash - 8-neighbor topology symmetry & +-180 deg antimeridian wrap-around") {
    const center = "ezs42";
    let nb = neighbors(center) catch unreachable;

    // Canonical 8-neighbor values for "ezs42"
    expect_eq(nb.n.eqlStr("ezs48"), true);
    expect_eq(nb.ne.eqlStr("ezs49"), true);
    expect_eq(nb.e.eqlStr("ezs43"), true);
    expect_eq(nb.se.eqlStr("ezs41"), true);
    expect_eq(nb.s.eqlStr("ezs40"), true);
    expect_eq(nb.sw.eqlStr("ezefp"), true);
    expect_eq(nb.w.eqlStr("ezefr"), true);
    expect_eq(nb.nw.eqlStr("ezefx"), true);

    const all_slices = nb.all();
    expect_eq(all_slices.len, 8);
    expect_eq(std.mem.eql(u8, all_slices[0], "ezs48"), true);
    expect_eq(std.mem.eql(u8, all_slices[2], "ezs43"), true);
    expect_eq(std.mem.eql(u8, all_slices[4], "ezs40"), true);
    expect_eq(std.mem.eql(u8, all_slices[6], "ezefr"), true);

    // Opposite-direction symmetry checks
    let e_then_w = neighbor(nb.e.slice(), .West) catch unreachable;
    expect_eq(e_then_w.eqlStr(center), true);
    let w_then_e = neighbor(nb.w.slice(), .East) catch unreachable;
    expect_eq(w_then_e.eqlStr(center), true);
    let n_then_s = neighbor(nb.n.slice(), .South) catch unreachable;
    expect_eq(n_then_s.eqlStr(center), true);
    let s_then_n = neighbor(nb.s.slice(), .North) catch unreachable;
    expect_eq(s_then_n.eqlStr(center), true);
    let ne_then_sw = neighbor(nb.ne.slice(), .SouthWest) catch unreachable;
    expect_eq(ne_then_sw.eqlStr(center), true);
    let nw_then_se = neighbor(nb.nw.slice(), .SouthEast) catch unreachable;
    expect_eq(nw_then_se.eqlStr(center), true);

    // Antimeridian (+-180 deg) wrap-around symmetry:
    // East edge at +180 deg ("zzzzz") wraps to -180 deg ("bpbpb") and back
    let east_edge = "zzzzz";
    let wrapped_e = neighbor(east_edge, .East) catch unreachable;
    const wrapped_e_box = decodeBounds(wrapped_e.slice()) catch unreachable;
    expect_eq(wrapped_e_box.min_lon < -179.0, true);
    let back_to_east = neighbor(wrapped_e.slice(), .West) catch unreachable;
    expect_eq(back_to_east.eqlStr(east_edge), true);

    // West edge at -180 deg ("80000") wraps to +180 deg and back
    let west_edge = "80000";
    let wrapped_w = neighbor(west_edge, .West) catch unreachable;
    const wrapped_w_box = decodeBounds(wrapped_w.slice()) catch unreachable;
    expect_eq(wrapped_w_box.max_lon > 179.0, true);
    let back_to_west = neighbor(wrapped_w.slice(), .East) catch unreachable;
    expect_eq(back_to_west.eqlStr(west_edge), true);

    // North pole clamping: North neighbor of "zzzzz" stays in northern hemisphere
    let north_pole_n = neighbor("zzzzz", .North) catch unreachable;
    const np_box = decodeBounds(north_pole_n.slice()) catch unreachable;
    expect_eq(np_box.min_lat > 80.0, true);
}

test("cap:geohash - 64-bit integer Geohash roundtrip, haversineMeters & precisionForRadiusMeters") {
    // 1. 64-bit integer Z-order Morton roundtrip across multiple bit depths
    const depths: [4]u6 = [ 10, 26, 52, 60 ];
    const lat = 35.6762;
    const lon = 139.6503;

    for (d of depths) {
        const bits = encodeInt(lat, lon, d) catch unreachable;
        const box = decodeInt(bits, d) catch unreachable;
        expect_eq(box.contains(lat, lon), true);
        expect_eq(approxEq(box.center_lat, lat, box.lat_err), true);
        expect_eq(approxEq(box.center_lon, lon, box.lon_err), true);
    }

    // 2. Z-order spatial monotonicity check: nearby points share higher prefix bits
    const b1 = encodeInt(37.7749, -122.4194, 52) catch unreachable;
    const b2 = encodeInt(37.7750, -122.4195, 52) catch unreachable;
    const b_far = encodeInt(-33.8688, 151.2093, 52) catch unreachable;
    expect_eq((b1 >> 32) == (b2 >> 32), true);
    expect_eq((b1 >> 32) != (b_far >> 32), true);

    // 3. Haversine distance checks (Earth radius 6371000.0 m)
    expect_eq(EARTH_RADIUS_METERS == 6371000.0, true);
    const beijing = LatLon.init(39.9042, 116.4074) catch unreachable;
    const shanghai = LatLon.init(31.2304, 121.4737) catch unreachable;
    expect_eq(haversineMeters(beijing, beijing) == 0.0, true);

    const bj_sh = haversineMeters(beijing, shanghai);
    expect_eq(bj_sh > 1060000.0 && bj_sh < 1075000.0, true);

    const eq_a = LatLon.init(0.0, 0.0) catch unreachable;
    const eq_b = LatLon.init(0.0, 1.0) catch unreachable;
    const one_deg_eq = haversineMeters(eq_a, eq_b);
    expect_eq(approxEq(one_deg_eq, 111194.9, 50.0), true);

    // 4. precisionForRadiusMeters monotonicity & range 1..12
    expect_eq(precisionForRadiusMeters(5000000.0), 1);
    expect_eq(precisionForRadiusMeters(5000.0), 5);
    expect_eq(precisionForRadiusMeters(1000.0), 6);
    expect_eq(precisionForRadiusMeters(25.0), 8);
    expect_eq(precisionForRadiusMeters(1.0), 10);
    expect_eq(precisionForRadiusMeters(0.01), 12);
    expect_eq(precisionForRadiusMeters(0.0), 12);
}

test("cap:geohash - error guards on invalid coordinates, precision, empty & non-Base32 chars") {
    // Invalid coordinates
    let got_oob_lat = false;
    * = encode(90.001, 0.0, 5) catch |e| {
        if (e == GeohashError.InvalidCoordinate) got_oob_lat = true;
    };
    expect_eq(got_oob_lat, true);

    let got_oob_lon = false;
    * = encode(0.0, -180.001, 5) catch |e| {
        if (e == GeohashError.InvalidCoordinate) got_oob_lon = true;
    };
    expect_eq(got_oob_lon, true);

    let got_nan = false;
    * = LatLon.init(std.math.nan(f64), 0.0) catch |e| {
        if (e == GeohashError.InvalidCoordinate) got_nan = true;
    };
    expect_eq(got_nan, true);

    // Invalid precision (0 and 13)
    let got_prec0 = false;
    * = encode(0.0, 0.0, 0) catch |e| {
        if (e == GeohashError.InvalidPrecision) got_prec0 = true;
    };
    expect_eq(got_prec0, true);

    let got_prec13 = false;
    * = encode(0.0, 0.0, 13) catch |e| {
        if (e == GeohashError.InvalidPrecision) got_prec13 = true;
    };
    expect_eq(got_prec13, true);

    let got_int_prec0 = false;
    * = encodeInt(0.0, 0.0, 0) catch |e| {
        if (e == GeohashError.InvalidPrecision) got_int_prec0 = true;
    };
    expect_eq(got_int_prec0, true);

    // Empty geohash
    let got_empty_dec = false;
    * = decode("") catch |e| {
        if (e == GeohashError.EmptyGeohash) got_empty_dec = true;
    };
    expect_eq(got_empty_dec, true);

    let got_empty_nb = false;
    * = neighbors("") catch |e| {
        if (e == GeohashError.EmptyGeohash) got_empty_nb = true;
    };
    expect_eq(got_empty_nb, true);

    // Non-Base32 characters: 'a', 'i', 'l', 'o', and uppercase 'E'
    const bad_hashes: [5]string = [ "ezsa2", "ezsi2", "ezsl2", "ezso2", "Ezs42" ];
    for (bad of bad_hashes) {
        let got_bad_char = false;
        * = decodeBounds(bad) catch |e| {
            if (e == GeohashError.InvalidGeohashChar) got_bad_char = true;
        };
        expect_eq(got_bad_char, true);
    }
}

test("cap.geohash - dynamic mounting via import cap from \"cap\" & std.testing.allocator zero-leak check") {
    const alloc = std.testing.allocator;

    // Heap-allocated slice via encodeAlloc must be freed cleanly with zero leaks
    const heap_hash = cap.geohash.encodeAlloc(alloc, 37.7749, -122.4194, 7) catch unreachable;
    defer alloc.free(heap_hash);
    expect_eq(heap_hash.len, 7);

    // Zero-allocation stack encode/decode/neighbors via cap.geohash
    let code = cap.geohash.encode(37.7749, -122.4194, 5) catch unreachable;
    expect_eq(code.eqlStr("9q8yy"), true);

    const decoded = cap.geohash.decode(code.slice()) catch unreachable;
    const bounds = cap.geohash.decodeBounds(code.slice()) catch unreachable;
    expect_eq(bounds.contains(decoded.lat, decoded.lon), true);

    let nb = cap.geohash.neighbors("ezs42") catch unreachable;
    expect_eq(nb.e.eqlStr("ezs43"), true);

    const bits = cap.geohash.encodeInt(39.9042, 116.4074, 52) catch unreachable;
    const int_box = cap.geohash.decodeInt(bits, 52) catch unreachable;
    expect_eq(int_box.contains(39.9042, 116.4074), true);

    const p1 = cap.geohash.LatLon.init(0.0, 0.0) catch unreachable;
    const p2 = cap.geohash.LatLon.init(0.0, 1.0) catch unreachable;
    expect_eq(cap.geohash.haversineMeters(p1, p2) > 111000.0, true);
    expect_eq(cap.geohash.precisionForRadiusMeters(1000.0), 6);
}
