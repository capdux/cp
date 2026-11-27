import { test, expect_eq } from "cap:test";
import { PreKind, parsePep440, comparePep440, matchesSpecifier } from "../../packages/pep440_rs/src/index.cp";

fn main() {
    test("parsePep440 parses epoch, release, pre, post, dev, and local segments", () => {
        const v1 = parsePep440("1!2.3.4a5.post6.dev7+local.1");
        expect_eq(v1.valid, true);
        expect_eq(v1.epoch, 1);
        expect_eq(v1.release[0], 2);
        expect_eq(v1.release[1], 3);
        expect_eq(v1.release[2], 4);
        expect_eq(v1.release_len, 3);
        expect_eq(v1.pre_kind, PreKind.Alpha);
        expect_eq(v1.pre_num, 5);
        expect_eq(v1.has_post, true);
        expect_eq(v1.post_num, 6);
        expect_eq(v1.has_dev, true);
        expect_eq(v1.dev_num, 7);

        const v2 = parsePep440("v1.0-2");
        expect_eq(v2.valid, true);
        expect_eq(v2.has_post, true);
        expect_eq(v2.post_num, 2);

        expect_eq(parsePep440("").valid, false);
        expect_eq(parsePep440("abc").valid, false);
        expect_eq(parsePep440("1.2.3+").valid, false);
    });

    test("comparePep440 enforces exact PEP 440 ordering (dev < a < b < rc < final < post)", () => {
        const v_dev = parsePep440("1.0.dev0");
        const v_a = parsePep440("1.0a1");
        const v_b = parsePep440("1.0b1");
        const v_rc = parsePep440("1.0rc1");
        const v_final = parsePep440("1.0");
        const v_post = parsePep440("1.0.post1");

        expect_eq(comparePep440(v_dev, v_a), -1);
        expect_eq(comparePep440(v_a, v_b), -1);
        expect_eq(comparePep440(v_b, v_rc), -1);
        expect_eq(comparePep440(v_rc, v_final), -1);
        expect_eq(comparePep440(v_final, v_post), -1);
        expect_eq(comparePep440(v_post, v_dev), 1);
    });

    test("comparePep440 handles zero-padded releases, epochs, and nested dev/post", () => {
        expect_eq(comparePep440(parsePep440("1.0"), parsePep440("1.0.0")), 0);
        expect_eq(comparePep440(parsePep440("0!2.0"), parsePep440("1!1.0")), -1);
        expect_eq(comparePep440(parsePep440("1.0a1.dev0"), parsePep440("1.0a1")), -1);
        expect_eq(comparePep440(parsePep440("1.0a1"), parsePep440("1.0a1.post0")), -1);
        expect_eq(comparePep440(parsePep440("1.0.post1.dev0"), parsePep440("1.0.post1")), -1);
    });

    test("matchesSpecifier handles ==, !=, >=, <=, >, < operators", () => {
        expect_eq(matchesSpecifier("1.2.0", "==1.2"), true);
        expect_eq(matchesSpecifier("1.2.3", "==1.2.*"), true);
        expect_eq(matchesSpecifier("1.3.0", "==1.2.*"), false);
        expect_eq(matchesSpecifier("1.3.0", "!=1.2.*"), true);
        expect_eq(matchesSpecifier("2.0.0", ">=2.0.0"), true);
        expect_eq(matchesSpecifier("2.0.0", ">2.0.0"), false);
        expect_eq(matchesSpecifier("1.9.9", "<2.0.0"), true);
        expect_eq(matchesSpecifier("2.0.0", "<=2.0.0"), true);
    });

    test("matchesSpecifier handles compatible release (~=)", () => {
        expect_eq(matchesSpecifier("1.4.2", "~=1.4.2"), true);
        expect_eq(matchesSpecifier("1.4.9", "~=1.4.2"), true);
        expect_eq(matchesSpecifier("1.4.1", "~=1.4.2"), false);
        expect_eq(matchesSpecifier("1.5.0", "~=1.4.2"), false);

        expect_eq(matchesSpecifier("2.2.0", "~=2.2"), true);
        expect_eq(matchesSpecifier("2.9.5", "~=2.2"), true);
        expect_eq(matchesSpecifier("3.0.0", "~=2.2"), false);
        expect_eq(matchesSpecifier("2.0", "~=2"), false);
    });

    test("matchesSpecifier handles comma-separated compound specifiers and invalid inputs", () => {
        expect_eq(matchesSpecifier("1.5.2", ">=1.2.0, <2.0.0, !=1.5.0"), true);
        expect_eq(matchesSpecifier("1.5.0", ">=1.2.0, <2.0.0, !=1.5.0"), false);
        expect_eq(matchesSpecifier("2.0.0", ">=1.2.0, <2.0.0"), false);
        expect_eq(matchesSpecifier("not-a-ver", ">=1.0"), false);
        expect_eq(matchesSpecifier("1.0", ""), false);
    });
}
