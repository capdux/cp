// Source: cp/lib/hpack/index.cp (Unified Exports for cap:hpack)
// Pure CP zero-C-dependency RFC 7541 HTTP/2 HPACK Header Compression library.

export {
    HpackError,
    huffmanEncodedLength,
    huffmanEncode,
    huffmanDecode,
} from "./huffman.cp";

export {
    STATIC_TABLE_LEN,
    ENTRY_OVERHEAD,
    HeaderField,
    TableMatch,
    DynamicTable,
    getStaticEntry,
    lookupEntry,
    findBestMatch,
} from "./table.cp";

export {
    IndexingMode,
    IntDecodeResult,
    StringDecodeResult,
    DecodedHeaders,
    HpackEncoder,
    HpackDecoder,
    encodeInt,
    decodeInt,
    encodeString,
    decodeString,
} from "./codec.cp";
