// Origin: Pure-CP Stdlib (Zero-Alloc OGC WKB / PostGIS EWKB Spatial Geometry Codec) | Created: 2026-11-22

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
