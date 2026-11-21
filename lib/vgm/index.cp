// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc VGM 1.00-1.71 Chiptune & GD3 Parser)

export {
    VGM_MAGIC,
    VGM_GD3_MAGIC,
    VGM_MIN_HEADER_SIZE,
    VGM_SAMPLE_RATE,
    CMD_WAIT_N,
    CMD_WAIT_60HZ,
    CMD_WAIT_50HZ,
    CMD_END,
    CMD_DATA_BLOCK,
    CMD_WAIT_SHORT_START,
    VgmHeader,
    VgmGd3,
    VgmCmdStats,
    isVgm,
    parseVgmHeader,
    parseVgmGd3,
    decodeUtf16LeToUtf8,
    scanVgmCommands,
} from "./vgm.cp";
