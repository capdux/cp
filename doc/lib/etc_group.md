# `cap:etc_group` — Linux `/etc/group` Zero-Allocation Parser

Zero-allocation, pure-CP streaming parser and membership query library for Linux `/etc/group` files (`name:passwd:gid:member1,member2`). Designed as a companion to `cap:passwd_parse`.

## Exports

### Types

- `GroupEntry`
  - Fields: `name: []const u8`, `passwd: []const u8`, `gid: u32`, `members: []const u8`
  - `GroupEntry.parseLine(raw: []const u8): GroupEntry?` — Parses a single 4-field colon-delimited line (strips trailing `\r`, rejects blank/comment/malformed lines and invalid/overflowing GIDs).
  - `hasMember(username: []const u8): bool` — Exact comma-delimited membership check (prevents substring false positives such as `"dev"` matching `"devops"`).
  - `memberCount(): usize` — Counts non-empty members in `members`.
  - `memberIter(): GroupMemberIter` — Returns a zero-allocation iterator over `members`.
- `GroupMemberIter`
  - `GroupMemberIter.init(members: []const u8): GroupMemberIter`
  - `next(*_): ([]const u8)?` — Yields each non-empty comma-delimited member slice.
- `EtcGroupIter`
  - `EtcGroupIter.init(content: []const u8): EtcGroupIter`
  - `next(*_): GroupEntry?` — Streams valid `GroupEntry` records across `\n` and `\r\n` lines, skipping blank lines, `#` comments, and malformed records.

### Lookup Functions

- `findGroupByName(content: []const u8, name: []const u8): GroupEntry?`
- `findGroupByGid(content: []const u8, gid: u32): GroupEntry?`
- `isUserInGroup(content: []const u8, group_name: []const u8, username: []const u8): bool`

## Example

```typescript
import {
    EtcGroupIter,
    findGroupByName,
    findGroupByGid,
    isUserInGroup,
} from "cap:etc_group";

const content =
    "# System groups\n" ++
    "root:x:0:\n" ++
    "sudo:x:27:alice,bob\n" ++
    "docker:x:999:alice,devops\n";

if (sudo := findGroupByName(content, "sudo")) {
    let it = sudo.memberIter();
    while (member := it.next()) {
        // yields "alice", then "bob"
    }
}

const in_docker = isUserInGroup(content, "docker", "devops"); // true
const false_pos = isUserInGroup(content, "docker", "dev");    // false
```
