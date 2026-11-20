// Source: cp/lib/openapi/spec.cp (OpenAPI 3.1.0 Specification AST, Builder & JSON Serializer)
// Pure CP OpenAPI 3.1.0 specification model, fluent builders, path/operationId/status validators,
// and RFC 8259 compliant JSON serializer (`cap:openapi` & `cap.openapi`).

const std = @import("std");

type Str = []const u8;

pub err OpenApiError {
    InvalidPath,
    DuplicateOperationId,
    InvalidStatusCode,
    RouteNotFound,
    MethodNotAllowed,
    MissingRequiredParam,
    InvalidParamType,
    OutOfMemory,
}

pub enum HttpMethod: u8 {
    Get = 0,
    Post = 1,
    Put = 2,
    Patch = 3,
    Delete = 4,
    Head = 5,
    Options = 6,

    fn asStr(): string {
        return switch (_) {
            .Get => "get",
            .Post => "post",
            .Put => "put",
            .Patch => "patch",
            .Delete => "delete",
            .Head => "head",
            .Options => "options",
        };
    }

    fn toUpperString(): string {
        return switch (_) {
            .Get => "GET",
            .Post => "POST",
            .Put => "PUT",
            .Patch => "PATCH",
            .Delete => "DELETE",
            .Head => "HEAD",
            .Options => "OPTIONS",
        };
    }

    static fn fromString(s: []const u8): HttpMethod? {
        if (std.ascii.eqlIgnoreCase(s, "GET")) return .Get;
        if (std.ascii.eqlIgnoreCase(s, "POST")) return .Post;
        if (std.ascii.eqlIgnoreCase(s, "PUT")) return .Put;
        if (std.ascii.eqlIgnoreCase(s, "PATCH")) return .Patch;
        if (std.ascii.eqlIgnoreCase(s, "DELETE")) return .Delete;
        if (std.ascii.eqlIgnoreCase(s, "HEAD")) return .Head;
        if (std.ascii.eqlIgnoreCase(s, "OPTIONS")) return .Options;
        return null;
    }
}

pub enum ParamIn: u8 {
    Path = 0,
    Query = 1,
    Header = 2,
    Cookie = 3,

    fn asStr(): string {
        return switch (_) {
            .Path => "path",
            .Query => "query",
            .Header => "header",
            .Cookie => "cookie",
        };
    }
}

pub enum SecuritySchemeType: u8 {
    ApiKey = 0,
    HttpBearer = 1,
    HttpBasic = 2,
    OAuth2 = 3,

    fn asStr(): string {
        return switch (_) {
            .ApiKey => "apiKey",
            .HttpBearer => "http",
            .HttpBasic => "http",
            .OAuth2 => "oauth2",
        };
    }
}

pub struct OpenApiInfo {
    title: []const u8 = "",
    version: []const u8 = "",
    summary: []const u8 = "",
    description: []const u8 = "",
    terms_of_service: []const u8 = "",
    contact_name: []const u8 = "",
    contact_url: []const u8 = "",
    contact_email: []const u8 = "",
    license_name: []const u8 = "",
    license_identifier: []const u8 = "",
    license_url: []const u8 = "",

    static fn init(title: []const u8, version: []const u8): OpenApiInfo {
        return OpenApiInfo{
            title: title,
            version: version,
        };
    }

    fn copy(): OpenApiInfo {
        return OpenApiInfo{
            title: _.title,
            version: _.version,
            summary: _.summary,
            description: _.description,
            terms_of_service: _.terms_of_service,
            contact_name: _.contact_name,
            contact_url: _.contact_url,
            contact_email: _.contact_email,
            license_name: _.license_name,
            license_identifier: _.license_identifier,
            license_url: _.license_url,
        };
    }

    fn withSummary(s: []const u8): OpenApiInfo {
        let c = _.copy();
        c.summary = s;
        return c;
    }

    fn withDescription(d: []const u8): OpenApiInfo {
        let c = _.copy();
        c.description = d;
        return c;
    }

    fn withContact(name: []const u8, email: []const u8, url: []const u8): OpenApiInfo {
        let c = _.copy();
        c.contact_name = name;
        c.contact_email = email;
        c.contact_url = url;
        return c;
    }

    fn withLicense(name: []const u8, identifier: []const u8, url: []const u8): OpenApiInfo {
        let c = _.copy();
        c.license_name = name;
        c.license_identifier = identifier;
        c.license_url = url;
        return c;
    }
}

pub struct OpenApiServer {
    url: []const u8 = "",
    description: []const u8 = "",

    static fn init(url: []const u8, description: []const u8): OpenApiServer {
        return OpenApiServer{
            url: url,
            description: description,
        };
    }
}

pub struct OpenApiTag {
    name: []const u8 = "",
    description: []const u8 = "",

    static fn init(name: []const u8, description: []const u8): OpenApiTag {
        return OpenApiTag{
            name: name,
            description: description,
        };
    }
}

pub struct OpenApiParam {
    name: []const u8 = "",
    param_in: ParamIn = .Query,
    required: bool = false,
    description: []const u8 = "",
    schema_type: []const u8 = "string",
    schema_format: []const u8 = "",
    schema_json: []const u8 = "",
    deprecated: bool = false,

    static fn init(name: []const u8, param_in: ParamIn, required: bool, schema_type: []const u8): OpenApiParam {
        return OpenApiParam{
            name: name,
            param_in: param_in,
            required: if (param_in == .Path) true else required,
            schema_type: if (schema_type.len > 0) schema_type else "string",
        };
    }

    static fn path(name: []const u8, schema_type: []const u8): OpenApiParam {
        return OpenApiParam.init(name, .Path, true, schema_type);
    }

    static fn query(name: []const u8, required: bool, schema_type: []const u8): OpenApiParam {
        return OpenApiParam.init(name, .Query, required, schema_type);
    }

    static fn header(name: []const u8, required: bool, schema_type: []const u8): OpenApiParam {
        return OpenApiParam.init(name, .Header, required, schema_type);
    }

