// Origin: Pure-CP Stdlib (Zero-Alloc RFC 1112/2236/3376 IGMP v1/v2/v3 Codec) | Created: 2026-11-22
// Unified exports for cap:igmp.

export {
    IGMP_IP_PROTO,
    IGMP_MIN_SIZE,
    IGMP_V3_QUERY_MIN_SIZE,
    IgmpType,
    IgmpV3RecordType,
    IgmpV3GroupRecord,
    IgmpPacket,
    IgmpV3RecordIter,
    igmpChecksum,
    verifyIgmpChecksum,
    igmpDecodeFloatCode,
    parseIgmp,
    encodeIgmpV2,
} from "./igmp.cp";
