// Source: cp/lib/capnp/index.cp (cap:capnp standard library entry point)
export {
    WORD_SIZE,
    MAX_SEGMENTS,
    DEFAULT_TRAVERSAL_LIMIT_WORDS,
    DEFAULT_NESTING_LIMIT,
    CapnpError,
    PointerKind,
    ElementSize,
    elementByteSize,
    TraversalLimit,
    readU16LE,
    readU32LE,
    readU64LE,
    writeU16LE,
    writeU32LE,
    writeU64LE,
    signExtend30,
    signExtend29,
} from "./types.cp";

export {
    ByteSlice,
    DecodedPointer,
    ResolvedPointer,
    encodeStructPointer,
    encodeListPointer,
    encodeFarPointer,
    decodePointer,
    computeListContentWords,
    resolvePointer,
} from "./pointer.cp";

export {
    StructReader,
    ListReader,
    MessageReader,
} from "./reader.cp";

export {
    SegBuf,
    StructBuilder,
    ListBuilder,
    MessageBuilder,
} from "./builder.cp";
