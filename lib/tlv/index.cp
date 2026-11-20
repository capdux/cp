// Source: cp/lib/tlv/index.cp (Unified Exports for cap:tlv)
// Pure CP zero-C-dependency binary TLV codec, LengthPrefixedFramer & Castagnoli CRC32C.

export {
    TLV_MAX_DEPTH,
    TlvError,
    FrameHeaderKind,
    TlvNestToken,
    TlvField,
    TlvWriter,
    TlvReader,
    LengthPrefixedFramer,
    crc32c,
} from "./tlv.cp";
