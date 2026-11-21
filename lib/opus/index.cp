// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Opus RFC 7845 & RFC 6716 TOC Parser)

export {
    OPUS_HEAD_MAGIC,
    OPUS_TAGS_MAGIC,
    OPUS_SAMPLE_RATE,
    OPUS_MAX_PACKET_DURATION_US,
    OpusMode,
    OpusBandwidth,
    OpusHead,
    OpusTag,
    OpusCommentIter,
    OpusTagsInfo,
    OpusToc,
    isOpusHead,
    isOpusTags,
    parseOpusHead,
    parseOpusTags,
    parseOpusToc,
} from "./opus.cp";
