// cap/test/testCapIsPlainObj.cp
// Comprehensive unit tests for @cap/is-plain-obj

import { test, expect_eq } from "cap:test";
import {
    PROTO_NULL,
    PROTO_OBJECT,
    PROTO_CUSTOM_CLASS,
    PROTO_ARRAY,
    PROTO_PRIMITIVE,
    PROTO_FUNCTION,
    ValueMeta,
    isPlainObjectMeta,
    isPlainJsonObject,
    isPlainObjectKey,
    hasDangerousProtoKey,
} from "../../packages/@cap/is-plain-obj/src/index.cp";

fn main() {
    test("1. @cap/is-plain-obj — ValueMeta plain & null-prototype objects accepted", () => {
        expect_eq(PROTO_NULL, 0 as u8);
        expect_eq(PROTO_OBJECT, 1 as u8);
        expect_eq(PROTO_CUSTOM_CLASS, 2 as u8);
        expect_eq(PROTO_ARRAY, 3 as u8);
        expect_eq(PROTO_PRIMITIVE, 4 as u8);
        expect_eq(PROTO_FUNCTION, 5 as u8);

        expect_eq(isPlainObjectMeta(ValueMeta.plain()), true);
        expect_eq(isPlainObjectMeta(ValueMeta.nullProto()), true);
    });

    test("2. @cap/is-plain-obj — ValueMeta rejects non-plain prototypes, null, toStringTag & iterators", () => {
        let custom = ValueMeta {
            proto_kind: PROTO_CUSTOM_CLASS,
            has_to_string_tag: false,
            is_iterator: false,
            is_null: false,
        };
        let arr = ValueMeta {
            proto_kind: PROTO_ARRAY,
            has_to_string_tag: false,
            is_iterator: false,
            is_null: false,
        };
        let prim = ValueMeta {
            proto_kind: PROTO_PRIMITIVE,
            has_to_string_tag: false,
            is_iterator: false,
            is_null: false,
        };
        let func = ValueMeta {
            proto_kind: PROTO_FUNCTION,
            has_to_string_tag: false,
            is_iterator: false,
            is_null: false,
        };
        let null_val = ValueMeta {
            proto_kind: PROTO_OBJECT,
            has_to_string_tag: false,
            is_iterator: false,
            is_null: true,
        };
        let tagged = ValueMeta {
            proto_kind: PROTO_OBJECT,
            has_to_string_tag: true,
            is_iterator: false,
            is_null: false,
        };
        let iter = ValueMeta {
            proto_kind: PROTO_NULL,
            has_to_string_tag: false,
            is_iterator: true,
            is_null: false,
        };

        expect_eq(isPlainObjectMeta(custom), false);
        expect_eq(isPlainObjectMeta(arr), false);
        expect_eq(isPlainObjectMeta(prim), false);
        expect_eq(isPlainObjectMeta(func), false);
        expect_eq(isPlainObjectMeta(null_val), false);
        expect_eq(isPlainObjectMeta(tagged), false);
        expect_eq(isPlainObjectMeta(iter), false);
    });

    test("3. @cap/is-plain-obj — isPlainJsonObject accepts valid object literals", () => {
        expect_eq(isPlainJsonObject("{}"), true);
        expect_eq(isPlainJsonObject("  {  } \n\t "), true);
        expect_eq(isPlainJsonObject("{\"a\": 1, \"b\": [1, 2, 3]}"), true);
        expect_eq(isPlainJsonObject("{\"nested\": {\"brace\": \"}\", \"quote\": \"\\\"ok\\\"\"}}"), true);
    });

    test("4. @cap/is-plain-obj — isPlainJsonObject rejects arrays, primitives & malformed objects", () => {
        expect_eq(isPlainJsonObject("[]"), false);
        expect_eq(isPlainJsonObject("[1, 2, 3]"), false);
        expect_eq(isPlainJsonObject("\"hello\""), false);
        expect_eq(isPlainJsonObject("null"), false);
        expect_eq(isPlainJsonObject("true"), false);
        expect_eq(isPlainJsonObject("42"), false);
        expect_eq(isPlainJsonObject(""), false);
        expect_eq(isPlainJsonObject("   "), false);
        expect_eq(isPlainJsonObject("{\"a\": 1"), false);
        expect_eq(isPlainJsonObject("{}{}"), false);
        expect_eq(isPlainJsonObject("{123: 456}"), false);
        expect_eq(isPlainJsonObject("{\"a\": [1, 2}"), false);
    });

    test("5. @cap/is-plain-obj — isPlainObjectKey blocks prototype-pollution keys", () => {
        expect_eq(isPlainObjectKey("__proto__"), false);
        expect_eq(isPlainObjectKey("constructor"), false);
        expect_eq(isPlainObjectKey("prototype"), false);

        expect_eq(isPlainObjectKey("name"), true);
        expect_eq(isPlainObjectKey("proto"), true);
        expect_eq(isPlainObjectKey("toString"), true);
        expect_eq(isPlainObjectKey(""), true);
    });

    test("6. @cap/is-plain-obj — hasDangerousProtoKey scans keys without false positives on values", () => {
        expect_eq(hasDangerousProtoKey("{\"__proto__\": {\"admin\": true}}"), true);
        expect_eq(hasDangerousProtoKey("{\"a\": {\"constructor\" : {\"prototype\": {}}}}"), true);
        expect_eq(hasDangerousProtoKey("{\"nested\": [{\"prototype\": 1}]}"), true);

        // Safe objects and string values containing dangerous words are not flagged
        expect_eq(hasDangerousProtoKey("{\"name\": \"alice\", \"age\": 30}"), false);
        expect_eq(hasDangerousProtoKey("{\"msg\": \"__proto__\", \"kind\": \"constructor\", \"ref\": \"prototype\"}"), false);
        expect_eq(hasDangerousProtoKey("{}"), false);
    });
}
