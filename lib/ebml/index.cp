// Origin: Pure-CP Stdlib (Zero-Alloc RFC 8794 EBML / WebM / MKV Parser) | Created: 2026-11-22

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
