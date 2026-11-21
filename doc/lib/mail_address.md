# `cap:mail_address` — RFC 5322 Email Mailbox & Address List Parser

Zero-allocation RFC 5322 email `addr-spec` validator, `"Display Name" <local@domain>` mailbox parser, and quoted-comma-aware address list iterator.

## Exports

- `isValidAddrSpec(addr): bool`
- `Mailbox` — `parse(raw): Mailbox?` (`display_name`, `address`, `local_part`, `domain`)
- `MailboxListIter` — `init(header)`, `next(*_): Mailbox?`
