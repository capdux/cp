import { test, expect_eq } from "cap:test";
import {
    CAT_NONE,
    CAT_IMAGE,
    CAT_AUDIO,
    CAT_VIDEO,
    CAT_ARCHIVE,
    CAT_FONT,
    CAT_EXECUTABLE,
    CAT_DOCUMENT,
    CAT_DATA_ML,
    isBinaryExtension,
    isBinaryPath,
    extensionCategory,
} from "../../packages/@cap/binary-extensions/src/index.cp";

fn main() {
    test("1. @cap/binary-extensions — Covers images, media, archives, fonts, execs, docs & AI models", () => {
        expect_eq(isBinaryExtension("png"), true);
        expect_eq(isBinaryExtension("webp"), true);
        expect_eq(isBinaryExtension("mp3"), true);
        expect_eq(isBinaryExtension("mp4"), true);
        expect_eq(isBinaryExtension("zst"), true);
        expect_eq(isBinaryExtension("woff2"), true);
        expect_eq(isBinaryExtension("wasm"), true);
        expect_eq(isBinaryExtension("pdf"), true);
        expect_eq(isBinaryExtension("gguf"), true);
        expect_eq(isBinaryExtension("safetensors"), true);
        expect_eq(isBinaryExtension("onnx"), true);
        expect_eq(isBinaryExtension("parquet"), true);
    });

    test("2. @cap/binary-extensions — Case-insensitive & leading-dot tolerance", () => {
        expect_eq(isBinaryExtension(".PNG"), true);
        expect_eq(isBinaryExtension("Jpeg"), true);
        expect_eq(isBinaryExtension(".WASM"), true);
        expect_eq(isBinaryExtension(".SafeTensors"), true);
        expect_eq(isBinaryExtension(".7Z"), true);
        expect_eq(isBinaryExtension("3GP"), true);
    });

    test("3. @cap/binary-extensions — isBinaryPath distinguishes dir.zip/readme.txt vs dir/file.ZIP", () => {
        expect_eq(isBinaryPath("dir.zip/readme.txt"), false);
        expect_eq(isBinaryPath("dir.png\\notes.md"), false);
        expect_eq(isBinaryPath("dir/file.ZIP"), true);
        expect_eq(isBinaryPath("C:\\img\\photo.PNG"), true);
        expect_eq(isBinaryPath("archive.tar.gz"), true);
        expect_eq(isBinaryPath("Makefile"), false);
        expect_eq(isBinaryPath("dir/trailing."), false);
    });

    test("4. @cap/binary-extensions — Dotfiles (.env, .gitignore) return false", () => {
        expect_eq(isBinaryPath(".gitignore"), false);
        expect_eq(isBinaryPath(".env"), false);
        expect_eq(isBinaryPath("/home/user/.bashrc"), false);
        expect_eq(isBinaryPath(".png"), false); // dotfile named ".png", no extension
        expect_eq(isBinaryPath(".hidden.png"), true); // hidden file WITH .png extension
    });

    test("5. @cap/binary-extensions — Common source & text extensions return false", () => {
        expect_eq(isBinaryExtension("cp"), false);
        expect_eq(isBinaryExtension("rs"), false);
        expect_eq(isBinaryExtension("ts"), false);
        expect_eq(isBinaryExtension("json"), false);
        expect_eq(isBinaryExtension("md"), false);
        expect_eq(isBinaryExtension("zig"), false);
        expect_eq(isBinaryExtension("svg"), false);
        expect_eq(isBinaryPath("src/index.cp"), false);
    });

    test("6. @cap/binary-extensions — extensionCategory classification & empty boundaries", () => {
        expect_eq(extensionCategory(".avif"), CAT_IMAGE);
        expect_eq(extensionCategory("flac"), CAT_AUDIO);
        expect_eq(extensionCategory("mkv"), CAT_VIDEO);
        expect_eq(extensionCategory("tar"), CAT_ARCHIVE);
        expect_eq(extensionCategory("ttf"), CAT_FONT);
        expect_eq(extensionCategory("dylib"), CAT_EXECUTABLE);
        expect_eq(extensionCategory("docx"), CAT_DOCUMENT);
        expect_eq(extensionCategory("gguf"), CAT_DATA_ML);
        expect_eq(extensionCategory("cp"), CAT_NONE);

        expect_eq(extensionCategory(""), CAT_NONE);
        expect_eq(extensionCategory("."), CAT_NONE);
        expect_eq(isBinaryExtension(""), false);
        expect_eq(isBinaryPath(""), false);
        expect_eq(isBinaryPath("/"), false);
    });
}
