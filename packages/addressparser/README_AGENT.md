# addressparser@1.0.0 (Agent Quickref)

> Zero-allocation RFC 5322 email address and recipient list parser and formatter for CP | License: MIT
> Import: `import { ... } from "addressparser";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseMailbox(raw: string): MailAddress;`
- `export fn parseAddressList(raw: string): AddressList;`
- `export fn formatMailbox(m: MailAddress, out: []u8): string;`
