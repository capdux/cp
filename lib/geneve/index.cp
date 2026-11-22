// Origin: Pure-CP Stdlib (Zero-Alloc RFC 8926 Geneve Tunneling Encapsulation Codec) | Created: 2026-11-22

export {
    GENEVE_MIN_HEADER_SIZE,
    GENEVE_DEFAULT_PORT,
    GENEVE_VERSION,
    GENEVE_MAX_VNI,
    GENEVE_MAX_OPTIONS_BYTES,
    GENEVE_MAX_SINGLE_OPT_DATA,
    GENEVE_FLAG_OAM,
    GENEVE_FLAG_CRITICAL,
    GENEVE_OPT_CRITICAL_BIT,
    PROTO_ETHERNET,
    PROTO_IPV4,
    PROTO_IPV6,
    GeneveHeader,
    GeneveOption,
    GeneveOptionIter,
    GenevePacket,
    GeneveWriter,
    isGeneve,
    parseGeneve,
} from "./geneve.cp";
