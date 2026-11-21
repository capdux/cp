import {
    PubKeyType as SshKeyType,
    SshPubKey,
    AuthorizedKeysIter,
} from "cap:ssh";
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

const SAMPLE_KEYS =
    "# Deploy keys\n" ++
    "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIMockKeyData0000000000000000000000000000000 alice@laptop\n" ++
    "no-pty,command=\"/usr/bin/git-shell -c\" sk-ssh-ed25519@openssh.com AAAAGnNrLXNzaC1lZDI1NTE5QG9wZW5zc2guY29t yubikey\r\n" ++
    "invalid line ignored\n" ++
    "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQ==\n";

test("SshPubKey and AuthorizedKeysIter parse standard, options, and SK keys", () => {
    let it = AuthorizedKeysIter.init(SAMPLE_KEYS);

    const k1 = it.next().?;
    expect(k1.key_type == SshKeyType.ed25519);
    expect(k1.options == null);
    expect(!k1.isSecurityKey());
    expect(strEq(k1.comment.?, "alice@laptop"));

    const k2 = it.next().?;
    expect(k2.key_type == SshKeyType.sk_ed25519);
    expect(k2.isSecurityKey());
    expect(strEq(k2.options.?, "no-pty,command=\"/usr/bin/git-shell -c\""));
    expect(strEq(k2.comment.?, "yubikey"));

    const k3 = it.next().?;
    expect(k3.key_type == SshKeyType.rsa);
    expect(k3.comment == null);

    expect(it.next() == null);
});

test("SshPubKey rejects malformed base64 and unknown key types", () => {
    expect(SshPubKey.parseLine("# comment") == null);
    expect(SshPubKey.parseLine("ssh-ed25519 NOT_VALID_B64!!!") == null);
    expect(SshPubKey.parseLine("ssh-unknown AAAA") == null);
    expect(SshPubKey.parseLine("command=\"unclosed ssh-ed25519 AAAA") == null);
});
