import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    MULTER_OK,
    ERR_LIMIT_FILE_SIZE,
    ERR_LIMIT_FILE_COUNT,
    ERR_LIMIT_FIELD_COUNT,
    ERR_LIMIT_FIELD_VALUE,
    ERR_UNEXPECTED_FILE,
    ERR_INVALID_CONTENT_TYPE,
    MulterLimits,
    extractMultipartBoundary,
    MulterValidator
} from "../../packages/multer/src/index.cp";

fn main() {
    test("1. @cap/multer — extractMultipartBoundary parses valid multipart/form-data headers", () => {
        let b1 = extractMultipartBoundary("multipart/form-data; boundary=----WebKitFormBoundary123");
        expect_eq(b1 != null, true);
        expect_eq(strEq(b1.?, "----WebKitFormBoundary123"), true);

        let b2 = extractMultipartBoundary("MULTIPART/FORM-DATA; charset=utf-8; boundary=\"abc-999\"");
        expect_eq(b2 != null, true);
        expect_eq(strEq(b2.?, "abc-999"), true);
    });

    test("2. @cap/multer — rejects non-multipart or missing-boundary Content-Type", () => {
        expect_eq(extractMultipartBoundary("application/json") == null, true);
        expect_eq(extractMultipartBoundary("multipart/form-data") == null, true);
        expect_eq(extractMultipartBoundary("multipart/mixed; boundary=xyz") == null, true);

        let v = MulterValidator.new(MulterLimits.defaults());
        expect_eq(v.validateContentType("text/plain"), ERR_INVALID_CONTENT_TYPE);
        expect_eq(v.validateContentType("multipart/form-data; boundary=ok"), MULTER_OK);
    });

    test("3. @cap/multer — accepts file and infers MIME type via @cap/mime-types when omitted", () => {
        let v = MulterValidator.new(MulterLimits.defaults());
        let rc1 = v.acceptFile("avatar", "photo.png", "", 4096 as u64);
        expect_eq(rc1, MULTER_OK);
        expect_eq(v.fileCount(), 1);
        expect_eq(strEq(v.files[0].mime_type, "image/png"), true);

        let rc2 = v.acceptFile("doc", "notes.unknownext", "", 128 as u64);
        expect_eq(rc2, MULTER_OK);
        expect_eq(strEq(v.files[1].mime_type, "application/octet-stream"), true);

        let rc3 = v.acceptFile("custom", "data.bin", "application/x-custom", 64 as u64);
        expect_eq(rc3, MULTER_OK);
        expect_eq(strEq(v.files[2].mime_type, "application/x-custom"), true);
    });

    test("4. @cap/multer — enforces allowed field whitelist (ERR_UNEXPECTED_FILE)", () => {
        let v = MulterValidator.new(MulterLimits.defaults());
        expect_eq(v.allowField("avatar"), true);
        expect_eq(v.allowField("banner"), true);

        expect_eq(v.acceptFile("avatar", "a.jpg", "image/jpeg", 512 as u64), MULTER_OK);
        expect_eq(v.acceptFile("evil", "x.exe", "application/octet-stream", 10 as u64), ERR_UNEXPECTED_FILE);
        expect_eq(v.fileCount(), 1);
    });

    test("5. @cap/multer — enforces file_size and files count limits", () => {
        let limits = MulterLimits {
            file_size: 1024 as u64,
            files: 2,
            fields: 10,
            field_size: 256 as u64
        };
        let v = MulterValidator.new(limits);

        expect_eq(v.acceptFile("f", "big.png", "", 2048 as u64), ERR_LIMIT_FILE_SIZE);
        expect_eq(v.acceptFile("f", "ok1.png", "", 512 as u64), MULTER_OK);
        expect_eq(v.acceptFile("f", "ok2.png", "", 1024 as u64), MULTER_OK);
        expect_eq(v.acceptFile("f", "ok3.png", "", 100 as u64), ERR_LIMIT_FILE_COUNT);
        expect_eq(v.fileCount(), 2);
    });

    test("6. @cap/multer — enforces text field count and field value size limits", () => {
        let limits = MulterLimits {
            file_size: 1024 as u64,
            files: 4,
            fields: 2,
            field_size: 100 as u64
        };
        let v = MulterValidator.new(limits);

        expect_eq(v.acceptField(150 as u64), ERR_LIMIT_FIELD_VALUE);
        expect_eq(v.acceptField(50 as u64), MULTER_OK);
        expect_eq(v.acceptField(100 as u64), MULTER_OK);
        expect_eq(v.acceptField(10 as u64), ERR_LIMIT_FIELD_COUNT);
        expect_eq(v.fieldCount(), 2);
    });
}
