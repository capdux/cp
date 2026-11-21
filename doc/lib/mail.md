# `cap:mail` — Email Mailbox, RFC 2047 Encoded-Word & Quoted-Printable Toolkit

Zero-allocation pure-CP email address, header encoding, and transfer-encoding toolkit converged from `mail_address`, `rfc2047_word`, and `quoted_printable`.

## Exports

### RFC 5322 Email Mailbox (`./address.cp`)
- `isValidAddrSpec(addr: []const u8): bool`
- `Mailbox` (`display_name`, `local_part`, `domain`, `parse(...)`)
- `MailboxListIter`

### RFC 2047 MIME Encoded-Word (`./rfc2047.cp`)
- `EncodedWordEncoding` (`base64`, `quoted_printable`)
- `EncodedWord`
- `parseEncodedWord(src: []const u8): EncodedWord?`
- `decodeEncodedWord(src: []const u8, out: []u8): ([]const u8)?`
- `decodeHeaderValue(src: []const u8, out: []u8): ([]const u8)?`
- `encodeUtf8Base64Word(src: []const u8, out: []u8): ([]const u8)?`

### RFC 2045 Quoted-Printable (`./qp.cp`)
- `encodeQuotedPrintable(src: []const u8, out: []u8): ([]const u8)?`
- `decodeQuotedPrintable(src: []const u8, out: []u8): ([]const u8)?`
