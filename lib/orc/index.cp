// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Apache ORC Columnar Container Parser)

export {
    ORC_MAGIC,
    OrcCompression,
    OrcPostScript,
    OrcLayout,
    OrcCompressionBlock,
    VarintResult,
    orcCompressionFromU64,
    readVarint,
    skipField,
    parseOrcPostScript,
    isOrc,
    parseOrcLayout,
    OrcBlockIter,
} from "./orc.cp";
