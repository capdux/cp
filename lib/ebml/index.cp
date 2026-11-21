// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 8794 EBML / WebM / MKV Parser)

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
