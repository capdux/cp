// Origin: Pure-CP Stdlib (Zero-Alloc ArcSight CEF Rev 25 SIEM Parser) | Created: 2026-11-22
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
