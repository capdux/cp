// Source: /data/dev/bun/src/http/CertificateInfo.cp
// lib/http/cert.cp
// CP HTTP TLS Certificate Info & Errors (cap:http)

const std = @import("std");

pub struct HTTPCertError {
    pub error_no: i32 = 0;
    pub code: []const u8 = "";
    pub reason: []const u8 = "";
}

pub struct CertificateInfo {
    pub cert: []const u8 = "";
    pub cert_error: HTTPCertError = HTTPCertError{};
    pub hostname: []const u8 = "";

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.cert.len > 0) allocator.free(_.cert);
        if (_.cert_error.code.len > 0) allocator.free(_.cert_error.code);
        if (_.cert_error.reason.len > 0) allocator.free(_.cert_error.reason);
        if (_.hostname.len > 0) allocator.free(_.hostname);
    }
}

export error InitError {
    FailedToOpenSocket,
    LoadCAFile,
    InvalidCAFile,
    InvalidCA,
}
