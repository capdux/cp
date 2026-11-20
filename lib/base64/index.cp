// Source: cp/lib/base64/index.cp (Unified export entry for cap:base64 & cap.base64)

export {
    Base64Variant,
    Base64Error,
    STANDARD_ALPHABET,
    URL_SAFE_ALPHABET,
    PAD_CHAR,
    INVALID_INDEX,
    STANDARD_DECODE_TABLE,
    URL_SAFE_DECODE_TABLE,
    hasPadding,
    alphabetFor,
    decodeTableFor,
    isWhitespace,
} from "./types.cp";

export {
    encodedLen,
    maxDecodedLen,
    exactDecodedLen,
    encodeToSlice,
    decodeToSlice,
    decodeIgnoreWhitespaceToSlice,
    encodeAlloc,
    decodeAlloc,
} from "./codec.cp";

export {
    StreamingBase64Encoder,
    StreamingBase64Decoder,
} from "./streaming.cp";
