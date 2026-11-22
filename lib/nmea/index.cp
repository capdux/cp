// Origin: Pure-CP Stdlib (Zero-Alloc NMEA 0183 GNSS Sentence & Stream Parser) | Created: 2026-11-22
// Unified exports for cap:nmea.

export {
    NmeaSentence,
    NmeaGga,
    NmeaRmc,
    NmeaVtg,
    NmeaFieldIter,
    NmeaLineIter,
    nmeaChecksum,
    verifyNmeaSentence,
    parseNmeaSentence,
    parseNmeaCoord,
    parseNmeaGga,
    parseNmeaRmc,
    parseNmeaVtg,
} from "./nmea.cp";
