// Source: cp/lib/audio/index.cp (Unified Exports for cap:audio)
// Unified Public Exports for RIFF/WAVE Audio Codec & Voice AI DSP/VAD Engine (cap:audio & cap.audio)

export {
    AudioError,
    WavAudio,
    WavFloatAudio,
    WavCodec,
} from "./wav.cp";

export {
    SpeechSegment,
    EnergyVad,
    AudioDsp,
} from "./dsp.cp";
