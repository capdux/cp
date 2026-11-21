// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 5880 BFD Wire Protocol Parser)
// Unified exports for cap:bfd.

export {
    BFD_UDP_PORT_SINGLE_HOP,
    BFD_UDP_PORT_ECHO,
    BFD_UDP_PORT_MULTI_HOP,
    BFD_VERSION,
    BFD_MIN_LEN,
    BfdDiag,
    BfdState,
    BfdAuthType,
    BfdAuth,
    BfdPacket,
    parseBfd,
    isBfd,
    detectionTimeUs,
} from "./bfd.cp";
