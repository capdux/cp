import { test, expect_eq } from "cap:test";
import {
    defaultPortForProtocol,
    requiresPort,
} from "../../packages/@cap/requires-port/src/index.cp";

fn main() {
    test("1. @cap/requires-port — returns false for standard default HTTP/HTTPS/WS/WSS ports", () => {
        expect_eq(requiresPort(80, "http"), false);
        expect_eq(requiresPort(80, "http:"), false);
        expect_eq(requiresPort(80, "http://"), false);
        expect_eq(requiresPort(443, "https:"), false);
        expect_eq(requiresPort(80, "ws"), false);
        expect_eq(requiresPort(443, "wss://"), false);
    });

    test("2. @cap/requires-port — returns true for non-default ports", () => {
        expect_eq(requiresPort(8080, "http"), true);
        expect_eq(requiresPort(8443, "https"), true);
        expect_eq(requiresPort(443, "http"), true);
        expect_eq(requiresPort(80, "https"), true);
    });

    test("3. @cap/requires-port — handles ftp, gopher, and file protocols", () => {
        expect_eq(requiresPort(21, "ftp"), false);
        expect_eq(requiresPort(2121, "ftp"), true);
        expect_eq(requiresPort(70, "gopher"), false);
        expect_eq(requiresPort(80, "file"), false);
    });

    test("4. @cap/requires-port — case-insensitive protocol matching", () => {
        expect_eq(requiresPort(443, "HTTPS://"), false);
        expect_eq(requiresPort(80, "HTTP:"), false);
    });

    test("5. @cap/requires-port — defaultPortForProtocol covers common infra protocols", () => {
        expect_eq(defaultPortForProtocol("ssh"), 22 as u16);
        expect_eq(defaultPortForProtocol("postgres://"), 5432 as u16);
        expect_eq(defaultPortForProtocol("mysql"), 3306 as u16);
        expect_eq(defaultPortForProtocol("redis"), 6379 as u16);
        expect_eq(defaultPortForProtocol("custom"), 0 as u16);
    });

    test("6. @cap/requires-port — port 0 returns false and unknown protocol with non-zero port returns true", () => {
        expect_eq(requiresPort(0, "http"), false);
        expect_eq(requiresPort(9000, "custom"), true);
    });
}
