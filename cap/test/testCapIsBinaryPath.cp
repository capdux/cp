import { test, expect_eq } from "cap:test";
import {
    extractExtension,
    isBinaryExtension,
    isBinaryPath,
} from "../../packages/@cap/is-binary-path/src/index.cp";

fn main() {
    test("1. @cap/is-binary-path — detects image, audio, and video paths", () => {
        expect_eq(isBinaryPath("assets/logo.png"), true);
        expect_eq(isBinaryPath("C:\\media\\CLIP.MP4"), true);
        expect_eq(isBinaryPath("sound/track.flac"), true);
        expect_eq(isBinaryPath("photo.AVIF"), true);
    });

    test("2. @cap/is-binary-path — detects archives, native binaries, and fonts", () => {
        expect_eq(isBinaryPath("dist/app.tar.gz"), true);
        expect_eq(isBinaryPath("build/module.wasm"), true);
        expect_eq(isBinaryPath("lib/libfoo.so"), true);
        expect_eq(isBinaryPath("fonts/inter.woff2"), true);
    });

    test("3. @cap/is-binary-path — detects documents, databases, and ML models", () => {
        expect_eq(isBinaryPath("docs/spec.pdf"), true);
        expect_eq(isBinaryPath("data/store.sqlite3"), true);
        expect_eq(isBinaryPath("models/llama.gguf"), true);
        expect_eq(isBinaryPath("weights/model.safetensors"), true);
    });

    test("4. @cap/is-binary-path — returns false for source code and text files", () => {
        expect_eq(isBinaryPath("src/index.cp"), false);
        expect_eq(isBinaryPath("README.md"), false);
        expect_eq(isBinaryPath("package.json"), false);
        expect_eq(isBinaryPath("styles/app.css"), false);
        expect_eq(isBinaryPath("script.ts"), false);
    });

    test("5. @cap/is-binary-path — handles dotfiles, no-extension paths, and directories", () => {
        expect_eq(isBinaryPath(".png"), false);
        expect_eq(isBinaryPath("/dir/.zip"), false);
        expect_eq(isBinaryPath("Makefile"), false);
        expect_eq(isBinaryPath("dir.png/file.txt"), false);
        expect_eq(isBinaryPath(""), false);
    });

    test("6. @cap/is-binary-path — extractExtension and isBinaryExtension work standalone", () => {
        expect_eq(extractExtension("a/b/c.tar.zst"), "zst");
        expect_eq(extractExtension("a/b/noext"), "");
        expect_eq(isBinaryExtension("ZIP"), true);
        expect_eq(isBinaryExtension("txt"), false);
    });
}
