// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc NASA FITS 2880B HDU & 80B Card Parser)

export {
    FITS_BLOCK_SIZE,
    FITS_CARD_SIZE,
    FITS_CARDS_PER_BLOCK,
    FitsCard,
    FitsHeader,
    FitsCardIter,
    isFits,
    parseFitsHeader,
    fitsBytesPerPixel,
    findFitsCard,
    parseFitsInt,
    parseFitsBool,
} from "./fits.cp";
