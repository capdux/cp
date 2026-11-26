import { test, expect_eq } from "cap:test";
import {
    CAT_NONE,
    CAT_IMAGE,
    CAT_ARCHIVE,
    CAT_DATA_ML,
    isBinaryPath,
    isTextPath,
    filterBinaryPaths,
    filterTextPaths,
    countBinaryPaths,
    detectPathKind,
} from "../../packages/@cap/is-binary-path/src/index.cp";

fn main() {
    test("1. @cap/is-binary-path — isBinaryPath & isTextPath basic classification", () => {
        expect_eq(isBinaryPath("assets/logo.png"), true);
        expect_eq(isTextPath("assets/logo.png"), false);
        expect_eq(isBinaryPath("src/index.cp"), false);
        expect_eq(isTextPath("src/index.cp"), true);
        expect_eq(isBinaryPath("models/weights.safetensors"), true);
        expect_eq(isTextPath("package.json"), true);
    });

    test("2. @cap/is-binary-path — Batch filterBinaryPaths, filterTextPaths & countBinaryPaths", () => {
        const files: [6]string = [
            "README.md",
            "assets/hero.webp",
            "src/app.cp",
            "dist/bundle.wasm",
            "Makefile",
            "archive/backup.tar.gz",
        ];
        expect_eq(countBinaryPaths(&files), 3);

        let bin_idx: [6]usize = undefined;
        const nb = filterBinaryPaths(&files, &bin_idx);
        expect_eq(nb, 3);
        expect_eq(bin_idx[0], 1);
        expect_eq(bin_idx[1], 3);
        expect_eq(bin_idx[2], 5);

        let txt_idx: [6]usize = undefined;
        const nt = filterTextPaths(&files, &txt_idx);
        expect_eq(nt, 3);
        expect_eq(txt_idx[0], 0);
        expect_eq(txt_idx[1], 2);
        expect_eq(txt_idx[2], 4);
    });

    test("3. @cap/is-binary-path — detectPathKind delegates to @cap/binary-extensions", () => {
        expect_eq(detectPathKind("models/llama-3.gguf"), CAT_DATA_ML);
        expect_eq(detectPathKind("images/icon.PNG"), CAT_IMAGE);
        expect_eq(detectPathKind("releases/cap-v1.zip"), CAT_ARCHIVE);
        expect_eq(detectPathKind("src/main.cp"), CAT_NONE);
        expect_eq(detectPathKind("dir.zip/notes.txt"), CAT_NONE);
    });

    test("4. @cap/is-binary-path — Windows paths & dotted parent directories", () => {
        expect_eq(isBinaryPath("C:\\Users\\dev\\photo.JPG"), true);
        expect_eq(isTextPath("C:\\Users\\dev\\photo.JPG"), false);
        expect_eq(isBinaryPath("C:\\release.zip\\CHANGELOG.md"), false);
        expect_eq(isTextPath("C:\\release.zip\\CHANGELOG.md"), true);
    });

    test("5. @cap/is-binary-path — Extensionless common text files (Makefile, Dockerfile, README, LICENSE)", () => {
        expect_eq(isTextPath("Makefile"), true);
        expect_eq(isTextPath("Dockerfile"), true);
        expect_eq(isTextPath("README"), true);
        expect_eq(isTextPath("LICENSE"), true);
        expect_eq(isTextPath(".gitignore"), true);
        expect_eq(isBinaryPath("Makefile"), false);
    });

    test("6. @cap/is-binary-path — Empty lists, empty path & small output buffer boundaries", () => {
        const one: [1]string = ["a.png"];
        const empty_paths: []const string = one[0..0];
        let out: [4]usize = undefined;

        expect_eq(countBinaryPaths(empty_paths), 0);
        expect_eq(filterBinaryPaths(empty_paths, &out), 0);
        expect_eq(filterTextPaths(empty_paths, &out), 0);
        expect_eq(isTextPath(""), false);
        expect_eq(isBinaryPath(""), false);
        expect_eq(detectPathKind(""), CAT_NONE);

        // Truncated output slice safety
        const many_bins: [3]string = ["a.png", "b.zip", "c.wasm"];
        let small: [2]usize = undefined;
        expect_eq(filterBinaryPaths(&many_bins, &small), 2);
        expect_eq(small[0], 0);
        expect_eq(small[1], 1);
    });
}
