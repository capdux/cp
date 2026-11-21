import {
    isValidOciDigest,
    DockerImageRef,
} from "cap:docker";
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

test("DockerImageRef parses official, custom registry:port, tag, and sha256 digest", () => {
    const r1 = DockerImageRef.parse("nginx").?;
    expect(r1.isOfficialDockerHub());
    expect(strEq(r1.effectiveRegistry(), "docker.io"));
    expect(strEq(r1.repository, "nginx"));
    expect(strEq(r1.effectiveTag().?, "latest"));

    const r2 = DockerImageRef.parse("localhost:5000/myorg/app:v1.2.3").?;
    expect(!r2.isOfficialDockerHub());
    expect(strEq(r2.registry.?, "localhost:5000"));
    expect(strEq(r2.repository, "myorg/app"));
    expect(strEq(r2.tag.?, "v1.2.3"));

    const dig = "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    expect(isValidOciDigest(dig));
    const r3 = DockerImageRef.parse("ghcr.io/cp-lang/cpc:1.0@" ++ dig).?;
    expect(strEq(r3.registry.?, "ghcr.io"));
    expect(strEq(r3.repository, "cp-lang/cpc"));
    expect(strEq(r3.tag.?, "1.0"));
    expect(strEq(r3.digest.?, dig));

    // Digest-only image has null effectiveTag
    const r4 = DockerImageRef.parse("alpine@" ++ dig).?;
    expect(r4.effectiveTag() == null);

    // Invalid forms
    expect(DockerImageRef.parse("") == null);
    expect(DockerImageRef.parse("nginx:bad tag") == null);
    expect(DockerImageRef.parse("nginx@sha256:short") == null);
});
