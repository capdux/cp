// Source: cp/lib/openapi/router.cp (OpenAPI 3.1.0 Path Template Matcher & Request Contract Validator)
// Pure CP OpenAPI path template specificity router (static > hybrid > {param}),
// 404 RouteNotFound / 405 MethodNotAllowed discriminator, and request contract validator.

const std = @import("std");
type Str = []const u8;
import {
    OpenApiError,
    HttpMethod,
    ParamIn,
    OpenApiParam,
    OpenApiOperation,
    OpenApiEndpoint,
    OpenApiDoc,
} from "./openapiSpec.cp";

pub struct KeyValuePair {
    key: []const u8 = "",
    value: []const u8 = "",
}

pub enum MatchStatus: u8 {
    Matched = 0,
    NotFound = 1,
    MethodNotAllowed = 2,
}

pub struct RouteMatch {
    allocator: std.mem.Allocator = undefined,
    path_template: []const u8 = "",
    method: HttpMethod = .Get,
    operation_id: []const u8 = "",
    operation: OpenApiOperation = OpenApiOperation{},
    path_params: []KeyValuePair = [],

    fn getParam(name: []const u8): ([]const u8)? {
        for (kv of _.path_params) {
            if (std.mem.eql(u8, kv.key, name)) {
                return kv.value;
            }
        }
        return null;
    }

    fn deinit(*_) {
        for (kv of _.path_params) {
            if (kv.key.len > 0) _.allocator.free(kv.key);
            if (kv.value.len > 0) _.allocator.free(kv.value);
        }
        if (_.path_params.len > 0) {
            _.allocator.free(_.path_params);
        }
        _.path_params = [];
    }
}

pub struct RouteMatchResult {
    allocator: std.mem.Allocator = undefined,
    status: MatchStatus = .NotFound,
    route: RouteMatch? = null,
    allowed_methods: []HttpMethod = [],

    fn deinit(*_) {
        if (*r := _.route) {
            r.deinit();
        }
        if (_.allowed_methods.len > 0) {
            _.allocator.free(_.allowed_methods);
        }
        _.allowed_methods = [];
    }
}

pub struct ValidationErrorItem {
    code: []const u8 = "",
    location: []const u8 = "",
    name: []const u8 = "",
    message: []const u8 = "",
}

pub struct ValidationReport {
    allocator: std.mem.Allocator = undefined,
    valid: bool = true,
    status_code: u16 = 200,
    operation_id: []const u8 = "",
    path_template: []const u8 = "",
    path_params: []KeyValuePair = [],
    allowed_methods: []HttpMethod = [],
    errors: []ValidationErrorItem = [],

    fn errorCount(): usize {
        return _.errors.len;
    }

    fn getPathParam(name: []const u8): ([]const u8)? {
        for (kv of _.path_params) {
            if (std.mem.eql(u8, kv.key, name)) {
                return kv.value;
            }
        }
        return null;
    }

    fn hasError(code: []const u8, name: []const u8): bool {
        for (err_item of _.errors) {
            if (std.mem.eql(u8, err_item.code, code)) {
                if (name.len == 0 || std.mem.eql(u8, err_item.name, name)) {
                    return true;
                }
            }
        }
        return false;
    }

    fn deinit(*_) {
        if (_.operation_id.len > 0) _.allocator.free(_.operation_id);
        if (_.path_template.len > 0) _.allocator.free(_.path_template);
        for (kv of _.path_params) {
            if (kv.key.len > 0) _.allocator.free(kv.key);
            if (kv.value.len > 0) _.allocator.free(kv.value);
        }
        if (_.path_params.len > 0) _.allocator.free(_.path_params);
        if (_.allowed_methods.len > 0) _.allocator.free(_.allowed_methods);
        for (e of _.errors) {
            if (e.name.len > 0) _.allocator.free(e.name);
            if (e.message.len > 0) _.allocator.free(e.message);
        }
        if (_.errors.len > 0) _.allocator.free(_.errors);
        _.path_params = [];
        _.allowed_methods = [];
        _.errors = [];
    }
}

