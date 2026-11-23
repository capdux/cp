// Source: Absorbed from /data/dev/bun/src/zlib/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pack/index.cp — CP 标准压缩、归档与校验和模块 (cap:pack / cap:compress) 对外统一出口
// ============================================================================

export {
    Z_MIN_WBITS,
    Z_MAX_WBITS,
    Z_DEFAULT_COMPRESSION,
    Z_NO_COMPRESSION,
    Z_BEST_SPEED,
    Z_BEST_COMPRESSION,
    CompressionFormat,
    ZlibFlush,
    ZlibStatus,
    BrotliOperation,
    CompressBoundEstimator,
    Zlib,
    Deflate,
    ZStream,
    NativeZlib,
    z_stream_s,
    zlibVersion,
    compress,
    compress2,
    compressBound,
    uncompress,
    ZSTD_compress,
    ZSTD_decompress,
    ZSTD_compressBound,
    ZSTD_findDecompressedSize,
} from "./_codecs.cp";

export * from "./checksum.cp";
export * from "./gzip.cp";
export * from "./zstd.cp";
export * from "./brotli.cp";
export * from "./_archive.cp";
export * from "./tar.cp";
export * from "./zip.cp";
export * from "./native.cp";
export * from "./zipWriter.cp";
export * from "./tarPax.cp";
export * from "./magic.cp";

// === Consolidated Pure-CP Compression Algorithms & Archive Containers ===

// --- bzip2 ---
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

// --- lz4 ---
export {
    Lz4Error,
    LZ4_FRAME_MAGIC,
    LZ4_SKIPPABLE_MAGIC_MIN,
    LZ4_SKIPPABLE_MAGIC_MAX,
    LZ4_FRAME_BLOCK_SIZE,
    Lz4FrameOptions,
    xxhash32,
    copyNonOverlappingScalar,
    copyNonOverlappingSimd,
    copyMatchScalar,
    copyMatchSimd,
    extendMatchLenScalar,
    extendMatchLenSimd,
    Lz4Block,
    Lz4Frame,
} from "./lz4.cp";

// --- snappy ---
export {
    SnappyError,
    compress as snappyCompress,
    decompress,
    decompressWithMaxSize,
    uncompressedLength,
} from "./snappy.cp";

// --- rle ---
export {
    PACKBITS_NOP,
    PCX_RUN_MASK,
    PCX_MAX_RUN,
    TGA_RLE_FLAG,
    TGA_MAX_PACKET,
    ZERO_RLE_MAX_RUN,
    packbitsMaxEncodedSize,
    packbitsEncode,
    packbitsDecode,
    pcxEncode,
    pcxDecode,
    tgaRleEncode,
    tgaRleDecode,
    zeroRleEncode,
    zeroRleDecode,
} from "./rle.cp";

// --- xz ---
export {
    XZ_HEADER_MAGIC,
    XZ_FOOTER_MAGIC,
    XZ_STREAM_HEADER_SIZE,
    XZ_STREAM_FOOTER_SIZE,
    CHECK_NONE,
    CHECK_CRC32,
    CHECK_CRC64,
    CHECK_SHA256,
    FILTER_DELTA,
    FILTER_X86,
    FILTER_POWERPC,
    FILTER_IA64,
    FILTER_ARM,
    FILTER_ARMTHUMB,
    FILTER_SPARC,
    FILTER_ARM64,
    FILTER_RISCV,
    FILTER_LZMA2,
    xzCrc32,
    XzVliResult,
    decodeXzVli,
    encodeXzVli,
    XzStreamHeader,
    XzStreamFooter,
    XzFilter,
    XzBlockHeader,
    XzIndexRecord,
    XzIndexRecordIter,
    XzIndex,
    isXz,
    parseXzHeader,
    parseXzFooter,
    parseXzBlockHeader,
    parseXzIndex,
} from "./xz.cp";

// --- lzss ---
export {
    LZSS_WINDOW_SIZE,
    LZSS_MIN_MATCH,
    LZSS_MAX_MATCH,
    LZSS_FRAME_HEADER_SIZE,
    lzssMaxCompressedSize,
    lzssCompress,
    lzssDecompress,
    lzssCompressFramed,
    lzssDecompressFramed,
} from "./lzss.cp";

