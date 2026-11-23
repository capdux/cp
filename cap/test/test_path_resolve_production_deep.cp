import { test, expect } from "cap:test";
import {
    ParentRelation,
    isParentOrEqual,
    isPathInside,
    longestCommonPath,
    normalizePosixBuf as normalizePosix,
    joinNormalized,
    relativePosixBuf as relativePosix,
} from "cap:path";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("path_resolve: normalizePosix, joinNormalized, and relativePosix", () => {
    let buf: [256]u8 = [0; 256];

    expect(strEq(normalizePosix("/a/b/../c//./d/", buf[0..]).?, "/a/c/d"));
    expect(strEq(normalizePosix("a/../../b", buf[0..]).?, "../b"));
    expect(strEq(normalizePosix("/../../etc/passwd", buf[0..]).?, "/etc/passwd"));
    expect(strEq(normalizePosix("", buf[0..]).?, "."));

    expect(strEq(joinNormalized("/app/src", "../lib/index.cp", buf[0..]).?, "/app/lib/index.cp"));
    expect(strEq(joinNormalized("/app/src", "/etc/hosts", buf[0..]).?, "/etc/hosts"));

    expect(strEq(relativePosix("/a/b/c", "/a/d/e.cp", buf[0..]).?, "../../d/e.cp"));
    expect(strEq(relativePosix("/a/b", "/a/b", buf[0..]).?, "."));
});

test("path_resolve: isParentOrEqual, isPathInside, and longestCommonPath", () => {
    expect(isParentOrEqual("/srv/www/", "/srv/www/index.html") == ParentRelation.parent);
    expect(isParentOrEqual("/srv/www", "/srv/www/") == ParentRelation.equal);
    expect(isParentOrEqual("/srv/www", "/srv/www2/index.html") == ParentRelation.unrelated);

    expect(isPathInside("/srv/www", "/srv/www/a/../b.js"));
    expect(!isPathInside("/srv/www", "/srv/www/../../etc/passwd"));

    const ps: [3][]const u8 = ["/repo/src/a/x.cp", "/repo/src/a/y.cp", "/repo/src/b/z.cp"];
    expect(strEq(longestCommonPath(ps[0..]).?, "/repo/src"));

    const ps2: [2][]const u8 = ["/a/b", "/c/d"];
    expect(strEq(longestCommonPath(ps2[0..]).?, "/"));
});
