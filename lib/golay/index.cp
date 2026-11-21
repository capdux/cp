// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Extended Binary Golay [24,12,8] & Perfect Golay [23,12,7] FEC Codec)

export {
    GOLAY24_DATA_BITS,
    GOLAY24_CODE_BITS,
    GOLAY24_MIN_DIST,
    GOLAY23_CODE_BITS,
    GolayStatus,
    GolayDecode,
    golay24Parity,
    golay24Encode,
    golay24Decode,
    golay23Encode,
    golay23Decode,
    golay24EncodeBytes,
    golay24DecodeBytes,
} from "./golay.cp";
