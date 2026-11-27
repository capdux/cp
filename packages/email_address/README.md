[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/email-address

Zero-heap RFC 5321/5322 email address parser and validator in pure CP supporting bare addresses and `Display Name <user@domain>` format.

## API

- `parseEmail(input: string): EmailAddress` (`local_part`, `domain`, `display_name`, `valid`)
- `isValidEmail(input: string): bool`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`email_address@0.2.9`)
- **Upstream Repository**: https://github.com/johnstonskj/rust-email_address
- **License**: MIT (clean-room CP implementation)
