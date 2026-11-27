// Migrated from TypeScript (library/css-parser.spec.ts) via cap from -ts

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
import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";

const { parseCSS, serializeSelector: serialize } = iso;

fn parse(selector: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] return parseCSS(selector, Set.init(["text", "not", "has", "react", "scope", "right-of", "is"])).selector;
}

fn main() {
    test("PS-050 should parse css", () => {
      expect_eq(serialize(parse("div")), "div");
      expect_eq(serialize(parse("div.class")), "div.class");
      expect_eq(serialize(parse(".class")), ".class");
      expect_eq(serialize(parse("#id")), "#id");
      expect_eq(serialize(parse(".class#id")), ".class#id");
      expect_eq(serialize(parse("div#id.class")), "div#id.class");
      expect_eq(serialize(parse("*")), "*");
      expect_eq(serialize(parse("*div")), "*div");
      expect_eq(serialize(parse("div[attr *= foo i]")), "div[attr *= foo i]");
      expect_eq(serialize(parse("div[attr~=\"Bar baz\"  ]")), "div[attr~=\"Bar baz\" ]");
      expect_eq(serialize(parse(`div    [ foo = 'bar'  s]`)), `div [ foo = "bar" s]`);

      expect_eq(serialize(parse(":hover")), ":hover");
      expect_eq(serialize(parse("div:hover")), "div:hover");
      expect_eq(serialize(parse("#id:active:hover")), "#id:active:hover");
      expect_eq(serialize(parse(":dir(ltr)")), ":dir(ltr)");
      expect_eq(serialize(parse("#foo-bar.cls:nth-child(3n + 10)")), "#foo-bar.cls:nth-child(3n + 10)");
      expect_eq(serialize(parse(":lang(en)")), ":lang(en)");
      expect_eq(serialize(parse("*:hover")), "*:hover");

      expect_eq(serialize(parse("div span")), "div span");
      expect_eq(serialize(parse("div>span")), "div > span");
      expect_eq(serialize(parse("div +span")), "div + span");
      expect_eq(serialize(parse("div~ span")), "div ~ span");
      expect_eq(serialize(parse("div   >.class #id+ span")), "div > .class #id + span");
      expect_eq(serialize(parse("div>span+.class")), "div > span + .class");
      expect_eq(serialize(parse(">span")), ":scope() > span");

      expect_eq(serialize(parse("div:not(span)")), "div:not(span)");
      expect_eq(serialize(parse(":not(span)#id")), "#id:not(span)");
      expect_eq(serialize(parse("div:not(span):hover")), "div:hover:not(span)");
      expect_eq(serialize(parse("div:has(span):hover")), "div:hover:has(span)");
      expect_eq(serialize(parse("div:right-of(span):hover")), "div:hover:right-of(span)");
      expect_eq(serialize(parse(":right-of(span):react(foobar)")), ":right-of(span):react(foobar)");
      expect_eq(serialize(parse("div:is(span):hover")), "div:hover:is(span)");
      expect_eq(serialize(parse("div:scope:hover")), "div:hover:scope()");
      expect_eq(serialize(parse("div:sCOpe:HOVER")), "div:HOVER:scope()");
      expect_eq(serialize(parse("div:NOT(span):hoVER")), "div:hoVER:not(span)");

      expect_eq(serialize(parse(":text(\"foo\")")), ":text(\"foo\")");
      expect_eq(serialize(parse(":text(\"*\")")), ":text(\"*\")");
      expect_eq(serialize(parse(":text(*)")), ":text(*)");
      expect_eq(serialize(parse(":text(\"foo\", normalize-space)")), ":text(\"foo\", normalize-space)");
      expect_eq(serialize(parse(":index(3, div    span)")), ":index(3, div span)");
      expect_eq(serialize(parse(":is(foo, bar>baz.cls+:not(qux))")), ":is(foo, bar > baz.cls + :not(qux))");
    });
    test("PS-051 should throw on malformed css", () => {
      fn expectError(selector: string) {
        let error = { message: "" };
        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          parse(selector);
        } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
          error = e;
        }
        expect_eq(std.mem.indexOf(u8, error.message, `while parsing css selector "${selector}"`) != null, true);
        expect_eq(std.mem.indexOf(u8, error.message, `Did you mean to CSS.escape it?`) != null, true);
      }

      expectError("");
      expectError(".");
      expectError("#");
      expectError("..");
      expectError("#.");
      expectError(".#");
      expectError("[attr=");
      expectError(":not(div");
      expectError("div)");
      expectError("()");
      expectError(":not(##)");
      expectError(":not()");
      expectError(":not(.)");
      expectError("div,");
      expectError(",div");
      expectError("div,,span");
      expectError("div > > span");
      expectError("div > > > > span");
      expectError("div >");
      expectError("\"foo\"");
      expectError("23");
      expectError("span, div>\"foo\"");
    });
}
