// Source: cp/lib/xml/index.cp (Unified Exports for cap:xml)
// Industrial-grade pure CP XML 1.0 DOM parser, strict zero-alloc validator, 16-lane SIMD scanner,
// entity codec, path query & serializer (hardened against RazrFalcon/roxmltree).

export {
    XML_MAX_DEPTH,
    XML_MAX_ATTRS_PER_ELEMENT,
    XML_MAX_NODES,
    XML_MAX_ENTITY_EXPANSIONS,
    XmlError,
    XmlAttr,
    XmlNodeKind,
    XmlNode,
    XmlDocument,
    XmlParser,
    simdFindXmlTextDelim,
    scalarFindXmlTextDelim,
    simdHasInvalidXmlControl,
    scalarHasInvalidXmlControl,
    simdFindAttrQuoteOrAmp,
    scalarFindAttrQuoteOrAmp,
    validateXmlStrict,
} from "./parser.cp";

export {
    XmlWriter,
    XmlSerializer,
} from "./writer.cp";

export {
    isValidXmlChar,
    escapeXmlText,
    escapeXmlAttr,
    wrapXmlCdata,
    unescapeXml,
} from "./escape.cp";