struct SegmentMatchInfo {
    matched: bool = false,
    tier: u8 = 0, // 2 = static, 1 = hybrid, 0 = pure {param}
    has_param: bool = false,
    param_name: []const u8 = "",
    param_value: []const u8 = "",
}

pub struct OpenApiValidator {
    doc: *const OpenApiDoc,

    static fn init(doc: *const OpenApiDoc): OpenApiValidator {
        return OpenApiValidator{
            doc: doc,
        };
    }

    static fn stripQueryAndFragment(raw_path: []const u8): []const u8 {
        let end: usize = raw_path.len;
        let i: usize = 0;
        while (i < raw_path.len) : (i += 1) {
            if (raw_path[i] == '?' || raw_path[i] == '#') {
                end = i;
                break;
            }
        }
        return raw_path[0..end];
    }

    static fn splitPathSegments(alloc: std.mem.Allocator, path: []const u8): ([]Str)! {
        let segs: std.ArrayList(Str) = .empty;
        errdefer segs.deinit(alloc);

        let i: usize = 0;
        while (i < path.len) {
            while (i < path.len && path[i] == '/') : (i += 1) {}
            if (i >= path.len) break;
            const start = i;
            while (i < path.len && path[i] != '/') : (i += 1) {}
            segs.append(alloc, path[start..i]) catch return OpenApiError.OutOfMemory;
        }
        return segs.toOwnedSlice(alloc) catch return OpenApiError.OutOfMemory;
    }

    static fn matchSegment(tmpl_seg: []const u8, raw_seg: []const u8): SegmentMatchInfo {
        if (raw_seg.len == 0) {
            return SegmentMatchInfo{ matched: false };
        }
        const open_opt = std.mem.indexOfScalar(u8, tmpl_seg, '{');
        if (open_opt == null) {
            if (std.mem.eql(u8, tmpl_seg, raw_seg)) {
                return SegmentMatchInfo{
                    matched: true,
                    tier: 2,
                    has_param: false,
                };
            }
            return SegmentMatchInfo{ matched: false };
        }

        const open_idx = open_opt.?;
        const close_idx = std.mem.indexOfScalarPos(u8, tmpl_seg, open_idx + 1, '}') ?? {
            return SegmentMatchInfo{ matched: false };
        };

        const prefix = tmpl_seg[0..open_idx];
        const pname = tmpl_seg[open_idx + 1 .. close_idx];
        const suffix = tmpl_seg[close_idx + 1 ..];

        if (pname.len == 0) {
            return SegmentMatchInfo{ matched: false };
        }
        if (raw_seg.len <= prefix.len + suffix.len) {
            return SegmentMatchInfo{ matched: false };
        }
        if (prefix.len > 0 && !std.mem.startsWith(u8, raw_seg, prefix)) {
            return SegmentMatchInfo{ matched: false };
        }
        if (suffix.len > 0 && !std.mem.endsWith(u8, raw_seg, suffix)) {
            return SegmentMatchInfo{ matched: false };
        }

        const captured = raw_seg[prefix.len .. raw_seg.len - suffix.len];
        const tier: u8 = if (prefix.len == 0 && suffix.len == 0) 0 else 1;
        return SegmentMatchInfo{
            matched: true,
            tier: tier,
            has_param: true,
            param_name: pname,
            param_value: captured,
        };
    }

    /// Returns > 0 if `a` has higher routing specificity than `b`,
    /// < 0 if `b` is more specific, 0 if equal.
    static fn compareSpecificity(a: []const u8, b: []const u8): i32 {
        if (a.len != b.len) {
            return if (a.len > b.len) 1 else -1;
        }
        let i: usize = 0;
        while (i < a.len) : (i += 1) {
            if (a[i] != b[i]) {
                return if (a[i] > b[i]) 1 else -1;
            }
        }
        return 0;
    }

