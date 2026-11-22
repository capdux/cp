// Origin: Pure-CP Stdlib (Zero-Alloc European Data Format EDF/EDF+ Biosignal Parser) | Created: 2026-11-22

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