// --- lzo ---
export {
    LZOP_MAGIC,
    METHOD_LZO1X_1,
    METHOD_LZO1X_1_15,
    METHOD_LZO1X_999,
    F_ADLER32_D,
    F_ADLER32_C,
    F_STDIN,
    F_STDOUT,
    F_EXTRA_FIELD,
    F_CRC32_D,
    F_CRC32_C,
    F_H_FILTER,
    F_H_CRC32,
    lzoAdler32,
    lzoCrc32,
    LzopHeader,
    LzopBlock,
    LzopBlockIter,
    isLzop,
    parseLzopHeader,
    iterLzopBlocks,
    lzo1xDecompress,
} from "./lzo.cp";

// --- bwt ---
export {
    BwtResult,
    bwtEncode,
    bwtDecode,
    mtfEncode,
    mtfDecode,
    bwtMtfEncode,
    bwtMtfDecode,
    zeroByteRatioPermille,
} from "./bwt.cp";

// --- rar ---
export {
    RAR4_SIGNATURE,
    RAR5_SIGNATURE,
    RAR_VER_4,
    RAR_VER_5,
    RAR5_HEAD_MAIN,
    RAR5_HEAD_FILE,
    RAR5_HEAD_SERVICE,
    RAR5_HEAD_CRYPT,
    RAR5_HEAD_ENDARC,
    HFL_EXTRA,
    HFL_DATA,
    HFL_SKIP_IF_UNKNOWN,
    HFL_SPLIT_BEFORE,
    HFL_SPLIT_AFTER,
    FILE_FLAG_DIR,
    FILE_FLAG_UTIME,
    FILE_FLAG_CRC32,
    FILE_FLAG_UNP_UNKNOWN,
    RAR4_TYPE_MAIN,
    RAR4_TYPE_FILE,
    RAR4_TYPE_ENDARC,
    RAR4_FLAG_LONG_BLOCK,
    detectRarVersion,
    isRar,
    rarCrc32,
    Rar5Vint,
    decodeRar5Vint,
    encodeRar5Vint,
    Rar5Block,
    Rar5BlockIter,
    iterRar5Blocks,
    Rar5FileEntry,
    parseRar5FileEntry,
    Rar4Block,
    Rar4BlockIter,
    iterRar4Blocks,
} from "./rar.cp";

// --- ar ---
export {
    AR_MAGIC,
    AR_HEADER_SIZE,
    AR_END_MAGIC,
    ArMemberKind,
    ArMember,
    isAr,
    ArIter,
    findArMember,
    countArMembers,
} from "./ar.cp";

// --- cpio ---
export {
    CPIO_MAGIC_NEWC,
    CPIO_MAGIC_CRC,
    CPIO_MAGIC_ODC,
    CPIO_NEWC_HEADER_SIZE,
    CPIO_ODC_HEADER_SIZE,
    CPIO_TRAILER,
    CpioFormat,
    CpioEntry,
    detectCpioFormat,
    isCpio,
    CpioIter,
    findCpioEntry,
    countCpioEntries,
} from "./cpio.cp";

// --- cab ---
export {
    CAB_MAGIC,
    CAB_HEADER_SIZE,
    CAB_FOLDER_SIZE,
    CAB_FILE_FIXED_SIZE,
    CAB_FLAG_PREV,
    CAB_FLAG_NEXT,
    CAB_FLAG_RESERVE,
    CAB_ATTR_RDONLY,
    CAB_ATTR_HIDDEN,
    CAB_ATTR_SYSTEM,
    CAB_ATTR_ARCH,
    CAB_ATTR_EXEC,
    CAB_ATTR_NAME_IS_UTF,
    CabCompression,
    decodeCabCompression,
    CabHeader,
    CabFolder,
    CabFile,
    isCab,
    parseCabHeader,
    CabFolderIter,
    CabFileIter,
    findCabFile,
} from "./cab.cp";