    static fn tryMatchTemplate(
        alloc: std.mem.Allocator,
        tmpl_path: []const u8,
        raw_segs: []const Str,
        out_tiers: *std.ArrayList(u8),
        out_params: *std.ArrayList(KeyValuePair),
    ): bool! {
        out_tiers.clearRetainingCapacity();
        out_params.clearRetainingCapacity();

        const tmpl_segs = OpenApiValidator.splitPathSegments(alloc, tmpl_path)?;
        defer alloc.free(tmpl_segs);

        if (tmpl_segs.len != raw_segs.len) {
            return false;
        }

        let i: usize = 0;
        while (i < tmpl_segs.len) : (i += 1) {
            const seg_info = OpenApiValidator.matchSegment(tmpl_segs[i], raw_segs[i]);
            if (!seg_info.matched) {
                return false;
            }
            out_tiers.append(alloc, seg_info.tier) catch return OpenApiError.OutOfMemory;
            if (seg_info.has_param) {
                out_params.append(alloc, KeyValuePair{
                    key: seg_info.param_name,
                    value: seg_info.param_value,
                }) catch return OpenApiError.OutOfMemory;
            }
        }
        return true;
    }

    fn matchRouteEx(alloc: std.mem.Allocator, method: HttpMethod, raw_path: []const u8): RouteMatchResult! {
        const clean_path = OpenApiValidator.stripQueryAndFragment(raw_path);
        if (clean_path.len == 0 || clean_path[0] != '/') {
            return RouteMatchResult{
                allocator: alloc,
                status: .NotFound,
            };
        }

        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();
        const ta = tmp_arena.allocator();

        const raw_segs = OpenApiValidator.splitPathSegments(ta, clean_path)?;

        let best_path: []const u8 = "";
        let best_tiers: []u8 = [];
        let best_params: []KeyValuePair = [];
        let has_match = false;

        let cur_tiers: std.ArrayList(u8) = .empty;
        let cur_params: std.ArrayList(KeyValuePair) = .empty;

        const endpoints = _.doc.getEndpoints();
        for (ep of endpoints) {
            const ok = OpenApiValidator.tryMatchTemplate(ta, ep.path, raw_segs, &cur_tiers, &cur_params)?;
            if (!ok) continue;

            if (!has_match || OpenApiValidator.compareSpecificity(cur_tiers.items, best_tiers) > 0) {
                has_match = true;
                best_path = ep.path;
                best_tiers = ta.dupe(u8, cur_tiers.items) catch return OpenApiError.OutOfMemory;
                best_params = ta.dupe(KeyValuePair, cur_params.items) catch return OpenApiError.OutOfMemory;
            }
        }

        if (!has_match) {
            return RouteMatchResult{
                allocator: alloc,
                status: .NotFound,
            };
        }

        // Gather all methods registered on `best_path` and look for `method`
        let allowed: std.ArrayList(HttpMethod) = .empty;
        errdefer allowed.deinit(alloc);

        let matched_ep: OpenApiEndpoint? = null;
        for (ep of endpoints) {
            if (std.mem.eql(u8, ep.path, best_path)) {
                let already = false;
                for (m of allowed.items) {
                    if (m == ep.method) {
                        already = true;
                        break;
                    }
                }
                if (!already) {
                    allowed.append(alloc, ep.method) catch return OpenApiError.OutOfMemory;
                }
                if (ep.method == method) {
                    matched_ep = ep;
                }
            }
        }

        const owned_allowed = allowed.toOwnedSlice(alloc) catch return OpenApiError.OutOfMemory;
        errdefer if (owned_allowed.len > 0) alloc.free(owned_allowed);

        if (hit := matched_ep) {
            let owned_params = alloc.alloc(KeyValuePair, best_params.len) catch return OpenApiError.OutOfMemory;
            let copied: usize = 0;
            errdefer {
                let k: usize = 0;
                while (k < copied) : (k += 1) {
                    if (owned_params[k].key.len > 0) alloc.free(owned_params[k].key);
                    if (owned_params[k].value.len > 0) alloc.free(owned_params[k].value);
                }
                alloc.free(owned_params);
            }
            while (copied < best_params.len) : (copied += 1) {
                const kv = best_params[copied];
                const k_dup = alloc.dupe(u8, kv.key) catch return OpenApiError.OutOfMemory;
                errdefer alloc.free(k_dup);
                const v_dup = alloc.dupe(u8, kv.value) catch return OpenApiError.OutOfMemory;
                owned_params[copied] = KeyValuePair{
                    key: k_dup,
                    value: v_dup,
                };
            }

            return RouteMatchResult{
                allocator: alloc,
                status: .Matched,
                route: RouteMatch{
                    allocator: alloc,
                    path_template: hit.path,
                    method: hit.method,
                    operation_id: hit.operation.operation_id,
                    operation: hit.operation,
                    path_params: owned_params,
                },
                allowed_methods: owned_allowed,
            };
        }

        return RouteMatchResult{
            allocator: alloc,
            status: .MethodNotAllowed,
            route: null,
            allowed_methods: owned_allowed,
        };
    }

