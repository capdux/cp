// Origin: Pure-CP Stdlib (Zero-Alloc RFC 7348 VXLAN & VXLAN-GPE Codec) | Created: 2026-11-22

export {
    VXLAN_HEADER_SIZE,
    VXLAN_DEFAULT_PORT,
    VXLAN_GPE_PORT,
    VXLAN_MAX_VNI,
    VXLAN_FLAG_I,
    VXLAN_GPE_FLAG_P,
    VXLAN_GPE_FLAG_O,
    VXLAN_GBP_FLAG_G,
    VxlanNextProto,
    vxlanNextProtoFromByte,
    VxlanHeader,
    VxlanPacket,
    vxlanVerifyHeader,
    parseVxlan,
    parseVxlanStrict,
    parseVxlanGpe,
    encodeVxlan,
    encodeVxlanGpe,
    encodeVxlanGbp,
} from "./vxlan.cp";
