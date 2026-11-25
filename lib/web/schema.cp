// Compatibility re-export shim:
//   - 16-lane SIMD URI decoder lives in `cap:url/fastDecode` (`lib/url/fastDecode.cp`)
//   - JSON Schema validator / exact-mirror / OpenAPI emitter lives in `cap:json/schema` (`lib/json/schema.cp`)
export {
    fastDecodeUriComponentOpts,
    fastDecodeUriComponent,
    fastDecodeFormComponent,
    QueryPair,
    parseQueryPair,
    findQueryParamDecoded,
} from "../url/fastDecode.cp";

export {
    SCHEMA_MAX_FIELDS,
    SCHEMA_MAX_ISSUES,
    SCHEMA_MAX_JSON_DEPTH,
    FieldKind,
    FieldRule,
    ObjectSchema,
    ValidationIssue,
    ValidationReport,
    validateJsonObject,
    exactMirrorJson,
    schemaToOpenApiJson,
} from "../json/schema.cp";
