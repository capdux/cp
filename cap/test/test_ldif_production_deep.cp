// Source: cp/cap/test/test_ldif_production_deep.cp (Production deep tests for cap:ldif)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LdifValueKind,
    LdifChangeType,
    LdifAttr,
    LdifEntry,
    LdifStats,
    LdifAttrIter,
    LdifIter,
    isLdif,
    summarizeLdif,
} from "cap:net";

const SAMPLE_DIRECTORY_LDIF: []const u8 =
    "# LDAP Directory Export\r\n" ++
    "version: 1\r\n" ++
    "\r\n" ++
    "dn: cn=Barbara Jensen, ou=Product Development, dc=airius, dc=com\r\n" ++
    "objectClass: top\r\n" ++
    "objectClass: person\r\n" ++
    "cn: Barbara Jensen\r\n" ++
    "mail: bjensen@airius.com\r\n" ++
    "description: A big sailing fan that does not like\r\n" ++
    "  to stay indoors during summer\r\n" ++
    " months.\r\n" ++
    "\r\n" ++
    "# Base64 DN and URL photo entry\n" ++
    "dn:: b3U95Za25qWt6YOoLG89QWlyaXVz\n" ++
    "objectClass: organizationalUnit\n" ++
    "ou:: 5Za25qWt6YOo\n" ++
    "jpegPhoto:< file:///usr/local/directory/photos/logo.jpg\n";

const SAMPLE_CHANGES_LDIF: []const u8 =
    "version: 1\n" ++
    "\n" ++
    "dn: cn=Fiona Jensen, ou=Marketing, dc=airius, dc=com\n" ++
    "changetype: add\n" ++
    "objectClass: person\n" ++
    "cn: Fiona Jensen\n" ++
    "sn: Jensen\n" ++
    "\n" ++
    "dn: cn=Barbara Jensen, ou=Product Development, dc=airius, dc=com\n" ++
    "changetype: modify\n" ++
    "replace: mail\n" ++
    "mail: barbara@airius.com\n" ++
    "-\n" ++
    "add: title\n" ++
    "title: Senior Architect\n" ++
    "-\n" ++
    "\n" ++
    "dn: cn=Old User, ou=Temp, dc=airius, dc=com\n" ++
    "changetype: delete\n" ++
    "\n" ++
    "dn: cn=Bob Smith, ou=Sales, dc=airius, dc=com\n" ++
    "changetype: modrdn\n" ++
    "newrdn: cn=Robert Smith\n" ++
    "deleteoldrdn: 1\n";

test("1. version: 1 header, multi-entry directory iteration, and case-insensitive findAttr", () => {
    expect_eq(isLdif(SAMPLE_DIRECTORY_LDIF), true);

    let it = LdifIter.init(SAMPLE_DIRECTORY_LDIF);
    const e1 = it.next() ?? unreachable;
    expect_eq(it.version, 1);
    expect_eq(e1.dn_is_base64, false);
    expect_eq(e1.change_type == LdifChangeType.content, true);
    expect_eq(std.mem.eql(u8, e1.dn, "cn=Barbara Jensen, ou=Product Development, dc=airius, dc=com"), true);

    // 大小写不敏感匹配 LDAP 属性名
    const oc = e1.findAttr("OBJECTCLASS") ?? unreachable;
    expect_eq(std.mem.eql(u8, oc.raw_value, "top"), true);
    const mail = e1.findAttr("Mail") ?? unreachable;
    expect_eq(std.mem.eql(u8, mail.raw_value, "bjensen@airius.com"), true);
    expect_eq(e1.findAttr("nonexistent") == null, true);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.dn_is_base64, true);
    expect_eq(std.mem.eql(u8, e2.dn, "b3U95Za25qWt6YOoLG89QWlyaXVz"), true);
    expect_eq(it.next() == null, true);
});

