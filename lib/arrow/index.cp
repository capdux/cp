// Origin: Pure-CP Stdlib (Zero-Alloc Apache Arrow IPC Stream & File Parser) | Created: 2026-11-22

export {
    ARROW_MAGIC,
    ARROW_FILE_HEADER_SIZE,
    ARROW_FILE_TRAILER_SIZE,
    ARROW_FILE_MIN_SIZE,
    ARROW_CONTINUATION,
    ArrowMetadataVersion,
    ArrowHeaderType,
    ArrowMessageMeta,
    ArrowMessageHeader,
    ArrowFooterInfo,
    ArrowMessageIter,
    readFbU32,
    readFbRootTable,
    readFbVtableOffset,
    parseArrowMessageMeta,
    isArrowFile,
    isArrowStream,
    parseArrowFileFooter,
} from "./arrow.cp";
