// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc MPEG-1/2/2.5 Layer I/II/III & Xing/Info/VBRI Parser)

export {
    MpegVersion,
    MpegLayer,
    MpegChannelMode,
    MpegFrameHeader,
    MpegFrame,
    MpegFrameIter,
    XingHeader,
    MpegSummary,
    skipId3v2,
    parseMpegFrameHeader,
    isMpegAudio,
    parseXingHeader,
    summarizeMpeg,
} from "./mpeg.cp";