test("2. RFC 2849 multi-line single-space folding and unfoldValue (CRLF + LF + double space)", () => {
    let it = LdifIter.init(SAMPLE_DIRECTORY_LDIF);
    const e1 = it.next() ?? unreachable;
    const desc = e1.findAttr("description") ?? unreachable;
    expect_eq(desc.kind == LdifValueKind.plain, true);

    // 第二行以双空格开头：剥离 "\r\n " 后保留第二个空格 -> "...like to stay..."
    // 第三行以单空格开头：剥离 "\r\n " 后直接拼接 -> "...summermonths."
    let buf: [128]u8 = [0; 128];
    const n = desc.unfoldValue(buf[0..]) ?? unreachable;
    const expected = "A big sailing fan that does not like to stay indoors during summermonths.";
    expect_eq(std.mem.eql(u8, buf[0..n], expected), true);

    // 验证 LF (\n ) 折行展开
    const lf_rec = "dn: cn=Test\ncomment: hello\n world\n!\n";
    let it2 = LdifIter.init(lf_rec);
    const e2 = it2.next() ?? unreachable;
    const c_attr = e2.findAttr("comment") ?? unreachable;
    const n2 = c_attr.unfoldValue(buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n2], "helloworld!"), true);
});

test("3. Base64 (::) and URL (:<) attribute value kind detection", () => {
    let it = LdifIter.init(SAMPLE_DIRECTORY_LDIF);
    * = it.next() ?? unreachable;
    const e2 = it.next() ?? unreachable;

    const ou = e2.findAttr("ou") ?? unreachable;
    expect_eq(ou.kind == LdifValueKind.base64, true);
    expect_eq(std.mem.eql(u8, ou.raw_value, "5Za25qWt6YOo"), true);

    const photo = e2.findAttr("jpegPhoto") ?? unreachable;
    expect_eq(photo.kind == LdifValueKind.url, true);
    expect_eq(std.mem.eql(u8, photo.raw_value, "file:///usr/local/directory/photos/logo.jpg"), true);
});

test("4. changetype add/modify/delete/modrdn parsing, '-' skipping, and summarizeLdif stats", () => {
    let it = LdifIter.init(SAMPLE_CHANGES_LDIF);

    const c1 = it.next() ?? unreachable;
    expect_eq(c1.change_type == LdifChangeType.add, true);

    const c2 = it.next() ?? unreachable;
    expect_eq(c2.change_type == LdifChangeType.modify, true);
    // 验证 modify 记录中 "-" 分隔行被自动跳过
    let ait = c2.attrs();
    let count: u32 = 0;
    while (a := ait.next()) {
        expect_eq(std.mem.eql(u8, a.name, "-"), false);
        count += 1;
    }
    // changetype + replace + mail + add + title = 5
    expect_eq(count, 5);

    const c3 = it.next() ?? unreachable;
    expect_eq(c3.change_type == LdifChangeType.delete, true);

    const c4 = it.next() ?? unreachable;
    expect_eq(c4.change_type == LdifChangeType.modrdn, true);
    expect_eq(it.next() == null, true);

    const stats = summarizeLdif(SAMPLE_CHANGES_LDIF);
    expect_eq(stats.version, 1);
    expect_eq(stats.entry_count, 4);
    expect_eq(stats.add_count, 1);
    expect_eq(stats.modify_count, 1);
    expect_eq(stats.delete_count, 1);
    expect_eq(stats.base64_attr_count, 0);

    const dir_stats = summarizeLdif(SAMPLE_DIRECTORY_LDIF);
    expect_eq(dir_stats.version, 1);
    expect_eq(dir_stats.entry_count, 2);
    expect_eq(dir_stats.base64_attr_count, 1); // ou::
});

test("5. Resilience against missing dn:, comment-only blocks, empty input, and small unfold buffer", () => {
    expect_eq(isLdif(""), false);
    expect_eq(isLdif("# only comments\n# nothing else\n"), false);
    expect_eq(isLdif("cn: Missing DN\nmail: test@example.com\n"), false);

    // 缺失 dn 的畸形块被跳过，后续合法块仍正常产出
    const mixed =
        "# Leading comment\n\n" ++
        "cn: Orphan Without DN\n" ++
        "mail: bad@example.com\n\n" ++
        "dn: cn=Valid User, dc=example, dc=com\n" ++
        "cn: Valid User\n";
    let it = LdifIter.init(mixed);
    const valid = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, valid.dn, "cn=Valid User, dc=example, dc=com"), true);
    expect_eq(it.next() == null, true);

    // unfoldValue 输出缓冲区不足时返回 null（不越界）
    const cn = valid.findAttr("cn") ?? unreachable;
    let tiny: [4]u8 = [0; 4];
    expect_eq(cn.unfoldValue(tiny[0..]) == null, true);
});
