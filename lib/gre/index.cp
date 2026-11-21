// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 2784/2890/2637 GRE Tunnel Codec)
// Unified exports for cap:gre.

export {
    GRE_IP_PROTO,
    GRE_MIN_HEADER_SIZE,
    GRE_FLAG_CHECKSUM,
    GRE_FLAG_ROUTING,
    GRE_FLAG_KEY,
    GRE_FLAG_SEQ,
    GRE_FLAG_STRICT_ROUTE,
    GRE_FLAG_ACK,
    GRE_PROTO_IPV4,
    GRE_PROTO_IPV6,
    GRE_PROTO_TEB,
    GRE_PROTO_MPLS_UC,
    GRE_PROTO_PPP,
    GrePacket,
    greChecksum,
    verifyGreChecksum,
    parseGre,
    encodeGre,
} from "./gre.cp";
