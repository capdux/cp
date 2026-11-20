// Source: cp/lib/syslog/types.cp (RFC 5424 & RFC 3164 Syslog Core Types)
// Pure CP zero-C-dependency Syslog data model, Facility/Severity enums,
// Structured Data (SD-ELEMENT / SD-PARAM), and RAII memory management.

const std = @import("std");

pub err SyslogError {
    InvalidPriority,
    InvalidVersion,
    MalformedHeader,
    MalformedStructuredData,
    InvalidTimestamp,
    OutOfMemory,
}

pub enum Facility {
    Kern,
    User,
    Mail,
    Daemon,
    Auth,
    Syslog,
    Lpr,
    News,
    Uucp,
    Cron,
    Authpriv,
    Ftp,
    Ntp,
    Audit,
    Alert,
    Clock,
    Local0,
    Local1,
    Local2,
    Local3,
    Local4,
    Local5,
    Local6,
    Local7,

    pub static fn fromCode(code: u8): Facility? {
        return switch (code) {
            0 => .Kern,
            1 => .User,
            2 => .Mail,
            3 => .Daemon,
            4 => .Auth,
            5 => .Syslog,
            6 => .Lpr,
            7 => .News,
            8 => .Uucp,
            9 => .Cron,
            10 => .Authpriv,
            11 => .Ftp,
            12 => .Ntp,
            13 => .Audit,
            14 => .Alert,
            15 => .Clock,
            16 => .Local0,
            17 => .Local1,
            18 => .Local2,
            19 => .Local3,
            20 => .Local4,
            21 => .Local5,
            22 => .Local6,
            23 => .Local7,
            else => null,
        };
    }

    pub fn toCode(): u8 {
        return switch (_) {
            .Kern => 0,
            .User => 1,
            .Mail => 2,
            .Daemon => 3,
            .Auth => 4,
            .Syslog => 5,
            .Lpr => 6,
            .News => 7,
            .Uucp => 8,
            .Cron => 9,
            .Authpriv => 10,
            .Ftp => 11,
            .Ntp => 12,
            .Audit => 13,
            .Alert => 14,
            .Clock => 15,
            .Local0 => 16,
            .Local1 => 17,
            .Local2 => 18,
            .Local3 => 19,
            .Local4 => 20,
            .Local5 => 21,
            .Local6 => 22,
            .Local7 => 23,
        };
    }

    pub fn toString(): []const u8 {
        return switch (_) {
            .Kern => "kern",
            .User => "user",
            .Mail => "mail",
            .Daemon => "daemon",
            .Auth => "auth",
            .Syslog => "syslog",
            .Lpr => "lpr",
            .News => "news",
            .Uucp => "uucp",
            .Cron => "cron",
            .Authpriv => "authpriv",
            .Ftp => "ftp",
            .Ntp => "ntp",
            .Audit => "audit",
            .Alert => "alert",
            .Clock => "clock",
            .Local0 => "local0",
            .Local1 => "local1",
            .Local2 => "local2",
            .Local3 => "local3",
            .Local4 => "local4",
            .Local5 => "local5",
            .Local6 => "local6",
            .Local7 => "local7",
        };
    }
}

pub enum Severity {
    Emerg,
    Alert,
    Crit,
    Err,
    Warning,
    Notice,
    Info,
    Debug,

    pub static fn fromCode(code: u8): Severity? {
        return switch (code) {
            0 => .Emerg,
            1 => .Alert,
            2 => .Crit,
            3 => .Err,
            4 => .Warning,
            5 => .Notice,
            6 => .Info,
            7 => .Debug,
            else => null,
        };
    }

    pub fn toCode(): u8 {
        return switch (_) {
            .Emerg => 0,
            .Alert => 1,
            .Crit => 2,
            .Err => 3,
            .Warning => 4,
            .Notice => 5,
            .Info => 6,
            .Debug => 7,
        };
    }

    pub fn toString(): []const u8 {
        return switch (_) {
            .Emerg => "emerg",
            .Alert => "alert",
            .Crit => "crit",
            .Err => "err",
            .Warning => "warning",
            .Notice => "notice",
            .Info => "info",
            .Debug => "debug",
        };
    }
}

pub struct SdParam {
    name: []const u8 = "",
    value: []const u8 = "",

    pub static fn init(name: []const u8, value: []const u8): SdParam {
        return SdParam{ name: name, value: value };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.name.len > 0) alloc.free(_.name);
        if (_.value.len > 0) alloc.free(_.value);
    }
}

pub struct SdElement {
    id: []const u8 = "",
    params: []SdParam = &[],

    pub static fn init(id: []const u8, params: []SdParam): SdElement {
        return SdElement{ id: id, params: params };
    }

    pub fn getParam(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.params.len) : (i += 1) {
            if (std.mem.eql(u8, _.params[i].name, name)) {
                return _.params[i].value;
            }
        }
        return null;
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.id.len > 0) alloc.free(_.id);
        let i: usize = 0;
        while (i < _.params.len) : (i += 1) {
            _.params[i].deinit(alloc);
        }
        if (_.params.len > 0) alloc.free(_.params);
    }
}

pub struct SyslogMessage {
    facility: Facility = Facility.User,
    severity: Severity = Severity.Info,
    priority: u8 = 14,
    version: u8 = 1,
    timestamp: ([]const u8)? = null,
    hostname: ([]const u8)? = null,
    app_name: ([]const u8)? = null,
    proc_id: ([]const u8)? = null,
    msg_id: ([]const u8)? = null,
    structured_data: []SdElement = &[],
    message: []const u8 = "",
    is_rfc5424: bool = true,
    allocator: std.mem.Allocator? = null,

    pub fn getSdElement(id: []const u8): SdElement? {
        let i: usize = 0;
        while (i < _.structured_data.len) : (i += 1) {
            if (std.mem.eql(u8, _.structured_data[i].id, id)) {
                return _.structured_data[i];
            }
        }
        return null;
    }

    pub fn deinit(*_) {
        if (alloc := _.allocator) {
            if (ts := _.timestamp) alloc.free(ts);
            if (h := _.hostname) alloc.free(h);
            if (a := _.app_name) alloc.free(a);
            if (p := _.proc_id) alloc.free(p);
            if (m := _.msg_id) alloc.free(m);
            let i: usize = 0;
            while (i < _.structured_data.len) : (i += 1) {
                _.structured_data[i].deinit(alloc);
            }
            if (_.structured_data.len > 0) alloc.free(_.structured_data);
            if (_.message.len > 0) alloc.free(_.message);
            _.allocator = null;
        }
    }
}
