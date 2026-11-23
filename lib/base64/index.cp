// Reference-hardened against marshallpierce/rust-base64 (MIT/Apache-2.0)
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
    encodedLenChecked,
    maxDecodedLen,
    decodedLenEstimate,
    exactDecodedLen,
    encodeToSlice,
    encodeToSliceScalar,
    encodeInto,
    decodeToSlice,
    decodeToSliceScalar,
    decodeInto,
    decodeIgnoreWhitespaceToSlice,
    encodeAlloc,
    decodeAlloc,
    simdValidateBlock16,
    simdValidateBlock16Scalar,
    simdValidateBase64Alphabet,
    simdEncodeChunk12To16,
    simdDecodeChunk16To12,
} from "./codec.cp";

export {
    StreamingBase64Encoder,
    StreamingBase64Decoder,
} from "./streaming.cp";
