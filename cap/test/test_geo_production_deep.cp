// Source: cp/cap/test/test_geo_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:geo & cap.geo

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    EARTH_RADIUS_M,
    GeoError,
    LatLng,
    BBox,
    Geo,
    GeohashDecoded,
    GeohashNeighbors,
    Geohash,
    GeoPointEntry,
    SpatialIndex,
} from "cap:geo";

fn testHaversineBearingAndDestination(): !void {
    expect_eq(@abs(EARTH_RADIUS_M - 6371008.8) < 0.001, true);

    // Invalid coordinate guards: out-of-range lat/lng, NaN, Inf
    let bad_lat = false;
    * = LatLng.init(90.001, 0.0) catch {
        bad_lat = true;
    };
    expect_eq(bad_lat, true);

    let bad_lng = false;
    * = LatLng.init(0.0, -180.5) catch {
        bad_lng = true;
    };
    expect_eq(bad_lng, true);

    let bad_nan = false;
    * = LatLng.init(std.math.nan(f64), 10.0) catch {
        bad_nan = true;
    };
    expect_eq(bad_nan, true);

    let bad_inf = false;
    * = LatLng.init(10.0, std.math.inf(f64)) catch {
        bad_inf = true;
    };
    expect_eq(bad_inf, true);

    // Real-world city geodesic distances
    const beijing = LatLng.init(39.9042, 116.4074)?;
    const shanghai = LatLng.init(31.2304, 121.4737)?;
    const bj_sh_m = Geo.haversineMeters(beijing, shanghai);
    // ~1,068 km
    expect_eq(bj_sh_m > 1065000.0 && bj_sh_m < 1071000.0, true);
    expect_eq(@abs(bj_sh_m - Geo.haversineMeters(shanghai, beijing)) < 0.001, true);
    expect_eq(Geo.haversineMeters(beijing, beijing), 0.0);

    const new_york = LatLng.init(40.7128, -74.0060)?;
    const london = LatLng.init(51.5074, -0.1278)?;
    const ny_ldn_m = Geo.haversineMeters(new_york, london);
    // ~5,570 km
    expect_eq(ny_ldn_m > 5560000.0 && ny_ldn_m < 5580000.0, true);

    // Cardinal bearings along equator & meridian
    const eq_origin = LatLng.init(0.0, 0.0)?;
    const eq_north = LatLng.init(1.0, 0.0)?;
    const eq_east = LatLng.init(0.0, 1.0)?;
    const eq_south = LatLng.init(-1.0, 0.0)?;
    const eq_west = LatLng.init(0.0, -1.0)?;

    expect_eq(@abs(Geo.initialBearingDeg(eq_origin, eq_north) - 0.0) < 0.001, true);
    expect_eq(@abs(Geo.initialBearingDeg(eq_origin, eq_east) - 90.0) < 0.001, true);
    expect_eq(@abs(Geo.initialBearingDeg(eq_origin, eq_south) - 180.0) < 0.001, true);
    expect_eq(@abs(Geo.initialBearingDeg(eq_origin, eq_west) - 270.0) < 0.001, true);

    const bj_sh_bearing = Geo.initialBearingDeg(beijing, shanghai);
    expect_eq(bj_sh_bearing > 150.0 && bj_sh_bearing < 157.0, true);

    // Destination point round-trip verification
    const target_dist_m: f64 = 50000.0;
    const target_bearing: f64 = 62.5;
    const projected = Geo.destinationPoint(beijing, target_bearing, target_dist_m);
    expect_eq(projected.isValid(), true);
    const roundtrip_dist = Geo.haversineMeters(beijing, projected);
    const roundtrip_bearing = Geo.initialBearingDeg(beijing, projected);
    expect_eq(@abs(roundtrip_dist - target_dist_m) < 0.01, true);
    expect_eq(@abs(roundtrip_bearing - target_bearing) < 0.001, true);

    // Antimeridian longitude wrap check (+179.5 deg going due East ~111.2 km -> ~-179.5 deg)
    const near_antimeridian = LatLng.init(0.0, 179.5)?;
    const crossed = Geo.destinationPoint(near_antimeridian, 90.0, 111195.0);
    expect_eq(crossed.isValid(), true);
    expect_eq(crossed.lng < -179.0 && crossed.lng > -180.0, true);
}

