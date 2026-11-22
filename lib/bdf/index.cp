// Origin: Pure-CP Stdlib (Zero-Alloc Adobe BDF 2.1/2.2 Bitmap Font Parser) | Created: 2026-11-22
// Unified exports for cap:bdf.

export {
    BdfBBox,
    BdfHeader,
    BdfProperty,
    BdfGlyph,
    BdfPropIter,
    BdfGlyphIter,
    isBdf,
    parseBdfHeader,
    bdfGlyphPixel,
    findBdfGlyph,
} from "./bdf.cp";
