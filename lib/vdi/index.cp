// Origin: Pure-CP Stdlib (Public Unified Export for cap:vdi & cap.vdi) | Created: 2026-11-22

export {
    VDI_SIGNATURE,
    VDI_MIN_FILE_HEADER_SIZE,
    VDI_V1_1_MIN_HEADER_SIZE,
    VDI_DEFAULT_SECTOR_SIZE,
    VDI_DEFAULT_BLOCK_SIZE,
    TYPE_DYNAMIC,
    TYPE_STATIC,
    TYPE_UNDO,
    TYPE_DIFF,
    VDI_BLOCK_FREE,
    VDI_BLOCK_ZERO,
    VdiHeader,
    VdiBlockMapping,
    isVdi,
    parseVdiHeader,
    decodeVdiBlockEntry,
    lookupVdiBlock,
} from "./vdi.cp";
