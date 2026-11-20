// Source: cp/lib/msgpack/index.cp (Unified Exports for cap:msgpack)
// Pure CP MessagePack binary serialization & deserialization engine.

export {
    DEFAULT_MAX_DEPTH,
    MsgPackError,
    MsgExt,
    MsgMapEntry,
    MsgValue,
    MsgPackWriter,
    MsgPackReader,
    encode,
    decode,
    decodeWithMaxDepth,
} from "./msgpack.cp";
