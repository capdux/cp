// Migrated from TypeScript (src/keyboard.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import os from "cap:os";

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, html } from "./utils.cp";

// [cap-test] describe("Keyboard", () => {
  setupTestBrowserHooks();

  // @see https://github.com/puppeteer/puppeteer/issues/1313
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should type into a textarea", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] const textarea = document.createElement("textarea");
    // [ts] document.body.appendChild(textarea);
    // [ts] textarea.focus();
    // [ts] });
    // [ts] const text = "Hello world. I am the text that was typed!";
    // [ts] page.keyboard.type(text);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe(text);
    // [ts] });
    // [ts] test("should move with the arrow keys", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.type("textarea", "Hello World!");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe("Hello World!");
    // [ts] for (const _ of "World!") {
    // [ts] page.keyboard.press("ArrowLeft");
    // [ts] }
    // [ts] page.keyboard.type("inserted ");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe("Hello inserted World!");
    // [ts] page.keyboard.down("Shift");
    // [ts] for (const _ of "inserted ") {
    // [ts] page.keyboard.press("ArrowLeft");
    // [ts] }
    // [ts] page.keyboard.up("Shift");
    // [ts] page.keyboard.press("Backspace");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe("Hello World!");
    // [ts] });
    // [ts] test("should trigger commands of keyboard shortcuts", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const cmdKey = if (os.platform() == "darwin") "Meta" else "Control";
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.type("textarea", "hello");
    // [ts] page.keyboard.down(cmdKey);
    // [ts] page.keyboard.press("a", {commands: ["SelectAll"]});
    // [ts] page.keyboard.up(cmdKey);
    // [ts] page.keyboard.down(cmdKey);
    // [ts] page.keyboard.down("c", {commands: ["Copy"]});
    // [ts] page.keyboard.up("c");
    // [ts] page.keyboard.up(cmdKey);
    // [ts] page.keyboard.down(cmdKey);
    // [ts] page.keyboard.press("v", {commands: ["Paste"]});
    // [ts] page.keyboard.press("v", {commands: ["Paste"]});
    // [ts] page.keyboard.up(cmdKey);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe("hellohello");
    // [ts] });
    // [ts] test("should trigger editing shortcuts automatically on macOS without commands option", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] if (os.platform() != "darwin") {
    // [ts] return;
    // [ts] }
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.type("textarea", "puppeteer");
    // [ts] page.keyboard.down("Meta");
    // [ts] page.keyboard.press("a");
    // [ts] page.keyboard.up("Meta");
    // [ts] page.keyboard.down("Meta");
    // [ts] page.keyboard.press("c");
    // [ts] page.keyboard.up("Meta");
    // [ts] page.keyboard.down("Meta");
    // [ts] page.keyboard.press("v");
    // [ts] page.keyboard.press("v");
    // [ts] page.keyboard.up("Meta");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe("puppeteerpuppeteer");
    // [ts] });
    // [ts] test("should send a character with ElementHandle.press", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] using textarea = (page.$("textarea"));
    // [ts] textarea.press("a");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe("a");
    // [ts] page.evaluate(() => {
    // [ts] return window.addEventListener(
    // [ts] "keydown",
    // [ts] e => {
    // [ts] return e.preventDefault();
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] });
    // [ts] textarea.press("b");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe("a");
    // [ts] });
    // [ts] test("should press the Space key", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] using textarea = (page.$("textarea"));
    // [ts] textarea.press("Space");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe(" ");
    // [ts] });
    // [ts] test("ElementHandle.press should not support |text| option", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] using textarea = (page.$("textarea"));
    // [ts] textarea.press("a", {text: "ÃÂÃÂ"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").value;
    // [ts] }),
    // [ts] ).toBe("a");
    // [ts] });
    // [ts] test("should send a character with sendCharacter", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).inputCount = 0;
    // [ts] (globalThis).keyDownCount = 0;
    // [ts] window.addEventListener(
    // [ts] "input",
    // [ts] () => {
    // [ts] (globalThis).inputCount += 1;
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] window.addEventListener(
    // [ts] "keydown",
    // [ts] () => {
    // [ts] (globalThis).keyDownCount += 1;
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] });
    // [ts] page.keyboard.sendCharacter("ÃÂ¥ÃÂÃÂ¨");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('textarea', textarea => {
    // [ts-skip] return {
    // [ts-skip] value: textarea.value,
    // [ts-skip] inputs: (globalThis as any).inputCount,
    // [ts-skip] keyDowns: (globalThis as any).keyDownCount,
    // [ts-skip] };
    // [ts-skip] }),
    // [ts] ).toMatchObject({value: "Ã¥ÂÂ¨", inputs: 1, keyDowns: 0});
    // [ts] page.keyboard.sendCharacter("a");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('textarea', textarea => {
    // [ts-skip] return {
    // [ts-skip] value: textarea.value,
    // [ts-skip] inputs: (globalThis as any).inputCount,
    // [ts-skip] keyDowns: (globalThis as any).keyDownCount,
    // [ts-skip] };
    // [ts-skip] }),
    // [ts] ).toMatchObject({value: "Ã¥ÂÂ¨a", inputs: 2, keyDowns: 0});
    // [ts] });
    // [ts] test("should send a character with sendCharacter in iframe", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <iframe
    // [ts] srcdoc="<iframe name='test' srcdoc='<textarea></textarea>'></iframe>"
    // [ts] ></iframe>
    // [ts] `);
    // [ts] const frame = page.waitForFrame(async frame => {
    // [ts] using element = frame.frameElement();
    // [ts] if (!element) {
    // [ts] return false;
    // [ts] }
    // [ts] const name = element.evaluate(frame => {
    // [ts] return frame.name;
    // [ts] });
    // [ts] return name == "test";
    // [ts] });
    // [ts] frame.focus("textarea");
    // [ts] frame.evaluate(() => {
    // [ts] (globalThis).inputCount = 0;
    // [ts] (globalThis).keyDownCount = 0;
    // [ts] window.addEventListener(
    // [ts] "input",
    // [ts] () => {
    // [ts] (globalThis).inputCount += 1;
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] window.addEventListener(
    // [ts] "keydown",
    // [ts] () => {
    // [ts] (globalThis).keyDownCount += 1;
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] });
    // [ts] page.keyboard.sendCharacter("ÃÂ¥ÃÂÃÂ¨");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await frame.$eval('textarea', textarea => {
    // [ts-skip] return {
    // [ts-skip] value: textarea.value,
    // [ts-skip] inputs: (globalThis as any).inputCount,
    // [ts-skip] keyDowns: (globalThis as any).keyDownCount,
    // [ts-skip] };
    // [ts-skip] }),
    // [ts] ).toMatchObject({value: "Ã¥ÂÂ¨", inputs: 1, keyDowns: 0});
    // [ts] page.keyboard.sendCharacter("a");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await frame.$eval('textarea', textarea => {
    // [ts-skip] return {
    // [ts-skip] value: textarea.value,
    // [ts-skip] inputs: (globalThis as any).inputCount,
    // [ts-skip] keyDowns: (globalThis as any).keyDownCount,
    // [ts-skip] };
    // [ts-skip] }),
    // [ts] ).toMatchObject({value: "Ã¥ÂÂ¨a", inputs: 2, keyDowns: 0});
    // [ts] });
    // [ts] test("should report modifiers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] const keyboard = page.keyboard;
    // Shift modifier
    // [ts] keyboard.down("Shift");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keydown: Shift ShiftLeft [Shift]");
    // [ts] keyboard.down("!");
    // Shift + ! produces an input event.
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe(`Keydown: ! Digit1 [Shift]\n` + `input: ! insertText false`);
    // [ts] keyboard.up("!");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: ! Digit1 [Shift]");
    // [ts] keyboard.up("Shift");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: Shift ShiftLeft []");
    // Alt modifier
    // [ts] keyboard.down("Alt");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keydown: Alt AltLeft [Alt]");
    // [ts] keyboard.down("!");
    // Alt + ! should NOT produce an input event in Puppeteer for cross-platform
    // consistency.
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keydown: ! Digit1 [Alt]");
    // [ts] keyboard.up("!");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: ! Digit1 [Alt]");
    // [ts] keyboard.up("Alt");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: Alt AltLeft []");
    // Control modifier
    // [ts] keyboard.down("Control");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keydown: Control ControlLeft [Control]");
    // [ts] keyboard.down("!");
    // Control + ! should NOT produce an input event.
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keydown: ! Digit1 [Control]");
    // [ts] keyboard.up("!");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: ! Digit1 [Control]");
    // [ts] keyboard.up("Control");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: Control ControlLeft []");
    // [ts] });
    // [ts] test("should report multiple modifiers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] const keyboard = page.keyboard;
    // [ts] keyboard.down("Control");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keydown: Control ControlLeft [Control]");
    // [ts] keyboard.down("Alt");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keydown: Alt AltLeft [Alt Control]");
    // [ts] keyboard.down(";");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keydown: ; Semicolon [Alt Control]");
    // [ts] keyboard.up(";");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: ; Semicolon [Alt Control]");
    // [ts] keyboard.up("Control");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: Control ControlLeft [Alt]");
    // [ts] keyboard.up("Alt");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe("Keyup: Alt AltLeft []");
    // [ts] });
    // [ts] test("should send proper codes while typing", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.type("!");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe(
    // [ts] [
    // [ts] "Keydown: ! Digit1 []",
    // [ts] "input: ! insertText false",
    // [ts] "Keyup: ! Digit1 []",
    // [ts] ].join("\n"),
    // [ts] );
    // [ts] page.keyboard.type("^");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe(
    // [ts] [
    // [ts] "Keydown: ^ Digit6 []",
    // [ts] "input: ^ insertText false",
    // [ts] "Keyup: ^ Digit6 []",
    // [ts] ].join("\n"),
    // [ts] );
    // [ts] });
    // [ts] test("should send proper codes while typing with shift", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] const keyboard = page.keyboard;
    // [ts] keyboard.down("Shift");
    // [ts] page.keyboard.type("~");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).getResult();
    // [ts] }),
    // [ts] ).toBe(
    // [ts] [
    // [ts] "Keydown: Shift ShiftLeft [Shift]",
    // [ts] "Keydown: ~ Backquote [Shift]",
    // [ts] "input: ~ insertText false",
    // [ts] "Keyup: ~ Backquote [Shift]",
    // [ts] ].join("\n"),
    // [ts] );
    // [ts] keyboard.up("Shift");
    // [ts] });
    // [ts] test("should send the Enter code for Enter", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).codes = [];
    // [ts] window.addEventListener(
    // [ts] "keydown",
    // [ts] event => {
    // [ts] (globalThis).codes.push(`${event.code} ${event.location}`);
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] });
    // [ts] using textarea = (page.$("textarea"));
    // [ts] textarea.press("Enter");
    // [ts] textarea.type("\n");
    // [ts] textarea.press("NumpadEnter");
    // [ts] const codes = page.evaluate(() => {
    // [ts] return (globalThis).codes;
    // [ts] });
    // [ts] expect_eq(codes.slice(0, 2), ["Enter 0", "Enter 0"]);
    // [ts] expect(codes[2]).toMatch("^NumpadEnter ");
    // [ts] });
    // [ts] test("should not type canceled events", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] page.evaluate(() => {
    // [ts] window.addEventListener(
    // [ts] "keydown",
    // [ts] event => {
    // [ts] event.stopPropagation();
    // [ts] event.stopImmediatePropagation();
    // [ts] if (event.key == "l") {
    // [ts] event.preventDefault();
    // [ts] }
    // [ts] if (event.key == "o") {
    // [ts] event.preventDefault();
    // [ts] }
    // [ts] },
    // [ts] false,
    // [ts] );
    // [ts] });
    // [ts] page.keyboard.type("Hello World!");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).textarea.value;
    // [ts] }),
    // [ts] ).toBe("He Wrd!");
    // [ts] });
    // [ts] test("should specify repeat property", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea").addEventListener(
    // [ts] "keydown",
    // [ts] e => {
    // [ts] return ((globalThis).lastEvent = e);
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] });
    // [ts] page.keyboard.down("a");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).lastEvent.repeat;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.keyboard.press("a");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).lastEvent.repeat;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.keyboard.down("b");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).lastEvent.repeat;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.keyboard.down("b");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).lastEvent.repeat;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.keyboard.up("a");
    // [ts] page.keyboard.down("a");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).lastEvent.repeat;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should type all kinds of characters", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] const text = "This text goes onto two lines.\nThis character is Ã¥ÂÂ¨.";
    // [ts] page.keyboard.type(text);
    // [ts] expect_eq(page.evaluate("result"), text);
    // [ts] });
    // [ts] test("should specify location", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.evaluate(() => {
    // [ts] window.addEventListener(
    // [ts] "keydown",
    // [ts] event => {
    // [ts] return ((globalThis).keyLocation = event.location);
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] });
    // [ts] using textarea = (page.$("textarea"));
    // [ts] textarea.press("Digit5");
    // [ts] expect_eq(page.evaluate("keyLocation"), 0);
    // [ts] textarea.press("ControlLeft");
    // [ts] expect_eq(page.evaluate("keyLocation"), 1);
    // [ts] textarea.press("ControlRight");
    // [ts] expect_eq(page.evaluate("keyLocation"), 2);
    // [ts] textarea.press("NumpadSubtract");
    // [ts] expect_eq(page.evaluate("keyLocation"), 3);
    // [ts] });
    // [ts] test("should specify location on keyup", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.evaluate(() => {
    // [ts] window.addEventListener(
    // [ts] "keyup",
    // [ts] event => {
    // [ts] return ((globalThis).keyLocation = event.location);
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] });
    // [ts] using textarea = (page.$("textarea"));
    // [ts] textarea.press("Digit5");
    // [ts] expect_eq(page.evaluate("keyLocation"), 0);
    // [ts] textarea.press("ControlLeft");
    // [ts] expect_eq(page.evaluate("keyLocation"), 1);
    // [ts] textarea.press("ControlRight");
    // [ts] expect_eq(page.evaluate("keyLocation"), 2);
    // [ts] textarea.press("NumpadSubtract");
    // [ts] expect_eq(page.evaluate("keyLocation"), 3);
    // [ts] });
    // [ts] test("should throw on unknown keys", () => {
    // [ts] const {page} = getTestState();
    // [ts] const error = page.keyboard
    // @ts-expect-error bad input
    // [ts] .press("NotARealKey")
    // [ts] .catch(error_ => {
    // [ts] return error_;
    // [ts] });
    // [ts] expect_eq(error.message, "Unknown key: \"NotARealKey\"");
    // [ts] });
    // [ts] test("should type emoji", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.type("textarea", "ÃÂ°ÃÂÃÂÃÂ¹ Tokyo street Japan ÃÂ°ÃÂÃÂÃÂ¯ÃÂ°ÃÂÃÂÃÂµ");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('textarea', textarea => {
    // [ts-skip] return textarea.value;
    // [ts-skip] }),
    // [ts] ).toBe("Ã°ÂÂÂ¹ Tokyo street Japan Ã°ÂÂÂ¯Ã°ÂÂÂµ");
    // [ts] });
    // [ts] test("should type emoji into an iframe", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "emoji-test",
    // [ts] server.PREFIX + "/input/textarea.html",
    // [ts] );
    // [ts] const frame = page.frames()[1];
    // [ts] using textarea = (frame.$("textarea"));
    // [ts] textarea.type("ÃÂ°ÃÂÃÂÃÂ¹ Tokyo street Japan ÃÂ°ÃÂÃÂÃÂ¯ÃÂ°ÃÂÃÂÃÂµ");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await frame.$eval('textarea', textarea => {
    // [ts-skip] return textarea.value;
    // [ts-skip] }),
    // [ts] ).toBe("Ã°ÂÂÂ¹ Tokyo street Japan Ã°ÂÂÂ¯Ã°ÂÂÂµ");
    // [ts] });
    // [ts] test("should press the meta key", () => {
    // This test only makes sense on macOS.
    // [ts] if (os.platform() != "darwin") {
    // [ts] return;
    // [ts] }
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).result = null;
    // [ts] document.addEventListener("keydown", event => {
    // [ts] (globalThis).result = [event.key, event.code, event.metaKey];
    // [ts] });
    // [ts] });
    // [ts] page.keyboard.press("Meta");
    // Have to do this because we lose a lot of type info when evaluating a
    // string not a function. This is why functions are recommended rather than
    // using strings (although we'll leave this test so we have coverage of both
    // approaches.)
    // [ts] const [key, code, metaKey] = (page.evaluate("result"))
    // [ts] string,
    // [ts] string,
    // [ts] boolean,
    // [ts] ];
    // [ts] expect_eq(key, "Meta");
    // [ts] expect_eq(code, "MetaLeft");
    // [ts] expect_eq(metaKey, true);
    // [ts] });
}