    static fn cookie(name: []const u8, required: bool, schema_type: []const u8): OpenApiParam {
        return OpenApiParam.init(name, .Cookie, required, schema_type);
    }

    fn copy(): OpenApiParam {
        return OpenApiParam{
            name: _.name,
            param_in: _.param_in,
            required: _.required,
            description: _.description,
            schema_type: _.schema_type,
            schema_format: _.schema_format,
            schema_json: _.schema_json,
            deprecated: _.deprecated,
        };
    }

    fn withDescription(desc: []const u8): OpenApiParam {
        let c = _.copy();
        c.description = desc;
        return c;
    }

    fn withFormat(fmt: []const u8): OpenApiParam {
        let c = _.copy();
        c.schema_format = fmt;
        return c;
    }

    fn withSchemaJson(json_str: []const u8): OpenApiParam {
        let c = _.copy();
        c.schema_json = json_str;
        return c;
    }

    fn withDeprecated(dep: bool): OpenApiParam {
        let c = _.copy();
        c.deprecated = dep;
        return c;
    }
}

pub struct OpenApiResponse {
    status_code: []const u8 = "200",
    description: []const u8 = "",
    content_type: []const u8 = "application/json",
    schema_ref: []const u8 = "",
    schema_json: []const u8 = "",

    static fn init(status_code: []const u8, description: []const u8): OpenApiResponse {
        return OpenApiResponse{
            status_code: status_code,
            description: description,
        };
    }

    static fn jsonRef(status_code: []const u8, description: []const u8, schema_name: []const u8): OpenApiResponse {
        return OpenApiResponse{
            status_code: status_code,
            description: description,
            content_type: "application/json",
            schema_ref: schema_name,
        };
    }

    static fn jsonSchema(status_code: []const u8, description: []const u8, schema_json: []const u8): OpenApiResponse {
        return OpenApiResponse{
            status_code: status_code,
            description: description,
            content_type: "application/json",
            schema_json: schema_json,
        };
    }

    fn copy(): OpenApiResponse {
        return OpenApiResponse{
            status_code: _.status_code,
            description: _.description,
            content_type: _.content_type,
            schema_ref: _.schema_ref,
            schema_json: _.schema_json,
        };
    }

    fn withContentType(ct: []const u8): OpenApiResponse {
        let c = _.copy();
        c.content_type = ct;
        return c;
    }

    fn withSchemaRef(schema_name: []const u8): OpenApiResponse {
        let c = _.copy();
        c.schema_ref = schema_name;
        return c;
    }

    fn withSchemaJson(schema_json: []const u8): OpenApiResponse {
        let c = _.copy();
        c.schema_json = schema_json;
        return c;
    }
}

pub struct OpenApiSecurityScheme {
    name: []const u8 = "",
    scheme_type: SecuritySchemeType = .HttpBearer,
    description: []const u8 = "",
    param_name: []const u8 = "",
    api_key_in: ParamIn = .Header,
    bearer_format: []const u8 = "JWT",
    oauth2_flows_json: []const u8 = "",

    static fn bearer(name: []const u8, bearer_format: []const u8): OpenApiSecurityScheme {
        return OpenApiSecurityScheme{
            name: name,
            scheme_type: .HttpBearer,
            bearer_format: bearer_format,
        };
    }

    static fn basic(name: []const u8): OpenApiSecurityScheme {
        return OpenApiSecurityScheme{
            name: name,
            scheme_type: .HttpBasic,
            bearer_format: "",
        };
    }

    static fn apiKey(name: []const u8, param_name: []const u8, key_in: ParamIn): OpenApiSecurityScheme {
        return OpenApiSecurityScheme{
            name: name,
            scheme_type: .ApiKey,
            param_name: param_name,
            api_key_in: key_in,
            bearer_format: "",
        };
    }

    static fn oauth2(name: []const u8, flows_json: []const u8): OpenApiSecurityScheme {
        return OpenApiSecurityScheme{
            name: name,
            scheme_type: .OAuth2,
            oauth2_flows_json: flows_json,
            bearer_format: "",
        };
    }

    fn copy(): OpenApiSecurityScheme {
        return OpenApiSecurityScheme{
            name: _.name,
            scheme_type: _.scheme_type,
            description: _.description,
            param_name: _.param_name,
            api_key_in: _.api_key_in,
            bearer_format: _.bearer_format,
            oauth2_flows_json: _.oauth2_flows_json,
        };
    }

    fn withDescription(desc: []const u8): OpenApiSecurityScheme {
        let c = _.copy();
        c.description = desc;
        return c;
    }
}

pub struct OpenApiOperation {
    operation_id: []const u8 = "",
    summary: []const u8 = "",
    description: []const u8 = "",
    tags: []const []const u8 = [],
    params: []const OpenApiParam = [],
    request_body_required: bool = false,
    request_body_content_type: []const u8 = "application/json",
    request_body_schema_ref: []const u8 = "",
    request_body_schema_json: []const u8 = "",
    request_body_description: []const u8 = "",
    responses: []const OpenApiResponse = [],
    security: []const []const u8 = [],
    deprecated: bool = false,

    static fn init(operation_id: []const u8): OpenApiOperation {
        return OpenApiOperation{
            operation_id: operation_id,
        };
    }

    fn copy(): OpenApiOperation {
        return OpenApiOperation{
            operation_id: _.operation_id,
            summary: _.summary,
            description: _.description,
            tags: _.tags,
            params: _.params,
            request_body_required: _.request_body_required,
            request_body_content_type: _.request_body_content_type,
            request_body_schema_ref: _.request_body_schema_ref,
            request_body_schema_json: _.request_body_schema_json,
            request_body_description: _.request_body_description,
            responses: _.responses,
            security: _.security,
            deprecated: _.deprecated,
        };
    }

    fn withSummary(s: []const u8): OpenApiOperation {
        let c = _.copy();
        c.summary = s;
        return c;
    }