fn testBBoxAndPolygonGeofence(): !void {
    let bad_bbox = false;
    * = BBox.init(40.0, 30.0, 100.0, 110.0) catch {
        bad_bbox = true;
    };
    expect_eq(bad_bbox, true);

    let box_a = BBox.init(39.8, 40.0, 116.2, 116.5)?;
    const inside_pt = LatLng.init(39.9, 116.35)?;
    const border_pt = LatLng.init(39.8, 116.2)?;
    const outside_pt = LatLng.init(40.2, 116.8)?;

    expect_eq(box_a.contains(inside_pt), true);
    expect_eq(box_a.contains(border_pt), true);
    expect_eq(box_a.contains(outside_pt), false);

    const box_overlap = BBox.init(39.95, 40.1, 116.4, 116.6)?;
    const box_disjoint = BBox.init(30.0, 31.0, 120.0, 121.0)?;
    expect_eq(box_a.intersects(box_overlap), true);
    expect_eq(box_a.intersects(box_disjoint), false);

    box_a.expand(outside_pt);
    expect_eq(box_a.contains(outside_pt), true);
    expect_eq(box_a.max_lat, 40.2);
    expect_eq(box_a.max_lng, 116.8);

    // Concave U-shaped geofence polygon (open ring)
    const u_shape: [8]LatLng = [
        LatLng.init(0.0, 0.0)?,
        LatLng.init(0.0, 10.0)?,
        LatLng.init(10.0, 10.0)?,
        LatLng.init(10.0, 7.0)?,
        LatLng.init(3.0, 7.0)?,
        LatLng.init(3.0, 3.0)?,
        LatLng.init(10.0, 3.0)?,
        LatLng.init(10.0, 0.0)?,
    ];

    // Inside left pillar, right pillar, and bottom bridge
    expect_eq(Geo.pointInPolygon(LatLng.init(7.0, 1.5)?, u_shape[0..]), true);
    expect_eq(Geo.pointInPolygon(LatLng.init(7.0, 8.5)?, u_shape[0..]), true);
    expect_eq(Geo.pointInPolygon(LatLng.init(1.5, 5.0)?, u_shape[0..]), true);
    // Inside concave notch -> MUST be outside polygon!
    expect_eq(Geo.pointInPolygon(LatLng.init(6.0, 5.0)?, u_shape[0..]), false);
    // Far outside
    expect_eq(Geo.pointInPolygon(LatLng.init(12.0, 5.0)?, u_shape[0..]), false);
    // On vertex & boundary edge
    expect_eq(Geo.pointInPolygon(LatLng.init(0.0, 0.0)?, u_shape[0..]), true);
    expect_eq(Geo.pointInPolygon(LatLng.init(5.0, 10.0)?, u_shape[0..]), true);

    // Degenerate polygon (< 3 vertices)
    const line_ring: [2]LatLng = [
        LatLng.init(0.0, 0.0)?,
        LatLng.init(1.0, 1.0)?,
    ];
    expect_eq(Geo.pointInPolygon(LatLng.init(0.5, 0.5)?, line_ring[0..]), false);
}

