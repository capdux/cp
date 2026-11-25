// Regression Test Suite for `@web/multipart`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import {
    MultipartConfig,
    MultipartForm,
    parseMultipartBody,
    registerMultipart,
} from "../src/index.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hUpload(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"uploaded\":true}");
}

fn main() {
    // 1. Parse valid multipart/form-data with text field + file field
    const boundary = "----CPBoundary123";
    const body =
        "------CPBoundary123\r\n" ++
        "Content-Disposition: form-data; name=\"title\"\r\n" ++
        "\r\n" ++
        "hello-cp\r\n" ++
        "------CPBoundary123\r\n" ++
        "Content-Disposition: form-data; name=\"avatar\"; filename=\"pic.png\"\r\n" ++
        "Content-Type: image/png\r\n" ++
        "\r\n" ++
        "PNGDATA12345\r\n" ++
        "------CPBoundary123--\r\n";

    let form = MultipartForm{};
    const rc = parseMultipartBody(boundary, body, MultipartConfig{
        allowed_mime_prefix: "image/",
    }, *form);
    if (rc != 0) @panic("Test 1a failed: expected rc == 0");
    if (form.count != 2) @panic("Test 1b failed: expected 2 parts");
    if (!strEql(form.parts[0].name, "title") || !strEql(form.parts[0].data, "hello-cp") || form.parts[0].is_file) {
        @panic("Test 1c failed: text part mismatch");
    }
    if (!strEql(form.parts[1].name, "avatar") || !strEql(form.parts[1].filename, "pic.png") ||
        !strEql(form.parts[1].content_type, "image/png") || !strEql(form.parts[1].data, "PNGDATA12345") ||
        !form.parts[1].is_file) {
        @panic("Test 1d failed: file part mismatch");
    }
    if (form.total_bytes != 20) @panic("Test 1e failed: total_bytes mismatch");

    // 2. Malformed body returns 400
    const bad_rc = parseMultipartBody(boundary, "not-a-multipart-body", MultipartConfig{}, *form);
    if (bad_rc != 400) @panic("Test 2 failed: expected 400 on malformed body");

    // 3. Exceeding max_file_bytes returns 413
    const big_file_rc = parseMultipartBody(boundary, body, MultipartConfig{
        max_file_bytes: 4,
    }, *form);
    if (big_file_rc != 413) @panic("Test 3a failed: expected 413 on max_file_bytes exceeded");

    // 4. Exceeding max_parts returns 413
    const max_parts_rc = parseMultipartBody(boundary, body, MultipartConfig{
        max_parts: 1,
    }, *form);
    if (max_parts_rc != 413) @panic("Test 3b failed: expected 413 on max_parts exceeded");

    // 5. Disallowed MIME prefix returns 415
    const bad_mime_rc = parseMultipartBody(boundary, body, MultipartConfig{
        allowed_mime_prefix: "text/",
    }, *form);
    if (bad_mime_rc != 415) @panic("Test 4 failed: expected 415 on disallowed MIME prefix");

    // 6. App guard hook blocks oversized multipart request with 413 and allows small ones
    let app = App.init();
    * = registerMultipart(*app, MultipartConfig{ max_total_bytes: 32 });
    * = app.post("/upload", hUpload);

    let ok_req = Request.init(AppMethod.POST, "/upload");
    * = ok_req.withHeader("Content-Type", "multipart/form-data; boundary=abc");
    * = ok_req.withBody("short-body");
    const ok_resp = app.dispatch(ok_req);
    if (ok_resp.status != 200) @panic("Test 5a failed: small multipart request rejected");

    let too_big_req = Request.init(AppMethod.POST, "/upload");
    * = too_big_req.withHeader("Content-Type", "multipart/form-data; boundary=abc");
    * = too_big_req.withBody("0123456789012345678901234567890123456789");
    const big_resp = app.dispatch(too_big_req);
    if (big_resp.status != 413) @panic("Test 5b failed: oversized multipart request did not return 413");

    @print("PASS @web/multipart (5/5 tests)");
}
