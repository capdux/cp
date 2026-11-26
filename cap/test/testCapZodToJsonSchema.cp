import { test, expect_eq } from "cap:test";
import {
    SchemaType,
    SchemaProp,
    schemaTypeName,
    prop,
    stringProp,
    intProp,
    arrayProp,
    toJsonSchema,
} from "../../packages/@cap/zod-to-json-schema/src/index.cp";

fn main() {
    test("1. @cap/zod-to-json-schema — emits Draft-07 object schema with required fields", () => {
        const fields: [2]SchemaProp = [
            stringProp("email", "User email", "email", true),
            prop("active", .Boolean, "", false),
        ];
        let buf: [512]u8 = [0; 512];
        const n = toJsonSchema("User", fields[0..2], false, buf[0..512]);
        expect_eq(
            buf[0..n] as string,
            "{\"$schema\":\"http://json-schema.org/draft-07/schema#\",\"title\":\"User\",\"type\":\"object\",\"properties\":{\"email\":{\"type\":\"string\",\"description\":\"User email\",\"format\":\"email\"},\"active\":{\"type\":\"boolean\"}},\"required\":[\"email\"],\"additionalProperties\":false}",
        );
    });

    test("2. @cap/zod-to-json-schema — emits integer min/max constraints", () => {
        const fields: [1]SchemaProp = [
            intProp("port", "TCP port", true, 1, 65535),
        ];
        let buf: [512]u8 = [0; 512];
        const n = toJsonSchema("", fields[0..1], true, buf[0..512]);
        expect_eq(
            buf[0..n] as string,
            "{\"$schema\":\"http://json-schema.org/draft-07/schema#\",\"type\":\"object\",\"properties\":{\"port\":{\"type\":\"integer\",\"description\":\"TCP port\",\"minimum\":1,\"maximum\":65535}},\"required\":[\"port\"]}",
        );
    });

    test("3. @cap/zod-to-json-schema — emits typed array items schema", () => {
        const fields: [1]SchemaProp = [
            arrayProp("tags", .String, "Labels", false),
        ];
        let buf: [512]u8 = [0; 512];
        const n = toJsonSchema("", fields[0..1], true, buf[0..512]);
        expect_eq(
            buf[0..n] as string,
            "{\"$schema\":\"http://json-schema.org/draft-07/schema#\",\"type\":\"object\",\"properties\":{\"tags\":{\"type\":\"array\",\"description\":\"Labels\",\"items\":{\"type\":\"string\"}}}}",
        );
    });

    test("4. @cap/zod-to-json-schema — escapes quotes and newlines in descriptions", () => {
        const fields: [1]SchemaProp = [
            prop("msg", .String, "say \"hi\"\nnow", false),
        ];
        let buf: [512]u8 = [0; 512];
        const n = toJsonSchema("", fields[0..1], true, buf[0..512]);
        expect_eq(
            buf[0..n] as string,
            "{\"$schema\":\"http://json-schema.org/draft-07/schema#\",\"type\":\"object\",\"properties\":{\"msg\":{\"type\":\"string\",\"description\":\"say \\\"hi\\\"\\nnow\"}}}",
        );
    });

    test("5. @cap/zod-to-json-schema — handles negative integer bounds", () => {
        const fields: [1]SchemaProp = [
            intProp("temp", "", true, -40, 125),
        ];
        let buf: [512]u8 = [0; 512];
        const n = toJsonSchema("", fields[0..1], true, buf[0..512]);
        expect_eq(
            buf[0..n] as string,
            "{\"$schema\":\"http://json-schema.org/draft-07/schema#\",\"type\":\"object\",\"properties\":{\"temp\":{\"type\":\"integer\",\"minimum\":-40,\"maximum\":125}},\"required\":[\"temp\"]}",
        );
    });

    test("6. @cap/zod-to-json-schema — schemaTypeName maps all 7 JSON Schema types", () => {
        expect_eq(schemaTypeName(.String), "string");
        expect_eq(schemaTypeName(.Number), "number");
        expect_eq(schemaTypeName(.Integer), "integer");
        expect_eq(schemaTypeName(.Boolean), "boolean");
        expect_eq(schemaTypeName(.Array), "array");
        expect_eq(schemaTypeName(.Object), "object");
        expect_eq(schemaTypeName(.Null), "null");
    });
}