    fn withDescription(d: []const u8): OpenApiOperation {
        let c = _.copy();
        c.description = d;
        return c;
    }

    fn withTags(tags: []const []const u8): OpenApiOperation {
        let c = _.copy();
        c.tags = tags;
        return c;
    }

    fn withParams(params: []const OpenApiParam): OpenApiOperation {
        let c = _.copy();
        c.params = params;
        return c;
    }

    fn withResponses(responses: []const OpenApiResponse): OpenApiOperation {
        let c = _.copy();
        c.responses = responses;
        return c;
    }

    fn withSecurity(schemes: []const []const u8): OpenApiOperation {
        let c = _.copy();
        c.security = schemes;
        return c;
    }

    fn withDeprecated(dep: bool): OpenApiOperation {
        let c = _.copy();
        c.deprecated = dep;
        return c;
    }

    fn withRequestBodyRequired(required: bool): OpenApiOperation {
        let c = _.copy();
        c.request_body_required = required;
        return c;
    }

    fn withRequestBodyRef(schema_name: []const u8, required: bool): OpenApiOperation {
        let c = _.copy();
        c.request_body_schema_ref = schema_name;
        c.request_body_required = required;
        return c;
    }

    fn withRequestBodyJson(schema_json: []const u8, required: bool): OpenApiOperation {
        let c = _.copy();
        c.request_body_schema_json = schema_json;
        c.request_body_required = required;
        return c;
    }

    fn withRequestBodyDescription(desc: []const u8): OpenApiOperation {
        let c = _.copy();
        c.request_body_description = desc;
        return c;
    }
}

pub struct OpenApiComponentSchema {
    name: []const u8 = "",
    json_schema: []const u8 = "",
}

pub struct OpenApiEndpoint {
    path: []const u8 = "",
    method: HttpMethod = .Get,
    operation: OpenApiOperation = OpenApiOperation{},
}

pub struct OpenApiDoc {
    parent_alloc: std.mem.Allocator = undefined,
    owned_arena: *std.heap.ArenaAllocator = undefined,
    openapi_version: []const u8 = "3.1.0",
    json_schema_dialect: []const u8 = "",
    info: OpenApiInfo = OpenApiInfo{},
    servers: std.ArrayList(OpenApiServer) = .empty,
    tags: std.ArrayList(OpenApiTag) = .empty,
    security_schemes: std.ArrayList(OpenApiSecurityScheme) = .empty,
    component_schemas: std.ArrayList(OpenApiComponentSchema) = .empty,
    endpoints: std.ArrayList(OpenApiEndpoint) = .empty,

    static fn init(alloc: std.mem.Allocator, info: OpenApiInfo): OpenApiDoc! {
        let arena_ptr = alloc.create(std.heap.ArenaAllocator) catch return OpenApiError.OutOfMemory;
        unsafe {
            arena_ptr.* = std.heap.ArenaAllocator.init(alloc);
        }
        errdefer {
            arena_ptr.deinit();
            alloc.destroy(arena_ptr);
        }

        const aa = arena_ptr.allocator();
        let owned_info = OpenApiInfo{
            title: aa.dupe(u8, info.title) catch return OpenApiError.OutOfMemory,
            version: aa.dupe(u8, info.version) catch return OpenApiError.OutOfMemory,
            summary: aa.dupe(u8, info.summary) catch return OpenApiError.OutOfMemory,
            description: aa.dupe(u8, info.description) catch return OpenApiError.OutOfMemory,
            terms_of_service: aa.dupe(u8, info.terms_of_service) catch return OpenApiError.OutOfMemory,
            contact_name: aa.dupe(u8, info.contact_name) catch return OpenApiError.OutOfMemory,
            contact_url: aa.dupe(u8, info.contact_url) catch return OpenApiError.OutOfMemory,
            contact_email: aa.dupe(u8, info.contact_email) catch return OpenApiError.OutOfMemory,
            license_name: aa.dupe(u8, info.license_name) catch return OpenApiError.OutOfMemory,
            license_identifier: aa.dupe(u8, info.license_identifier) catch return OpenApiError.OutOfMemory,
            license_url: aa.dupe(u8, info.license_url) catch return OpenApiError.OutOfMemory,
        };

        return OpenApiDoc{
            parent_alloc: alloc,
            owned_arena: arena_ptr,
            info: owned_info,
        };
    }

    fn deinit(*_) {
        _.owned_arena.deinit();
        _.parent_alloc.destroy(_.owned_arena);
    }

    fn dupeStr(*_, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        return _.owned_arena.allocator().dupe(u8, s) catch return OpenApiError.OutOfMemory;
    }

    fn setJsonSchemaDialect(*_, dialect: []const u8): ! {
        _.json_schema_dialect = _.dupeStr(dialect)?;
    }

    fn addServer(*_, url: []const u8, description: []const u8): ! {
        const aa = _.owned_arena.allocator();
        _.servers.append(aa, OpenApiServer{
            url: _.dupeStr(url)?,
            description: _.dupeStr(description)?,
        }) catch return OpenApiError.OutOfMemory;
    }

    fn addTag(*_, name: []const u8, description: []const u8): ! {
        const aa = _.owned_arena.allocator();
        _.tags.append(aa, OpenApiTag{
            name: _.dupeStr(name)?,
            description: _.dupeStr(description)?,
        }) catch return OpenApiError.OutOfMemory;
    }

    fn addSecurityScheme(*_, scheme: OpenApiSecurityScheme): ! {
        const aa = _.owned_arena.allocator();
        _.security_schemes.append(aa, OpenApiSecurityScheme{
            name: _.dupeStr(scheme.name)?,
            scheme_type: scheme.scheme_type,
            description: _.dupeStr(scheme.description)?,
            param_name: _.dupeStr(scheme.param_name)?,
            api_key_in: scheme.api_key_in,
            bearer_format: _.dupeStr(scheme.bearer_format)?,
            oauth2_flows_json: _.dupeStr(scheme.oauth2_flows_json)?,
        }) catch return OpenApiError.OutOfMemory;
    }