    fn matchRoute(alloc: std.mem.Allocator, method: HttpMethod, raw_path: []const u8): RouteMatch! {
        let res = _.matchRouteEx(alloc, method, raw_path)?;
        switch (res.status) {
            .Matched => {
                if (res.allowed_methods.len > 0) {
                    alloc.free(res.allowed_methods);
                }
                return res.route.?;
            },
            .MethodNotAllowed => {
                res.deinit();
                return OpenApiError.MethodNotAllowed;
            },
            .NotFound => {
                res.deinit();
                return OpenApiError.RouteNotFound;
            },
        }
    }

    fn allowedMethods(alloc: std.mem.Allocator, raw_path: []const u8): ([]HttpMethod)! {
        let res = _.matchRouteEx(alloc, .Get, raw_path)?;
        switch (res.status) {
            .NotFound => {
                res.deinit();
                return OpenApiError.RouteNotFound;
            },
            .Matched | .MethodNotAllowed => {
                if (*r := res.route) {
                    r.deinit();
                }
                return res.allowed_methods;
            },
        }
    }

    // -------------------------------------------------------------------------
    // Parameter Format & Request Contract Validation
    // -------------------------------------------------------------------------

    static fn isStrictInteger(val: []const u8): bool {
        if (val.len == 0) return false;
        let idx: usize = 0;
        if (val[0] == '-' || val[0] == '+') {
            idx = 1;
        }
        if (idx >= val.len) return false;
        while (idx < val.len) : (idx += 1) {
            if (val[idx] < '0' || val[idx] > '9') return false;
        }
        const parsed = std.fmt.parseInt(i64, val, 10) catch return false;
        return parsed >= std.math.minInt(i64);
    }

    static fn isStrictNumber(val: []const u8): bool {
        if (val.len == 0) return false;
        for (c of val) {
            if (c == '_') return false;
        }
        const f = std.fmt.parseFloat(f64, val) catch return false;
        return std.math.isFinite(f);
    }

    static fn isStrictBoolean(val: []const u8): bool {
        if (std.ascii.eqlIgnoreCase(val, "true") || std.ascii.eqlIgnoreCase(val, "false")) {
            return true;
        }
        return std.mem.eql(u8, val, "1") || std.mem.eql(u8, val, "0");
    }

    static fn checkParamValueType(schema_type: []const u8, val: []const u8): bool {
        if (std.mem.eql(u8, schema_type, "integer")) {
            return OpenApiValidator.isStrictInteger(val);
        }
        if (std.mem.eql(u8, schema_type, "number")) {
            return OpenApiValidator.isStrictNumber(val);
        }
        if (std.mem.eql(u8, schema_type, "boolean")) {
            return OpenApiValidator.isStrictBoolean(val);
        }
        return true;
    }

