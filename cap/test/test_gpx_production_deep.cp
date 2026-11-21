// Source: cp/cap/test/test_gpx_production_deep.cp (Production deep tests for cap:gpx)

import { test, expect_eq } from "cap:test";
import {
    GPX_EARTH_RADIUS_M,
    GpxHeader,
    GpxPoint,
    GpxTrackPoint,
    GpxStats,
    isGpx,
    parseGpxHeader,
    GpxWaypointIter,
    GpxTrackPointIter,
    haversineMeters,
    summarizeGpx,
} from "cap:gpx";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Full GPX 1.1 document parses header, metadata, waypoints, and multi-segment trackpoints", () => {
    const xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
        "<gpx version=\"1.1\" creator=\"CapGPS\">\n" ++
        "  <metadata><name>Morning Run</name><desc>Loop</desc><time>2026-11-21T08:00:00Z</time></metadata>\n" ++
        "  <wpt lat=\"31.2304\" lon=\"121.4737\"><ele>12.5</ele><name>Start</name><sym>Flag</sym></wpt>\n" ++
        "  <trk><name>T1</name>\n" ++
        "    <trkseg>\n" ++
        "      <trkpt lat=\"31.2300\" lon=\"121.4700\"><ele>10.0</ele><time>2026-11-21T08:01:00Z</time></trkpt>\n" ++
        "      <trkpt lat=\"31.2310\" lon=\"121.4700\"><ele>25.0</ele><time>2026-11-21T08:02:00Z</time></trkpt>\n" ++
        "    </trkseg>\n" ++
        "    <trkseg>\n" ++
        "      <trkpt lat=\"31.2400\" lon=\"121.4800\"><ele>15.0</ele></trkpt>\n" ++
        "    </trkseg>\n" ++
        "  </trk>\n" ++
        "</gpx>";

    expect_eq(isGpx(xml), true);
    const hdr = parseGpxHeader(xml) ?? unreachable;
    expect_eq(sliceEq(hdr.version, "1.1"), true);
    expect_eq(sliceEq(hdr.creator, "CapGPS"), true);
    expect_eq(sliceEq(hdr.name, "Morning Run"), true);
    expect_eq(sliceEq(hdr.desc, "Loop"), true);
    expect_eq(sliceEq(hdr.time, "2026-11-21T08:00:00Z"), true);

    let wit = GpxWaypointIter.init(xml);
    const w0 = wit.next() ?? unreachable;
    expect_eq(w0.lat > 31.2303 && w0.lat < 31.2305, true);
    expect_eq(w0.lon > 121.4736 && w0.lon < 121.4738, true);
    expect_eq(w0.has_ele, true);
    expect_eq(w0.ele > 12.4 && w0.ele < 12.6, true);
    expect_eq(sliceEq(w0.name, "Start"), true);
    expect_eq(sliceEq(w0.sym, "Flag"), true);
    expect_eq(wit.next() == null, true);

    let tit = GpxTrackPointIter.init(xml);
    const tp0 = tit.next() ?? unreachable;
    expect_eq(tp0.track_index, 0);
    expect_eq(tp0.segment_index, 0);
    expect_eq(tp0.point.ele > 9.9 && tp0.point.ele < 10.1, true);

    const tp1 = tit.next() ?? unreachable;
    expect_eq(tp1.track_index, 0);
    expect_eq(tp1.segment_index, 0);

    const tp2 = tit.next() ?? unreachable;
    expect_eq(tp2.track_index, 0);
    expect_eq(tp2.segment_index, 1);
    expect_eq(tit.next() == null, true);
});

test("2. Single-quoted attributes, self-closing wpt/trkpt tags, and UTF-8 BOM handling", () => {
    const xml = "\xEF\xBB\xBF<!-- leading comment --><gpx version='1.0' creator='Mini'>" ++
        "<wpt lat='-33.8688' lon='151.2093' />" ++
        "<trk><trkseg><trkpt lat='35.6762' lon='139.6503'/></trkseg></trk></gpx>";

    expect_eq(isGpx(xml), true);
    const hdr = parseGpxHeader(xml) ?? unreachable;
    expect_eq(sliceEq(hdr.version, "1.0"), true);
    expect_eq(sliceEq(hdr.creator, "Mini"), true);

    let wit = GpxWaypointIter.init(xml);
    const w0 = wit.next() ?? unreachable;
    expect_eq(w0.lat < -33.86 && w0.lat > -33.87, true);
    expect_eq(w0.lon > 151.20 && w0.lon < 151.21, true);
    expect_eq(w0.has_ele, false);

    let tit = GpxTrackPointIter.init(xml);
    const tp0 = tit.next() ?? unreachable;
    expect_eq(tp0.point.lat > 35.67 && tp0.point.lat < 35.68, true);
});