    fn registerComponentSchema(*_, name: []const u8, json_schema_str: []const u8): ! {
        const aa = _.owned_arena.allocator();
        const owned_name = _.dupeStr(name)?;
        const trimmed = std.mem.trim(u8, json_schema_str, " \t\r\n");
        const owned_schema = _.dupeStr(if (trimmed.len > 0) trimmed else "{}")?;
        // Overwrite if name already exists, else append
        let i: usize = 0;
        while (i < _.component_schemas.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.component_schemas.items[i].name, owned_name)) {
                _.component_schemas.items[i].json_schema = owned_schema;
                return;
            }
        }
        _.component_schemas.append(aa, OpenApiComponentSchema{
            name: owned_name,
            json_schema: owned_schema,
        }) catch return OpenApiError.OutOfMemory;
    }

    /// Validates an OpenAPI path template:
    /// - Must start with '/'
    /// - Must not contain whitespace, '?', or '#'
    /// - All '{param}' placeholders must be non-empty, balanced, non-nested,
    ///   without '/', and unique within the same path template.
    static fn validatePathTemplate(path: []const u8): ! {
        if (path.len == 0 || path[0] != '/') {
            return OpenApiError.InvalidPath;
        }

        let param_names: [32][]const u8 = undefined;
        let param_count: usize = 0;

        let in_brace = false;
        let brace_start: usize = 0;
        let i: usize = 0;
        while (i < path.len) : (i += 1) {
            const c = path[i];
            if (c <= 0x20 || c == '?' || c == '#') {
                return OpenApiError.InvalidPath;
            }
            if (c == '{') {
                if (in_brace) {
                    return OpenApiError.InvalidPath;
                }
                in_brace = true;
                brace_start = i + 1;
            } else if (c == '}') {
                if (!in_brace) {
                    return OpenApiError.InvalidPath;
                }
                const pname = path[brace_start..i];
                if (pname.len == 0) {
                    return OpenApiError.InvalidPath;
                }
                if (param_count < 32) {
                    let k: usize = 0;
                    while (k < param_count) : (k += 1) {
                        if (std.mem.eql(u8, param_names[k], pname)) {
                            return OpenApiError.InvalidPath;
                        }
                    }
                    param_names[param_count] = pname;
                    param_count += 1;
                }
                in_brace = false;
            } else if (c == '/' && in_brace) {
                return OpenApiError.InvalidPath;
            }
        }
        if (in_brace) {
            return OpenApiError.InvalidPath;
        }
    }

    static fn isValidStatusCode(code: []const u8): bool {
        if (std.mem.eql(u8, code, "default")) return true;
        if (code.len != 3) return false;
        if (code[0] < '1' || code[0] > '5') return false;
        if (code[1] == 'X' && code[2] == 'X') return true;
        return (code[1] >= '0' && code[1] <= '9') && (code[2] >= '0' && code[2] <= '9');
    }

    fn cloneOperation(*_, op: OpenApiOperation): OpenApiOperation! {
        const aa = _.owned_arena.allocator();

        let owned_tags = aa.alloc(Str, op.tags.len) catch return OpenApiError.OutOfMemory;
        let ti: usize = 0;
        while (ti < op.tags.len) : (ti += 1) {
            owned_tags[ti] = _.dupeStr(op.tags[ti])?;
        }

        let owned_params = aa.alloc(OpenApiParam, op.params.len) catch return OpenApiError.OutOfMemory;
        let pi: usize = 0;
        while (pi < op.params.len) : (pi += 1) {
            const p = op.params[pi];
            owned_params[pi] = OpenApiParam{
                name: _.dupeStr(p.name)?,
                param_in: p.param_in,
                required: if (p.param_in == .Path) true else p.required,
                description: _.dupeStr(p.description)?,
                schema_type: _.dupeStr(if (p.schema_type.len > 0) p.schema_type else "string")?,
                schema_format: _.dupeStr(p.schema_format)?,
                schema_json: _.dupeStr(p.schema_json)?,
                deprecated: p.deprecated,
            };
        }

        let owned_resps = aa.alloc(OpenApiResponse, op.responses.len) catch return OpenApiError.OutOfMemory;
        let ri: usize = 0;
        while (ri < op.responses.len) : (ri += 1) {
            const r = op.responses[ri];
            owned_resps[ri] = OpenApiResponse{
                status_code: _.dupeStr(r.status_code)?,
                description: _.dupeStr(r.description)?,
                content_type: _.dupeStr(if (r.content_type.len > 0) r.content_type else "application/json")?,
                schema_ref: _.dupeStr(r.schema_ref)?,
                schema_json: _.dupeStr(r.schema_json)?,
            };
        }

        let owned_sec = aa.alloc(Str, op.security.len) catch return OpenApiError.OutOfMemory;
        let si: usize = 0;
        while (si < op.security.len) : (si += 1) {
            owned_sec[si] = _.dupeStr(op.security[si])?;
        }

        return OpenApiOperation{
            operation_id: _.dupeStr(op.operation_id)?,
            summary: _.dupeStr(op.summary)?,
            description: _.dupeStr(op.description)?,
            tags: owned_tags,
            params: owned_params,
            request_body_required: op.request_body_required,
            request_body_content_type: _.dupeStr(if (op.request_body_content_type.len > 0) op.request_body_content_type else "application/json")?,
            request_body_schema_ref: _.dupeStr(op.request_body_schema_ref)?,
            request_body_schema_json: _.dupeStr(op.request_body_schema_json)?,
            request_body_description: _.dupeStr(op.request_body_description)?,
            responses: owned_resps,
            security: owned_sec,
            deprecated: op.deprecated,
        };
    }

    fn addOperation(*_, path: []const u8, method: HttpMethod, op: OpenApiOperation): ! {
        OpenApiDoc.validatePathTemplate(path)?;

        for (resp of op.responses) {
            if (!OpenApiDoc.isValidStatusCode(resp.status_code)) {
                return OpenApiError.InvalidStatusCode;
            }
        }

        if (op.operation_id.len > 0) {
            for (ep of _.endpoints.items) {
                if (ep.operation.operation_id.len > 0 && std.mem.eql(u8, ep.operation.operation_id, op.operation_id)) {
                    return OpenApiError.DuplicateOperationId;
                }
            }
        }

        for (ep of _.endpoints.items) {
            if (ep.method == method && std.mem.eql(u8, ep.path, path)) {
                return OpenApiError.InvalidPath;
            }
        }

        const aa = _.owned_arena.allocator();
        const owned_path = _.dupeStr(path)?;
        const cloned_op = _.cloneOperation(op)?;
        _.endpoints.append(aa, OpenApiEndpoint{
            path: owned_path,
            method: method,
            operation: cloned_op,
        }) catch return OpenApiError.OutOfMemory;
    }

    fn endpointCount(): usize {
        return _.endpoints.items.len;
    }

    fn getEndpoints(): []const OpenApiEndpoint {
        return _.endpoints.items;
    }

    fn findOperationById(op_id: []const u8): OpenApiEndpoint? {
        if (op_id.len == 0) return null;
        for (ep of _.endpoints.items) {
            if (std.mem.eql(u8, ep.operation.operation_id, op_id)) {
                return ep;
            }
        }
        return null;
    }

    // -------------------------------------------------------------------------
    // RFC 8259 JSON Serialization Helpers
    // -------------------------------------------------------------------------

    static fn writeJsonString(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): ! {
        out.append(alloc, '"') catch return OpenApiError.OutOfMemory;
        let i: usize = 0;
        let flush_start: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '"' || c == '\\' || c < 0x20) {
                if (i > flush_start) {
                    out.appendSlice(alloc, s[flush_start..i]) catch return OpenApiError.OutOfMemory;
                }
                switch (c) {
                    '"' => out.appendSlice(alloc, "\\\"") catch return OpenApiError.OutOfMemory,
                    '\\' => out.appendSlice(alloc, "\\\\") catch return OpenApiError.OutOfMemory,
                    0x08 => out.appendSlice(alloc, "\\b") catch return OpenApiError.OutOfMemory,
                    0x0C => out.appendSlice(alloc, "\\f") catch return OpenApiError.OutOfMemory,
                    '\n' => out.appendSlice(alloc, "\\n") catch return OpenApiError.OutOfMemory,
                    '\r' => out.appendSlice(alloc, "\\r") catch return OpenApiError.OutOfMemory,
                    '\t' => out.appendSlice(alloc, "\\t") catch return OpenApiError.OutOfMemory,
                    else => {
                        const hex = "0123456789abcdef";
                        let seq: [6]u8 = ['\\', 'u', '0', '0', hex[(c >> 4) & 0xF], hex[c & 0xF]];
                        out.appendSlice(alloc, seq[0..6]) catch return OpenApiError.OutOfMemory;
                    },
                }
                flush_start = i + 1;
            }
        }
        if (i > flush_start) {
            out.appendSlice(alloc, s[flush_start..i]) catch return OpenApiError.OutOfMemory;
        }
        out.append(alloc, '"') catch return OpenApiError.OutOfMemory;
    }

    static fn writeRaw(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): ! {
        out.appendSlice(alloc, s) catch return OpenApiError.OutOfMemory;
    }

    static fn writeKey(out: *std.ArrayList(u8), alloc: std.mem.Allocator, key: []const u8): ! {
        OpenApiDoc.writeJsonString(out, alloc, key)?;
        OpenApiDoc.writeRaw(out, alloc, ":")?;
    }

    static fn writeSchemaRefOrInline(
        out: *std.ArrayList(u8),
        alloc: std.mem.Allocator,
        schema_ref: []const u8,
        schema_json: []const u8,
    ): ! {
        if (schema_ref.len > 0) {
            OpenApiDoc.writeRaw(out, alloc, "{\"$ref\":")?;
            if (std.mem.startsWith(u8, schema_ref, "#/")) {
                OpenApiDoc.writeJsonString(out, alloc, schema_ref)?;
            } else {
                const full_ref = std.fmt.allocPrint(alloc, "#/components/schemas/{s}", .{schema_ref}) catch return OpenApiError.OutOfMemory;
                defer alloc.free(full_ref);
                OpenApiDoc.writeJsonString(out, alloc, full_ref)?;
            }
            OpenApiDoc.writeRaw(out, alloc, "}")?;
            return;
        }
        const trimmed = std.mem.trim(u8, schema_json, " \t\r\n");
        if (trimmed.len > 0) {
            OpenApiDoc.writeRaw(out, alloc, trimmed)?;
        } else {
            OpenApiDoc.writeRaw(out, alloc, "{}")?;
        }
    }

    static fn writeParam(out: *std.ArrayList(u8), alloc: std.mem.Allocator, p: OpenApiParam): ! {
        OpenApiDoc.writeRaw(out, alloc, "{")?;
        OpenApiDoc.writeKey(out, alloc, "name")?;
        OpenApiDoc.writeJsonString(out, alloc, p.name)?;
        OpenApiDoc.writeRaw(out, alloc, ",")?;
        OpenApiDoc.writeKey(out, alloc, "in")?;
        OpenApiDoc.writeJsonString(out, alloc, p.param_in.asStr())?;
        OpenApiDoc.writeRaw(out, alloc, ",")?;
        OpenApiDoc.writeKey(out, alloc, "required")?;
        const req = if (p.param_in == .Path) true else p.required;
        OpenApiDoc.writeRaw(out, alloc, if (req) "true" else "false")?;

        if (p.description.len > 0) {
            OpenApiDoc.writeRaw(out, alloc, ",")?;
            OpenApiDoc.writeKey(out, alloc, "description")?;
            OpenApiDoc.writeJsonString(out, alloc, p.description)?;
        }
        if (p.deprecated) {
            OpenApiDoc.writeRaw(out, alloc, ",\"deprecated\":true")?;
        }

        OpenApiDoc.writeRaw(out, alloc, ",")?;
        OpenApiDoc.writeKey(out, alloc, "schema")?;
        const trimmed_schema = std.mem.trim(u8, p.schema_json, " \t\r\n");
        if (trimmed_schema.len > 0) {
            OpenApiDoc.writeRaw(out, alloc, trimmed_schema)?;
        } else {
            OpenApiDoc.writeRaw(out, alloc, "{\"type\":")?;
            OpenApiDoc.writeJsonString(out, alloc, if (p.schema_type.len > 0) p.schema_type else "string")?;
            if (p.schema_format.len > 0) {
                OpenApiDoc.writeRaw(out, alloc, ",\"format\":")?;
                OpenApiDoc.writeJsonString(out, alloc, p.schema_format)?;
            }
            OpenApiDoc.writeRaw(out, alloc, "}")?;
        }
        OpenApiDoc.writeRaw(out, alloc, "}")?;
    }

    static fn writeOperation(out: *std.ArrayList(u8), alloc: std.mem.Allocator, op: OpenApiOperation): ! {
        OpenApiDoc.writeRaw(out, alloc, "{")?;
        let need_comma = false;

        if (op.operation_id.len > 0) {
            OpenApiDoc.writeKey(out, alloc, "operationId")?;
            OpenApiDoc.writeJsonString(out, alloc, op.operation_id)?;
            need_comma = true;
        }
        if (op.summary.len > 0) {
            if (need_comma) OpenApiDoc.writeRaw(out, alloc, ",")?;
            OpenApiDoc.writeKey(out, alloc, "summary")?;
            OpenApiDoc.writeJsonString(out, alloc, op.summary)?;
            need_comma = true;
        }
        if (op.description.len > 0) {
            if (need_comma) OpenApiDoc.writeRaw(out, alloc, ",")?;
            OpenApiDoc.writeKey(out, alloc, "description")?;
            OpenApiDoc.writeJsonString(out, alloc, op.description)?;
            need_comma = true;
        }
        if (op.tags.len > 0) {
            if (need_comma) OpenApiDoc.writeRaw(out, alloc, ",")?;
            OpenApiDoc.writeKey(out, alloc, "tags")?;
            OpenApiDoc.writeRaw(out, alloc, "[")?;
            let ti: usize = 0;
            while (ti < op.tags.len) : (ti += 1) {
                if (ti > 0) OpenApiDoc.writeRaw(out, alloc, ",")?;
                OpenApiDoc.writeJsonString(out, alloc, op.tags[ti])?;
            }
            OpenApiDoc.writeRaw(out, alloc, "]")?;
            need_comma = true;
        }
        if (op.deprecated) {
            if (need_comma) OpenApiDoc.writeRaw(out, alloc, ",")?;
            OpenApiDoc.writeRaw(out, alloc, "\"deprecated\":true")?;
            need_comma = true;
        }
        if (op.params.len > 0) {
            if (need_comma) OpenApiDoc.writeRaw(out, alloc, ",")?;
            OpenApiDoc.writeKey(out, alloc, "parameters")?;
            OpenApiDoc.writeRaw(out, alloc, "[")?;
            let pi: usize = 0;
            while (pi < op.params.len) : (pi += 1) {
                if (pi > 0) OpenApiDoc.writeRaw(out, alloc, ",")?;
                OpenApiDoc.writeParam(out, alloc, op.params[pi])?;
            }
            OpenApiDoc.writeRaw(out, alloc, "]")?;
            need_comma = true;
        }

        const has_body_spec = op.request_body_required ||
            op.request_body_schema_ref.len > 0 ||
            op.request_body_schema_json.len > 0 ||
            op.request_body_description.len > 0;
        if (has_body_spec) {
            if (need_comma) OpenApiDoc.writeRaw(out, alloc, ",")?;
            OpenApiDoc.writeKey(out, alloc, "requestBody")?;
            OpenApiDoc.writeRaw(out, alloc, "{\"required\":")?;
            OpenApiDoc.writeRaw(out, alloc, if (op.request_body_required) "true" else "false")?;
            if (op.request_body_description.len > 0) {
                OpenApiDoc.writeRaw(out, alloc, ",")?;
                OpenApiDoc.writeKey(out, alloc, "description")?;
                OpenApiDoc.writeJsonString(out, alloc, op.request_body_description)?;
            }
            OpenApiDoc.writeRaw(out, alloc, ",\"content\":{")?;
            const ct = if (op.request_body_content_type.len > 0) op.request_body_content_type else "application/json";
            OpenApiDoc.writeJsonString(out, alloc, ct)?;
            OpenApiDoc.writeRaw(out, alloc, ":{\"schema\":")?;
            OpenApiDoc.writeSchemaRefOrInline(out, alloc, op.request_body_schema_ref, op.request_body_schema_json)?;
            OpenApiDoc.writeRaw(out, alloc, "}}}")?;
            need_comma = true;
        }

        // responses (required in OpenAPI 3.1 Operation Object)
        if (need_comma) OpenApiDoc.writeRaw(out, alloc, ",")?;
        OpenApiDoc.writeKey(out, alloc, "responses")?;
        OpenApiDoc.writeRaw(out, alloc, "{")?;
        if (op.responses.len == 0) {
            OpenApiDoc.writeRaw(out, alloc, "\"200\":{\"description\":\"OK\"}")?;
        } else {
            let ri: usize = 0;
            while (ri < op.responses.len) : (ri += 1) {
                if (ri > 0) OpenApiDoc.writeRaw(out, alloc, ",")?;
                const resp = op.responses[ri];
                OpenApiDoc.writeJsonString(out, alloc, resp.status_code)?;
                OpenApiDoc.writeRaw(out, alloc, ":{\"description\":")?;
                OpenApiDoc.writeJsonString(out, alloc, resp.description)?;
                if (resp.schema_ref.len > 0 || resp.schema_json.len > 0) {
                    OpenApiDoc.writeRaw(out, alloc, ",\"content\":{")?;
                    const rct = if (resp.content_type.len > 0) resp.content_type else "application/json";
                    OpenApiDoc.writeJsonString(out, alloc, rct)?;
                    OpenApiDoc.writeRaw(out, alloc, ":{\"schema\":")?;
                    OpenApiDoc.writeSchemaRefOrInline(out, alloc, resp.schema_ref, resp.schema_json)?;
                    OpenApiDoc.writeRaw(out, alloc, "}}")?;
                }
                OpenApiDoc.writeRaw(out, alloc, "}")?;
            }
        }
        OpenApiDoc.writeRaw(out, alloc, "}")?;

        if (op.security.len > 0) {
            OpenApiDoc.writeRaw(out, alloc, ",")?;
            OpenApiDoc.writeKey(out, alloc, "security")?;
            OpenApiDoc.writeRaw(out, alloc, "[")?;
            let si: usize = 0;
            while (si < op.security.len) : (si += 1) {
                if (si > 0) OpenApiDoc.writeRaw(out, alloc, ",")?;
                OpenApiDoc.writeRaw(out, alloc, "{")?;
                OpenApiDoc.writeJsonString(out, alloc, op.security[si])?;
                OpenApiDoc.writeRaw(out, alloc, ":[]}")?;
            }
            OpenApiDoc.writeRaw(out, alloc, "]")?;
        }

        OpenApiDoc.writeRaw(out, alloc, "}")?;
    }

    fn toJson(alloc: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        OpenApiDoc.writeRaw(&out, alloc, "{\"openapi\":")?;
        OpenApiDoc.writeJsonString(&out, alloc, _.openapi_version)?;

        if (_.json_schema_dialect.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",")?;
            OpenApiDoc.writeKey(&out, alloc, "jsonSchemaDialect")?;
            OpenApiDoc.writeJsonString(&out, alloc, _.json_schema_dialect)?;
        }

        // info
        OpenApiDoc.writeRaw(&out, alloc, ",\"info\":{\"title\":")?;
        OpenApiDoc.writeJsonString(&out, alloc, _.info.title)?;
        OpenApiDoc.writeRaw(&out, alloc, ",\"version\":")?;
        OpenApiDoc.writeJsonString(&out, alloc, _.info.version)?;
        if (_.info.summary.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",\"summary\":")?;
            OpenApiDoc.writeJsonString(&out, alloc, _.info.summary)?;
        }
        if (_.info.description.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",\"description\":")?;
            OpenApiDoc.writeJsonString(&out, alloc, _.info.description)?;
        }
        if (_.info.terms_of_service.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",\"termsOfService\":")?;
            OpenApiDoc.writeJsonString(&out, alloc, _.info.terms_of_service)?;
        }
        if (_.info.contact_name.len > 0 || _.info.contact_email.len > 0 || _.info.contact_url.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",\"contact\":{")?;
            let c_comma = false;
            if (_.info.contact_name.len > 0) {
                OpenApiDoc.writeKey(&out, alloc, "name")?;
                OpenApiDoc.writeJsonString(&out, alloc, _.info.contact_name)?;
                c_comma = true;
            }
            if (_.info.contact_email.len > 0) {
                if (c_comma) OpenApiDoc.writeRaw(&out, alloc, ",")?;
                OpenApiDoc.writeKey(&out, alloc, "email")?;
                OpenApiDoc.writeJsonString(&out, alloc, _.info.contact_email)?;
                c_comma = true;
            }
            if (_.info.contact_url.len > 0) {
                if (c_comma) OpenApiDoc.writeRaw(&out, alloc, ",")?;
                OpenApiDoc.writeKey(&out, alloc, "url")?;
                OpenApiDoc.writeJsonString(&out, alloc, _.info.contact_url)?;
            }
            OpenApiDoc.writeRaw(&out, alloc, "}")?;
        }
        if (_.info.license_name.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",\"license\":{\"name\":")?;
            OpenApiDoc.writeJsonString(&out, alloc, _.info.license_name)?;
            if (_.info.license_identifier.len > 0) {
                OpenApiDoc.writeRaw(&out, alloc, ",\"identifier\":")?;
                OpenApiDoc.writeJsonString(&out, alloc, _.info.license_identifier)?;
            }
            if (_.info.license_url.len > 0) {
                OpenApiDoc.writeRaw(&out, alloc, ",\"url\":")?;
                OpenApiDoc.writeJsonString(&out, alloc, _.info.license_url)?;
            }
            OpenApiDoc.writeRaw(&out, alloc, "}")?;
        }
        OpenApiDoc.writeRaw(&out, alloc, "}")?;

        // servers
        if (_.servers.items.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",\"servers\":[")?;
            let i: usize = 0;
            while (i < _.servers.items.len) : (i += 1) {
                if (i > 0) OpenApiDoc.writeRaw(&out, alloc, ",")?;
                const srv = _.servers.items[i];
                OpenApiDoc.writeRaw(&out, alloc, "{\"url\":")?;
                OpenApiDoc.writeJsonString(&out, alloc, srv.url)?;
                if (srv.description.len > 0) {
                    OpenApiDoc.writeRaw(&out, alloc, ",\"description\":")?;
                    OpenApiDoc.writeJsonString(&out, alloc, srv.description)?;
                }
                OpenApiDoc.writeRaw(&out, alloc, "}")?;
            }
            OpenApiDoc.writeRaw(&out, alloc, "]")?;
        }

        // tags
        if (_.tags.items.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",\"tags\":[")?;
            let i: usize = 0;
            while (i < _.tags.items.len) : (i += 1) {
                if (i > 0) OpenApiDoc.writeRaw(&out, alloc, ",")?;
                const tag = _.tags.items[i];
                OpenApiDoc.writeRaw(&out, alloc, "{\"name\":")?;
                OpenApiDoc.writeJsonString(&out, alloc, tag.name)?;
                if (tag.description.len > 0) {
                    OpenApiDoc.writeRaw(&out, alloc, ",\"description\":")?;
                    OpenApiDoc.writeJsonString(&out, alloc, tag.description)?;
                }
                OpenApiDoc.writeRaw(&out, alloc, "}")?;
            }
            OpenApiDoc.writeRaw(&out, alloc, "]")?;
        }

        // paths (group endpoints by unique path in insertion order)
        OpenApiDoc.writeRaw(&out, alloc, ",\"paths\":{")?;
        let unique_paths: std.ArrayList(Str) = .empty;
        defer unique_paths.deinit(alloc);

        for (ep of _.endpoints.items) {
            let seen = false;
            for (up of unique_paths.items) {
                if (std.mem.eql(u8, up, ep.path)) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                unique_paths.append(alloc, ep.path) catch return OpenApiError.OutOfMemory;
            }
        }

        let path_idx: usize = 0;
        while (path_idx < unique_paths.items.len) : (path_idx += 1) {
            if (path_idx > 0) OpenApiDoc.writeRaw(&out, alloc, ",")?;
            const p = unique_paths.items[path_idx];
            OpenApiDoc.writeJsonString(&out, alloc, p)?;
            OpenApiDoc.writeRaw(&out, alloc, ":{")?;
            let first_method = true;
            for (ep of _.endpoints.items) {
                if (std.mem.eql(u8, ep.path, p)) {
                    if (!first_method) OpenApiDoc.writeRaw(&out, alloc, ",")?;
                    first_method = false;
                    OpenApiDoc.writeJsonString(&out, alloc, ep.method.asStr())?;
                    OpenApiDoc.writeRaw(&out, alloc, ":")?;
                    OpenApiDoc.writeOperation(&out, alloc, ep.operation)?;
                }
            }
            OpenApiDoc.writeRaw(&out, alloc, "}")?;
        }
        OpenApiDoc.writeRaw(&out, alloc, "}")?;

        // components
        if (_.component_schemas.items.len > 0 || _.security_schemes.items.len > 0) {
            OpenApiDoc.writeRaw(&out, alloc, ",\"components\":{")?;
            let comp_comma = false;

            if (_.component_schemas.items.len > 0) {
                OpenApiDoc.writeRaw(&out, alloc, "\"schemas\":{")?;
                let i: usize = 0;
                while (i < _.component_schemas.items.len) : (i += 1) {
                    if (i > 0) OpenApiDoc.writeRaw(&out, alloc, ",")?;
                    const cs = _.component_schemas.items[i];
                    OpenApiDoc.writeJsonString(&out, alloc, cs.name)?;
                    OpenApiDoc.writeRaw(&out, alloc, ":")?;
                    OpenApiDoc.writeRaw(&out, alloc, cs.json_schema)?;
                }
                OpenApiDoc.writeRaw(&out, alloc, "}")?;
                comp_comma = true;
            }

            if (_.security_schemes.items.len > 0) {
                if (comp_comma) OpenApiDoc.writeRaw(&out, alloc, ",")?;
                OpenApiDoc.writeRaw(&out, alloc, "\"securitySchemes\":{")?;
                let i: usize = 0;
                while (i < _.security_schemes.items.len) : (i += 1) {
                    if (i > 0) OpenApiDoc.writeRaw(&out, alloc, ",")?;
                    const ss = _.security_schemes.items[i];
                    OpenApiDoc.writeJsonString(&out, alloc, ss.name)?;
                    OpenApiDoc.writeRaw(&out, alloc, ":{\"type\":")?;
                    OpenApiDoc.writeJsonString(&out, alloc, ss.scheme_type.asStr())?;
                    if (ss.description.len > 0) {
                        OpenApiDoc.writeRaw(&out, alloc, ",\"description\":")?;
                        OpenApiDoc.writeJsonString(&out, alloc, ss.description)?;
                    }
                    switch (ss.scheme_type) {
                        .ApiKey => {
                            OpenApiDoc.writeRaw(&out, alloc, ",\"name\":")?;
                            OpenApiDoc.writeJsonString(&out, alloc, if (ss.param_name.len > 0) ss.param_name else ss.name)?;
                            OpenApiDoc.writeRaw(&out, alloc, ",\"in\":")?;
                            OpenApiDoc.writeJsonString(&out, alloc, ss.api_key_in.asStr())?;
                        },
                        .HttpBearer => {
                            OpenApiDoc.writeRaw(&out, alloc, ",\"scheme\":\"bearer\"")?;
                            if (ss.bearer_format.len > 0) {
                                OpenApiDoc.writeRaw(&out, alloc, ",\"bearerFormat\":")?;
                                OpenApiDoc.writeJsonString(&out, alloc, ss.bearer_format)?;
                            }
                        },
                        .HttpBasic => {
                            OpenApiDoc.writeRaw(&out, alloc, ",\"scheme\":\"basic\"")?;
                        },
                        .OAuth2 => {
                            OpenApiDoc.writeRaw(&out, alloc, ",\"flows\":")?;
                            const trimmed_flows = std.mem.trim(u8, ss.oauth2_flows_json, " \t\r\n");
                            OpenApiDoc.writeRaw(&out, alloc, if (trimmed_flows.len > 0) trimmed_flows else "{}")?;
                        },
                    }
                    OpenApiDoc.writeRaw(&out, alloc, "}")?;
                }
                OpenApiDoc.writeRaw(&out, alloc, "}")?;
            }

            OpenApiDoc.writeRaw(&out, alloc, "}")?;
        }

        OpenApiDoc.writeRaw(&out, alloc, "}")?;
        return out.toOwnedSlice(alloc) catch return OpenApiError.OutOfMemory;
    }
}
