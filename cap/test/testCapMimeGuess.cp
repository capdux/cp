// cap/test/testCapMimeGuess.cp
// Unit tests for @cap/mime-guess (Zero-heap Rust mime_guess v2 API over @cap/mime-types)

import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    MimeGuess,
    extractExt,
    getExtForMime,
    getCharsetForMime,
} from "../../packages/mime_guess/src/index.cp";

fn main() {
    test("1. @cap/mime-guess — extractExt POSIX, Windows, dotfiles & multi-ext", () => {
        expect_eq(strEq(extractExt("/var/www/index.html").?, "html"), true);
        expect_eq(strEq(extractExt("C:\\Users\\dev\\REPORT.PDF").?, "PDF"), true);
        expect_eq(strEq(extractExt("archive.tar.gz").?, "gz"), true);
        expect_eq(strEq(extractExt("/home/u/.env.json").?, "json"), true);

        expect_eq(extractExt("/a.b/file") == null, true);
        expect_eq(extractExt(".gitignore") == null, true);
        expect_eq(extractExt("/home/u/.env") == null, true);
        expect_eq(extractExt("trailing.") == null, true);
        expect_eq(extractExt("/dir/only/") == null, true);
        expect_eq(extractExt("") == null, true);
    });

    test("2. @cap/mime-guess — fromPath POSIX & Windows paths (case-insensitive)", () => {
        const g1 = MimeGuess.fromPath("/srv/app/data.JSON");
        expect_eq(g1.found, true);
        expect_eq(strEq(g1.first().?, "application/json"), true);
        expect_eq(g1.isCompressible(), true);

        const g2 = MimeGuess.fromPath("C:\\Media\\clip.MP4");
        expect_eq(g2.found, true);
        expect_eq(strEq(g2.first().?, "video/mp4"), true);
        expect_eq(g2.isVideo(), true);
        expect_eq(g2.isImage(), false);
    });

    test("3. @cap/mime-guess — fromExt with and without leading dot", () => {
        const g1 = MimeGuess.fromExt("png");
        expect_eq(g1.found, true);
        expect_eq(strEq(g1.first().?, "image/png"), true);
        expect_eq(g1.isImage(), true);

        const g2 = MimeGuess.fromExt(".CSS");
        expect_eq(g2.found, true);
        expect_eq(strEq(g2.first().?, "text/css"), true);
        expect_eq(g2.isText(), true);
        expect_eq(g2.isCompressible(), true);

        const g3 = MimeGuess.fromExt(".mp3");
        expect_eq(g3.found, true);
        expect_eq(g3.isAudio(), true);
    });

    test("4. @cap/mime-guess — fallback methods for unknown & dotfile paths", () => {
        const g_dot = MimeGuess.fromPath("/repo/.gitignore");
        expect_eq(g_dot.found, false);
        expect_eq(g_dot.first() == null, true);
        expect_eq(strEq(g_dot.firstOrOctetStream(), "application/octet-stream"), true);
        expect_eq(strEq(g_dot.firstOrTextPlain(), "text/plain"), true);
        expect_eq(g_dot.isText(), false);
        expect_eq(g_dot.isCompressible(), false);

        const g_unk = MimeGuess.fromPath("file.unknown_ext_xyz");
        expect_eq(g_unk.found, false);
        expect_eq(strEq(g_unk.firstOrOctetStream(), "application/octet-stream"), true);
    });

    test("5. @cap/mime-guess — getExtForMime & getCharsetForMime delegation", () => {
        const e1 = getExtForMime("application/json");
        expect_eq(e1 != null, true);
        expect_eq(strEq(e1.?, "json"), true);

        const e2 = getExtForMime("image/png");
        expect_eq(e2 != null, true);
        expect_eq(strEq(e2.?, "png"), true);

        const cs1 = getCharsetForMime("text/html");
        expect_eq(cs1 != null, true);
        expect_eq(strEq(cs1.?, "UTF-8"), true);

        expect_eq(getCharsetForMime("image/png") == null, true);
    });

    test("6. @cap/mime-guess — invalid ext edge cases rejected safely", () => {
        expect_eq(MimeGuess.fromExt("").found, false);
        expect_eq(MimeGuess.fromExt(".").found, false);
        expect_eq(MimeGuess.fromExt("foo/bar.png").found, false);
        expect_eq(MimeGuess.fromPath("/a.b/README").found, false);
    });
}
