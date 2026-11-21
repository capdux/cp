import {
    WordIter,
    toSnakeCase,
    toScreamingSnakeCase,
    toKebabCase,
    toCamelCase,
    toPascalCase,
} from "cap:text";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("acronym-aware case conversions across all 5 styles", () => {
    let buf: [64]u8 = [0; 64];
    expect(strEq(toSnakeCase("HTTPServerConfig", buf[0..]).?, "http_server_config"));
    expect(strEq(toScreamingSnakeCase("maxRetryCount", buf[0..]).?, "MAX_RETRY_COUNT"));
    expect(strEq(toKebabCase("getJSONResponse", buf[0..]).?, "get-json-response"));
    expect(strEq(toCamelCase("SCREAMING_SNAKE_CASE", buf[0..]).?, "screamingSnakeCase"));
    expect(strEq(toPascalCase("http-status_code2", buf[0..]).?, "HttpStatusCode2"));

    let tiny: [3]u8 = [0; 3];
    expect(toSnakeCase("helloWorld", tiny[0..]) == null);
});

test("WordIter handles separators, digits, and empty strings", () => {
    let it = WordIter.init("__v2API_client-ID__");
    expect(strEq(it.next().?, "v2"));
    expect(strEq(it.next().?, "API"));
    expect(strEq(it.next().?, "client"));
    expect(strEq(it.next().?, "ID"));
    expect(it.next() == null);
});
