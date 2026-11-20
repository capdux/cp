// Source: cp/lib/wav/index.cp (Unified Exports for cap:wav & cap.wav)
// Pure CP RIFF/WAVE PCM Audio Container, Mixer & Resampler.

export {
    WavError,
    SampleFormat,
    WavAudio,
    decode,
    encode,
    toMono,
    resampleLinear,
    peakAmplitude,
    normalizePeak,
} from "./wav.cp";
