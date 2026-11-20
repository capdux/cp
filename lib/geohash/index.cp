// Source: cp/lib/geohash/index.cp (Unified Exports for cap:geohash)
// Pure CP zero-C-dependency Geohash spatial grid encoding, 8-neighbor lookup,
// 64-bit Z-order Morton encoding, and WGS-84 Haversine distance.

export {
    EARTH_RADIUS_METERS,
    MAX_PRECISION,
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
} from "./geohash.cp";
