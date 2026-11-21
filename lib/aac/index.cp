// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc AAC ADTS Frame & AudioSpecificConfig Parser)

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
