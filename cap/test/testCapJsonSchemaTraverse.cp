// cap/test/testCapJsonSchemaTraverse.cp
// Comprehensive unit tests for @cap/json-schema-traverse

import { test, expect_eq } from "cap:test";
import {
    SchemaNodeKind,
    SchemaVisitEvent,
    traverseJsonSchema,
    countSchemaNodes,
    findSubschemaByPointer,
} from "../../packages/@cap/json-schema-traverse/src/index.cp";

fn main() {
    test("1. @cap/json-schema-traverse — root and nested properties traversal", () => {
        const schema = "{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\"},\"age\":{\"type\":\"integer\"}}}";
        let events: [8]SchemaVisitEvent = undefined;
        const n = traverseJsonSchema(schema, events[0..8]);
        expect_eq(n, 3 as usize);

        expect_eq(events[0].kind == .Root, true);
        expect_eq(events[0].depth, 0 as usize);
        expect_eq(std.mem.eql(u8, events[0].pointer(), ""), true);

        expect_eq(events[1].kind == .Property, true);
        expect_eq(events[1].depth, 1 as usize);
        expect_eq(std.mem.eql(u8, events[1].pointer(), "/properties/name"), true);
        expect_eq(std.mem.eql(u8, events[1].schemaSlice(schema), "{\"type\":\"string\"}"), true);

        expect_eq(events[2].kind == .Property, true);
        expect_eq(events[2].depth, 1 as usize);
        expect_eq(std.mem.eql(u8, events[2].pointer(), "/properties/age"), true);
    });

    test("2. @cap/json-schema-traverse — items (object & tuple array) and additionalProperties", () => {
        const schema = "{\"type\":\"object\",\"properties\":{\"tags\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}},\"pair\":{\"items\":[{\"type\":\"number\"},{\"type\":\"boolean\"}]}},\"additionalProperties\":{\"type\":\"null\"}}";
        let events: [16]SchemaVisitEvent = undefined;
        const n = traverseJsonSchema(schema, events[0..16]);
        expect_eq(n, 7 as usize);

        expect_eq(std.mem.eql(u8, events[2].pointer(), "/properties/tags/items"), true);
        expect_eq(events[2].kind == .Items, true);
        expect_eq(events[2].depth, 2 as usize);

        expect_eq(std.mem.eql(u8, events[4].pointer(), "/properties/pair/items/0"), true);
        expect_eq(events[4].kind == .Items, true);
        expect_eq(std.mem.eql(u8, events[5].pointer(), "/properties/pair/items/1"), true);

        expect_eq(std.mem.eql(u8, events[6].pointer(), "/additionalProperties"), true);
        expect_eq(events[6].kind == .AdditionalProperties, true);
    });

    test("3. @cap/json-schema-traverse — combinators allOf, anyOf, oneOf, not", () => {
        const schema = "{\"allOf\":[{\"type\":\"object\"},{\"required\":[\"id\"]}],\"anyOf\":[{\"type\":\"string\"}],\"oneOf\":[{\"type\":\"integer\"}],\"not\":{\"type\":\"null\"}}";
        let events: [16]SchemaVisitEvent = undefined;
        const n = traverseJsonSchema(schema, events[0..16]);
        expect_eq(n, 6 as usize);

        expect_eq(events[1].kind == .AllOf, true);
        expect_eq(std.mem.eql(u8, events[1].pointer(), "/allOf/0"), true);
        expect_eq(events[2].kind == .AllOf, true);
        expect_eq(std.mem.eql(u8, events[2].pointer(), "/allOf/1"), true);
        expect_eq(events[3].kind == .AnyOf, true);
        expect_eq(std.mem.eql(u8, events[3].pointer(), "/anyOf/0"), true);
        expect_eq(events[4].kind == .OneOf, true);
        expect_eq(std.mem.eql(u8, events[4].pointer(), "/oneOf/0"), true);
        expect_eq(events[5].kind == .Not, true);
        expect_eq(std.mem.eql(u8, events[5].pointer(), "/not"), true);
    });

    test("4. @cap/json-schema-traverse — $defs, definitions, patternProperties & RFC 6901 escaping", () => {
        const schema = "{\"$defs\":{\"Addr\":{\"type\":\"object\",\"properties\":{\"city\":{\"type\":\"string\"}}}},\"definitions\":{\"Legacy\":{\"type\":\"number\"}},\"patternProperties\":{\"^a/b~c$\":{\"type\":\"boolean\"}}}";
        let events: [16]SchemaVisitEvent = undefined;
        const n = traverseJsonSchema(schema, events[0..16]);
        expect_eq(n, 5 as usize);

        expect_eq(events[1].kind == .Defs, true);
        expect_eq(std.mem.eql(u8, events[1].pointer(), "/$defs/Addr"), true);
        expect_eq(std.mem.eql(u8, events[2].pointer(), "/$defs/Addr/properties/city"), true);
        expect_eq(events[3].kind == .Defs, true);
        expect_eq(std.mem.eql(u8, events[3].pointer(), "/definitions/Legacy"), true);
        expect_eq(events[4].kind == .Property, true);
        expect_eq(std.mem.eql(u8, events[4].pointer(), "/patternProperties/^a~1b~0c$"), true);
    });

    test("5. @cap/json-schema-traverse — countSchemaNodes & findSubschemaByPointer", () => {
        const schema = "{\"type\":\"object\",\"properties\":{\"user\":{\"type\":\"object\",\"properties\":{\"email\":{\"type\":\"string\"}}}},\"allOf\":[{\"minProperties\":1}]}";
        expect_eq(countSchemaNodes(schema), 4 as usize);

        const root_opt = findSubschemaByPointer(schema, "");
        expect_eq(root_opt != null, true);

        const email_opt = findSubschemaByPointer(schema, "/properties/user/properties/email");
        expect_eq(email_opt != null, true);
        if (sub := email_opt) {
            expect_eq(std.mem.eql(u8, sub, "{\"type\":\"string\"}"), true);
        }

        const allof_opt = findSubschemaByPointer(schema, "/allOf/0");
        expect_eq(allof_opt != null, true);
        if (sub := allof_opt) {
            expect_eq(std.mem.eql(u8, sub, "{\"minProperties\":1}"), true);
        }

        expect_eq(findSubschemaByPointer(schema, "/properties/missing") == null, true);
    });

    test("6. @cap/json-schema-traverse — buffer truncation & invalid/non-object JSON handling", () => {
        const schema = "{\"properties\":{\"a\":{\"type\":\"string\"},\"b\":{\"type\":\"number\"},\"c\":{\"type\":\"boolean\"}}}";
        let small: [2]SchemaVisitEvent = undefined;
        const written = traverseJsonSchema(schema, small[0..2]);
        expect_eq(written, 2 as usize);
        expect_eq(countSchemaNodes(schema), 4 as usize);

        expect_eq(countSchemaNodes(""), 0 as usize);
        expect_eq(countSchemaNodes("[1,2,3]"), 0 as usize);
        expect_eq(findSubschemaByPointer("not json", "") == null, true);
    });
}
