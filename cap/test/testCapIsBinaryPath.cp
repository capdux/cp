import { test, expect_eq } from "cap:test";
import {
    BinaryCategory,
    extractExtension,
    classifyBinaryExtension,
    classifyBinaryPath,
    isBinaryExtension,
    isBinaryPath,
    isTextPath,
    isImagePath,
    isMediaPath,
    isArchivePath,
    isExecutablePath,
    isFontPath,
} from "../../packages/is-binary-path/src/index.cp";

fn main() {
    test("1. is-binary-path — detects image, audio, and video paths", () => {
        expect_eq(isBinaryPath("assets/logo.png"), true);
        expect_eq(isBinaryPath("C:\\media\\CLIP.MP4"), true);
        expect_eq(isBinaryPath("sound/track.flac"), true);
        expect_eq(isBinaryPath("photo.AVIF"), true);
        expect_eq(isBinaryPath("raw/shot.dng"), true);
    });

    test("2. is-binary-path — detects archives, native binaries, and fonts", () => {
        expect_eq(isBinaryPath("dist/app.tar.gz"), true);
        expect_eq(isBinaryPath("build/module.wasm"), true);
        expect_eq(isBinaryPath("lib/libfoo.so"), true);
        expect_eq(isBinaryPath("fonts/inter.woff2"), true);
        expect_eq(isBinaryPath("drivers/net.ko"), true);
    });

    test("3. is-binary-path — detects documents, databases, and ML models", () => {
        expect_eq(isBinaryPath("docs/spec.pdf"), true);
        expect_eq(isBinaryPath("data/store.sqlite3"), true);
        expect_eq(isBinaryPath("models/llama.gguf"), true);
        expect_eq(isBinaryPath("weights/model.safetensors"), true);
    });

    test("4. is-binary-path — returns false for source code and text files, true for isTextPath", () => {
        expect_eq(isBinaryPath("src/index.cp"), false);
        expect_eq(isBinaryPath("README.md"), false);
        expect_eq(isBinaryPath("package.json"), false);
        expect_eq(isBinaryPath("styles/app.css"), false);
        expect_eq(isBinaryPath("script.ts"), false);

        expect_eq(isTextPath("src/index.cp"), true);
        expect_eq(isTextPath("README.md"), true);
        expect_eq(isTextPath("Makefile"), true);
        expect_eq(isTextPath("assets/logo.png"), false);
        expect_eq(isTextPath(""), false);
    });

    test("5. is-binary-path — handles dotfiles, no-extension paths, and directories", () => {
        expect_eq(isBinaryPath(".png"), false);
        expect_eq(isBinaryPath("/dir/.zip"), false);
        expect_eq(isBinaryPath("Makefile"), false);
        expect_eq(isBinaryPath("dir.png/file.txt"), false);
        expect_eq(isBinaryPath(""), false);
    });

    test("6. is-binary-path — extractExtension and isBinaryExtension work standalone", () => {
        expect_eq(extractExtension("a/b/c.tar.zst"), "zst");
        expect_eq(extractExtension("a/b/noext"), "");
        expect_eq(isBinaryExtension("ZIP"), true);
        expect_eq(isBinaryExtension("txt"), false);
    });

    test("7. is-binary-path — classifyBinaryPath & category predicates", () => {
        expect_eq(classifyBinaryPath("img/pic.webp") == .Image, true);
        expect_eq(classifyBinaryPath("video/demo.mkv") == .AudioVideo, true);
        expect_eq(classifyBinaryPath("pkg/release.tar.zst") == .Archive, true);
        expect_eq(classifyBinaryPath("bin/app.wasm") == .Executable, true);
        expect_eq(classifyBinaryPath("ui/font.ttf") == .Font, true);
        expect_eq(classifyBinaryPath("db/main.parquet") == .DocumentData, true);
        expect_eq(classifyBinaryExtension("rs") == .None, true);

        expect_eq(isImagePath("a/b.PNG"), true);
        expect_eq(isImagePath("a/b.mp3"), false);
        expect_eq(isMediaPath("a/b.opus"), true);
        expect_eq(isArchivePath("a/b.7z"), true);
        expect_eq(isExecutablePath("a/b.dylib"), true);
        expect_eq(isFontPath("a/b.otf"), true);
    });
}
