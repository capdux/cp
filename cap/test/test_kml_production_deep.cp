// Source: cp/cap/test/test_kml_production_deep.cp (Production deep tests for cap:kml)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    KmlGeomKind,
    KmlCoord,
    KmlCoordIter,
    KmlPlacemark,
    KmlPlacemarkIter,
    KmlStats,
    isKml,
    summarizeKml,
} from "cap:geo";

test("1. KML 2.2 Document/Folder with Point, LineString, and Polygon Placemarks", () => {
    const kml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n<Document>\n  <Folder>\n    <Placemark id=\"p1\">\n      <name>HQ</name>\n      <styleUrl>#pin</styleUrl>\n      <Point><coordinates>-122.5,37.25,15.5</coordinates></Point>\n    </Placemark>\n    <Placemark id=\"l1\">\n      <name>Route</name>\n      <LineString><coordinates>-122.0,37.0 -121.5,37.5</coordinates></LineString>\n    </Placemark>\n    <Placemark id=\"g1\">\n      <name>Area</name>\n      <Polygon><outerBoundaryIs><LinearRing><coordinates>0,0 10,0 10,10 0,0</coordinates></LinearRing></outerBoundaryIs></Polygon>\n    </Placemark>\n  </Folder>\n</Document>\n</kml>";

    expect_eq(isKml(kml), true);

    let it = KmlPlacemarkIter.init(kml);

    const p1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p1.id, "p1"), true);
    expect_eq(std.mem.eql(u8, p1.name, "HQ"), true);
    expect_eq(std.mem.eql(u8, p1.style_url, "#pin"), true);
    expect_eq(p1.geom_kind == KmlGeomKind.point, true);
    let c1 = p1.coords();
    const pt = c1.next() ?? unreachable;
    expect_eq(pt.lon, -122.5);
    expect_eq(pt.lat, 37.25);
    expect_eq(pt.alt, 15.5);
    expect_eq(pt.has_alt, true);
    expect_eq(c1.next() == null, true);

    const l1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, l1.id, "l1"), true);
    expect_eq(l1.geom_kind == KmlGeomKind.linestring, true);

    const g1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, g1.id, "g1"), true);
    expect_eq(g1.geom_kind == KmlGeomKind.polygon, true);

    expect_eq(it.next() == null, true);
});

test("2. CDATA description unwrapping and comment skipping inside Placemark", () => {
    const kml =
        "<kml><Placemark><name><![CDATA[<b>Peak & View</b>]]></name><description><![CDATA[Elevation: <b>1200m</b>]]></description><!-- <Point/> --><Point><coordinates>116.25,39.75</coordinates></Point></Placemark></kml>";

    let it = KmlPlacemarkIter.init(kml);
    const pm = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, pm.name, "<b>Peak & View</b>"), true);
    expect_eq(std.mem.eql(u8, pm.description, "Elevation: <b>1200m</b>"), true);
    expect_eq(pm.geom_kind == KmlGeomKind.point, true);
});

test("3. KmlCoordIter parses mixed 2D/3D whitespace-separated coordinates accurately", () => {
    const raw = "  -180.0,-90.0\t0.5,45.25,100.0\r\n180.0,90.0,-5.5  ";
    let cit = KmlCoordIter.init(raw);

    const a = cit.next() ?? unreachable;
    expect_eq(a.lon, -180.0);
    expect_eq(a.lat, -90.0);
    expect_eq(a.has_alt, false);

    const b = cit.next() ?? unreachable;
    expect_eq(b.lon, 0.5);
    expect_eq(b.lat, 45.25);
    expect_eq(b.alt, 100.0);
    expect_eq(b.has_alt, true);

    const c = cit.next() ?? unreachable;
    expect_eq(c.lon, 180.0);
    expect_eq(c.lat, 90.0);
    expect_eq(c.alt, -5.5);
    expect_eq(c.has_alt, true);

    expect_eq(cit.next() == null, true);
});

test("4. summarizeKml aggregates geometry counts and global bounding box", () => {
    const kml =
        "<kml><Document><Placemark><Point><coordinates>-10.0,20.0</coordinates></Point></Placemark><Placemark><LineString><coordinates>5.0,-15.0 30.0,40.0</coordinates></LineString></Placemark></Document></kml>";

    const stats = summarizeKml(kml);
    expect_eq(stats.placemark_count, 2);
    expect_eq(stats.point_count, 1);
    expect_eq(stats.linestring_count, 1);
    expect_eq(stats.polygon_count, 0);
    expect_eq(stats.coord_count, 3);
    expect_eq(stats.has_bbox, true);
    expect_eq(stats.min_lon, -10.0);
    expect_eq(stats.max_lon, 30.0);
    expect_eq(stats.min_lat, -15.0);
    expect_eq(stats.max_lat, 40.0);
});

test("5. non-KML XML, unclosed tags, and out-of-range lon/lat coordinates fail safely", () => {
    expect_eq(isKml("<?xml version=\"1.0\"?><svg></svg>"), false);
    expect_eq(isKml(""), false);

    // Unclosed Placemark stops cleanly
    let it = KmlPlacemarkIter.init("<kml><Placemark><name>Broken</name>");
    expect_eq(it.next() == null, true);

    // Out-of-range longitude (181.0 > 180.0) and latitude (-91.0 < -90.0)
    let c_lon = KmlCoordIter.init("181.0,10.0");
    expect_eq(c_lon.next() == null, true);

    let c_lat = KmlCoordIter.init("10.0,-91.0");
    expect_eq(c_lat.next() == null, true);

    // Malformed coordinate token (missing comma / non-numeric)
    let c_bad = KmlCoordIter.init("not_a_coord");
    expect_eq(c_bad.next() == null, true);
});
