# `cap:resolv_conf` — POSIX `/etc/resolv.conf` Parser

Zero-allocation POSIX `/etc/resolv.conf` parser supporting `nameserver`, `search`/`domain` (last-wins per `resolv.conf(5)`), `options` (`ndots`, `timeout`, `attempts`, `rotate`, `edns0`, `use-vc`, `trust-ad`), and `#`/`;` comments.

## Exports

- `NameserverIter` (`init(content)`, `next(*_): ([]const u8)?`)
- `ResolvOptions` (`defaults()`, `parse(content)`)
- `effectiveSearchRaw(content): ([]const u8)?`
- `firstNameserver(content): ([]const u8)?`
