// Source: cp/lib/tlv/index.cp (Unified Exports for cap:tlv)
// Pure CP zero-C-dependency ASN.1 BER/DER Tag-Length-Value (TLV) binary codec & OID codec.

export {
    TLV_MAX_DEPTH,
    TAG_NUM_BOOLEAN,
    TAG_NUM_INTEGER,
    TAG_NUM_BIT_STRING,
    TAG_NUM_OCTET_STRING,
    TAG_NUM_NULL,
    TAG_NUM_OID,
    TAG_NUM_UTF8_STRING,
    TAG_NUM_PRINTABLE_STRING,
    TAG_NUM_IA5_STRING,
    TAG_NUM_UTC_TIME,
    TAG_NUM_GENERALIZED_TIME,
    TAG_NUM_SEQUENCE,
    TAG_NUM_SET,
    TlvError,
    TagClass,
    TlvTagDecodeResult,
    TlvLengthDecodeResult,
    TlvTag,
    TlvNode,
    derLengthSize,
    encodeDerLengthTo,
    decodeDerLength,
} from "./types.cp";

export {
    encodeOidBytes,
    decodeOid,
} from "./oid.cp";

export {
    decodeDer,
    decodeDerWithMaxDepth,
} from "./decoder.cp";

export {
    encodeDerNode,
    TlvWriter,
} from "./encoder.cp";
