// Source: cp/lib/syslog/index.cp (Unified Exports for cap:syslog & cap.syslog)
// Pure CP zero-C-dependency RFC 5424 & RFC 3164 Syslog parser and formatter.

export {
    SyslogError,
    Facility,
    Severity,
    SdParam,
    SdElement,
    SyslogMessage,
} from "./types.cp";

export {
    parse,
    parseRfc5424,
    parseRfc3164,
} from "./parser.cp";

export {
    formatRfc5424,
    formatRfc3164,
} from "./formatter.cp";
