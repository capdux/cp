// Source: cp/lib/mime/index.cp (Unified Exports for cap:mime & cap.mime)
// Pure CP zero-C-dependency RFC 6838 MediaType, RFC 9110 ContentNegotiator, and MIME Database.

export {
    MIME_MAX_NAME_LEN,
    MIME_MAX_PARAMS,
    MimeError,
    MimeParam,
    MediaType,
    ContentNegotiator,
    simdIsValidMimeToken,
    simdIsValidRestrictedName,
    simdFindMimeDelim,
    simdIndexOfSemicolon,
} from "./mime.cp";

export {
    MimeDb,
} from "./db.cp";

export {
    MimeMessageSplit,
    MimeHeader,
    splitHeadersAndBody,
    MimeHeaderIter,
    findMimeHeader,
    countMimeHeader,
} from "./headers.cp";

export * from "./sniff.cp";
export * from "./multipart.cp";

export {
    DispositionType,
    ContentDisposition,
    decodeRfc5987,
    sanitizeFilename,
    formatAttachment,
} from "./disposition.cp";

export {
    MimeCategory,
    ContentType,
    mimeFromExtension,
    categoryFromMime,
} from "./lookup.cp";

export {
    Category,
    classifyMime,
    byExtension,
    byPath,
} from "./type.cp";

export {
    FileKind,
    FileTypeInfo,
    detectFileType,
} from "./fileType.cp";
