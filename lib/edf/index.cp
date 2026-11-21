// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc European Data Format EDF/EDF+ Biosignal Parser)

export {
    EDF_MAIN_HEADER_SIZE,
    EDF_SIGNAL_HEADER_SIZE,
    EdfHeader,
    EdfSignal,
    isEdf,
    parseEdfHeader,
    parseEdfSignal,
    edfRecordByteSize,
    readEdfDigitalSample,
} from "./edf.cp";
