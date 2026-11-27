// Migrated from TypeScript (library/unit/json-schema.spec.ts) via cap from -ts

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import { iso } from "../../../packages/playwright-core/lib/coreBundle.cp";
import { iso as _iso } from "../../../packages/playwright-core/lib/coreBundle.cp";

const { validate } = _iso;
type JsonSchema = iso.JsonSchema;









fn main() {
    test("JS-001 should validate string type", () => {
      expect_eq(validate("hello", { type: "string" }, "$"), []);
      expect_eq(validate(123, { type: "string" }, "$"), ["$: expected string, got number"]);
    });
    test("JS-002 should validate string pattern", () => {
      expect_eq(validate("@tag", { type: "string", pattern: "^@" }, "$"), []);
      expect_eq(validate("tag", { type: "string", pattern: "^@" }, "$"), ["$: must match pattern \"^@\""]);
    });
    test("JS-003 should validate array type", () => {
      expect_eq(validate([], { type: "array" }, "$"), []);
      expect_eq(validate("not-array", { type: "array" }, "$"), ["$: expected array, got string"]);
    });
    test("JS-004 should validate array items", () => {
      const schema: JsonSchema = { type: "array", items: { type: "string" } };
      expect_eq(validate(["a", "b"], schema, "$"), []);
      expect_eq(validate(["a", 1], schema, "$"), ["$[1]: expected string, got number"]);
    });
    test("JS-005 should validate object type", () => {
      expect_eq(validate({}, { type: "object" }, "$"), []);
      expect_eq(validate(null, { type: "object" }, "$"), ["$: expected object, got object"]);
      expect_eq(validate([], { type: "object" }, "$"), ["$: expected object, got array"]);
      expect_eq(validate("str", { type: "object" }, "$"), ["$: expected object, got string"]);
    });
    test("JS-006 should validate required properties", () => {
      const schema: JsonSchema = { type: "object", properties: { name: { type: "string" } }, required: ["name"] };
      expect_eq(validate({ name: "test" }, schema, "$"), []);
      expect_eq(validate({}, schema, "$"), ["$.name: required"]);
    });
    test("JS-007 should validate optional properties", () => {
      const schema: JsonSchema = { type: "object", properties: { name: { type: "string" } } };
      expect_eq(validate({}, schema, "$"), []);
      expect_eq(validate({ name: "test" }, schema, "$"), []);
      expect_eq(validate({ name: 123 }, schema, "$"), ["$.name: expected string, got number"]);
    });
    test("JS-008 should validate oneOf", () => {
      const schema: JsonSchema = { oneOf: [{ type: "string" }, { type: "array", items: { type: "string" } }] };
      expect_eq(validate("hello", schema, "$"), []);
      expect_eq(validate(["a", "b"], schema, "$"), []);
      expect_eq(validate(123, schema, "$"), ["$: does not match any of the expected types"]);
    });
    test("JS-009 should validate nested objects", () => {
      const schema: JsonSchema = {
        type: "object",
        properties: {
          annotation: {
            type: "object",
            properties: { type: { type: "string" } },
            required: ["type"],
          }
        }
      };
      expect_eq(validate({ annotation: { type: "info" } }, schema, "$"), []);
      expect_eq(validate({ annotation: {} }, schema, "$"), ["$.annotation.type: required"]);
      expect_eq(validate({ annotation: { type: 123 } }, schema, "$"), ["$.annotation.type: expected string, got number"]);
    });
    test("JS-010 should validate test details schema", () => {
      const testAnnotationSchema: JsonSchema = {
        type: "object",
        properties: { type: { type: "string" }, description: { type: "string" } },
        required: ["type"],
      };
      const testDetailsSchema: JsonSchema = {
        type: "object",
        properties: {
          tag: { oneOf: [{ type: "string", pattern: "^@" }, { type: "array", items: { type: "string", pattern: "^@" } }] },
          annotation: { oneOf: [testAnnotationSchema, { type: "array", items: testAnnotationSchema }] },
        },
      };

      // Valid cases.
      expect_eq(validate({}, testDetailsSchema, "$"), []);
      expect_eq(validate({ tag: "@fast" }, testDetailsSchema, "$"), []);
      expect_eq(validate({ tag: ["@fast", "@slow"] }, testDetailsSchema, "$"), []);
      expect_eq(validate({ annotation: { type: "issue" } }, testDetailsSchema, "$"), []);
      expect_eq(validate({ annotation: { type: "issue", description: "bug" } }, testDetailsSchema, "$"), []);
      expect_eq(validate({ annotation: [{ type: "a" }, { type: "b" }] }, testDetailsSchema, "$"), []);
      expect_eq(validate({ tag: "@fast", annotation: { type: "issue" } }, testDetailsSchema, "$"), []);

      // Invalid cases.
      expect_eq(validate({ tag: "no-at" }, testDetailsSchema, "$").length > 0, true);
      expect_eq(validate({ tag: ["@ok", "no-at"] }, testDetailsSchema, "$").length > 0, true);
      expect_eq(validate({ annotation: "not-object" }, testDetailsSchema, "$").length > 0, true);
      expect_eq(validate({ annotation: { description: "missing type" } }, testDetailsSchema, "$").length > 0, true);
    });
}
