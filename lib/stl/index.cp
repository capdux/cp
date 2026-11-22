// Origin: Pure-CP Stdlib (Zero-Alloc 3D STL Binary & ASCII Mesh Parser) | Created: 2026-11-22

export {
    StlFormat,
    StlVec3,
    StlTriangle,
    StlBinaryHeader,
    StlBounds,
    detectStlFormat,
    parseStlBinaryHeader,
    StlBinaryIter,
    writeStlBinary,
    computeStlBinaryBounds,
    countAsciiStlFacets,
} from "./stl.cp";
