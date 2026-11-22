// Origin: Pure-CP Stdlib (Unified Audio, Video, Container, Subtitle & Metadata Suite: cap:media) | Created: 2026-11-22
// ============================================================================
// Pure-CP Zero-C-Dependency Multimedia Containers, Audio/Video Codecs,
// Subtitles (SRT/WebVTT), Audio Metadata (ID3) & Voice AI DSP/VAD.
// ============================================================================

// --- Audio Codec & Voice AI DSP/VAD ---
export {
    AudioError,
    WavAudio,
    WavFloatAudio,
    WavCodec,
} from "./audio_wav.cp";

export {
    SpeechSegment,
    EnergyVad,
    AudioDsp,
} from "./audio_dsp.cp";

export {
    FLAC_MAGIC_SIZE,
    FLAC_BLOCK_HEADER_SIZE,
    FLAC_STREAMINFO_SIZE,
    FlacBlockType,
    FlacMetadataBlock,
    FlacStreamInfo,
    isFlac,
    FlacBlockIter,
    parseStreamInfoPayload,
    parseFlacStreamInfo,
} from "./flac.cp";

export {
    OGG_FLAG_CONTINUATION,
    OGG_FLAG_BOS,
    OGG_FLAG_EOS,
    OggPage,
    oggCrc32,
    isOgg,
    OggPageIter,
} from "./ogg.cp";

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

export {
    ADTS_HEADER_SIZE,
    ADTS_HEADER_CRC_SIZE,
    AAC_SAMPLES_PER_BLOCK,
    AAC_SAMPLE_RATES,
    AacProfile,
    aacSampleRate,
    aacChannelCount,
    aacProfileFromObjectType,
    AdtsFrame,
    AudioSpecificConfig,
    AdtsStats,
    isAdts,
    AdtsIter,
    parseAudioSpecificConfig,
    summarizeAdts,
} from "./aac.cp";

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

export {
    MidiError,
    MidiFormat,
    NoteEventData,
    PolyPressureData,
    ControlChangeData,
    ProgramChangeData,
    ChannelPressureData,
    PitchBendData,
    MetaOtherData,
    MidiEventKind,
    MidiEvent,
    MidiTrack,
    MidiFile,
    parse,
    encode,
} from "./midi.cp";

// --- Video & Binary Media Containers ---
export {
    MP4_BOX_HEADER_SIZE,
    MP4_LARGE_HEADER_SIZE,
    MP4_UUID_SIZE,
    Mp4Box,
    FtypBox,
    MvhdBox,
    MdhdBox,
    TkhdBox,
    BoxCursor,
    fourccEquals,
    findBox,
    findBoxPath,
    parseFtyp,
    isMp4,
    parseMvhd,
    parseMdhd,
    parseTkhd,
} from "./mp4.cp";

export {
    ID_EBML,
    ID_DOC_TYPE,
    ID_SEGMENT,
    ID_INFO,
    ID_TIMECODE_SCALE,
    ID_DURATION,
    ID_MUXING_APP,
    ID_WRITING_APP,
    ID_TRACKS,
    ID_TRACK_ENTRY,
    ID_TRACK_NUMBER,
    ID_TRACK_TYPE,
    ID_CODEC_ID,
    ID_VIDEO,
    ID_PIXEL_WIDTH,
    ID_PIXEL_HEIGHT,
    ID_AUDIO,
    ID_SAMPLING_FREQ,
    ID_CHANNELS,
    ID_CLUSTER,
    MKV_DEFAULT_TIMECODE_SCALE_NS,
    MkvTrackType,
    mkvTrackTypeFromInt,
    MkvInfo,
    MkvTrack,
    MkvTrackIter,
    MkvHeader,
    isMkv,
    isWebm,
    parseMkvHeader,
} from "./mkv.cp";

export {
    FLV_HEADER_SIZE,
    FLV_TAG_HEADER_SIZE,
    FLV_TAG_AUDIO,
    FLV_TAG_VIDEO,
    FLV_TAG_SCRIPT,
    FlvTagType,
    FlvHeader,
    FlvTag,
    FlvAudioInfo,
    FlvVideoInfo,
    FlvTagIter,
    isFlv,
    parseFlvHeader,
    parseFlvAudioHeader,
    parseFlvVideoHeader,
} from "./flv.cp";

export {
    EBML_ID_HEADER,
    EBML_ID_VERSION,
    EBML_ID_DOCTYPE,
    EBML_ID_SEGMENT,
    EbmlVint,
    EbmlElement,
    decodeEbmlId,
    decodeEbmlSize,
    isEbmlHeader,
    EbmlElementIter,
    decodeEbmlUint,
} from "./ebml.cp";

export {
    RIFF_HEADER_SIZE,
    RIFF_CHUNK_HEADER_SIZE,
    RiffHeader,
    RiffChunk,
    parseRiffHeader,
    parseListType,
    RiffChunkIter,
    findRiffChunk,
} from "./riff.cp";

// --- Subtitles & Audio Metadata ---
export {
    SrtCue,
    parseSrtTimestampMs,
    formatSrtTimestampMs,
    SrtCueIter,
    formatSrtCue,
    shiftSrtCue,
} from "./srt.cp";

export {
    VttCue,
    isWebVttHeader,
    parseVttTimestampMs,
    formatVttTimestampMs,
    VttCueIter,
    formatVttCue,
} from "./vtt.cp";

export {
    Id3v2Header,
    Id3v2Frame,
    Id3v1Tag,
    decodeSyncsafeU32,
    parseId3v2Header,
    Id3v2FrameIter,
    parseId3v1Tail,
} from "./id3.cp";
