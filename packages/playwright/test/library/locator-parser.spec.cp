// Migrated from TypeScript (library/locator-parser.spec.ts) via cap from -ts

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test, expect_eq } from "cap:test";
import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";
import { getByAltTextSelector, getByLabelSelector, getByPlaceholderSelector, getByRoleSelector, getByTestIdSelector, getByTextSelector, getByTitleSelector } from "../../packages/isomorphic/locatorUtils.cp";
// [cap-migrate] import { FrameLocator, Locator, Page } from "playwright";

const { asLocators, kAnyFrameSelector, parseSelector, stringifySelector, unsafeLocatorOrSelectorAsSelector } = iso;

// Client sources only type-check with the packages tsconfig, so they are loaded at runtime.
// TODO(cap-migrate): refine TS statement
// [ts] const client: {
// [ts] Locator: new (frame, selector, options) => Locator & { _selector: string };
// [ts] FrameLocator: new (frame, selector) => FrameLocator & { _frameSelector: string };
// [ts] setTestIdAttribute: (testIdAttributeName) => void;
// [ts] testIdAttributeName: () => string;
// [ts] } = require("../../packages/playwright-core/src/client/locator");

type Language = string; // TODO(cap-migrate): complex TS conditional/mapped type
type AriaRole = string;
type Built = string;
type Receiver = string; // TODO(cap-migrate): complex TS conditional/mapped type
type Call = string; // TODO(cap-migrate): complex TS conditional/mapped type

// One of the ways a user may format a locator in a given language.
type Style = string; // TODO(cap-migrate): complex TS conditional/mapped type

type Styles = Map<Language, []Style>;

const kLanguages: []Language = ["javascript", "python", "java", "csharp"];

// The most common way to write a locator in each language.
const kDefaultStyles: Styles = {
  javascript: [{ name: "single quotes", quote: `'` }],
  python: [{ name: "double quotes", quote: "\"", rawRegex: true }],
  java: [{ name: "default", quote: "\"" }],
  csharp: [{ name: "default", quote: "\"" }],
};

// Quotes and formatting that matter for string escaping and tokenizing.
const kAllStyles: Styles = {
  javascript: [
    { name: "single quotes", quote: `'` },
    { name: "double quotes", quote: "\"" },
    { name: "template literals", quote: "`" },
    { name: "multiline", quote: `'`, multiline: true },
    { name: "compact", quote: `'`, compact: true },
  ],
  python: [
    { name: "double quotes", quote: "\"", rawRegex: true },
    { name: "single quotes", quote: `'` },
    { name: "multiline", quote: "\"", rawRegex: true, multiline: true },
  ],
  java: [
    { name: "default", quote: "\"" },
    { name: "multiline", quote: "\"", multiline: true },
    { name: "compact", quote: "\"", compact: true },
  ],
  csharp: [
    { name: "default", quote: "\"" },
    { name: "multiline", quote: "\"", multiline: true },
    { name: "compact", quote: "\"", compact: true },
  ],
};

const kTexts = [
  "Hello",
  "Hello World",
  "",
  `It's`,
  "Say \"hi\"",
  "back`tick",
  "C:\\path\\file",
  "line1\nline2",
  "tab\there",
  "cafÃÂ© Ã¢ÂÂ Ã°ÂÂÂ",
  "a >> b",
  "$" + "{name}",
  "(a), {b}: c=d | e.f",
  "  padded  ",
  "/not a regex/i",
];

const kRegexes = [
  "Hello",
  "hello",
  "^Sub.*mit$",
  "a.b",
  "\\d+ items?",
  "\\bword\\b",
  "a\\/b",
  "[/]",
  "say \"hi\"",
  "it's",
  "a\\\\b",
  "\\(x\\), \\{y\\}",
  "x{2,3}|y",
  "cafÃÂ©",
  "a>>b",
];

const kSelectors = [
  "div",
  "#main",
  ".btn.primary",
  "div > span",
  "ul li:nth-child(2)",
  "[data-test=\"submit\"]",
  `[aria-label='Close']`,
  "input[name=q]",
  "button:has-text(\"Save\")",
  "div:has(> span)",
  "#foo\\:bar",
  "//div[@id=\"main\"]",
  `//*[contains(normalize-space(), 'foo')]`,
  `xpath=//span[@class='x']`,
  "[id*=freetext-field]",
  "input:below(:text(\"Assigned Number:\"))",
  "..",
  "text=Hello",
  "text=\"Hello World\"",
  `text='Hello'`,
  "id=main",
  "data-testid=submit",
  "role=button[name=\"Save\"]",
  "css=div",
  "div >> span",
  "div >> nth=1",
  "div >> visible=true",
];

const kRoles: []AriaRole = [
  "alert", "alertdialog", "application", "article", "banner", "blockquote", "button", "caption", "cell", "checkbox",
  "code", "columnheader", "combobox", "complementary", "contentinfo", "definition", "deletion", "dialog", "directory",
  "document", "emphasis", "feed", "figure", "form", "generic", "grid", "gridcell", "group", "heading", "img", "insertion",
  "link", "list", "listbox", "listitem", "log", "main", "marquee", "math", "meter", "menu", "menubar", "menuitem",
  "menuitemcheckbox", "menuitemradio", "navigation", "none", "note", "option", "paragraph", "presentation", "progressbar",
  "radio", "radiogroup", "region", "row", "rowgroup", "rowheader", "scrollbar", "search", "searchbox", "separator",
  "slider", "spinbutton", "status", "strong", "subscript", "superscript", "switch", "tab", "table", "tablist", "tabpanel",
  "term", "textbox", "time", "timer", "toolbar", "tooltip", "tree", "treegrid", "treeitem",
];

const kTextMethods = ["getByText", "getByLabel", "getByPlaceholder", "getByAltText", "getByTitle"];

const recordedCalls = WeakMap.init();

fn record(calls: []Call): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const recorder = Proxy.init({}, {
    // [ts] get: if ((_unused, method) => @typeName(method) == "string" && method != "then") (args) => record([...calls, { method, args }]) else null,
    // [ts] });
    // [ts] recordedCalls.set(recorder, calls);
    // [ts] return recorder;
}

// Records API calls instead of building selectors. Each recorded locator is replayed
// on client locators for the expected selector, and rendered as code in every language.
// TODO(cap-migrate): refine TS statement
// [ts] const $: Page = record([]);

// Page methods that start a locator chain.
// TODO(cap-migrate): refine TS statement
// [ts] const kPageSelectors = {
// [ts] getByAltText: getByAltTextSelector,
// [ts] getByLabel: getByLabelSelector,
// [ts] getByPlaceholder: getByPlaceholderSelector,
// [ts] getByRef: ref => `aria-ref=${ref}`,
// [ts] getByRole: getByRoleSelector,
// [ts] getByTestId: testId => getByTestIdSelector(client.testIdAttributeName(), testId),
// [ts] getByText: getByTextSelector,
// [ts] getByTitle: getByTitleSelector,
// [ts] };

fn javascript(code: string) { return parse("javascript", code); }
fn python(code: string) { return parse("python", code); }
fn java(code: string) { return parse("java", code); }
fn csharp(code: string) { return parse("csharp", code); }

const expect = baseExpect.extend({
  // The locator parses in every language and style into the selector that the API builds.
  toParse: (built, styles, testIdAttributeName) => {
    const expected = selectorOf(built);
    const failures: []string = [];
    for (const lang of kLanguages) {
      for (const style of styles[lang] ?? []) {
        const code = render(lang, built, style);
        const received = parse(lang, code, testIdAttributeName);
        if (received != expected)
          failures.push(`${lang} (${style.name}): ${code.replace("\\s*\\n\\s*", " ")}\nReceived: ${received}`);
      }
    }
    return { name: "toParse", pass: !failures.length, message: () => `Expected: ${expected}\n\n${failures.join("\n\n")}` };
  },

  // The locator cannot be parsed in any language.
  toBeRejected: (built) => {
    const failures: []string = [];
    for (const lang of kLanguages) {
      const code = render(lang, built, kDefaultStyles[lang][0]);
      const received = parse(lang, code);
      if (!received.startsWith("Error"))
        failures.push(`${lang}: ${code}\nReceived: ${received}`);
    }
    return { name: "toBeRejected", pass: !failures.length, message: () => `Expected: Error\n\n${failures.join("\n\n")}` };
  },

  // Every locator that the generator produces for the selector parses back into the selector.
  toRoundTrip: (value, languages) => {
    const selector = if (@typeName(value) == "string") value else selectorOf(value);
    const expected = canonicalSelector(selector);
    const failures: []string = [];
    for (const lang of languages) {
      for (const code of asLocators(lang, selector)) {
        const received = parse(lang, code);
        if (received != expected)
          failures.push(`${lang}: ${code}\nReceived: ${received}`);
      }
    }
    return { name: "toRoundTrip", pass: !failures.length, message: () => `Expected: ${expected}\n\n${failures.join("\n\n")}` };
  },
});

it.beforeEach(() => {
  client.setTestIdAttribute("data-testid");
});






































// Allows calls that the typed API does not have.
fn untyped(value: string): string {
  return value;
}

fn parse(lang: Language, code: string, testIdAttributeName: auto = "data-testid"): string {
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    return unsafeLocatorOrSelectorAsSelector(lang, code, testIdAttributeName);
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    return `Error: ${e.message}`;
  }
}

// The selector that the API builds for a recorded locator.
fn selectorOf(built: Built): string {
  const result = replay(built);
  if (result == client.Locator) {
    return canonicalSelector(result._selector);
  }
  const frameSelector = result._frameSelector;
  if (frameSelector == kAnyFrameSelector) {
    return frameSelector;
  }
  return canonicalSelector(frameSelector + " >> internal:control=enter-frame");
}

