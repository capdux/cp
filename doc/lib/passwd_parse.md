# `cap:passwd_parse` — POSIX `/etc/passwd` & `/etc/group` Parser

Zero-allocation POSIX `/etc/passwd` and `/etc/group` streaming parsers with username/UID/GID lookups and group membership checks.

## Exports

- `PasswdEntry` (`parseLine`, `isNologinShell`), `PasswdIter`, `lookupUserByName`, `lookupUserByUid`.
- `GroupEntry` (`parseLine`, `hasMember`), `GroupIter`, `lookupGroupByName`, `lookupGroupByGid`.
