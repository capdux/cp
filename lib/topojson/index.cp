// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc TopoJSON v1.0 Quantized Arc & Topology Parser)
// Unified exports for cap:topojson.

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
