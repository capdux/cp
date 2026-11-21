// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc 2D Simple Polygon Ear-Clipping Triangulation & Geometry Primitives)

export {
    Point2D,
    TriangleIdx,
    signedArea,
    polygonArea,
    isCounterClockwise,
    isConvex,
    pointInTriangle,
    pointInPolygon,
    triangulate,
    triangulationTotalArea,
} from "./earcut.cp";