fn testGeohashEncodeDecodeAndNeighbors(): !void {
    const alloc = std.heap.page_allocator;

    // Authoritative vector 1: (42.6, -5.6) at precision 5 => "ezs42"
    const pt_ezs = LatLng.init(42.6, -5.6)?;
    const h_ezs = Geohash.encode(alloc, pt_ezs, 5)?;
    defer alloc.free(h_ezs);
    expect_eq(std.mem.eql(u8, h_ezs, "ezs42"), true);

    const dec_ezs = Geohash.decode("ezs42")?;
    expect_eq(dec_ezs.bbox.contains(pt_ezs), true);
    expect_eq(@abs(dec_ezs.center.lat - pt_ezs.lat) <= dec_ezs.lat_err, true);
    expect_eq(@abs(dec_ezs.center.lng - pt_ezs.lng) <= dec_ezs.lng_err, true);

    // Authoritative vector 2: Beijing (39.9042, 116.4074) at precision 6 => "wx4g0b"
    const pt_bj = LatLng.init(39.9042, 116.4074)?;
    const h_bj = Geohash.encode(alloc, pt_bj, 6)?;
    defer alloc.free(h_bj);
    expect_eq(std.mem.eql(u8, h_bj, "wx4g0b"), true);

    const dec_bj = Geohash.decode("wx4g0b")?;
    expect_eq(dec_bj.bbox.contains(pt_bj), true);
    expect_eq(dec_bj.lat_err > 0.0 && dec_bj.lng_err > 0.0, true);

    // Precision sweep 1..12 round-trip
    let p: u8 = 1;
    while (p <= 12) : (p += 1) {
        const enc = Geohash.encode(alloc, pt_bj, p)?;
        defer alloc.free(enc);
        expect_eq(enc.len, p as usize);
        const decoded = Geohash.decode(enc)?;
        expect_eq(decoded.bbox.contains(pt_bj), true);
    }

    // Precision & invalid geohash error guards
    let err_prec0 = false;
    * = Geohash.encode(alloc, pt_bj, 0) catch {
        err_prec0 = true;
    };
    expect_eq(err_prec0, true);

    let err_prec13 = false;
    * = Geohash.encode(alloc, pt_bj, 13) catch {
        err_prec13 = true;
    };
    expect_eq(err_prec13, true);

    let err_empty = false;
    * = Geohash.decode("") catch {
        err_empty = true;
    };
    expect_eq(err_empty, true);

    let err_bad_char = false;
    * = Geohash.decode("ezs4a") catch {
        err_bad_char = true;
    };
    expect_eq(err_bad_char, true);

    // 8-neighbor verification for odd-length "ezs42"
    let nb = Geohash.neighbors(alloc, "ezs42")?;
    defer nb.deinit(alloc);
    expect_eq(std.mem.eql(u8, nb.n, "ezs48"), true);
    expect_eq(std.mem.eql(u8, nb.ne, "ezs49"), true);
    expect_eq(std.mem.eql(u8, nb.e, "ezs43"), true);
    expect_eq(std.mem.eql(u8, nb.se, "ezs41"), true);
    expect_eq(std.mem.eql(u8, nb.s, "ezs40"), true);
    expect_eq(std.mem.eql(u8, nb.sw, "ezefp"), true);
    expect_eq(std.mem.eql(u8, nb.w, "ezefr"), true);
    expect_eq(std.mem.eql(u8, nb.nw, "ezefx"), true);

    // Even-length 8-neighbor geometry consistency for "wx4g0b"
    let nb_bj = Geohash.neighbors(alloc, "wx4g0b")?;
    defer nb_bj.deinit(alloc);
    const dec_n = Geohash.decode(nb_bj.n)?;
    const dec_s = Geohash.decode(nb_bj.s)?;
    const dec_e = Geohash.decode(nb_bj.e)?;
    const dec_w = Geohash.decode(nb_bj.w)?;
    expect_eq(dec_n.center.lat > dec_bj.center.lat, true);
    expect_eq(dec_s.center.lat < dec_bj.center.lat, true);
    expect_eq(dec_e.center.lng > dec_bj.center.lng, true);
    expect_eq(dec_w.center.lng < dec_bj.center.lng, true);
    expect_eq(dec_bj.bbox.intersects(dec_n.bbox), true);
    expect_eq(dec_bj.bbox.intersects(dec_w.bbox), true);
}

