import {
    GroupEntry,
    GroupMemberIter,
    EtcGroupIter,
    findGroupByName,
    findGroupByGid,
    isUserInGroup,
} from "cap:etc_group";
import { test, expect, expect_eq } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

const SAMPLE_ETC_GROUP =
    "# Linux /etc/group sample\n" ++
    "\n" ++
    "   # indented comment\r\n" ++
    "root:x:0:\n" ++
    "daemon:x:1:root,daemon\r\n" ++
    "sudo:x:27:alice,bob,devops\n" ++
    "malformed:only:three\n" ++
    "too:many:100:a:extra\n" ++
    "badgid:x:notanumber:alice\n" ++
    ":x:200:noname\n" ++
    "docker:x:999:alice,charlie\n" ++
    "nobody:x:65534:\n" ++
    "maxgid:x:4294967295:u1,u2,u3";

test("EtcGroupIter streams valid entries and skips comments, blanks, and malformed lines", () => {
    let it = EtcGroupIter.init(SAMPLE_ETC_GROUP);

    const g0 = it.next().?;
    expect(strEq(g0.name, "root"));
    expect(strEq(g0.passwd, "x"));
    expect_eq(g0.gid, 0);
    expect_eq(g0.members.len, 0);
    expect_eq(g0.memberCount(), 0);

    const g1 = it.next().?;
    expect(strEq(g1.name, "daemon"));
    expect_eq(g1.gid, 1);
    expect_eq(g1.memberCount(), 2);

    const g2 = it.next().?;
    expect(strEq(g2.name, "sudo"));
    expect_eq(g2.gid, 27);
    expect_eq(g2.memberCount(), 3);

    const g3 = it.next().?;
    expect(strEq(g3.name, "docker"));
    expect_eq(g3.gid, 999);
    expect_eq(g3.memberCount(), 2);

    const g4 = it.next().?;
    expect(strEq(g4.name, "nobody"));
    expect_eq(g4.gid, 65534);
    expect_eq(g4.memberCount(), 0);

    const g5 = it.next().?;
    expect(strEq(g5.name, "maxgid"));
    expect_eq(g5.gid, 4294967295);
    expect_eq(g5.memberCount(), 3);

    expect(it.next() == null);
    expect(it.next() == null);
});

test("GroupMemberIter handles empty, single, multi, and consecutive/trailing commas", () => {
    let empty_it = GroupMemberIter.init("");
    expect(empty_it.next() == null);

    let single_it = GroupMemberIter.init("alice");
    expect(strEq(single_it.next().?, "alice"));
    expect(single_it.next() == null);

    let messy_it = GroupMemberIter.init(",alice,,bob,charlie,,");
    expect(strEq(messy_it.next().?, "alice"));
    expect(strEq(messy_it.next().?, "bob"));
    expect(strEq(messy_it.next().?, "charlie"));
    expect(messy_it.next() == null);

    const entry = GroupEntry.parseLine("wheel:x:10:,alice,,bob,").?;
    expect_eq(entry.memberCount(), 2);
    let m_it = entry.memberIter();
    expect(strEq(m_it.next().?, "alice"));
    expect(strEq(m_it.next().?, "bob"));
    expect(m_it.next() == null);
});

test("GroupEntry.hasMember prevents substring false positives", () => {
    const sudo = findGroupByName(SAMPLE_ETC_GROUP, "sudo").?;
    expect(sudo.hasMember("alice"));
    expect(sudo.hasMember("bob"));
    expect(sudo.hasMember("devops"));

    // Substring / prefix / suffix / empty checks must all be false
    expect(!sudo.hasMember("dev"));
    expect(!sudo.hasMember("ops"));
    expect(!sudo.hasMember("ali"));
    expect(!sudo.hasMember("lice"));
    expect(!sudo.hasMember("alice,bob"));
    expect(!sudo.hasMember(""));

    const root = findGroupByName(SAMPLE_ETC_GROUP, "root").?;
    expect(!root.hasMember("root"));
    expect(!root.hasMember(""));
});

test("findGroupByName, findGroupByGid, and isUserInGroup lookups", () => {
    const by_name = findGroupByName(SAMPLE_ETC_GROUP, "docker").?;
    expect_eq(by_name.gid, 999);
    expect(strEq(by_name.members, "alice,charlie"));

    const by_gid = findGroupByGid(SAMPLE_ETC_GROUP, 27).?;
    expect(strEq(by_gid.name, "sudo"));

    expect(findGroupByName(SAMPLE_ETC_GROUP, "nonexistent") == null);
    expect(findGroupByName(SAMPLE_ETC_GROUP, "") == null);
    expect(findGroupByGid(SAMPLE_ETC_GROUP, 123456) == null);

    expect(isUserInGroup(SAMPLE_ETC_GROUP, "sudo", "devops"));
    expect(!isUserInGroup(SAMPLE_ETC_GROUP, "sudo", "dev"));
    expect(isUserInGroup(SAMPLE_ETC_GROUP, "docker", "charlie"));
    expect(!isUserInGroup(SAMPLE_ETC_GROUP, "docker", "bob"));
    expect(!isUserInGroup(SAMPLE_ETC_GROUP, "missing", "alice"));
    expect(!isUserInGroup(SAMPLE_ETC_GROUP, "", "alice"));
    expect(!isUserInGroup(SAMPLE_ETC_GROUP, "sudo", ""));
});

test("Malformed line rejection and u32 GID overflow protection", () => {
    // Empty, whitespace, comments
    expect(GroupEntry.parseLine("") == null);
    expect(GroupEntry.parseLine("   \t  ") == null);
    expect(GroupEntry.parseLine("#root:x:0:") == null);
    expect(GroupEntry.parseLine("  # comment") == null);

    // Column count mismatches
    expect(GroupEntry.parseLine("onlyone") == null);
    expect(GroupEntry.parseLine("a:b") == null);
    expect(GroupEntry.parseLine("a:b:1") == null);
    expect(GroupEntry.parseLine("a:b:1:c:d") == null);

    // Invalid GID values (negative, non-digit, empty, overflow u32::MAX + 1)
    expect(GroupEntry.parseLine("g:x::alice") == null);
    expect(GroupEntry.parseLine("g:x:-1:alice") == null);
    expect(GroupEntry.parseLine("g:x:12a:alice") == null);
    expect(GroupEntry.parseLine("g:x:4294967296:alice") == null);
    expect(GroupEntry.parseLine("g:x:99999999999:alice") == null);

    // Exact u32::MAX boundary and empty passwd field are valid
    const edge = GroupEntry.parseLine("edge::4294967295:u1\r").?;
    expect(strEq(edge.name, "edge"));
    expect_eq(edge.passwd.len, 0);
    expect_eq(edge.gid, 4294967295);
    expect(edge.hasMember("u1"));

    // Empty input to iterators and lookups
    let empty_iter = EtcGroupIter.init("");
    expect(empty_iter.next() == null);
    expect(findGroupByName("", "root") == null);
    expect(findGroupByGid("", 0) == null);
    expect(!isUserInGroup("", "root", "root"));
});
