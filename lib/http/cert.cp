// Source: Custom implementation by Antigravity AI
/**
 * CP HTTP TLS Certificate Info & Errors (cap:http)
 * Ported from Bun.js http/CertificateInfo.cp & HTTPCertError.cp
 */


pub const HTTPCertError = struct {
    error_no: i32 = 0,
    code: []const u8 = "",
    reason: []const u8 = "",
};

pub const CertificateInfo = struct {
    cert: []const u8 = "",
    cert_error: HTTPCertError = HTTPCertError{},
    hostname: []const u8 = "",

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.cert.len > 0) allocator.free(_.cert);
        if (_.cert_error.code.len > 0) allocator.free(_.cert_error.code);
        if (_.cert_error.reason.len > 0) allocator.free(_.cert_error.reason);
        if (_.hostname.len > 0) allocator.free(_.hostname);
    }
};

export error InitError {
    FailedToOpenSocket,
    LoadCAFile,
    InvalidCAFile,
    InvalidCA,
}
