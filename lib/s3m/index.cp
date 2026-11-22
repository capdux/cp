// Origin: Pure-CP Stdlib (Zero-Alloc Scream Tracker 3 S3M Module Parser) | Created: 2026-11-22

export {
    S3M_MAGIC,
    S3M_HEADER_SIZE,
    S3M_SUB_BYTE,
    S3M_TYPE_MODULE,
    S3M_INST_SIZE,
    S3M_ORDER_SKIP,
    S3M_ORDER_END,
    S3mHeader,
    S3mInstrument,
    S3mPattern,
    isS3m,
    parseS3mHeader,
    parseS3mInstrument,
    parseS3mPattern,
} from "./s3m.cp";
