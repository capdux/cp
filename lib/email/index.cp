// Source: cp/lib/email/index.cp (Unified Exports for cap:email)
// Unified Public Exports for RFC 5322 Address Parser, MIME Composer/Parser & RFC 5321 SMTP Codec (cap:email & cap.email)

export {
    EmailError,
    EmailAddress,
    containsHeaderInjection,
} from "./address.cp";

export {
    QuotedPrintable,
    EmailAttachment,
    EmailHeader,
    ParsedPart,
    ParsedEmail,
    EmailBuilder,
    MimeParser,
} from "./mime.cp";

export {
    SmtpReply,
    SmtpCodec,
} from "./smtp.cp";
