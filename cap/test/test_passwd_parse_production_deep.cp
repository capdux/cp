import {
    PasswdEntry,
    lookupUserByName,
    lookupUserByUid,
    lookupGroupByName,
    lookupGroupByGid,
} from "cap:passwd_parse";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

const SAMPLE_PASSWD =
    "# comment\n" ++
    "root:x:0:0:root:/root:/bin/bash\n" ++
    "nobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin\r\n" ++
    "alice:x:1000:1000:Alice Smith:/home/alice:/bin/zsh\n" ++
    "malformed:only:3\n";

const SAMPLE_GROUP =
    "root:x:0:\n" ++
    "sudo:x:27:alice,bob\n" ++
    "docker:x:999:alice\n";

test("PasswdEntry parsing and lookups", () => {
    const root = lookupUserByName(SAMPLE_PASSWD, "root").?;
    expect(root.uid == 0 && root.gid == 0);
    expect(strEq(root.home_dir, "/root"));
    expect(!root.isNologinShell());

    const nob = lookupUserByUid(SAMPLE_PASSWD, 65534).?;
    expect(strEq(nob.username, "nobody"));
    expect(nob.isNologinShell());

    const alice = lookupUserByUid(SAMPLE_PASSWD, 1000).?;
    expect(strEq(alice.gecos, "Alice Smith"));
    expect(strEq(alice.shell, "/bin/zsh"));

    expect(lookupUserByName(SAMPLE_PASSWD, "missing") == null);
    expect(PasswdEntry.parseLine("a:b:notnum:0::/:/bin/sh") == null);
    expect(PasswdEntry.parseLine("a:b:1:2:3:4:5:extra") == null);
});

test("GroupEntry parsing, lookups, and membership", () => {
    const sudo = lookupGroupByName(SAMPLE_GROUP, "sudo").?;
    expect(sudo.gid == 27);
    expect(sudo.hasMember("alice"));
    expect(sudo.hasMember("bob"));
    expect(!sudo.hasMember("charlie"));

    const docker = lookupGroupByGid(SAMPLE_GROUP, 999).?;
    expect(strEq(docker.name, "docker"));
    expect(docker.hasMember("alice"));
    expect(lookupGroupByGid(SAMPLE_GROUP, 12345) == null);
});
