// Origin: Pure-CP Stdlib (Public Unified Export for cap:bzip2) | Created: 2026-11-22

export {
    BZIP2_MAGIC,
    BZIP2_HEADER_SIZE,
    BZIP2_BLOCK_MAGIC,
    BZIP2_EOS_MAGIC,
    BZIP2_MIN_LEVEL,
    BZIP2_MAX_LEVEL,
    bzip2Crc32,
    bzip2CombineCrc,
    Bzip2BitReader,
    Bzip2StreamHeader,
    Bzip2BlockHeader,
    Bzip2EosFooter,
    Bzip2StreamProbe,
    isBzip2,
    parseBzip2Header,
    parseBzip2BlockHeader,
    parseBzip2Eos,
    probeBzip2Stream,
} from "./bzip2.cp";
