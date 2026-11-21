// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ISO 2709 / MARC 21 Binary Parser)
// Unified exports for cap:marc.

export {
    MARC_LEADER_SIZE,
    MARC_DIR_ENTRY_SIZE,
    MARC_RECORD_TERMINATOR,
    MARC_FIELD_TERMINATOR,
    MARC_SUBFIELD_DELIM,
    MarcLeader,
    MarcSubfield,
    MarcSubfieldIter,
    MarcField,
    MarcFieldIter,
    MarcRecord,
    MarcRecordIter,
    isMarcRecord,
    parseMarcLeader,
} from "./marc.cp";
