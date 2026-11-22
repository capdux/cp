// Origin: Pure-CP Stdlib (Zero-Alloc VESA EDID 1.3 / 1.4 Block & Timing Parser) | Created: 2026-11-22

export {
    EDID_BLOCK_SIZE,
    EDID_HEADER_MAGIC,
    DESC_SERIAL,
    DESC_UNSPECIFIED_TEXT,
    DESC_RANGE_LIMITS,
    DESC_MONITOR_NAME,
    DESC_DUMMY,
    CEA_EXT_TAG,
    edidChecksum,
    verifyEdidChecksum,
    decodePnpId,
    encodePnpId,
    EdidDetailedTiming,
    EdidDescriptor,
    EdidStandardTiming,
    EdidInfo,
    isEdid,
    parseEdid,
} from "./edid.cp";
