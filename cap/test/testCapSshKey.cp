import { test, expect_eq } from "cap:test";
import {
    ALG_ED25519,
    ALG_RSA,
    ALG_SK_ED25519,
    ALG_UNKNOWN,
    SshPublicKey,
    identifyAlgorithm,
    isValidBase64,
    parsePublicKey,
    formatPublicKey,
    countAuthorizedKeys,
    isModernAlgorithm,
} from "../../packages/@cap/ssh-key/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ssh-key — parse ssh-ed25519 key with comment", () => {
        let line: string = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIOMqqnkVzrm0SdG6UOoqKLsabgH5C9okWi0dh2l9GKJl alice@workstation";
        let k: SshPublicKey = parsePublicKey(line);
        expect_eq(k.valid, true);
        expect_eq(k.alg, ALG_ED25519);
        expect_eq(eqStr(k.key_type, "ssh-ed25519"), true);
        expect_eq(eqStr(k.comment, "alice@workstation"), true);
        expect_eq(isModernAlgorithm(k.alg), true);
    });

    test("2. @cap/ssh-key — parse ssh-rsa without comment & FIDO2 sk-ssh-ed25519", () => {
        let rsa: SshPublicKey = parsePublicKey("  ssh-rsa AAAAB3NzaC1yc2EA  \r\n");
        expect_eq(rsa.valid, true);
        expect_eq(rsa.alg, ALG_RSA);
        expect_eq(rsa.comment.len, 0);
        expect_eq(isModernAlgorithm(rsa.alg), false);

        let sk: SshPublicKey = parsePublicKey("sk-ssh-ed25519@openssh.com AAAAGnNrLXNzaC1lZDI1NTE5QG9wZW5zc2guY29t yubikey 5");
        expect_eq(sk.valid, true);
        expect_eq(sk.alg, ALG_SK_ED25519);
        expect_eq(eqStr(sk.comment, "yubikey 5"), true);
    });

    test("3. @cap/ssh-key — formatPublicKey roundtrip", () => {
        let orig: string = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIOMqqnkVzrm0SdG6UOoqKLsabgH5C9okWi0dh2l9GKJl deploy@prod";
        let k: SshPublicKey = parsePublicKey(orig);
        let buf: [256]u8 = undefined;
        let n: usize = formatPublicKey(k, buf[0..]);
        expect_eq(eqStr(buf[0..n], orig), true);
    });

    test("4. @cap/ssh-key — countAuthorizedKeys skipping comments & blank lines", () => {
        let file: string = "# team keys\n\nssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIOMq user1\n# disabled\nssh-rsa AAAAB3NzaC1yc2EA user2\ninvalid line\n";
        expect_eq(countAuthorizedKeys(file), 2);
    });

    test("5. @cap/ssh-key — reject unknown algorithms, comments & malformed Base64", () => {
        expect_eq(identifyAlgorithm("ssh-dss"), ALG_UNKNOWN);
        expect_eq(parsePublicKey("# ssh-ed25519 AAAAC3Nz").valid, false);
        expect_eq(parsePublicKey("ssh-ed25519 NOT_VALID!==").valid, false);
        expect_eq(parsePublicKey("ssh-ed25519 AAA").valid, false);
        expect_eq(isValidBase64("AAAA"), true);
        expect_eq(isValidBase64("AA=="), true);
        expect_eq(isValidBase64("A==="), false);
    });

    test("6. @cap/ssh-key — small output buffer protection", () => {
        let k: SshPublicKey = parsePublicKey("ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIOMq user");
        let tiny: [10]u8 = undefined;
        expect_eq(formatPublicKey(k, tiny[0..]), 0);
    });
}
