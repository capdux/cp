// Origin: Pure-CP Stdlib (Zero-Alloc Impulse Tracker 2.14 .it Binary Module Parser) | Created: 2026-11-22

export {
    IT_MAGIC,
    IT_INST_MAGIC,
    IT_SAMPLE_MAGIC,
    IT_HEADER_SIZE,
    IT_SAMPLE_HEADER_SIZE,
    IT_FLAG_STEREO,
    IT_FLAG_USE_INSTRUMENTS,
    IT_FLAG_LINEAR_SLIDES,
    IT_SPECIAL_HAS_MESSAGE,
    ItHeader,
    ItSample,
    ItInstrument,
    ItPattern,
    isIt,
    parseItHeader,
    parseItSample,
    parseItInstrument,
    parseItPattern,
} from "./it.cp";
