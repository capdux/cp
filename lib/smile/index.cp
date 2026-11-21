// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Jackson Smile 1.0 Binary JSON Parser)

export {
    SMILE_HEADER_MAGIC,
    SMILE_HEADER_SIZE,
    SMILE_FLAG_SHARED_NAMES,
    SMILE_FLAG_SHARED_VALUES,
    SMILE_FLAG_RAW_BINARY,
    SMILE_END_OF_STRING,
    SMILE_END_OF_CONTENT,
    SmileHeader,
    SmileVInt,
    SmileTokenKind,
    SmileKeyKind,
    SmileToken,
    SmileKeyToken,
    isSmile,
    parseSmileHeader,
    readSmileVInt,
    smileZigZagDecode,
    SmileReader,
} from "./smile.cp";
