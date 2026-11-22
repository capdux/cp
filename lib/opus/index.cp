// Origin: Pure-CP Stdlib (Zero-Alloc Opus RFC 7845 & RFC 6716 TOC Parser) | Created: 2026-11-22

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
