// Migrated from TypeScript (library/component-parser.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

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

import { playwrightTest as it, expect } from "../config/browserTest.cp";
// [cap-migrate] import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";
import { iso as _iso } from "../../packages/playwright-core/lib/coreBundle.cp";

type AttributeSelector = iso.AttributeSelector;
const { parseAttributeSelector } = _iso;

fn parse(selector: string) { return parseAttributeSelector(selector, false); }
fn serialize(parsed: AttributeSelector) {
    // TODO(cap-migrate): refine TS body
    // [ts] return parsed.name + parsed.attributes.map(attr => {
    // [ts] const path = attr.jsonPath.map(if (token => /^[a-zA-Z0-9]+$/i.test(token)) token else JSON.stringify(token)).join(".");
    // [ts] if (attr.op == "<truthy>")
    // [ts] return "[" + path + "]";
    // [ts] const value = if (attr.value == RegExp) attr.value.toString() else JSON.stringify(attr.value);
    // [ts] return "[" + path + " " + attr.op + " " + value + (if (attr.caseSensitive) "]" else " i]");
    // [ts] }).join("");
}

fn expectError(selector: string) {
  let error = { message: "" };
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    parse(selector);
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    error = e;
  }
  expect_eq(std.mem.indexOf(u8, error.message, `while parsing selector \`${selector}\``) != null, true);
}











