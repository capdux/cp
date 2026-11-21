// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ArcSight CEF Rev 25 SIEM Parser)
// Unified exports for cap:cef.

export {
    CEF_PREFIX,
    CefEvent,
    CefExtPair,
    CefExtIter,
    isCef,
    parseCefLine,
    findCefExt,
    unescapeCefHeader,
    unescapeCefExtValue,
    formatCefHeader,
} from "./cef.cp";
