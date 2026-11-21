// Source: cp/lib/xml/index.cp (Unified Exports for cap:xml)
// Industrial-grade pure CP XML 1.0 DOM parser, entity codec, path query & serializer.

export {
    XmlError,
    XmlAttr,
    XmlNodeKind,
    XmlNode,
    XmlDocument,
    XmlParser,
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
