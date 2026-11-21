// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc W3C WOFF2 Font Header & Table Directory Parser)

export {
    WOFF2_HEADER_SIZE,
    WOFF2_SIGNATURE,
    WOFF2_FLAVOR_TRUETYPE,
    WOFF2_FLAVOR_CFF,
    WOFF2_FLAVOR_COLLECTION,
    Woff2Header,
    UIntBase128Result,
    Woff2TableEntry,
    decodeUIntBase128,
    isWoff2,
    parseWoff2Header,
    Woff2TableIter,
} from "./woff2.cp";
