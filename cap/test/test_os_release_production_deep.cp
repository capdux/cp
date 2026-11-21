import {
    getOsReleaseField,
    isOsLike,
} from "cap:sys";
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

const SAMPLE_OS =
    "# Ubuntu 24.04\n" ++
    "PRETTY_NAME=\"Ubuntu 24.04 \\\"Noble\\\" LTS\"\n" ++
    "NAME=\"Ubuntu\"\n" ++
    "VERSION_ID=\"24.04\"\n" ++
    "ID=ubuntu\n" ++
    "ID_LIKE=\"debian posix\"\n" ++
    "HOME_URL='https://www.ubuntu.com/'\n";

test("getOsReleaseField and isOsLike", () => {
    let buf: [128]u8 = [0; 128];
    expect(strEq(getOsReleaseField(SAMPLE_OS, "PRETTY_NAME", buf[0..]).?, "Ubuntu 24.04 \"Noble\" LTS"));
    expect(strEq(getOsReleaseField(SAMPLE_OS, "ID", buf[0..]).?, "ubuntu"));
    expect(strEq(getOsReleaseField(SAMPLE_OS, "VERSION_ID", buf[0..]).?, "24.04"));
    expect(strEq(getOsReleaseField(SAMPLE_OS, "HOME_URL", buf[0..]).?, "https://www.ubuntu.com/"));
    expect(getOsReleaseField(SAMPLE_OS, "MISSING", buf[0..]) == null);

    expect(isOsLike(SAMPLE_OS, "ubuntu"));
    expect(isOsLike(SAMPLE_OS, "debian"));
    expect(isOsLike(SAMPLE_OS, "posix"));
    expect(!isOsLike(SAMPLE_OS, "alpine"));
});