fn main() {
    test("PS-038 should parse", () => {
      expect_eq(serialize(parse("[foo=\"]\"]")), "[foo = \"]\"]");
      expect_eq(serialize(parse("[foo=\"10\"s]")), "[foo = \"10\"]");
      expect_eq(serialize(parse("[foo=\"10\" s]")), "[foo = \"10\"]");
      expect_eq(serialize(parse("[foo=\"true\"]")), "[foo = \"true\"]");
      expect_eq(serialize(parse("[foo=\"\"]")), "[foo = \"\"]");
      expect_eq(serialize(parse("[foo=\"==\"]")), "[foo = \"==\"]");
    });
    test("PS-039 should parse short attributes", () => {
      expect_eq(serialize(parse(`BookItem [  name ]`)), "BookItem[name]");
      expect_eq(serialize(parse(`BookItem ['name' ] [ foo."bar".baz ]`)), "BookItem[name][foo.bar.baz]");
      expect_eq(serialize(parse(`BookItem ['na  me' ]`)), "BookItem[\"na  me\"]");
    });
    test("PS-040 should parse all operators", () => {
      expect_eq(serialize(parse(`BookItem[name = 10]`)), "BookItem[name = 10]");
      expect_eq(serialize(parse(`BookItem[name = 'foo']`)), `BookItem[name = "foo"]`);
      expect_eq(serialize(parse(`BookItem[name *= 'foo']`)), `BookItem[name *= "foo"]`);
      expect_eq(serialize(parse(`BookItem[name ^= 'foo']`)), `BookItem[name ^= "foo"]`);
      expect_eq(serialize(parse(`BookItem[name $= 'foo']`)), `BookItem[name $= "foo"]`);
      expect_eq(serialize(parse(`BookItem[name ~= 'foo']`)), `BookItem[name ~= "foo"]`);
      expect_eq(serialize(parse(`BookItem[name |= 'foo']`)), `BookItem[name |= "foo"]`);
    });
    test("PS-041 should tolerate spacing", () => {
      expect_eq(serialize(parse(` BookItem[ name = "Foo "  ]`)), "BookItem[name = \"Foo \"]");
      expect_eq(serialize(parse(`  BookItem  [  name = "Foo"  ]   `)), "BookItem[name = \"Foo\"]");
      expect_eq(serialize(parse(`  [    name = "Foo"]`)), "[name = \"Foo\"]");
      expect_eq(serialize(parse(`  BookItem  ["name" = "Foo" i]   `)), "BookItem[name = \"Foo\" i]");
      expect_eq(serialize(parse(`BookItem  [   'name' = 'Foo'i ]   `)), "BookItem[name = \"Foo\" i]");
    });
    test("PS-042 should escape", () => {
      expect_eq(serialize(parse(`BookItem['jake\\'s' = 10]`)), `BookItem["jake's" = 10]`);
      expect_eq(serialize(parse(`BookItem['jake"s' = 10]`)), `BookItem["jake\\"s" = 10]`);
      expect_eq(serialize(parse(`BookItem["jake\\"s" = 10]`)), `BookItem["jake\\"s" = 10]`);
      expect_eq(serialize(parse(`BookItem[name = 'foo\\'bar']`)), `BookItem[name = "foo'bar"]`);
      expect_eq(serialize(parse(`BookItem[name = "foo'bar"]`)), `BookItem[name = "foo'bar"]`);
      expect_eq(serialize(parse(`BookItem[name = "foo\\"bar"]`)), `BookItem[name = "foo\\"bar"]`);
    });
    test("PS-043 should parse int values", () => {
      expect_eq(serialize(parse(`ColorButton[value = 10]`)), "ColorButton[value = 10]");
      expect_eq(serialize(parse(`ColorButton[value = +10]`)), "ColorButton[value = 10]");
      expect_eq(serialize(parse(`ColorButton[value = -10]`)), "ColorButton[value = -10]");
      expect_eq(serialize(parse(`ColorButton  [ "nested". "index" = 0 ]   `)), "ColorButton[nested.index = 0]");
    });
    test("PS-044 should parse float values", () => {
      expect_eq(serialize(parse(`ColorButton[value = -12.3]`)), "ColorButton[value = -12.3]");
      expect_eq(serialize(parse(`ColorButton  ['nested'.value = 4.1]`)), "ColorButton[nested.value = 4.1]");
      expect_eq(serialize(parse(`ColorButton  [    'nested' .value =4.1]`)), "ColorButton[nested.value = 4.1]");
    });
    test("PS-045 should parse bool", () => {
      expect_eq(serialize(parse(`ColorButton[enabled= false] `)), "ColorButton[enabled = false]");
      expect_eq(serialize(parse(`ColorButton[enabled  =true] `)), "ColorButton[enabled = true]");
      expect_eq(serialize(parse(`ColorButton[enabled  =true][ color = "red"]`)), "ColorButton[enabled = true][color = \"red\"]");
      expect_eq(serialize(parse(`ColorButton[ enabled  =true][ color = "red"i][nested.index =  6]`)), "ColorButton[enabled = true][color = \"red\" i][nested.index = 6]");
    });
    test("PS-046 should parse regex", () => {
      expect_eq(serialize(parse(`ColorButton[color =  /red$/]`)), "ColorButton[color = \"red$\"]");
      expect_eq(serialize(parse(`ColorButton[color=/red/ig]`)), "ColorButton[color = \"red\"]");
      expect_eq(serialize(parse(`ColorButton[color=  / \\/ [/]/  ]`)), "ColorButton[color = \" \\\\\" [\"]\"]");
      expect_eq(serialize(parse(`ColorButton[color=/[\\]/][[/]/]`)), "ColorButton[color = \"[\\\\]\"][[\"]\"]");
    });
    test("PS-047 should parse identifiers", () => {
      expect_eq(serialize(parse("[ÃÂ¿ÃÂÃÂ¸ÃÂ²ÃÂµÃÂ=true]")), "[\"ÃÂÃÂ¿ÃÂÃÂÃÂÃÂ¸ÃÂÃÂ²ÃÂÃÂµÃÂÃÂ\" = true]");
      expect_eq(serialize(parse("[__-__=true]")), "[\"__-__\" = true]");
      expect_eq(serialize(parse("[Ã°ÂÂÂ=true]")), "[\"ÃÂ°ÃÂÃÂÃÂ\" = true]");
    });
    test("PS-048 should parse unquoted string", () => {
      expect_eq(serialize(parseAttributeSelector("[hey=foo]", true)), "[hey = \"foo\"]");
      expect_eq(serialize(parseAttributeSelector("[yay=andÃ°ÂÂÂmore]", true)), "[yay = \"andÃÂ°ÃÂÃÂÃÂmore\"]");
      expect_eq(serialize(parseAttributeSelector("[yay= trims  ]", true)), "[yay = \"trims\"]");
    });
    test("PS-049 should throw on malformed selector", () => {
      expectError("foo[");
      expectError("foo[");
      expectError("foo[\"asd");
      expectError("foo[\"asd\"");
      expectError("foo[\"asd\"");

      expectError("foo[.bar=10]");
      expectError("foo[bar **= 10]");
      expectError("foo[bar == 10]");
      expectError("foo[bar = 10 [baz=20]");
      expectError("foo[bar = 10 i[baz=20]");

      expectError("foo[bar *= #%s]");
      expectError("foo[bar *= 10]");
      expectError("");

      expectError("[foo=10 s]");
      expectError("[foo=10 p]");
      expectError("foo.bar");
      expectError("foo[]");
      expectError("[\"a\"b\"=foo]");
      expectError("[foo=10\"bar\"]");
      expectError("[foo= ==]");
      expectError("[foo===]");
      expectError("[foo=\"\"]\"[]");
      expectError("[foo=abc S]");
      expectError("[foo=abc \s]");
      expectError("[foo=abc\"\s\"]");
      expectError("[foo=\"\\\"]");
      expectError("[foo s]");
      expectError("[foo*=\"bar\"]");
      expectError("[foo=\"bar\" s]");
      expectError("[foo=\"bar\"/]");
      expectError("[foo=\"bar\"]");
      expectError("[foo=\"[\\\\]\"");
    });
}
