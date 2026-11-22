// Origin: Pure-CP Stdlib (Zero-Alloc Apache ORC Columnar Container Parser) | Created: 2026-11-22

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