// --- squashfs ---
export {
    SQUASHFS_MAGIC,
    SQUASHFS_SUPER_SIZE,
    SQUASHFS_MAJOR,
    SQUASHFS_MINOR,
    SQUASHFS_METADATA_MAX_SIZE,
    SQUASHFS_META_UNCOMPRESSED_BIT,
    SQUASHFS_DATA_UNCOMPRESSED_BIT,
    SQUASHFS_INVALID_OFFSET,
    COMP_GZIP,
    COMP_LZMA,
    COMP_LZO,
    COMP_XZ,
    COMP_LZ4,
    COMP_ZSTD,
    FLAG_UNCOMPRESSED_INODES,
    FLAG_UNCOMPRESSED_DATA,
    FLAG_UNCOMPRESSED_FRAGMENTS,
    FLAG_NO_FRAGMENTS,
    FLAG_ALWAYS_FRAGMENTS,
    FLAG_DUPLICATES,
    FLAG_EXPORTABLE,
    FLAG_UNCOMPRESSED_XATTRS,
    FLAG_NO_XATTRS,
    FLAG_COMPRESSOR_OPTIONS,
    FLAG_UNCOMPRESSED_IDS,
    INODE_DIR,
    INODE_REG,
    INODE_SYMLINK,
    INODE_BLKDEV,
    INODE_CHRDEV,
    INODE_FIFO,
    INODE_SOCK,
    SquashfsSuperblock,
    isSquashfs,
    parseSquashfsSuperblock,
    SquashfsInodeRef,
    decodeInodeRef,
    encodeInodeRef,
    SquashfsMetaHeader,
    parseMetaBlock,
    SquashfsDataBlockSize,
    decodeDataBlockSize,
    SquashfsFragmentEntry,
    parseFragmentEntry,
} from "./squashfs.cp";

// --- cdc_chunker ---
export {
    FastCdcConfig,
    CdcChunkSpan,
    FastCdcChunker,
    CdcDedupSlot,
    CdcDedupStats,
    CdcDedupTable,
} from "./cdcChunker.cp";

// --- delta_varint ---
export {
    VARINT_MAX_U64_BYTES,
    VarintDecodeResult,
    SignedVarintDecodeResult,
    DeltaBlockStats,
    zigzagEncodeI64,
    zigzagDecodeU64,
    encodeUvarint64,
    decodeUvarint64,
    encodeSvarint64,
    decodeSvarint64,
    encodeDeltaU64,
    decodeDeltaU64,
    encodeDeltaOfDeltaI64,
    decodeDeltaOfDeltaI64,
    computeDeltaCompressionStats,
} from "./deltaVarint.cp";

// --- for_bitpack ---
export {
    FOR_BLOCK_HEADER_LEN,
    FOR_MAX_BIT_WIDTH,
    ForBlockMode,
    ForPackStatus,
    ForBlockHeader,
    ForPackResult,
    ForDecodeResult,
    ForSingleResult,
    bitWidthU32,
    packedByteLen,
    blockMayContain,
    decodeForBlockHeader,
    packBitsU32,
    unpackBitsU32,
    unpackSingleU32,
    encodeForBlockU32,
    decodeForBlockU32,
    encodeDeltaForBlockU32,
    decodeDeltaForBlockU32,
} from "./forBitpack.cp";

// --- rle_hybrid ---
export {
    HYBRID_RLE_MAX_BIT_WIDTH,
    HYBRID_RLE_MIN_RUN_LEN,
    HYBRID_RLE_GROUP_SIZE,
    HYBRID_RLE_MAX_BITPACK_GROUPS,
    hybridBitWidthU32,
    hybridMinBitWidthSlice,
    hybridValueByteWidth,
    hybridMaxEncodedSize,
    HybridRleStatus,
    HybridRleEncodeResult,
    HybridRleDecodeResult,
    encodeHybridRleU32,
    HybridRleDecoder,
    decodeHybridRleU32,
} from "./rleHybrid.cp";

// --- zstd_decode (Pure-CP RFC 8878 Zstandard Decompressor: FSE, Huffman 1/4-Stream, Dictionary & xxHash64) ---
export {
    ZSTD_DEC_MAGIC,
    ZSTD_DICT_MAGIC,
    ZSTD_DEC_SKIP_MAGIC_LO,
    ZSTD_DEC_SKIP_MAGIC_HI,
    ZSTD_MAX_BLOCK_BYTES,
    ZSTD_MAX_WINDOW_BYTES,
    ZstdDecodeError,
    xxhash64Zstd,
    ZstdFrameInfo,
    zstdInspectFrame,
    ZstdWorkspace,
    zstdDecompressWithDictInto,
    zstdDecompressInto,
    zstdDecompress,
} from "./zstdDecode.cp";