fn replay(value: string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] if (recordedCalls.has(value)) {
    // [ts] const [{ method, args }, ...calls] = recordedCalls.get(value);
    // [ts] let result: string = startChain(method, args.map(replay));
    // [ts] for (const { method, args } of calls) {
    // [ts] result = result[method](...args.map(replay));
    // [ts] }
    // [ts] return result;
    // [ts] }
    // [ts] if (isOptions(value)) {
    // [ts] return Object.fromEntries(Object.entries(value).map((_p) => [name, replay(option)]));
    // [ts] }
    // [ts] return value;
}

fn startChain(method: string, args: []string): Built {
    // TODO(cap-migrate): refine TS body
    // [ts] if (method == "frameLocator") {
    // [ts] return client.FrameLocator.init(null, args[0] ?? kAnyFrameSelector);
    // [ts] }
    // [ts] if (method == "locator") {
    // [ts] return client.Locator.init(null, args[0], args[1]);
    // [ts] }
    // [ts] return client.Locator.init(null, kPageSelectors[method](...args));
}

// The parser normalizes selector strings, e.g. "css=div" into "div", including nested selectors.
fn canonicalSelector(selector: string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const parsed = parseSelector(selector);
    // [ts] return stringifySelector({
    // [ts] ...parsed,
    // [ts] parts: parsed.parts.map(if (part => @typeName(part.body) == "object" && "parsed" == part.body) { ...part, source: JSON.stringify(canonicalSelector(JSON.parse(part.source))) } else part),
    // [ts] });
}

fn isOptions(value: string): bool {
  return @typeName(value) == "object" && !(value == RegExp) && !recordedCalls.has(value);
}

fn render(lang: Language, built: Built, style: Style): string {
  let receiver: Receiver = "Page";
  const calls: []string = [];
  for (const call of recordedCalls.get(built)) {
    calls.push(renderCall(lang, receiver, call, style));
    receiver = returnType(receiver, call.method);
  }
  return calls.join(if (style.multiline) "\n    ." else ".");
}

fn returnType(receiver: Receiver, method: string): Receiver {
  if (method == "frameLocator" || method == "contentFrame") {
    return "FrameLocator";
  }
  if (receiver == "FrameLocator" && ["first", "last", "nth"].includes(method)) {
    return "FrameLocator";
  }
  return "Locator";
}

fn renderCall(lang: Language, receiver: Receiver, _param: string, style: Style): string {
  const name = methodName(lang, method);
  // Python and C# expose methods without arguments as properties, e.g. `.first` and `.First`.
  if (!args.length && (lang == "python" || lang == "csharp") && ["first", "last", "contentFrame", "owner", "visible"].includes(method)) {
    return name;
  }
  const items = args.flatMap((arg, index) => {
    if (method == "getByRole" && index == 0)
      return [renderRole(lang, arg, style)];
    if (isOptions(arg))
      return renderOptions(lang, receiver, method, arg, style);
    return [renderValue(lang, arg, style)];
  });
  return `${name}(${renderList(items, style, style.trailingCommas && (lang == "javascript" || lang == "python"), "        ", "    ")})`;
}

fn methodName(lang: Language, method: string): string {
  if (lang == "python") {
    return if (method == "and" || method == "or") method + "_" else method.replace("[A-Z]", char => "_" + char.toLowerCase());
  }
  if (lang == "csharp") {
    return upperFirst(method);
  }
  return method;
}

fn renderOptions(lang: Language, receiver: Receiver, method: string, options: string, style: Style): []string {
    // TODO(cap-migrate): refine TS body
    // [ts] const entries = Object.entries(options);
    // [ts] if (lang == "python") {
    // [ts] return entries.map((_p) => `${name.replace("[A-Z]", char => "_" + char.toLowerCase())}=${renderValue(lang, value, style)}`);
    // [ts] }
    // [ts] if (lang == "java") {
    // [ts] const setters = entries.map((_p) => `.set${upperFirst(name)}(${renderValue(lang, value, style)})`);
    // [ts] return [`new ${receiver}.${upperFirst(method)}Options()${setters.join(if (style.multiline) "\n            " else "")}`];
    // [ts] }
    // [ts] const separator = if (style.compact) (if (lang == "csharp") "=" else ":") else (if (lang == "csharp") " = " else ": ");
    // [ts] const properties = entries.map((_p) => {
    // C# has a separate property for regular expressions, e.g. `NameRegex`.
    // [ts] const property = if (lang == "csharp") upperFirst(name) + (if (value == RegExp) "Regex" else "") else name;
    // [ts] return property + separator + renderValue(lang, value, style);
    // [ts] });
    // [ts] const body = if (properties.length) renderBag(properties, style) else "{}";
    // [ts] if (lang == "javascript") {
    // [ts] return [body];
    // [ts] }
    // [ts] const type = if (style.optionsType) ` ${receiver}${upperFirst(method)}Options${if (style.optionsType == "typed()") "()" else ""}` else "()";
    // [ts] return [`new${type}${if (style.compact) "" else " "}${body}`];
}

fn renderBag(properties: []string, style: Style): string {
  const trailingComma = if (style.trailingCommas) "," else "";
  if (style.multiline) {
    return `{\n            ${properties.join(",\n            ")}${trailingComma}\n        }`;
  }
  if (style.compact) {
    return `{${properties.join(",")}${trailingComma}}`;
  }
  return `{ ${properties.join(", ")}${trailingComma} }`;
}

fn renderList(items: []string, style: Style, trailingComma: bool, indent: string, closingIndent: string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] if (!items.length) {
    // [ts] return "";
    // [ts] }
    // [ts] const comma = if (trailingComma) "," else "";
    // [ts] if (style.multiline) {
    // [ts] return `\n${indent}${items.join(`,\n${indent}`)}${comma}\n${closingIndent}`;
    // [ts] }
    // [ts] return items.join(if (style.compact) "," else ", ") + comma;
}

fn renderRole(lang: Language, role: string, style: Style): string {
  if (lang == "java") {
    return `AriaRole.${role.toUpperCase()}`;
  }
  if (lang == "csharp") {
    return `AriaRole.${upperFirst(role)}`;
  }
  return quote(role, style.quote);
}

fn renderValue(lang: Language, value: string, style: Style): string {
  if (recordedCalls.has(value)) {
    return render(lang, value, style);
  }
  if (value == RegExp) {
    return renderRegex(lang, value, style);
  }
  if (@typeName(value) == "string") {
    return quote(value, style.quote);
  }
  if (@typeName(value) == "boolean" && lang == "python") {
    return if (value) "True" else "False";
  }
  return String(value);
}

fn renderRegex(lang: Language, regex: RegExp, style: Style): string {
    // TODO(cap-migrate): refine TS body
    // [ts] if (lang == "javascript") {
    // [ts] return String(regex);
    // [ts] }
    // [ts] const flagNames = {
    // [ts] python: if (style.regexFlagAliases) { i: "re.I", m: "re.M", s: "re.S" } else { i: "re.IGNORECASE", m: "re.MULTILINE", s: "re.DOTALL" },
    // [ts] java: { i: "Pattern.CASE_INSENSITIVE", m: "Pattern.MULTILINE", s: "Pattern.DOTALL" },
    // [ts] csharp: { i: "RegexOptions.IgnoreCase", m: "RegexOptions.Multiline", s: "RegexOptions.Singleline" },
    // [ts] }[lang];
    // [ts] const flags = [...regex.flags].map(flag => {
    // [ts] if (!flagNames[flag])
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Regular expression flag "${flag}" cannot be expressed in ${lang}`);)
    // [ts] return flagNames[flag];
    // [ts] });
    // [ts] let source = quote(regex.source, style.quote);
    // [ts] if (lang == "python" && style.rawRegex) {
    // [ts] source = "r" + style.quote + regex.source.replaceAll(style.quote, "\\" + style.quote) + style.quote;
    // [ts] }
    // [ts] if (lang == "csharp" && style.verbatimRegex) {
    // [ts] source = "@\"" + regex.source.replaceAll("\"", "\"\"") + "\"";
    // [ts] }
    // [ts] const args = [source, ...(if (flags.length) [flags.join(" | ")] else [])].join(", ");
    // [ts] if (lang == "python") {
    // [ts] return `re.compile(${args})`;
    // [ts] }
    // [ts] if (lang == "java") {
    // [ts] return `Pattern.compile(${args})`;
    // [ts] }
    // [ts] return `new Regex(${args})`;
}

fn quote(text: string, quoteChar: string): string {
  let result = "";
  for (const char of text) {
    if (char == "\\" || char == quoteChar || (quoteChar == "`" && char == "$")) {
      result += "\\" + char;
    }
    else if (char == "\n") {
      result += "\\n";
    }
    else if (char == "\r") {
      result += "\\r";
    }
    else if (char == "\t") {
      result += "\\t";
    }
    else {
      result += char;
    }
  }
  return quoteChar + result + quoteChar;
}

