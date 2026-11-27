import { test, expect_eq } from "cap:test";
import {
    FormidableOptions,
    FormField,
    FormFile,
    ParsedForm,
    FORM_OK,
    ERR_INVALID_CONTENT_TYPE,
    ERR_MALFORMED_MULTIPART,
    ERR_MAX_FIELDS_EXCEEDED,
    ERR_MAX_FILES_EXCEEDED,
    ERR_MAX_FILE_SIZE_EXCEEDED,
    ERR_EMPTY_FILE_NOT_ALLOWED,
    parseMultipartForm
} from "../../packages/formidable/src/index.cp";

fn main() {
    test("formidable: parses mixed fields and file uploads with zero-copy offsets", () => {
        let ct = "multipart/form-data; boundary=----CapForm123";
        let body = "------CapForm123\r\n" +
            "Content-Disposition: form-data; name=\"username\"\r\n" +
            "\r\n" +
            "alice\r\n" +
            "------CapForm123\r\n" +
            "Content-Disposition: form-data; name=\"avatar\"; filename=\"photo.png\"\r\n" +
            "Content-Type: image/png\r\n" +
            "\r\n" +
            "PNGDATA12345\r\n" +
            "------CapForm123\r\n" +
            "Content-Disposition: form-data; name=\"bio\"\r\n" +
            "\r\n" +
            "hello world\r\n" +
            "------CapForm123--\r\n";

        let opts = FormidableOptions.defaults();
        let form = parseMultipartForm(ct, body, opts);

        expect_eq(form.valid, true);
        expect_eq(form.err_code, FORM_OK);
        expect_eq(form.field_count, 2 as usize);
        expect_eq(form.file_count, 1 as usize);

        let user = form.getField("username");
        expect_eq(user != null, true);
        expect_eq(user.?, "alice");

        let bio = form.getField("bio");
        expect_eq(bio != null, true);
        expect_eq(bio.?, "hello world");

        expect_eq(form.getField("missing") == null, true);

        let file_opt = form.getFile("avatar");
        expect_eq(file_opt != null, true);
        let file = file_opt.?;
        expect_eq(file.field_name, "avatar");
        expect_eq(file.original_filename, "photo.png");
        expect_eq(file.mime_type, "image/png");
        expect_eq(file.body_len, 12 as usize);
        expect_eq(body[file.body_start..(file.body_start + file.body_len)], "PNGDATA12345");
    });

    test("formidable: rejects invalid content-type or missing boundary", () => {
        let opts = FormidableOptions.defaults();

        let r1 = parseMultipartForm("application/json", "{}", opts);
        expect_eq(r1.valid, false);
        expect_eq(r1.err_code, ERR_INVALID_CONTENT_TYPE);

        let r2 = parseMultipartForm("multipart/form-data", "--abc--", opts);
        expect_eq(r2.valid, false);
        expect_eq(r2.err_code, ERR_INVALID_CONTENT_TYPE);

        let r3 = parseMultipartForm("multipart/mixed; boundary=abc", "--abc--", opts);
        expect_eq(r3.valid, false);
        expect_eq(r3.err_code, ERR_INVALID_CONTENT_TYPE);
    });

    test("formidable: enforces max_fields and max_files limits", () => {
        let ct = "multipart/form-data; boundary=B1";
        let body = "--B1\r\n" +
            "Content-Disposition: form-data; name=\"f1\"\r\n\r\nv1\r\n" +
            "--B1\r\n" +
            "Content-Disposition: form-data; name=\"f2\"\r\n\r\nv2\r\n" +
            "--B1--\r\n";

        let opts = FormidableOptions.defaults();
        opts.max_fields = 1;
        let r_fields = parseMultipartForm(ct, body, opts);
        expect_eq(r_fields.valid, false);
        expect_eq(r_fields.err_code, ERR_MAX_FIELDS_EXCEEDED);

        let file_body = "--B1\r\n" +
            "Content-Disposition: form-data; name=\"a\"; filename=\"a.txt\"\r\n\r\nAA\r\n" +
            "--B1\r\n" +
            "Content-Disposition: form-data; name=\"b\"; filename=\"b.txt\"\r\n\r\nBB\r\n" +
            "--B1--\r\n";

        let opts2 = FormidableOptions.defaults();
        opts2.max_files = 1;
        let r_files = parseMultipartForm(ct, file_body, opts2);
        expect_eq(r_files.valid, false);
        expect_eq(r_files.err_code, ERR_MAX_FILES_EXCEEDED);
    });

    test("formidable: enforces max_file_size and allow_empty_files policy", () => {
        let ct = "multipart/form-data; boundary=B2";
        let empty_file_body = "--B2\r\n" +
            "Content-Disposition: form-data; name=\"doc\"; filename=\"empty.txt\"\r\n\r\n" +
            "\r\n--B2--\r\n";

        let opts_strict = FormidableOptions.defaults();
        let r_empty = parseMultipartForm(ct, empty_file_body, opts_strict);
        expect_eq(r_empty.valid, false);
        expect_eq(r_empty.err_code, ERR_EMPTY_FILE_NOT_ALLOWED);

        let opts_allow = FormidableOptions.defaults();
        opts_allow.allow_empty_files = true;
        let r_ok = parseMultipartForm(ct, empty_file_body, opts_allow);
        expect_eq(r_ok.valid, true);
        expect_eq(r_ok.file_count, 1 as usize);
        let f_opt = r_ok.getFile("doc");
        expect_eq(f_opt != null, true);
        let f_empty = f_opt.?;
        expect_eq(f_empty.body_len, 0 as usize);
        expect_eq(f_empty.mime_type, "application/octet-stream");

        let big_body = "--B2\r\n" +
            "Content-Disposition: form-data; name=\"doc\"; filename=\"big.bin\"\r\n\r\n" +
            "0123456789ABCDEF\r\n--B2--\r\n";
        let opts_small = FormidableOptions.defaults();
        opts_small.max_file_size = 8 as u64;
        let r_big = parseMultipartForm(ct, big_body, opts_small);
        expect_eq(r_big.valid, false);
        expect_eq(r_big.err_code, ERR_MAX_FILE_SIZE_EXCEEDED);
    });

    test("formidable: supports quoted boundary, LF line endings, and filename* fallback", () => {
        let ct = "Multipart/Form-Data; Boundary=\"quoted-bound-9\"";
        let body = "--quoted-bound-9\n" +
            "Content-Disposition: form-data; name=\"upload\"; filename*=UTF-8''report%202026.pdf\n" +
            "Content-Type: application/pdf\n" +
            "\n" +
            "%PDF-1.7\n" +
            "--quoted-bound-9--\n";

        let opts = FormidableOptions.defaults();
        let form = parseMultipartForm(ct, body, opts);
        expect_eq(form.valid, true);
        expect_eq(form.file_count, 1 as usize);
        let f = form.getFile("upload").?;
        expect_eq(f.original_filename, "UTF-8''report%202026.pdf");
        expect_eq(f.mime_type, "application/pdf");
        expect_eq(body[f.body_start..(f.body_start + f.body_len)], "%PDF-1.7");
    });

    test("formidable: rejects malformed multipart payloads missing terminator or headers", () => {
        let ct = "multipart/form-data; boundary=B3";
        let opts = FormidableOptions.defaults();

        let no_end = "--B3\r\nContent-Disposition: form-data; name=\"x\"\r\n\r\nval";
        let r1 = parseMultipartForm(ct, no_end, opts);
        expect_eq(r1.valid, false);
        expect_eq(r1.err_code, ERR_MALFORMED_MULTIPART);

        let no_cd = "--B3\r\nContent-Type: text/plain\r\n\r\nval\r\n--B3--\r\n";
        let r2 = parseMultipartForm(ct, no_cd, opts);
        expect_eq(r2.valid, false);
        expect_eq(r2.err_code, ERR_MALFORMED_MULTIPART);

        let empty_form = "--B3--\r\n";
        let r3 = parseMultipartForm(ct, empty_form, opts);
        expect_eq(r3.valid, true);
        expect_eq(r3.field_count, 0 as usize);
        expect_eq(r3.file_count, 0 as usize);
    });
}
