// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Android DEX Binary Header & String Table Parser)

export {
    DEX_HEADER_SIZE,
    DEX_ENDIAN_CONSTANT,
    DEX_REVERSE_ENDIAN_CONSTANT,
    DEX_TYPE_HEADER_ITEM,
    DEX_TYPE_STRING_ID_ITEM,
    DEX_TYPE_TYPE_ID_ITEM,
    DEX_TYPE_PROTO_ID_ITEM,
    DEX_TYPE_FIELD_ID_ITEM,
    DEX_TYPE_METHOD_ID_ITEM,
    DEX_TYPE_CLASS_DEF_ITEM,
    DEX_TYPE_MAP_LIST,
    DEX_TYPE_STRING_DATA_ITEM,
    DexHeader,
    Uleb128Result,
    DexMapItem,
    DexMapIter,
    isDex,
    parseDexHeader,
    readUleb128,
    readDexString,
} from "./dex.cp";
