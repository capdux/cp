// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T14:26:00Z (Unified Mail Domain Barrel)
export {
    isValidAddrSpec,
    Mailbox,
    MailboxListIter,
} from "./address.cp";

export {
    EncodedWordEncoding,
    EncodedWord,
    parseEncodedWord,
    decodeEncodedWord,
    decodeHeaderValue,
    encodeUtf8Base64Word,
} from "./rfc2047.cp";

export {
    encodeQuotedPrintable,
    decodeQuotedPrintable,
} from "./qp.cp";
