// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc MMA Downloadable Sounds .dls RIFF Parser)
// Unified exports for cap:dls.

export {
    DLS_RIFF_MAGIC,
    DLS_FORM_TYPE,
    DLS_BANK_DRUM_FLAG,
    DlsVersion,
    DlsInfo,
    DlsHeader,
    DlsInstrument,
    DlsWave,
    DlsInstIter,
    DlsWaveIter,
    isDls,
    parseDls,
} from "./dls.cp";
