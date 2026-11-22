// Source: cp/lib/geo/index.cp (Unified Exports for cap:geo)
// Unified Public Exports for Geospatial Computation & Geohash Proximity Engine (cap:geo & cap.geo)

export {
    EARTH_RADIUS_M,
    GeoError,
    LatLng,
    BBox,
    Geo,
} from "./coord.cp";

export {
    GeohashDecoded,
    GeohashNeighbors,
    Geohash,
} from "./geohash.cp";

export {
    GeoPointEntry,
    SpatialIndex,
} from "./spatial.cp";

// ============================================================================
// Pure-CP Extended Geospatial, GIS Formats, Navigation & Spatial Indexing
// ============================================================================
export {
    MortonError,
    Point3D,
    mortonEncode3D,
    mortonDecode3D,
    quantizeEncode3D,
} from "./geohash_3d.cp";

export {
    GeoJsonError,
    Position,
    BBox as GeoJsonBBox,
    PolygonData,
    Geometry,
    Feature,
    FeatureCollection,
    GeoJsonDoc,
    parse,
    stringify,
    computeBBox,
    computeDocBBox,
    pointInPolygon,
    simplifyDouglasPeucker,
} from "./geojson.cp";

export {
    TopoTransform,
    TopoPoint,
    TopoBBox,
    TopoArcRef,
    TopoHeader,
    TopoObjectEntry,
    TopoArcIter,
    TopoArcPointIter,
    TopoObjectIter,
    isTopoJson,
    parseTopoHeader,
    resolveArcRef,
    arcSliceAt,
    countArcPoints,
} from "./topojson.cp";

export {
    WktError,
    WkbByteOrder,
    WktCoord,
    WktPolygon,
    WktGeometry,
    parseWkt,
    formatWkt,
    encodeWkb,
    decodeWkb,
} from "./wkt.cp";

export {
    WKB_XDR,
    WKB_NDR,
    EWKB_FLAG_Z,
    EWKB_FLAG_M,
    EWKB_FLAG_SRID,
    MAX_WKB_DEPTH,
    WkbGeomType,
    wkbGeomTypeFromCode,
    WkbPoint,
    WkbHeader,
    parseWkbHeader,
    wkbByteLength,
    WkbPointIter,
    WkbRingIter,
    WkbChildIter,
    WkbReader,
    WkbWriter,
} from "./wkb.cp";

export {
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
} from "./gpx.cp";

export {
    KmlGeomKind,
    KmlCoord,
    KmlCoordIter,
    KmlPlacemark,
    KmlPlacemarkIter,
    KmlStats,
    isKml,
    summarizeKml,
} from "./kml.cp";

export {
    NmeaSentence,
    NmeaGga,
    NmeaRmc,
    NmeaVtg,
    NmeaFieldIter,
    NmeaLineIter,
    nmeaChecksum,
    verifyNmeaSentence,
    parseNmeaSentence,
    parseNmeaCoord,
    parseNmeaGga,
    parseNmeaRmc,
    parseNmeaVtg,
} from "./nmea.cp";

export {
    OLC_ALPHABET,
    OLC_SEPARATOR,
    OLC_SEPARATOR_POS,
    OLC_PADDING,
    OLC_DEFAULT_LEN,
    OLC_MAX_LEN,
    OlcArea,
    isValidOlc,
    isShortOlc,
    isFullOlc,
    olcEncode,
    olcDecode,
    olcShorten,
    olcRecoverNearest,
} from "./olc.cp";

export {
    AIS_LON_UNAVAILABLE,
    AIS_LAT_UNAVAILABLE,
    AisSentence,
    AisPositionReport,
    AisClassBPosition,
    AisBitReader,
    aisUnarmorChar,
    aisMessageType,
    parseAisSentence,
    parseAisPositionReport,
    parseAisClassBPosition,
} from "./ais.cp";

export {
    GeomError,
    Orientation,
    Point2,
    Segment2,
    Aabb2,
    Circle2,
    Geom2D,
} from "./geom2d.cp";

export {
    QuadError,
    Rect,
    PointItem,
    QuadTree,
} from "./geo_quadtree.cp";