test("3. haversineMeters accuracy on known city coordinates and identical points", () => {
    expect_eq(GPX_EARTH_RADIUS_M > 6371008.0, true);
    // Identical point => 0 meters
    const d0 = haversineMeters(31.2304, 121.4737, 31.2304, 121.4737);
    expect_eq(d0 < 0.001, true);

    // Beijing (39.9042, 116.4074) to Shanghai (31.2304, 121.4737) ~ 1068 km
    const bj_sh = haversineMeters(39.9042, 116.4074, 31.2304, 121.4737);
    expect_eq(bj_sh > 1_060_000.0 && bj_sh < 1_075_000.0, true);

    // 1 degree of latitude along a meridian ~ 111.195 km
    const deg1 = haversineMeters(0.0, 0.0, 1.0, 0.0);
    expect_eq(deg1 > 111_100.0 && deg1 < 111_300.0, true);
});

test("4. summarizeGpx computes elevation gain/loss, BBox, and never connects across trkseg boundaries", () => {
    // Seg 0: (0,0,100m) -> (0.01,0,130m) -> (0.02,0,115m): ~2223.9m, gain=30m, loss=15m
    // Seg 1 (far away at lat=10.0, only 1 point): must add 0 distance and 0 elevation change!
    const xml = "<gpx version=\"1.1\" creator=\"test\"><trk>" ++
        "<trkseg>" ++
        "<trkpt lat=\"0.0\" lon=\"0.0\"><ele>100.0</ele></trkpt>" ++
        "<trkpt lat=\"0.01\" lon=\"0.0\"><ele>130.0</ele></trkpt>" ++
        "<trkpt lat=\"0.02\" lon=\"0.0\"><ele>115.0</ele></trkpt>" ++
        "</trkseg>" ++
        "<trkseg>" ++
        "<trkpt lat=\"10.0\" lon=\"20.0\"><ele>500.0</ele></trkpt>" ++
        "</trkseg>" ++
        "</trk></gpx>";

    const st = summarizeGpx(xml);
    expect_eq(st.track_count, 1);
    expect_eq(st.segment_count, 2);
    expect_eq(st.trackpoint_count, 4);
    // Must be ~2224m (NOT ~2,400,000m across segment gap!)
    expect_eq(st.total_distance_m > 2200.0 && st.total_distance_m < 2250.0, true);
    expect_eq(st.elevation_gain_m > 29.9 && st.elevation_gain_m < 30.1, true);
    expect_eq(st.elevation_loss_m > 14.9 && st.elevation_loss_m < 15.1, true);
    expect_eq(st.min_lat > -0.001 && st.min_lat < 0.001, true);
    expect_eq(st.max_lat > 9.99 && st.max_lat < 10.01, true);
    expect_eq(st.min_lon > -0.001 && st.min_lon < 0.001, true);
    expect_eq(st.max_lon > 19.99 && st.max_lon < 20.01, true);
});

test("5. Malformed GPX protection: non-GPX XML, out-of-range lat/lon, and unclosed tags", () => {
    // Non-GPX XML rejected
    expect_eq(isGpx("<html><body>hi</body></html>"), false);
    expect_eq(parseGpxHeader("<svg></svg>") == null, true);

    // Out-of-range lat (95.0) and lon (-190.0) skipped safely
    const bad_coords = "<gpx version=\"1.1\">" ++
        "<wpt lat=\"95.0\" lon=\"10.0\"/>" ++
        "<wpt lat=\"10.0\" lon=\"-190.0\"/>" ++
        "<wpt lat=\"45.0\" lon=\"90.0\"/>" ++
        "</gpx>";
    let wit = GpxWaypointIter.init(bad_coords);
    const valid = wit.next() ?? unreachable;
    expect_eq(valid.lat > 44.9 && valid.lat < 45.1, true);
    expect_eq(wit.next() == null, true);

    // Unclosed <wpt> tag terminates safely without panic
    const unclosed = "<gpx version=\"1.1\"><wpt lat=\"1.0\" lon=\"2.0\"><ele>5";
    let uw = GpxWaypointIter.init(unclosed);
    expect_eq(uw.next() == null, true);
});
