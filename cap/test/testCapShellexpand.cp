import { test, expect_eq } from "cap:test";
import {
    EnvEntry,
    tildeWithHome,
    envExpand,
    fullExpand,
} from "../../packages/shellexpand/src/index.cp";

fn main() {
    test("1. @cap/shellexpand — tildeWithHome expands ~ and ~/subpath", () => {
        let buf: [128]u8 = undefined;
        let n1 = tildeWithHome("~", "/home/alice", buf[0..128]);
        expect_eq(buf[0..n1], "/home/alice");

        let n2 = tildeWithHome("~/projects/cp", "/home/alice", buf[0..128]);
        expect_eq(buf[0..n2], "/home/alice/projects/cp");

        let n3 = tildeWithHome("~/bin", "/home/alice/", buf[0..128]);
        expect_eq(buf[0..n3], "/home/alice/bin");
    });

    test("2. @cap/shellexpand — tildeWithHome preserves ~other and non-tilde paths", () => {
        let buf: [128]u8 = undefined;
        let n1 = tildeWithHome("~bob/docs", "/home/alice", buf[0..128]);
        expect_eq(buf[0..n1], "~bob/docs");

        let n2 = tildeWithHome("/etc/hosts", "/home/alice", buf[0..128]);
        expect_eq(buf[0..n2], "/etc/hosts");
    });

    test("3. @cap/shellexpand — envExpand expands $VAR and ${VAR}", () => {
        let vars: [2]EnvEntry = [
            EnvEntry { key: "USER", value: "alice" },
            EnvEntry { key: "PORT", value: "8080" },
        ];
        let buf: [128]u8 = undefined;
        let n = envExpand("/srv/$USER:${PORT}/api", vars[0..2], false, buf[0..128]);
        expect_eq(buf[0..n], "/srv/alice:8080/api");
    });

    test("4. @cap/shellexpand — envExpand supports ${VAR:-default} for missing or empty vars", () => {
        let vars: [1]EnvEntry = [
            EnvEntry { key: "EMPTY", value: "" },
        ];
        let buf: [128]u8 = undefined;
        let n1 = envExpand("${HOST:-127.0.0.1}:${EMPTY:-3000}", vars[0..1], false, buf[0..128]);
        expect_eq(buf[0..n1], "127.0.0.1:3000");
    });

    test("5. @cap/shellexpand — envExpand escapes $$ and respects keep_unknown", () => {
        let vars: [1]EnvEntry = [
            EnvEntry { key: "A", value: "ok" },
        ];
        let buf: [128]u8 = undefined;
        let n1 = envExpand("$$A=$A:$MISSING", vars[0..1], true, buf[0..128]);
        expect_eq(buf[0..n1], "$A=ok:$MISSING");

        let n2 = envExpand("$MISSING/end", vars[0..1], false, buf[0..128]);
        expect_eq(buf[0..n2], "/end");
    });

    test("6. @cap/shellexpand — fullExpand combines tilde and env expansion", () => {
        let vars: [2]EnvEntry = [
            EnvEntry { key: "ENV", value: "prod" },
            EnvEntry { key: "APP", value: "cap" },
        ];
        let buf: [128]u8 = undefined;
        let n = fullExpand("~/.config/$APP/${ENV:-dev}.toml", "/home/alice", vars[0..2], buf[0..128]);
        expect_eq(buf[0..n], "/home/alice/.config/cap/prod.toml");
    });
}