    static fn lookupParam(pairs: []const KeyValuePair, name: []const u8, case_insensitive: bool): ([]const u8)? {
        for (kv of pairs) {
            const eq = if (case_insensitive)
                std.ascii.eqlIgnoreCase(kv.key, name)
            else
                std.mem.eql(u8, kv.key, name);
            if (eq) return kv.value;
        }
        return null;
    }

    static fn lookupCookieFromHeaders(headers: []const KeyValuePair, cookie_name: []const u8): ([]const u8)? {
        for (h of headers) {
            if (!std.ascii.eqlIgnoreCase(h.key, "cookie")) continue;
            let pos: usize = 0;
            const raw = h.value;
            while (pos < raw.len) {
                let semi = std.mem.indexOfScalarPos(u8, raw, pos, ';') ?? raw.len;
                const part = std.mem.trim(u8, raw[pos..semi], " \t");
                if (eq_idx := std.mem.indexOfScalar(u8, part, '=')) {
                    const k = std.mem.trim(u8, part[0..eq_idx], " \t");
                    const v = std.mem.trim(u8, part[eq_idx + 1 ..], " \t");
                    if (std.mem.eql(u8, k, cookie_name)) return v;
                }
                pos = if (semi < raw.len) semi + 1 else raw.len;
            }
        }
        return null;
    }

    static fn appendValidationError(
        alloc: std.mem.Allocator,
        errs: *std.ArrayList(ValidationErrorItem),
        code: []const u8,
        location: []const u8,
        name: []const u8,
        msg: []const u8,
    ): ! {
        const name_dup = if (name.len > 0) alloc.dupe(u8, name) catch return OpenApiError.OutOfMemory else "";
        errdefer if (name_dup.len > 0) alloc.free(name_dup);
        const msg_dup = alloc.dupe(u8, msg) catch return OpenApiError.OutOfMemory;
        errdefer alloc.free(msg_dup);

        errs.append(alloc, ValidationErrorItem{
            code: code,
            location: location,
            name: name_dup,
            message: msg_dup,
        }) catch return OpenApiError.OutOfMemory;
    }

    static fn freeErrorList(alloc: std.mem.Allocator, errs: *std.ArrayList(ValidationErrorItem)) {
        for (e of errs.items) {
            if (e.name.len > 0) alloc.free(e.name);
            if (e.message.len > 0) alloc.free(e.message);
        }
        errs.deinit(alloc);
    }

