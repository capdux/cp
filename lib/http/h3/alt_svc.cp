// Source: /data/dev/bun/src/http/h3_client/AltSvc.cp
// lib/http/h3/alt_svc.cp
// CP HTTP/3 Alt-Svc Parser (cap:http)
// RFC 7838 Alt-Svc header handling for HTTP/3 client upgrade

const std = @import("std");

pub struct AltSvcEntry {
    pub protocol: []const u8 = "h3";
    pub port: u16 = 443;
    pub max_age: u32 = 86400;
}

pub struct AltSvc {
    pub static fn parse(field_value: []const u8): AltSvcEntry? {
        const trimmed = std.mem.trim(u8, field_value, " \t");
        if (trimmed.len == 0 || std.mem.eql(u8, trimmed, "clear")) {
            return null;
        }

        // Example: h3=":443"; ma=86400, h3-29=":443"
        let entries = std.mem.splitScalar(u8, trimmed, ',');
        while (raw_entry := entries.next()) {
            const entry = std.mem.trim(u8, raw_entry, " \t");
            if (entry.len == 0) continue;

            let params = std.mem.splitScalar(u8, entry, ';');
            const alt = std.mem.trim(u8, params.first(), " \t");

            if (eq := std.mem.indexOfScalar(u8, alt, '=')) {
                const proto = alt[0..eq];
                if (!std.mem.eql(u8, proto, "h3")) continue;

                let auth = std.mem.trim(u8, alt[eq + 1 ..], " \t");
                if (auth.len >= 2 && auth[0] == '"' && auth[auth.len - 1] == '"') {
                    auth = auth[1 .. auth.len - 1];
                }
                if (colon := std.mem.lastIndexOfScalar(u8, auth, ':')) {
                    const port_str = auth[colon + 1 ..];
                    const port = std.fmt.parseInt(u16, port_str, 10) catch 0;
                    if (port == 0) continue;

                    let ma: u32 = 86400;
                    while (raw_p := params.next()) {
                        const p = std.mem.trim(u8, raw_p, " \t");
                        if (peq := std.mem.indexOfScalar(u8, p, '=')) {
                            const k = p[0..peq];
                            const v = p[peq + 1 ..];
                            if (std.mem.eql(u8, k, "ma")) {
                                ma = std.fmt.parseInt(u32, v, 10) catch 86400;
                            }
                        }
                    }

                    return AltSvcEntry{
                        protocol: "h3",
                        port: port,
                        max_age: ma,
                    };
                }
            }
        }
        return null;
    }
}
