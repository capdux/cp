// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc OGC WKB / PostGIS EWKB Spatial Geometry Codec)

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