    fn validateRequest(
        alloc: std.mem.Allocator,
        method: HttpMethod,
        raw_path: []const u8,
        query_params: []const KeyValuePair,
        headers: []const KeyValuePair,
        has_body: bool,
    ): ValidationReport! {
        let match_res = _.matchRouteEx(alloc, method, raw_path)?;

        if (match_res.status == .NotFound) {
            match_res.deinit();
            let errs: std.ArrayList(ValidationErrorItem) = .empty;
            errdefer OpenApiValidator.freeErrorList(alloc, &errs);
            const msg = std.fmt.allocPrint(alloc, "No route matches path '{s}'", .{raw_path}) catch return OpenApiError.OutOfMemory;
            defer alloc.free(msg);
            OpenApiValidator.appendValidationError(alloc, &errs, "route_not_found", "route", "", msg)?;
            const owned_errs = errs.toOwnedSlice(alloc) catch return OpenApiError.OutOfMemory;
            return ValidationReport{
                allocator: alloc,
                valid: false,
                status_code: 404,
                errors: owned_errs,
            };
        }

        if (match_res.status == .MethodNotAllowed) {
            const allowed = match_res.allowed_methods;
            match_res.allowed_methods = [];
            match_res.deinit();
            errdefer if (allowed.len > 0) alloc.free(allowed);

            let errs: std.ArrayList(ValidationErrorItem) = .empty;
            errdefer OpenApiValidator.freeErrorList(alloc, &errs);
            const msg = std.fmt.allocPrint(alloc, "Method '{s}' is not allowed for '{s}'", .{ method.toUpperString(), raw_path }) catch return OpenApiError.OutOfMemory;
            defer alloc.free(msg);
            OpenApiValidator.appendValidationError(alloc, &errs, "method_not_allowed", "route", "", msg)?;
            const owned_errs = errs.toOwnedSlice(alloc) catch return OpenApiError.OutOfMemory;

            return ValidationReport{
                allocator: alloc,
                valid: false,
                status_code: 405,
                allowed_methods: allowed,
                errors: owned_errs,
            };
        }

        // Matched
        if (match_res.allowed_methods.len > 0) {
            alloc.free(match_res.allowed_methods);
            match_res.allowed_methods = [];
        }
        let route = match_res.route.?;
        const path_params = route.path_params;
        route.path_params = [];
        const op = route.operation;
        const tmpl = route.path_template;
        errdefer {
            for (kv of path_params) {
                if (kv.key.len > 0) alloc.free(kv.key);
                if (kv.value.len > 0) alloc.free(kv.value);
            }
            if (path_params.len > 0) alloc.free(path_params);
        }

        const op_id_dup = if (op.operation_id.len > 0) alloc.dupe(u8, op.operation_id) catch return OpenApiError.OutOfMemory else "";
        errdefer if (op_id_dup.len > 0) alloc.free(op_id_dup);
        const tmpl_dup = if (tmpl.len > 0) alloc.dupe(u8, tmpl) catch return OpenApiError.OutOfMemory else "";
        errdefer if (tmpl_dup.len > 0) alloc.free(tmpl_dup);

        let errs: std.ArrayList(ValidationErrorItem) = .empty;
        errdefer OpenApiValidator.freeErrorList(alloc, &errs);

        for (param of op.params) {
            const loc_str = param.param_in.asStr();
            const is_required = if (param.param_in == .Path) true else param.required;

            let val_opt: ([]const u8)? = null;
            switch (param.param_in) {
                .Path => {
                    val_opt = OpenApiValidator.lookupParam(path_params, param.name, false);
                },
                .Query => {
                    val_opt = OpenApiValidator.lookupParam(query_params, param.name, false);
                },
                .Header => {
                    val_opt = OpenApiValidator.lookupParam(headers, param.name, true);
                },
                .Cookie => {
                    val_opt = OpenApiValidator.lookupCookieFromHeaders(headers, param.name);
                },
            }

            if (val := val_opt) {
                if (!OpenApiValidator.checkParamValueType(param.schema_type, val)) {
                    const msg = std.fmt.allocPrint(
                        alloc,
                        "Parameter '{s}' in {s} expected type '{s}', got '{s}'",
                        .{ param.name, loc_str, param.schema_type, val },
                    ) catch return OpenApiError.OutOfMemory;
                    defer alloc.free(msg);
                    OpenApiValidator.appendValidationError(alloc, &errs, "invalid_param_type", loc_str, param.name, msg)?;
                }
            } else if (is_required) {
                const msg = std.fmt.allocPrint(
                    alloc,
                    "Missing required {s} parameter '{s}'",
                    .{ loc_str, param.name },
                ) catch return OpenApiError.OutOfMemory;
                defer alloc.free(msg);
                OpenApiValidator.appendValidationError(alloc, &errs, "missing_required_param", loc_str, param.name, msg)?;
            }
        }

        if (op.request_body_required && !has_body) {
            OpenApiValidator.appendValidationError(
                alloc,
                &errs,
                "missing_request_body",
                "body",
                "body",
                "Required request body is missing",
            )?;
        }

        const is_valid = errs.items.len == 0;
        const owned_errs = errs.toOwnedSlice(alloc) catch return OpenApiError.OutOfMemory;
        return ValidationReport{
            allocator: alloc,
            valid: is_valid,
            status_code: if (is_valid) 200 else 400,
            operation_id: op_id_dup,
            path_template: tmpl_dup,
            path_params: path_params,
            errors: owned_errs,
        };
    }
}