fn testSpatialIndexRadiusNearestAndRemove(): !void {
    const alloc = std.heap.page_allocator;
    let index = SpatialIndex.init(alloc);
    defer index.deinit();

    const origin = LatLng.init(39.9042, 116.4074)?;

    // Ownership-safe ID copy check
    let id_buf: [16]u8 = undefined;
    @memcpy(id_buf[0..5], "poi_a");
    index.upsert(id_buf[0..5], Geo.destinationPoint(origin, 0.0, 500.0))?;
    @memcpy(id_buf[0..5], "XXXXX");

    index.upsert("poi_b", Geo.destinationPoint(origin, 90.0, 1200.0))?;
    index.upsert("poi_c", Geo.destinationPoint(origin, 180.0, 250.0))?;
    index.upsert("poi_d", Geo.destinationPoint(origin, 270.0, 5000.0))?;
    expect_eq(index.len(), 4);

    // Upsert update moves "poi_b" closer (100m) without increasing len()
    index.upsert("poi_b", Geo.destinationPoint(origin, 90.0, 100.0))?;
    expect_eq(index.len(), 4);

    // Radius query within 600m -> poi_b (100m), poi_c (250m), poi_a (500m)
    const within = index.withinRadius(alloc, origin, 600.0)?;
    defer alloc.free(within);
    expect_eq(within.len, 3);
    expect_eq(std.mem.eql(u8, within[0].id, "poi_b"), true);
    expect_eq(std.mem.eql(u8, within[1].id, "poi_c"), true);
    expect_eq(std.mem.eql(u8, within[2].id, "poi_a"), true);
    expect_eq(within[0].distance_m < within[1].distance_m, true);
    expect_eq(within[1].distance_m < within[2].distance_m, true);
    expect_eq(@abs(within[0].distance_m - 100.0) < 0.5, true);

    // nearestK(2) -> poi_b, poi_c
    const top2 = index.nearestK(alloc, origin, 2)?;
    defer alloc.free(top2);
    expect_eq(top2.len, 2);
    expect_eq(std.mem.eql(u8, top2[0].id, "poi_b"), true);
    expect_eq(std.mem.eql(u8, top2[1].id, "poi_c"), true);

    // Remove middle entry & absent entry check
    expect_eq(index.remove("poi_c"), true);
    expect_eq(index.remove("poi_c"), false);
    expect_eq(index.len(), 3);

    // nearestK(10) when k > len() returns all 3 remaining entries sorted
    const all_rem = index.nearestK(alloc, origin, 10)?;
    defer alloc.free(all_rem);
    expect_eq(all_rem.len, 3);
    expect_eq(std.mem.eql(u8, all_rem[0].id, "poi_b"), true);
    expect_eq(std.mem.eql(u8, all_rem[1].id, "poi_a"), true);
    expect_eq(std.mem.eql(u8, all_rem[2].id, "poi_d"), true);
}

fn testCapGeoAutomaticLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    const a = cap.geo.LatLng.init(39.9042, 116.4074)?;
    const b = cap.geo.LatLng.init(31.2304, 121.4737)?;
    const d = cap.geo.Geo.haversineMeters(a, b);
    expect_eq(d > 1065000.0 && d < 1071000.0, true);

    const hash = cap.geo.Geohash.encode(alloc, a, 6)?;
    defer alloc.free(hash);
    expect_eq(std.mem.eql(u8, hash, "wx4g0b"), true);

    const decoded: cap.geo.GeohashDecoded = cap.geo.Geohash.decode(hash)?;
    expect_eq(decoded.bbox.contains(a), true);

    let nb: cap.geo.GeohashNeighbors = cap.geo.Geohash.neighbors(alloc, "ezs42")?;
    defer nb.deinit(alloc);
    expect_eq(std.mem.eql(u8, nb.n, "ezs48"), true);

    let idx = cap.geo.SpatialIndex.init(alloc);
    defer idx.deinit();
    idx.upsert("shanghai", b)?;
    const hits = idx.withinRadius(alloc, a, d + 100.0)?;
    defer alloc.free(hits);
    expect_eq(hits.len, 1);
    expect_eq(std.mem.eql(u8, hits[0].id, "shanghai"), true);
}

fn main() {
    test("1. LatLng Validation, Haversine City Distances, Bearing & Destination Round-Trip", () => {
        testHaversineBearingAndDestination()?;
    });
    test("2. BBox Operations & Concave Polygon Point-in-Polygon Geofence", () => {
        testBBoxAndPolygonGeofence()?;
    });
    test("3. Geohash Encode/Decode Test Vectors (ezs42 / wx4g0b) & 8-Neighbor Grid", () => {
        testGeohashEncodeDecodeAndNeighbors()?;
    });
    test("4. SpatialIndex Upsert, Swap-Remove, withinRadius & nearestK Ranking", () => {
        testSpatialIndexRadiusNearestAndRemove()?;
    });
    test("5. cap.geo Automatic Dynamic Lazy Mounting Verification", () => {
        testCapGeoAutomaticLazyMount()?;
    });
}