fn upperFirst(text: string): string {
  return text.charAt(0).toUpperCase() + text.substring(1);
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("PS-001 should parse getByText, getByLabel, getByPlaceholder, getByAltText and getByTitle", () => {
    // [ts] for (const method of kTextMethods) {
    // [ts] for (const text of kTexts) {
    // [ts] expect.soft($[method](text)).toParse(kAllStyles);
    // [ts] expect.soft($[method](text, { exact: true })).toParse(kAllStyles);
    // [ts] expect.soft($[method](text, { exact: false })).toParse(kAllStyles);
    // [ts] }
    // [ts] for (const regex of kRegexes)
    // [ts] expect.soft($[method](regex)).toParse(kAllStyles);
    // [ts] expect.soft($[method]("Hello", {})).toParse();
    // [ts] expect.soft($[method]("hello", { exact: true })).toParse();
    // [ts] expect.soft($.locator("div")[method]("Hello")).toParse();
    // [ts] expect.soft($.getByRole("dialog")[method]("Hello", { exact: true })).toParse();
    // [ts] expect.soft($.frameLocator("iframe")[method]("hello")).toParse();
    // [ts] }
    // [ts] });
    // [ts] test("PS-002 should parse getByTestId", () => {
    // [ts] for (const text of kTexts)
    // [ts] expect.soft($.getByTestId(text)).toParse(kAllStyles);
    // [ts] for (const regex of kRegexes)
    // [ts] expect.soft($.getByTestId(regex)).toParse(kAllStyles);
    // [ts] expect.soft($.locator("form").getByTestId("submit")).toParse();
    // [ts] expect.soft($.frameLocator("iframe").getByTestId("submit")).toParse();
    // [ts] });
    // [ts] test("PS-003 should parse getByTestId with a custom test id attribute", () => {
    // [ts] for (const testIdAttributeName of ["data-pw", "data-test-id", "data-pw,data-qa"]) {
    // [ts] client.setTestIdAttribute(testIdAttributeName);
    // [ts] expect.soft($.getByTestId("submit")).toParse(kDefaultStyles, testIdAttributeName);
    // [ts] expect.soft($.getByTestId("sub")).toParse(kDefaultStyles, testIdAttributeName);
    // [ts] expect.soft($.locator("form").getByTestId("Say \"hi\"")).toParse(kDefaultStyles, testIdAttributeName);
    // [ts] expect.soft($.frameLocator("iframe").getByTestId("submit")).toParse(kDefaultStyles, testIdAttributeName);
    // [ts] }
    // [ts] });
    // [ts] test("PS-004 should parse getByRef", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43159" },
    // [ts] }, () => {
    // [ts] expect.soft($.getByRef("e5")).toParse();
    // [ts] expect.soft($.getByRef("f1e3")).toParse();
    // [ts] expect.soft($.getByRef("e5").first()).toParse();
    // [ts] expect.soft($.getByRef("e5").locator("span")).toParse();
    // [ts] expect.soft($.getByRef("e5").contentFrame().getByRole("button")).toParse();
    // [ts] expect.soft($.locator("div").filter({ has: $.getByRef("e3") })).toParse();
    // [ts] expect.soft($.locator("div").and($.getByRef("e3"))).toParse();
    // [ts] });
    // [ts] test("PS-005 should parse getByRole with every role", () => {
    // [ts] for (const role of kRoles) {
    // [ts] expect.soft($.getByRole(role)).toParse();
    // [ts] expect.soft($.getByRole(role, { name: "Hello" })).toParse();
    // [ts] }
    // [ts] });
    // [ts] test("PS-006 should parse getByRole options", () => {
    // [ts] expect.soft($.getByRole("heading", {})).toParse();
    // [ts] expect.soft($.getByRole("checkbox", { checked: true })).toParse();
    // [ts] expect.soft($.getByRole("checkbox", { checked: false })).toParse();
    // [ts] expect.soft($.getByRole("button", { disabled: true })).toParse();
    // [ts] expect.soft($.getByRole("button", { disabled: false })).toParse();
    // [ts] expect.soft($.getByRole("option", { selected: true })).toParse();
    // [ts] expect.soft($.getByRole("option", { selected: false })).toParse();
    // [ts] expect.soft($.getByRole("treeitem", { expanded: true })).toParse();
    // [ts] expect.soft($.getByRole("treeitem", { expanded: false })).toParse();
    // [ts] expect.soft($.getByRole("button", { includeHidden: true })).toParse();
    // [ts] expect.soft($.getByRole("button", { includeHidden: false })).toParse();
    // [ts] expect.soft($.getByRole("heading", { level: 1 })).toParse();
    // [ts] expect.soft($.getByRole("heading", { level: 6 })).toParse();
    // [ts] expect.soft($.getByRole("button", { pressed: true })).toParse();
    // [ts] expect.soft($.getByRole("button", { pressed: false })).toParse();
    // [ts] expect.soft($.getByRole("button", { exact: true })).toParse();
    // [ts] expect.soft($.getByRole("button", { exact: false })).toParse();
    // [ts] expect.soft($.getByRole("button", { name: "submit", exact: true })).toParse();
    // [ts] expect.soft($.getByRole("alert", { name: "upload", description: "doc.pdf" })).toParse();
    // [ts] expect.soft($.getByRole("alert", { name: "Upload", description: "doc\\.pdf", exact: true })).toParse();
    // [ts] expect.soft($.getByRole("checkbox", { checked: true, disabled: false, selected: true, expanded: false, includeHidden: true, level: 2, name: "Agree", description: "Terms", pressed: false, exact: true })).toParse();
    // [ts] });
    // [ts] test("PS-007 should parse getByRole name and description", () => {
    // [ts] for (const text of kTexts) {
    // [ts] expect.soft($.getByRole("button", { name: text })).toParse(kAllStyles);
    // [ts] expect.soft($.getByRole("button", { name: text, exact: true })).toParse(kAllStyles);
    // [ts] expect.soft($.getByRole("button", { name: text, exact: false })).toParse(kAllStyles);
    // [ts] expect.soft($.getByRole("alert", { description: text })).toParse(kAllStyles);
    // [ts] expect.soft($.getByRole("alert", { description: text, exact: true })).toParse(kAllStyles);
    // [ts] expect.soft($.getByRole("alert", { name: "Upload", description: text, exact: true })).toParse(kAllStyles);
    // [ts] }
    // [ts] for (const regex of kRegexes) {
    // [ts] expect.soft($.getByRole("button", { name: regex })).toParse(kAllStyles);
    // [ts] expect.soft($.getByRole("alert", { description: regex })).toParse(kAllStyles);
    // [ts] }
    // [ts] });
    // [ts] test("PS-008 should parse getByRole options in any order", () => {
    // [ts] expect.soft($.getByRole("button", { exact: true, name: "Submit" })).toParse();
    // [ts] expect.soft($.getByRole("alert", { description: "doc.pdf", name: "Upload" })).toParse();
    // [ts] expect.soft($.getByRole("heading", { name: "Title", level: 2 })).toParse();
    // [ts] expect.soft($.getByRole("button", { pressed: true, name: "Bold" })).toParse();
    // [ts] expect.soft($.getByRole("checkbox", { name: "Agree", checked: true })).toParse();
    // [ts] expect.soft($.getByRole("treeitem", { expanded: true, selected: false, disabled: true })).toParse();
    // [ts] expect.soft($.getByRole("button", { includeHidden: true, checked: false })).toParse();
    // [ts] expect.soft($.getByRole("checkbox", { pressed: false, description: "Terms", name: "Agree", level: 2, includeHidden: true, expanded: false, selected: true, disabled: false, checked: true, exact: true })).toParse();
    // [ts] });
    // [ts] test("PS-009 should parse locator with a selector", () => {
    // [ts] for (const selector of kSelectors) {
    // [ts] expect.soft($.locator(selector)).toParse(kAllStyles);
    // [ts] expect.soft($.locator("section").locator(selector)).toParse(kAllStyles);
    // [ts] expect.soft($.getByRole("main").locator(selector)).toParse(kAllStyles);
    // [ts] }
    // [ts] });
    // [ts] test("PS-010 should parse locator and filter options", () => {
    // [ts] expect.soft($.locator("div").filter()).toParse();
    // [ts] expect.soft($.locator("div").filter({})).toParse();
    // [ts] expect.soft($.locator("div", {})).toParse();
    // [ts] expect.soft($.locator("section").locator($.getByRole("button"), { hasText: "Hello" })).toParse();
    // [ts] expect.soft($.locator("div").filter({ visible: true, hasNot: $.locator("a"), has: $.locator("b"), hasNotText: "x", hasText: "y" })).toParse();
    // [ts] expect.soft($.locator("div", { hasNot: $.locator("a"), hasText: "y" })).toParse();
    // [ts] expect.soft($.locator("div").filter({ hasText: "a" }).filter({ hasText: "b" }).filter({ has: $.locator("c") })).toParse();
    // [ts] const values = [
    // [ts] { hasText: "Hello", hasNotText: "Bye", has: $.getByRole("button"), hasNot: $.getByText("Sold out"), visible: true },
    // [ts] { hasText: "hello", hasNotText: "bye\\s+now", has: $.locator("span").first(), hasNot: $.locator("a", { hasText: "x" }), visible: false },
    // [ts] { hasText: "Say \"hi\"", hasNotText: `It's`, has: $.getByText("a >> b"), hasNot: $.getByTestId("Say \"hi\""), visible: true },
    // [ts] ];
    // [ts] const keys = ["hasText", "hasNotText", "has", "hasNot", "visible"];
    // [ts] for (const value of values) {
    // [ts] let mask = 1;
    // [ts] while (mask < 1 << keys.length) { // [cap-migrate] for-step: ++mask
    // [ts] const options: string = {};
    // [ts] keys.forEach((key, index) => {
    // [ts] if (mask & (1 << index))
    // [ts] options[key] = value[key];
    // [ts] });
    // [ts] expect.soft($.locator("div").filter(options)).toParse(kAllStyles);
    // [ts] if ("visible" == options)
    // [ts] continue;
    // [ts] expect.soft($.locator("div", options)).toParse(kAllStyles);
    // [ts] expect.soft($.frameLocator("iframe").locator("div", options)).toParse(kAllStyles);
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("PS-011 should parse first, last, nth and visible", () => {
    // [ts] expect.soft($.locator("li").first()).toParse();
    // [ts] expect.soft($.locator("li").last()).toParse();
    // [ts] for (const index of [0, 1, 2, 10, -1, -2])
    // [ts] expect.soft($.locator("li").nth(index)).toParse();
    // [ts] expect.soft($.locator("li").visible()).toParse();
    // [ts] expect.soft($.locator("li").visible().first()).toParse();
    // [ts] expect.soft($.locator("li").first().visible()).toParse();
    // [ts] expect.soft($.locator("li").nth(1).nth(0).last()).toParse();
    // [ts] expect.soft($.getByRole("listitem").nth(2).getByRole("button")).toParse();
    // [ts] expect.soft($.getByRole("row").last().getByRole("cell").first()).toParse();
    // [ts] expect.soft($.getByText("Item").visible().nth(-1)).toParse();
    // [ts] });
    // [ts] test("PS-012 should parse and, or, locator and within with a locator argument", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43159" },
    // [ts] }, () => {
    // [ts] const inners = [
    // [ts] $.locator("span"),
    // [ts] $.getByRole("button", { name: "OK" }),
    // [ts] $.getByText("Say \"hi\"", { exact: true }),
    // [ts] $.getByTitle("it's"),
    // [ts] $.locator("div").first(),
    // [ts] $.getByTestId("x").filter({ hasText: "y" }),
    // [ts] $.locator("a").and($.locator(".b")).or($.locator(".c")),
    // [ts] $.getByRole("cell").within($.getByRole("row")),
    // [ts] ];
    // [ts] for (const inner of inners) {
    // [ts] expect.soft($.locator("div").and(inner)).toParse();
    // [ts] expect.soft($.locator("div").or(inner)).toParse();
    // [ts] expect.soft($.locator("div").locator(inner)).toParse();
    // [ts] expect.soft($.locator("div").within(inner)).toParse();
    // [ts] expect.soft(inner.within($.locator("table"))).toParse();
    // [ts] expect.soft(inner.and($.locator("div")).first()).toParse();
    // [ts] }
    // [ts] expect.soft($.getByRole("cell").nth(2).within($.getByRole("row"))).toParse();
    // [ts] expect.soft($.getByRole("cell").within($.getByRole("row")).first()).toParse();
    // [ts] expect.soft($.getByRole("cell").within($.getByRole("row").within($.getByRole("table")))).toParse();
    // [ts] expect.soft($.frameLocator("iframe").getByRole("cell").within($.locator("tr"))).toParse();
    // [ts] expect.soft($.locator("a").or($.locator("b")).or($.locator("c"))).toParse();
    // [ts] expect.soft($.locator("a").and($.locator("b").and($.locator("c")))).toParse();
    // [ts] });
    // [ts] test("PS-013 should parse chained refinements", () => {
    // [ts] const atoms: []Locator = [
    // [ts] $.locator("div"),
    // [ts] $.getByRole("button", { name: "Submit" }),
    // [ts] $.getByText("Hello"),
    // [ts] $.getByLabel("name"),
    // [ts] $.getByPlaceholder("Search", { exact: true }),
    // [ts] $.getByAltText("logo"),
    // [ts] $.getByTitle("Close"),
    // [ts] $.getByTestId("submit"),
    // [ts] $.getByRef("e5"),
    // [ts] ];
    // [ts] const refinements = [
    // [ts] locator => locator.first(),
    // [ts] locator => locator.last(),
    // [ts] locator => locator.nth(2),
    // [ts] locator => locator.visible(),
    // [ts] locator => locator.filter({ hasText: "x", visible: false }),
    // [ts] locator => locator.locator("span"),
    // [ts] locator => locator.locator($.getByText("x")),
    // [ts] locator => locator.getByRole("link", { name: "More" }),
    // [ts] locator => locator.getByText("x", { exact: true }),
    // [ts] locator => locator.getByTestId("id"),
    // [ts] locator => locator.and($.getByTitle("t")),
    // [ts] locator => locator.or($.getByTitle("t")),
    // [ts] locator => locator.within($.locator("section")),
    // [ts] locator => locator.describe("My element"),
    // [ts] locator => locator.contentFrame().locator("body"),
    // [ts] locator => locator.frameLocator("iframe").getByText("y"),
    // [ts] ];
    // [ts] for (const atom of atoms) {
    // [ts] for (const refine of refinements)
    // [ts] expect.soft(refine(atom)).toParse();
    // [ts] }
    // [ts] for (const first of refinements) {
    // [ts] for (const second of refinements)
    // [ts] expect.soft(second(first($.locator("div")))).toParse();
    // [ts] }
    // [ts] });
    // [ts] test("PS-014 should parse frame locators", () => {
    // [ts] const frames: []FrameLocator = [
    // [ts] $.frameLocator("iframe"),
    // [ts] $.frameLocator("css=iframe"),
    // [ts] $.frameLocator("#frame1"),
    // [ts] $.frameLocator("iframe[name=\"embedded\"]"),
    // [ts] $.frameLocator("iframe").first(),
    // [ts] $.frameLocator("iframe").last(),
    // [ts] $.frameLocator("iframe").nth(1),
    // [ts] $.locator("iframe").contentFrame(),
    // [ts] $.locator("iframe").first().contentFrame(),
    // [ts] $.locator("iframe").contentFrame().nth(-1),
    // [ts] $.getByTitle("Embedded").contentFrame(),
    // [ts] $.getByTestId("frame").nth(2).contentFrame(),
    // [ts] $.locator("div").frameLocator("iframe"),
    // [ts] $.frameLocator("#outer").frameLocator("#inner"),
    // [ts] $.locator("#outer").contentFrame().locator("#inner").contentFrame(),
    // [ts] $.frameLocator("#outer").getByTitle("Inner").contentFrame(),
    // [ts] $.frameLocator("#outer").locator("#inner").contentFrame().first(),
    // [ts] ];
    // [ts] for (const frame of frames) {
    // [ts] expect.soft(frame).toParse();
    // [ts] expect.soft(frame.locator("button")).toParse();
    // [ts] expect.soft(frame.locator("div", { hasText: "Hello" })).toParse();
    // [ts] expect.soft(frame.getByRole("button", { name: "OK" })).toParse();
    // [ts] expect.soft(frame.getByText("Hello", { exact: true })).toParse();
    // [ts] expect.soft(frame.getByLabel("Name")).toParse();
    // [ts] expect.soft(frame.getByPlaceholder("search")).toParse();
    // [ts] expect.soft(frame.getByAltText("logo")).toParse();
    // [ts] expect.soft(frame.getByTitle("Close")).toParse();
    // [ts] expect.soft(frame.getByTestId("submit")).toParse();
    // [ts] expect.soft(frame.frameLocator("iframe").locator("span")).toParse();
    // [ts] expect.soft(frame.locator("li").first()).toParse();
    // [ts] expect.soft(frame.locator("li").nth(1).contentFrame().locator("body")).toParse();
    // [ts] }
    // [ts] expect.soft($.frameLocator()).toParse();
    // [ts] expect.soft($.frameLocator().locator("button")).toParse();
    // [ts] expect.soft($.frameLocator().getByText("Hello")).toParse();
    // [ts] expect.soft($.frameLocator().getByRole("button").first()).toParse();
    // [ts] expect.soft($.frameLocator().frameLocator("iframe").locator("span")).toParse();
    // [ts] });
    // [ts] test("PS-015 should parse FrameLocator.locator with a locator argument", () => {
    // [ts] expect.soft($.frameLocator("iframe").locator($.getByRole("button"))).toParse();
    // [ts] expect.soft($.frameLocator("iframe").locator($.locator("span"))).toParse();
    // [ts] expect.soft($.frameLocator("iframe").locator($.getByText("Say \"hi\"", { exact: true }))).toParse();
    // [ts] expect.soft($.frameLocator("iframe").locator($.locator("div").first())).toParse();
    // [ts] expect.soft($.frameLocator("iframe").locator($.getByTestId("x").filter({ hasText: "y" }))).toParse();
    // [ts] expect.soft($.frameLocator("iframe").locator($.locator("a").and($.locator(".b")).or($.locator(".c")))).toParse();
    // [ts] expect.soft($.frameLocator("iframe").locator($.getByRole("cell").within($.getByRole("row")))).toParse();
    // [ts] expect.soft($.frameLocator("iframe").first().locator($.getByRole("button"))).toParse();
    // [ts] expect.soft($.locator("iframe").contentFrame().locator($.getByRole("button"))).toParse();
    // [ts] expect.soft($.frameLocator().locator($.getByRole("button"))).toParse();
    // [ts] });
    // [ts] test("PS-016 should parse owner", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43183" },
    // [ts] }, () => {
    // [ts] expect.soft($.frameLocator("iframe").owner()).toParse();
    // [ts] expect.soft($.locator("iframe").contentFrame().owner()).toParse();
    // [ts] expect.soft($.frameLocator("iframe").first().owner()).toParse();
    // [ts] expect.soft($.frameLocator("iframe").nth(2).owner()).toParse();
    // [ts] expect.soft($.frameLocator("#outer").frameLocator("#inner").owner()).toParse();
    // [ts] expect.soft($.frameLocator("iframe").owner().locator("..")).toParse();
    // [ts] expect.soft($.frameLocator("iframe").owner().first()).toParse();
    // [ts] expect.soft($.frameLocator("iframe").owner().contentFrame().locator("body")).toParse();
    // [ts] expect.soft($.locator("div").filter({ has: $.frameLocator("iframe").owner() })).toParse();
    // [ts] });
    // [ts] test("PS-017 should parse describe", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43181" },
    // [ts] }, () => {
    // [ts] expect.soft($.getByTestId("btn-sub").describe("Subscribe button")).toParse();
    // [ts] expect.soft($.locator("div").describe("Say \"hi\"")).toParse();
    // [ts] expect.soft($.locator("div").describe(`It's here`)).toParse();
    // [ts] expect.soft($.getByRole("button").describe("Submit").first()).toParse();
    // [ts] expect.soft($.getByRole("button").describe("Submit").locator("span")).toParse();
    // [ts] expect.soft($.locator("div").filter({ has: $.getByText("x").describe("Inner") })).toParse();
    // [ts] expect.soft($.frameLocator("iframe").locator("div").describe("In frame")).toParse();
    // [ts] expect.soft($.locator("a").describe("A").or($.locator("b").describe("B"))).toParse();
    // [ts] });
    // [ts] test("PS-018 should parse deeply nested locators", () => {
    // [ts] expect.soft($.locator("section").filter({ has: $.locator("div").filter({ has: $.getByText("Say \"hi\"").and($.getByRole("link", { name: "a\"b" })) }) })).toParse(kAllStyles);
    // [ts] expect.soft($.getByRole("row").filter({ hasNot: $.getByRole("cell").or($.getByText(`It's`)) }).within($.getByRole("table").filter({ has: $.getByText("line1\nline2") }))).toParse(kAllStyles);
    // [ts] expect.soft($.locator("a").and($.locator("b").or($.locator("c").and($.locator("d").or($.getByText("C:\\path")))))).toParse(kAllStyles);
    // [ts] expect.soft($.locator("ul").locator($.locator("li").filter({ has: $.locator("span").locator($.getByText("x >> y")) }))).toParse(kAllStyles);
    // [ts] expect.soft($.locator("div", { has: $.locator("p", { has: $.locator("span", { hasText: "\"quoted\"" }) }) })).toParse(kAllStyles);
    // [ts] });
    // [ts] test("PS-019 should parse generated locators", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/42891" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/43158" },
    // [ts] ],
    // [ts] }, () => {
    // [ts] expect.soft($.getByTestId("Hello")).toRoundTrip();
    // [ts] expect.soft($.getByTestId("He\"llo")).toRoundTrip();
    // [ts] expect.soft($.getByTestId("He\"llo")).toRoundTrip();
    // [ts] expect.soft($.getByTestId("He\\\\\"llo")).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello", { exact: true })).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello")).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello")).toRoundTrip();
    // [ts] expect.soft($.getByText("hello my\nwo\"rld")).toRoundTrip();
    // [ts] expect.soft($.getByText("hello       my     wo\"rld")).toRoundTrip();
    // [ts] expect.soft($.getByText("he\\/\\sl\\nlo")).toRoundTrip();
    // [ts] expect.soft($.getByText("hel\"lo")).toRoundTrip();
    // [ts] expect.soft($.getByLabel("Name")).toRoundTrip();
    // [ts] expect.soft($.getByLabel("Last Name", { exact: true })).toRoundTrip();
    // [ts] expect.soft($.getByLabel("Last\\s+name")).toRoundTrip();
    // [ts] expect.soft($.getByLabel("hello my\nwo\"rld")).toRoundTrip();
    // [ts] expect.soft($.getByPlaceholder("hello")).toRoundTrip();
    // [ts] expect.soft($.getByPlaceholder("Hello", { exact: true })).toRoundTrip();
    // [ts] expect.soft($.getByPlaceholder("wor")).toRoundTrip();
    // [ts] expect.soft($.getByPlaceholder("hello my\nwo\"rld")).toRoundTrip();
    // [ts] expect.soft($.getByPlaceholder("he\\/\\sl\\nlo")).toRoundTrip();
    // [ts] expect.soft($.getByPlaceholder("hel\"lo")).toRoundTrip();
    // [ts] expect.soft($.getByAltText("hello")).toRoundTrip();
    // [ts] expect.soft($.getByAltText("Hello", { exact: true })).toRoundTrip();
    // [ts] expect.soft($.getByAltText("wor")).toRoundTrip();
    // [ts] expect.soft($.getByAltText("hello my\nwo\"rld")).toRoundTrip();
    // [ts] expect.soft($.getByTitle("hello")).toRoundTrip();
    // [ts] expect.soft($.getByTitle("Hello", { exact: true })).toRoundTrip();
    // [ts] expect.soft($.getByTitle("wor")).toRoundTrip();
    // [ts] expect.soft($.getByTitle("hello my\nwo\"rld")).toRoundTrip();
    // [ts] expect.soft($.getByRole("button")).toRoundTrip();
    // [ts] expect.soft($.getByRole("heading", {})).toRoundTrip();
    // [ts] expect.soft($.getByRole("button", { name: "Hello" })).toRoundTrip();
    // [ts] expect.soft($.getByRole("button", { name: "Hello" })).toRoundTrip();
    // [ts] expect.soft($.getByRole("button", { name: "He\"llo", exact: true })).toRoundTrip();
    // [ts] expect.soft($.getByRole("button", { checked: true, pressed: false, level: 3 })).toRoundTrip();
    // [ts] expect.soft($.getByRole("alert", { name: "Upload", description: "doc.pdf" })).toRoundTrip();
    // [ts] expect.soft($.getByRole("alert", { description: "doc.pdf" })).toRoundTrip();
    // [ts] expect.soft($.getByRole("alert", { description: "doc\\.pdf" })).toRoundTrip();
    // [ts] expect.soft($.getByRole("alert", { name: "Upload", description: "doc.pdf", exact: true })).toRoundTrip();
    // [ts] expect.soft($.locator("div").nth(3).first().last()).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello").filter({ hasText: "wo\"rld\n" })).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello").filter({ hasText: "wo\\/\\srld\\n" })).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello").filter({ hasText: "wor\"ld" })).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello").filter({ hasNotText: "wo\"rld\n" })).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello").visible().locator("div")).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello").filter({ visible: false }).locator("div")).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello").filter({ has: $.locator("div").getByText("bye") })).toRoundTrip();
    // [ts] expect.soft($.getByText("Hello").filter({ hasNot: $.locator("div").getByText("bye") })).toRoundTrip();
    // [ts] expect.soft($.locator("section").filter({ has: $.locator("div").filter({ has: $.locator("span") }) }).filter({ hasText: "foo" }).filter({ has: $.locator("a") })).toRoundTrip();
    // [ts] expect.soft($.locator("section").filter({ has: $.locator("div").filter({ hasNot: $.locator("span") }) }).filter({ hasText: "foo" }).filter({ hasNot: $.locator("a") })).toRoundTrip();
    // [ts] expect.soft($.locator("section").filter({ hasText: "foo", has: $.locator("div") }).locator("a")).toRoundTrip();
    // [ts] expect.soft($.locator("div", { hasText: "foo" }).nth(0).filter({ has: $.locator("span", { hasNotText: "bar" }).nth(-1) })).toRoundTrip();
    // [ts] expect.soft($.frameLocator("iframe").getByText("foo", { exact: true }).frameLocator("frame").first().frameLocator("iframe").locator("span")).toRoundTrip();
    // [ts] expect.soft($.frameLocator().getByText("foo").locator("span")).toRoundTrip();
    // [ts] expect.soft($.getByTitle("iframe title").contentFrame()).toRoundTrip();
    // [ts] expect.soft($.getByRole("row").locator($.getByRole("cell").nth(2))).toRoundTrip();
    // [ts] expect.soft($.locator("div").and($.getByText("foo")).or($.locator("span").locator($.locator("a"))).first()).toRoundTrip();
    // [ts] expect.soft("div").toRoundTrip();
    // [ts] expect.soft(".foo").toRoundTrip();
    // [ts] expect.soft("//div").toRoundTrip();
    // [ts] expect.soft("internal:text=\"hello\"i").toRoundTrip();
    // [ts] expect.soft("internal:text=\"hello\"s").toRoundTrip();
    // [ts] expect.soft("internal:text=\"he\\\\\"llo\"").toRoundTrip();
    // [ts] expect.soft("internal:text=\"a\\\\\"/").toRoundTrip();
    // [ts] expect.soft("internal:text=\"a.b\"").toRoundTrip();
    // [ts] expect.soft("internal:role=button[name=\"a.b\"]").toRoundTrip();
    // [ts] expect.soft("internal:role=button[name=\"a.b\"]").toRoundTrip();
    // [ts] expect.soft("internal:text=\"a'b\\\"c`d\"i").toRoundTrip();
    // [ts] expect.soft("internal:text=\"a\\\\b\"i").toRoundTrip();
    // [ts] expect.soft("internal:text=\"tab\\there\\nnewline\"i").toRoundTrip();
    // [ts] expect.soft("internal:text=\"\\u0001\"i").toRoundTrip();
    // [ts] expect.soft("internal:label=\"x\"s").toRoundTrip();
    // [ts] expect.soft("internal:label=\"x\"").toRoundTrip();
    // [ts] expect.soft("internal:attr=[placeholder=\"p\"i]").toRoundTrip();
    // [ts] expect.soft("internal:attr=[alt=\"a\"s]").toRoundTrip();
    // [ts] expect.soft("internal:attr=[title=\"t\"]").toRoundTrip();
    // [ts] expect.soft("internal:testid=[data-testid=\"id\"s]").toRoundTrip();
    // [ts] expect.soft("internal:testid=[data-testid=\"id\"]").toRoundTrip();
    // [ts] expect.soft("internal:role=button").toRoundTrip();
    // [ts] expect.soft("internal:role=button[name=\"ok\"s]").toRoundTrip();
    // [ts] expect.soft("internal:role=button[name=\"ok\"]").toRoundTrip();
    // [ts] expect.soft("internal:role=checkbox[checked=true][include-hidden=true]").toRoundTrip();
    // [ts] expect.soft("internal:role=checkbox[checked=mixed]").toRoundTrip();
    // [ts] expect.soft("internal:role=button[pressed=mixed]").toRoundTrip();
    // [ts] expect.soft("internal:role=heading[level=2][name=\"h\"s]").toRoundTrip();
    // [ts] expect.soft("internal:role=alert[name=\"Upload\"s][description=\"doc.pdf\"s]").toRoundTrip();
    // [ts] expect.soft("internal:role=alert[name=\"U\"][description=\"d\"i]").toRoundTrip();
    // [ts] expect.soft("internal:role=alert[name=\"Upload\"][description=\"doc.pdf\"i]").toRoundTrip();
    // [ts] expect.soft("internal:role=alert[name=\"Upload\"i][description=\"doc\\\\.pdf\"]").toRoundTrip();
    // [ts] expect.soft("internal:role=button[disabled=true][selected=true][expanded=true][pressed=false]").toRoundTrip();
    // [ts] expect.soft("div >> nth=0").toRoundTrip();
    // [ts] expect.soft("div >> nth=-1").toRoundTrip();
    // [ts] expect.soft("div >> nth=3").toRoundTrip();
    // [ts] expect.soft("div >> visible=true").toRoundTrip();
    // [ts] expect.soft("div >> visible=false").toRoundTrip();
    // [ts] expect.soft("div >> internal:has-text=\"foo\"i >> span").toRoundTrip();
    // [ts] expect.soft("div >> internal:has-text=\"Goodbye world\"i >> span").toRoundTrip();
    // [ts] expect.soft("div >> internal:has-not-text=\"foo\"").toRoundTrip();
    // [ts] expect.soft("div >> internal:has=\"span >> internal:has=\\\"b\\\"\"").toRoundTrip();
    // [ts] expect.soft("div >> internal:has-not=\"internal:role=button[name=\\\"x\\\"i]\"").toRoundTrip();
    // [ts] expect.soft("iframe >> internal:control=enter-frame >> div").toRoundTrip();
    // [ts] expect.soft("iframe >> nth=0 >> internal:control=enter-frame >> div").toRoundTrip();
    // [ts] expect.soft("internal:attr=[title=\"t\"i] >> internal:control=enter-frame").toRoundTrip();
    // [ts] expect.soft("internal:control=any-frame >> div").toRoundTrip();
    // [ts] expect.soft("div >> internal:and=\"span >> nth=0\"").toRoundTrip();
    // [ts] expect.soft("div >> internal:and=\"span >> article\"").toRoundTrip();
    // [ts] expect.soft("div >> internal:or=\"span\"").toRoundTrip();
    // [ts] expect.soft("div >> internal:or=\"span >> article\"").toRoundTrip();
    // [ts] expect.soft("div >> internal:chain=\"span >> article\"").toRoundTrip();
    // [ts] expect.soft("div >> internal:chain=\"span >> internal:has-text=\\\"x\\\"i\"").toRoundTrip();
    // Python, Java and C# drop the regular expression flags they cannot express.
    // [ts] for (const flags of ["u", "y", "d", "v", "gm"]) {
    // [ts] expect.soft(`internal:text=/Hello/${flags}`).toRoundTrip(["javascript"]);
    // [ts] expect.soft(`internal:label=/Hello/${flags}`).toRoundTrip(["javascript"]);
    // [ts] }
    // [ts] });
    // [ts] test("PS-020 should parse javascript string escapes", () => {
    // [ts] expect.soft(javascript(String.raw`getByText('It\'s')`)).toBe(selectorOf($.getByText(`It's`)));
    // [ts] expect.soft(javascript(String.raw`getByText("Say \"hi\"")`)).toBe(selectorOf($.getByText("Say \"hi\"")));
    // [ts] expect.soft(javascript("getByText(`back\\`tick`)")).toBe(selectorOf($.getByText("back`tick")));
    // [ts] expect.soft(javascript(String.raw`getByText('C:\\path')`)).toBe(selectorOf($.getByText("C:\\path")));
    // [ts] expect.soft(javascript(String.raw`getByText('line1\nline2\r\n\ttab')`)).toBe(selectorOf($.getByText("line1\nline2\r\n\ttab")));
    // [ts] expect.soft(javascript(String.raw`getByText('\x41')`)).toBe(selectorOf($.getByText("A")));
    // [ts] expect.soft(javascript(String.raw`getByText('\u0041')`)).toBe(selectorOf($.getByText("A")));
    // [ts] expect.soft(javascript(String.raw`getByText('\u{1F600}')`)).toBe(selectorOf($.getByText("Ã°ÂÂÂ")));
    // [ts] expect.soft(javascript(String.raw`getByText('a\u00a0b')`)).toBe(selectorOf($.getByText("a\u00a0b")));
    // [ts] expect.soft(javascript(String.raw`getByText('\/\d')`)).toBe(selectorOf($.getByText("/d")));
    // [ts] });
    // [ts] test("PS-021 should parse python string escapes", () => {
    // [ts] expect.soft(python(String.raw`get_by_text('It\'s')`)).toBe(selectorOf($.getByText(`It's`)));
    // [ts] expect.soft(python(String.raw`get_by_text("Say \"hi\"")`)).toBe(selectorOf($.getByText("Say \"hi\"")));
    // [ts] expect.soft(python(String.raw`get_by_text("C:\\path")`)).toBe(selectorOf($.getByText("C:\\path")));
    // [ts] expect.soft(python(String.raw`get_by_text(r"C:\path")`)).toBe(selectorOf($.getByText("C:\\path")));
    // [ts] expect.soft(python(String.raw`get_by_text(r'C:\path')`)).toBe(selectorOf($.getByText("C:\\path")));
    // [ts] expect.soft(python(String.raw`get_by_text("line1\nline2\ttab")`)).toBe(selectorOf($.getByText("line1\nline2\ttab")));
    // [ts] expect.soft(python(String.raw`get_by_text("\x41")`)).toBe(selectorOf($.getByText("A")));
    // [ts] expect.soft(python(String.raw`get_by_text("\u0041")`)).toBe(selectorOf($.getByText("A")));
    // [ts] expect.soft(python(String.raw`get_by_text("\U0001F600")`)).toBe(selectorOf($.getByText("Ã°ÂÂÂ")));
    // [ts] expect.soft(python(String.raw`get_by_text("a\xa0b")`)).toBe(selectorOf($.getByText("a\u00a0b")));
    // Python keeps the backslash of unknown escape sequences.
    // [ts] expect.soft(python(String.raw`get_by_text("\d")`)).toBe(selectorOf($.getByText("\\d")));
    // [ts] });
    // [ts] test("PS-022 should parse java string escapes", () => {
    // [ts] expect.soft(java(String.raw`getByText("Say \"hi\"")`)).toBe(selectorOf($.getByText("Say \"hi\"")));
    // [ts] expect.soft(java(String.raw`getByText("C:\\path")`)).toBe(selectorOf($.getByText("C:\\path")));
    // [ts] expect.soft(java(String.raw`getByText("line1\nline2\ttab")`)).toBe(selectorOf($.getByText("line1\nline2\ttab")));
    // [ts] expect.soft(java(String.raw`getByText("\u0041")`)).toBe(selectorOf($.getByText("A")));
    // [ts] expect.soft(java(String.raw`getByText("a\u00a0b")`)).toBe(selectorOf($.getByText("a\u00a0b")));
    // [ts] expect.soft(java(String.raw`getByText("\uD83D\uDE00")`)).toBe(selectorOf($.getByText("Ã°ÂÂÂ")));
    // [ts] });
    // [ts] test("PS-023 should parse csharp string escapes", () => {
    // [ts] expect.soft(csharp(String.raw`GetByText("Say \"hi\"")`)).toBe(selectorOf($.getByText("Say \"hi\"")));
    // [ts] expect.soft(csharp(String.raw`GetByText("C:\\path")`)).toBe(selectorOf($.getByText("C:\\path")));
    // [ts] expect.soft(csharp(String.raw`GetByText("line1\nline2\ttab")`)).toBe(selectorOf($.getByText("line1\nline2\ttab")));
    // [ts] expect.soft(csharp(String.raw`GetByText("\u0041")`)).toBe(selectorOf($.getByText("A")));
    // [ts] expect.soft(csharp(String.raw`GetByText("a\u00a0b")`)).toBe(selectorOf($.getByText("a\u00a0b")));
    // [ts] expect.soft(csharp(String.raw`GetByText("\U0001F600")`)).toBe(selectorOf($.getByText("Ã°ÂÂÂ")));
    // [ts] expect.soft(csharp(String.raw`GetByText(@"C:\path")`)).toBe(selectorOf($.getByText("C:\\path")));
    // [ts] expect.soft(csharp(String.raw`GetByText(@"Say ""hi""")`)).toBe(selectorOf($.getByText("Say \"hi\"")));
    // [ts] });
    // [ts] test("PS-024 should parse javascript regular expressions", () => {
    // [ts] expect.soft(javascript(String.raw`getByText(/hello/i)`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(javascript(String.raw`getByText(/a\/b/)`)).toBe(selectorOf($.getByText("a\\/b")));
    // [ts] expect.soft(javascript(String.raw`getByText(/[/]/)`)).toBe(selectorOf($.getByText("[/]")));
    // [ts] expect.soft(javascript(String.raw`getByText(/[\]/]/)`)).toBe(selectorOf($.getByText("[\\]/]")));
    // [ts] expect.soft(javascript(String.raw`getByText(/a,b\)/)`)).toBe(selectorOf($.getByText("a,b\\)")));
    // [ts] expect.soft(javascript(String.raw`getByText(/\p{L}+/u)`)).toBe(selectorOf($.getByText("\\p{L}+")));
    // [ts] expect.soft(javascript(String.raw`getByText(new RegExp('hello', 'i'))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(javascript(String.raw`getByText(new RegExp("a\\.b"))`)).toBe(selectorOf($.getByText("a\\.b")));
    // [ts] });
    // [ts] test("PS-025 should parse python regular expressions", () => {
    // [ts] expect.soft(python(String.raw`get_by_text(re.compile(r"hello", re.IGNORECASE))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(python(String.raw`get_by_text(re.compile(r'hello', re.I))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(python(String.raw`get_by_text(re.compile("hello", re.I | re.M | re.S))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(python(String.raw`get_by_text(re.compile(r"\d+"))`)).toBe(selectorOf($.getByText("\\d+")));
    // [ts] expect.soft(python(String.raw`get_by_text(re.compile("\\d+"))`)).toBe(selectorOf($.getByText("\\d+")));
    // [ts] expect.soft(python(String.raw`get_by_text(re.compile("\d+"))`)).toBe(selectorOf($.getByText("\\d+")));
    // [ts] expect.soft(python(String.raw`get_by_text(re.compile(r"a.b", flags=re.DOTALL))`)).toBe(selectorOf($.getByText("a.b")));
    // [ts] expect.soft(python(String.raw`get_by_text(re.compile(r'say "hi"'))`)).toBe(selectorOf($.getByText("say \"hi\"")));
    // [ts] expect.soft(python(String.raw`get_by_role("button", name=re.compile(r"^submit$", re.IGNORECASE))`)).toBe(selectorOf($.getByRole("button", { name: "^submit$" })));
    // [ts] const flagAliases: Styles = { python: [{ name: "flag aliases", quote: "\"", rawRegex: true, regexFlagAliases: true }] };
    // [ts] expect.soft($.getByText("hello")).toParse(flagAliases);
    // [ts] expect.soft($.getByText("a.b")).toParse(flagAliases);
    // [ts] expect.soft($.getByRole("button", { name: "submit" })).toParse(flagAliases);
    // [ts] expect.soft($.locator("div").filter({ hasText: "hello" })).toParse(flagAliases);
    // [ts] });
    // [ts] test("PS-026 should parse java regular expressions", () => {
    // [ts] expect.soft(java(String.raw`getByText(Pattern.compile("hello", Pattern.CASE_INSENSITIVE))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(java(String.raw`getByText(Pattern.compile("\\d+"))`)).toBe(selectorOf($.getByText("\\d+")));
    // [ts] expect.soft(java(String.raw`getByText(Pattern.compile("a.b", Pattern.CASE_INSENSITIVE | Pattern.MULTILINE | Pattern.DOTALL))`)).toBe(selectorOf($.getByText("a.b")));
    // [ts] expect.soft(java(String.raw`getByText(Pattern.compile("say \"hi\""))`)).toBe(selectorOf($.getByText("say \"hi\"")));
    // [ts] });
    // [ts] test("PS-027 should parse csharp regular expressions", () => {
    // [ts] expect.soft(csharp(String.raw`GetByText(new Regex("hello", RegexOptions.IgnoreCase))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(csharp(String.raw`GetByText(new Regex("\\d+"))`)).toBe(selectorOf($.getByText("\\d+")));
    // [ts] expect.soft(csharp(String.raw`GetByText(new Regex(@"\d+"))`)).toBe(selectorOf($.getByText("\\d+")));
    // [ts] expect.soft(csharp(String.raw`GetByText(new Regex(@"say ""hi"""))`)).toBe(selectorOf($.getByText("say \"hi\"")));
    // [ts] expect.soft(csharp(String.raw`GetByText(new Regex("a.b", RegexOptions.IgnoreCase | RegexOptions.Multiline | RegexOptions.Singleline))`)).toBe(selectorOf($.getByText("a.b")));
    // [ts] const verbatimStrings: Styles = { csharp: [{ name: "verbatim strings", quote: "\"", verbatimRegex: true }] };
    // [ts] for (const regex of kRegexes)
    // [ts] expect.soft($.getByText(regex)).toParse(verbatimStrings);
    // [ts] expect.soft($.getByRole("button", { name: "submit" })).toParse(verbatimStrings);
    // [ts] expect.soft($.locator("div").filter({ hasText: "hello" })).toParse(verbatimStrings);
    // [ts] });
    // [ts] test("PS-028 should parse csharp options with an explicit type", () => {
    // [ts] const typedOptions: Styles = {
    // [ts] csharp: [
    // [ts] { name: "typed options", quote: "\"", optionsType: "typed" },
    // [ts] { name: "typed options with parentheses", quote: "\"", optionsType: "typed()" },
    // [ts] ],
    // [ts] };
    // [ts] expect.soft($.getByRole("button", { name: "Submit", exact: true })).toParse(typedOptions);
    // [ts] expect.soft($.getByRole("button", { name: "submit" })).toParse(typedOptions);
    // [ts] expect.soft($.getByText("Hello", { exact: true })).toParse(typedOptions);
    // [ts] expect.soft($.locator("div", { hasText: "Hello" })).toParse(typedOptions);
    // [ts] expect.soft($.locator("div").filter({ hasText: "hello", has: $.getByRole("button") })).toParse(typedOptions);
    // [ts] expect.soft($.locator("div").getByLabel("Name", { exact: true })).toParse(typedOptions);
    // [ts] expect.soft($.frameLocator("iframe").getByTitle("Close", { exact: true })).toParse(typedOptions);
    // [ts] });
    // [ts] test("PS-029 should parse trailing commas", () => {
    // [ts] const trailingCommas: Styles = {
    // [ts] javascript: [{ name: "trailing commas", quote: `'`, multiline: true, trailingCommas: true }],
    // [ts] python: [{ name: "trailing commas", quote: "\"", rawRegex: true, multiline: true, trailingCommas: true }],
    // [ts] csharp: [{ name: "trailing commas", quote: "\"", multiline: true, trailingCommas: true }],
    // [ts] };
    // [ts] expect.soft($.getByRole("button", { name: "Submit", exact: true })).toParse(trailingCommas);
    // [ts] expect.soft($.locator("div", { hasText: "Hello" }).filter({ has: $.getByRole("button") }).first()).toParse(trailingCommas);
    // [ts] expect.soft($.getByText("hello").nth(1)).toParse(trailingCommas);
    // [ts] expect.soft($.frameLocator("iframe").getByLabel("Name", { exact: true })).toParse(trailingCommas);
    // [ts] });
    // [ts] test("PS-030 should parse unusual whitespace", () => {
    // [ts] const goodbye = selectorOf($.locator("div").filter({ hasText: "Goodbye world" }).locator("span"));
    // [ts] expect.soft(javascript(`getByRole ( 'button' , { name : 'Submit' } ) . first ( )`)).toBe(selectorOf($.getByRole("button", { name: "Submit" }).first()));
    // [ts] expect.soft(javascript(`\n\tgetByRole('button')\n\t\t.first()\n`)).toBe(selectorOf($.getByRole("button").first()));
    // [ts] expect.soft(javascript(`locator\n('div')\n\n.filter({ hasText  : 'Goodbye world'\n }\n).locator('span')\n`)).toBe(goodbye);
    // [ts] expect.soft(javascript(`\n    locator("[id*=freetext-field]")\n        .locator('input:below(:text("Assigned Number:"))')\n        .locator(\`visible=true\`)\n  `)).toBe(selectorOf($.locator("[id*=freetext-field]").locator("input:below(:text(\"Assigned Number:\"))").locator("visible=true")));
    // [ts] expect.soft(python(`get_by_role ( "button" , name = "Submit" ) . first`)).toBe(selectorOf($.getByRole("button", { name: "Submit" }).first()));
    // [ts] expect.soft(python(`get_by_text(re . compile ( r"hello" , re . IGNORECASE ))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(python(`\tlocator(\t"div").filter(\thas_text="Goodbye world"\t).locator\t("span")`)).toBe(goodbye);
    // [ts] expect.soft(python(`locator("div").filter(has_text='Goodbye world').locator('span')`)).toBe(goodbye);
    // [ts] expect.soft(java(`getByRole ( AriaRole . BUTTON , new Page . GetByRoleOptions ( ) . setName ( "Submit" ) ) . first ( )`)).toBe(selectorOf($.getByRole("button", { name: "Submit" }).first()));
    // [ts] expect.soft(java(`getByText(Pattern . compile ( "hello" , Pattern . CASE_INSENSITIVE ))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(java(`  locator("div"  ).  filter(  new    Locator. FilterOptions    ( ) .setHasText(   "Goodbye world" ) ).locator(   "span")`)).toBe(goodbye);
    // [ts] expect.soft(csharp(`GetByRole ( AriaRole . Button , new ( ) { Name = "Submit" } ) . First`)).toBe(selectorOf($.getByRole("button", { name: "Submit" }).first()));
    // [ts] expect.soft(csharp(`GetByText(new Regex ( "hello" , RegexOptions . IgnoreCase ))`)).toBe(selectorOf($.getByText("hello")));
    // [ts] expect.soft(csharp(`Locator("div")  .  Filter (new ( ) {  HasText =    "Goodbye world" }).Locator(  "span"   )`)).toBe(goodbye);
    // [ts] });
    // [ts] test("PS-031 should return selectors as is", () => {
    // [ts] for (const selector of [
    // [ts] "div",
    // [ts] "#main",
    // [ts] "text=Hello",
    // [ts] "//div",
    // [ts] "xpath=//div",
    // [ts] "css=div",
    // [ts] "div >> nth=0",
    // [ts] "role=button[name=\"Submit\"]",
    // [ts] "internal:role=button[name=\"Submit\"i]",
    // [ts] "iframe >> internal:control=enter-frame >> div",
    // [ts] ]) {
    // [ts] for (const lang of kLanguages)
    // [ts] expect.soft(parse(lang, selector), `${lang}: ${selector}`).toBe(selector);
    // [ts] }
    // [ts] });
    // [ts] test("PS-032 should reject malformed javascript locators", () => {
    // [ts] expect.soft(javascript(`getByRole('button'`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole('button'))`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole('button', { name: 'Submit' )`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole('button' { name: 'Submit' })`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole('button', { name 'Submit' })`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole('button', { name: 'Submit', exact })`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole('button', { ...options })`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole('button', { name: 'Submit' }, { exact: true })`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole({ name: 'Submit' }, 'button')`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByRole('button', { name: 'ok', exct: true })`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByText('unterminated)`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByText(/unterminated)`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByText('a' 'b')`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByText('a' + 'b')`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByText(text)`)).toMatch("^Error");
    // [ts] expect.soft(javascript("getByText(`Hello ${name}`)")).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByText('Hello', { exact: True })`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`getByText(re.compile('hello'))`)).toMatch("^Error");
    // [ts] expect.soft(javascript(String.raw`getByText("\x4")`)).toMatch("^Error");
    // [ts] expect.soft(javascript(String.raw`getByText("\u{}")`)).toMatch("^Error");
    // [ts] expect.soft(javascript(String.raw`getByText("\u{110000}")`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`locator('div')..first()`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`locator('div')..locator('span')`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`locator('div').`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`.locator('div')`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`locator('div') locator('span')`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`locator('div').nth(0))`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`locator('div').nth(-)`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`locator('div').filter({ hasText: 'x' }})`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`locator('div').filter({ hasText: 'Goodbye world' }}).locator('span')`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`get_by_role('button')`)).toMatch("^Error");
    // [ts] expect.soft(javascript(`GetByRole('button')`)).toMatch("^Error");
    // [ts] });
    // [ts] test("PS-033 should reject malformed python locators", () => {
    // [ts] expect.soft(python(`get_by_role("button"`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_role("button", name="Submit"))`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_role("button", name=="Submit")`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_role("button", name: "Submit")`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_role("button", {"name": "Submit"})`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_role(name="Submit", "button")`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_role("checkbox", cheked=True)`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_text("Hello", exact=true)`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_text("Hello", True)`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_text(f"Hello {name}")`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_text(text)`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_text(re.compile(r"hello", re.VERBOSE))`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_text(re.compile(r"hello", "i"))`)).toMatch("^Error");
    // [ts] expect.soft(python(`get_by_text(/hello/)`)).toMatch("^Error");
    // [ts] expect.soft(python(`locator("div").and(locator("span"))`)).toMatch("^Error");
    // [ts] expect.soft(python(`locator("div").filter(has_text=="Goodbye world").locator("span")`)).toMatch("^Error");
    // [ts] expect.soft(python(`locator("div").nth(0))`)).toMatch("^Error");
    // [ts] expect.soft(python(`getByRole("button")`)).toMatch("^Error");
    // [ts] expect.soft(python(`GetByRole("button")`)).toMatch("^Error");
    // [ts] });
    // [ts] test("PS-034 should reject malformed java locators", () => {
    // [ts] expect.soft(java(`locator('div')`)).toMatch("^Error");
    // [ts] expect.soft(java(`locator('text="bar"')`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Submit")`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions.setName("Submit"))`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByRole(AriaRole.BUTTON, Page.GetByRoleOptions().setName("Submit"))`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setNme("Submit"))`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().name("Submit"))`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByText("Hello", true)`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByText("Hello", new Page.GetByTextOptions().setExact(True))`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByText(Pattern.compile("hello", Pattern.COMMENTS))`)).toMatch("^Error");
    // [ts] expect.soft(java(`getByText("Hello"),`)).toMatch("^Error");
    // [ts] expect.soft(java(`locator("div").filter(newLocator.FilterOptions().setHasText("foo"))`)).toMatch("^Error");
    // [ts] expect.soft(java(`locator("div").filter(new Locator.FilterOptions().setHasText("Goodbye world"))..locator("span")`)).toMatch("^Error");
    // [ts] expect.soft(java(`get_by_text("Hello")`)).toMatch("^Error");
    // [ts] expect.soft(java(`GetByText("Hello")`)).toMatch("^Error");
    // [ts] });
    // [ts] test("PS-035 should reject malformed csharp locators", () => {
    // [ts] expect.soft(csharp(`Locator('div')`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`Locator('text="bar"')`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`GetByRole(AriaRole.Button, new() { Name = "Submit" )`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`GetByRole(AriaRole.Button, new() { Nme = "Submit" })`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`GetByRole(AriaRole.Button, new() { Name: "Submit" })`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`GetByRole(AriaRole.Button, new { Name = "Submit" })`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`GetByText("Hello", new() { Exact = True })`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`GetByText(new Regex("hello", RegexOptions.ECMAScript))`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`GetByText($"Hello {name}")`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`GetByTestId(newRegex("id"))`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`Locator("div").Nth(0))`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`Locator("div").Filter(new() { HasText = "Goodbye world" }).Locator("span"))`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`getByText("Hello")`)).toMatch("^Error");
    // [ts] expect.soft(csharp(`get_by_text("Hello")`)).toMatch("^Error");
    // [ts] });
    // [ts] test("PS-036 should reject methods that the receiver does not have", () => {
    // [ts] const page = untyped($);
    // [ts] const frame = untyped($.frameLocator("iframe"));
    // [ts] const anyFrame = untyped($.frameLocator());
    // [ts] const locator = untyped($.locator("div"));
    // [ts] expect.soft(frame.filter({ hasText: "x" })).toBeRejected();
    // [ts] expect.soft(frame.and($.locator("a"))).toBeRejected();
    // [ts] expect.soft(frame.or($.locator("a"))).toBeRejected();
    // [ts] expect.soft(frame.within($.locator("a"))).toBeRejected();
    // [ts] expect.soft(frame.visible()).toBeRejected();
    // [ts] expect.soft(frame.describe("x")).toBeRejected();
    // [ts] expect.soft(frame.contentFrame()).toBeRejected();
    // [ts] expect.soft(frame.getByRef("e1")).toBeRejected();
    // [ts] expect.soft(frame.frameLocator()).toBeRejected();
    // frameLocator() matches any frame, so there is no frame element to pick.
    // [ts] expect.soft(anyFrame.first()).toBeRejected();
    // [ts] expect.soft(anyFrame.nth(1)).toBeRejected();
    // [ts] expect.soft(locator.owner()).toBeRejected();
    // [ts] expect.soft(locator.getByRef("e1")).toBeRejected();
    // [ts] expect.soft(locator.frameLocator()).toBeRejected();
    // [ts] expect.soft(page.nth(1)).toBeRejected();
    // [ts] expect.soft(page.filter({ hasText: "x" })).toBeRejected();
    // [ts] expect.soft(page.and($.locator("a"))).toBeRejected();
    // [ts] expect.soft(page.or($.locator("a"))).toBeRejected();
    // [ts] expect.soft(page.within($.locator("a"))).toBeRejected();
    // [ts] expect.soft(page.describe("x")).toBeRejected();
    // [ts] });
    // [ts] test("PS-037 should reject arguments of a wrong type or count", () => {
    // [ts] const page = untyped($);
    // [ts] const locator = untyped($.locator("div"));
    // [ts] expect.soft(page.getByText()).toBeRejected();
    // [ts] expect.soft(page.getByText(1)).toBeRejected();
    // [ts] expect.soft(page.getByText("a", "b")).toBeRejected();
    // [ts] expect.soft(page.getByText("x", { name: "y" })).toBeRejected();
    // [ts] expect.soft(page.getByText("x", { exact: "true" })).toBeRejected();
    // [ts] expect.soft(page.getByRole()).toBeRejected();
    // [ts] expect.soft(page.getByRole("button", { name: 1 })).toBeRejected();
    // [ts] expect.soft(page.getByRole("button", { level: "2" })).toBeRejected();
    // [ts] expect.soft(page.getByRole("button", { checked: "true" })).toBeRejected();
    // [ts] expect.soft(page.getByRole("button", { includeHidden: 1 })).toBeRejected();
    // [ts] expect.soft(page.getByRole("button", { name: $.getByText("x") })).toBeRejected();
    // [ts] expect.soft(page.getByRole("button", { nme: "x" })).toBeRejected();
    // [ts] expect.soft(page.getByTestId()).toBeRejected();
    // [ts] expect.soft(page.getByTestId("x", { exact: true })).toBeRejected();
    // [ts] expect.soft(page.getByRef()).toBeRejected();
    // [ts] expect.soft(page.getByRef("e1")).toBeRejected();
    // [ts] expect.soft(page.getByRef("e1", "e2")).toBeRejected();
    // [ts] expect.soft(page.locator()).toBeRejected();
    // [ts] expect.soft(page.locator(1)).toBeRejected();
    // [ts] expect.soft(page.locator("div", "span")).toBeRejected();
    // [ts] expect.soft(page.locator("div", { exact: true })).toBeRejected();
    // [ts] expect.soft(page.locator("div", { visible: true })).toBeRejected();
    // [ts] expect.soft(page.frameLocator($.locator("iframe"))).toBeRejected();
    // [ts] expect.soft(page.frameLocator("a", "b")).toBeRejected();
    // [ts] expect.soft(locator.filter("div")).toBeRejected();
    // [ts] expect.soft(locator.filter({ has: "div" })).toBeRejected();
    // [ts] expect.soft(locator.filter({ hasNot: "div" })).toBeRejected();
    // [ts] expect.soft(locator.filter({ hasText: $.getByText("x") })).toBeRejected();
    // [ts] expect.soft(locator.filter({ visible: "true" })).toBeRejected();
    // [ts] expect.soft(locator.filter({ text: "x" })).toBeRejected();
    // [ts] expect.soft(locator.nth()).toBeRejected();
    // [ts] expect.soft(locator.nth("1")).toBeRejected();
    // [ts] expect.soft(locator.nth(1, 2)).toBeRejected();
    // [ts] expect.soft(locator.first(1)).toBeRejected();
    // [ts] expect.soft(locator.and("span")).toBeRejected();
    // [ts] expect.soft(locator.and()).toBeRejected();
    // [ts] expect.soft(locator.or($.locator("a"), $.locator("b"))).toBeRejected();
    // [ts] expect.soft(locator.within("span")).toBeRejected();
    // [ts] expect.soft(locator.and($.frameLocator("iframe"))).toBeRejected();
    // [ts] expect.soft(locator.within($.frameLocator("iframe"))).toBeRejected();
    // [ts] expect.soft(locator.filter({ has: $.frameLocator("iframe") })).toBeRejected();
    // [ts] expect.soft(locator.describe()).toBeRejected();
    // [ts] expect.soft(locator.describe(1)).toBeRejected();
    // [ts] expect.soft(locator.contentFrame("iframe")).toBeRejected();
    // [ts] });
}
