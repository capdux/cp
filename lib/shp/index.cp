// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ESRI Shapefile .shp Parser)

export {
    SHP_FILE_CODE,
    SHP_VERSION,
    SHP_HEADER_SIZE,
    SHP_RECORD_HEADER_SIZE,
    ShpType,
    ShpBBox,
    ShpHeader,
    ShpRecord,
    ShpPoint,
    ShpPolyHeader,
    ShpRecordIter,
    parseShpHeader,
    isShp,
    parseShpPoint,
    parseShpPoly,
} from "./shp.cp";
