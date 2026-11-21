// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux LUKS1 & LUKS2 Disk Encryption Header Parser)

export {
    LUKS_MAGIC,
    LUKS2_MAGIC_SECONDARY,
    LUKS_V1,
    LUKS_V2,
    LUKS1_HEADER_SIZE,
    LUKS1_NUM_KEYSLOTS,
    LUKS1_KEYSLOT_SIZE,
    LUKS1_KEY_ENABLED,
    LUKS1_KEY_DISABLED,
    LUKS2_BIN_HEADER_SIZE,
    LUKS2_MIN_HDR_SIZE,
    peekLuksVersion,
    isLuks,
    Luks1Keyslot,
    Luks1Header,
    parseLuks1Header,
    Luks2Header,
    parseLuks2Header,
    selectActiveLuks2,
} from "./luks.cp";
