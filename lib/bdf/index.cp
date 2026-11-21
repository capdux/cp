// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Adobe BDF 2.1/2.2 Bitmap Font Parser)
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
