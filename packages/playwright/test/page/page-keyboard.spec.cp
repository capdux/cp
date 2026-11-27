// Migrated from TypeScript (page/page-keyboard.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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

import { test as it, expect } from "./pageTest.cp";
import { attachFrame } from "../config/utils.cp";









fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => isAndroid);
    // [ts] test("KB-001 should type into a textarea @smoke", () => {
    // [ts] page.evaluate(() => {
    // [ts] const textarea = document.createElement("textarea");
    // [ts] document.body.appendChild(textarea);
    // [ts] textarea.focus();
    // [ts] });
    // [ts] const text = "Hello world. I am the text that was typed!";
    // [ts] page.keyboard.type(text);
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), text);
    // [ts] });
    // [ts] test("KB-002 should move with the arrow keys", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.type("textarea", "Hello World!");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), "Hello World!");
    // [ts] let i = 0;
    // [ts] while (i < "World!".length) // [cap-migrate] for-step: i++
    // [ts] page.keyboard.press("ArrowLeft");
    // [ts] page.keyboard.type("inserted ");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), "Hello inserted World!");
    // [ts] page.keyboard.down("Shift");
    // [ts] let i = 0;
    // [ts] while (i < "inserted ".length) // [cap-migrate] for-step: i++
    // [ts] page.keyboard.press("ArrowLeft");
    // [ts] page.keyboard.up("Shift");
    // [ts] page.keyboard.press("Backspace");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), "Hello World!");
    // [ts] });
    // [ts] test("KB-003 should send a character with ElementHandle.press", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const textarea = page.$("textarea");
    // [ts] textarea.press("a");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), "a");
    // [ts] page.evaluate(() => window.addEventListener("keydown", e => e.preventDefault(), true));
    // [ts] textarea.press("b");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), "a");
    // [ts] });
    // [ts] test("KB-004 should send a character with insertText", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] page.keyboard.insertText("ÃÂ¥ÃÂÃÂ¨");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), "ÃÂÃÂ¥ÃÂÃÂÃÂÃÂ¨");
    // [ts] page.evaluate(() => window.addEventListener("keydown", e => e.preventDefault(), true));
    // [ts] page.keyboard.insertText("a");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), "ÃÂÃÂ¥ÃÂÃÂÃÂÃÂ¨a");
    // [ts] });
    // [ts] test("KB-005 should not type text with Alt modifier", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42928" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`<input><textarea></textarea><div contenteditable></div>`);
    // [ts] for (const locator of [page.locator("input"), page.locator("textarea"), page.locator("div")]) {
    // [ts] locator.press("Alt+KeyA");
    // [ts] locator.press("Alt+Digit1");
    // [ts] locator.press("Alt+Shift+KeyA");
    // [ts] page.keyboard.down("Alt");
    // [ts] page.keyboard.press("b");
    // [ts] page.keyboard.up("Alt");
    // [ts] locator.press("c");
    // [ts] }
    // [ts] expect(page.locator("input")).toHaveValue("c");
    // [ts] expect(page.locator("textarea")).toHaveValue("c");
    // [ts] expect(page.locator("div")).toHaveText("c");
    // [ts] });
    // [ts] test("KB-006 insertText should only emit input event", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] page.on("console", m => @print(m.text()));
    // [ts] const events = page.evaluateHandle(() => {
    // [ts] const events = [];
    // [ts] document.addEventListener("keydown", e => events.push(e.type));
    // [ts] document.addEventListener("keyup", e => events.push(e.type));
    // [ts] document.addEventListener("keypress", e => events.push(e.type));
    // [ts] document.addEventListener("input", e => events.push(e.type));
    // [ts] return events;
    // [ts] });
    // [ts] page.keyboard.insertText("hello world");
    // [ts] expect_eq(events.jsonValue(), ["input"]);
    // [ts] });
    // [ts] test("KB-007 should emit keydown, keypress, textInput and input when typing a character", () => {
    // [ts] page.setContent(`<input>`);
    // [ts] const events = page.evaluateHandle(() => {
    // [ts] const events: []string = [];
    // [ts] for (const type of ["keydown", "keypress", "textInput", "input", "keyup"])
    // [ts] document.querySelector("input").addEventListener(type, () => events.push(type));
    // [ts] return events;
    // [ts] });
    // [ts] page.focus("input");
    // [ts] page.keyboard.press("f");
    // [ts] expect_eq(events.jsonValue(), ["keydown", "keypress", "textInput", "input", "keyup"]);
    // [ts] });
    // [ts] test("KB-008 should dispatch key events in separate tasks", () => {
    // [ts] test(browserName == "firefox" && !isBidi, "Firefox/Juggler dispatches keydown and keypress in the same task");
    // [ts] page.setContent(`<input>`);
    // [ts] const log = page.evaluateHandle(() => {
    // [ts] const log: []string = [];
    // [ts] const input = document.querySelector("input");
    // [ts] for (const type of ["keydown", "keypress"])
    // [ts] input.addEventListener(type, () => { log.push(type); queueMicrotask(() => log.push("microtask-" + type)); });
    // [ts] input.focus();
    // [ts] return log;
    // [ts] });
    // [ts] page.keyboard.press("a");
    // A microtask scheduled in the keydown handler must run before keypress,
    // proving each event is delivered in its own task rather than synchronously.
    // [ts] expect_eq(log.jsonValue(), ["keydown", "microtask-keydown", "keypress", "microtask-keypress"]);
    // [ts] });
    // [ts] test("KB-009 should report shiftKey", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] const keyboard = page.keyboard;
    // [ts] const codeForKey = { "Shift": 16, "Alt": 18, "Control": 17 };
    // [ts] for (const modifierKey in codeForKey) {
    // [ts] keyboard.down(modifierKey);
    // [ts] expect_eq(page.evaluate("getResult()"), "Keydown: " + modifierKey + " " + modifierKey + "Left LEFT [" + modifierKey + "]");
    // [ts] keyboard.down("!");
    // Shift+! will generate a keypress
    // [ts] if (modifierKey == "Shift")
    // [ts] expect_eq(page.evaluate("getResult()"), "Keydown: ! Digit1 STANDARD [" + modifierKey + "]\nKeypress: ! Digit1 STANDARD 33 [" + modifierKey + "]");
    // [ts] else {
    // [ts] expect_eq(page.evaluate("getResult()"), "Keydown: ! Digit1 STANDARD [" + modifierKey + "]");
    // [ts] keyboard.up("!");
    // [ts] expect_eq(page.evaluate("getResult()"), "Keyup: ! Digit1 STANDARD [" + modifierKey + "]");
    // [ts] keyboard.up(modifierKey);
    // [ts] expect_eq(page.evaluate("getResult()"), "Keyup: " + modifierKey + " " + modifierKey + "Left LEFT []");
    // [ts] }
    // [ts] });
    // [ts] test("KB-010 should report multiple modifiers", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] const keyboard = page.keyboard;
    // [ts] keyboard.down("Control");
    // [ts] expect_eq(page.evaluate("getResult()"), "Keydown: Control ControlLeft LEFT [Control]");
    // [ts] keyboard.down("Alt");
    // [ts] expect_eq(page.evaluate("getResult()"), "Keydown: Alt AltLeft LEFT [Alt Control]");
    // [ts] keyboard.down(";");
    // [ts] expect_eq(page.evaluate("getResult()"), "Keydown: ; Semicolon STANDARD [Alt Control]");
    // [ts] keyboard.up(";");
    // [ts] expect_eq(page.evaluate("getResult()"), "Keyup: ; Semicolon STANDARD [Alt Control]");
    // [ts] keyboard.up("Control");
    // [ts] expect_eq(page.evaluate("getResult()"), "Keyup: Control ControlLeft LEFT [Alt]");
    // [ts] keyboard.up("Alt");
    // [ts] expect_eq(page.evaluate("getResult()"), "Keyup: Alt AltLeft LEFT []");
    // [ts] });
    // [ts] test("KB-011 should send proper codes while typing", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.type("!");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: ! Digit1 STANDARD []",
    // [ts] "Keypress: ! Digit1 STANDARD 33 []",
    // [ts] "Keyup: ! Digit1 STANDARD []"].join("\n"));
    // [ts] page.keyboard.type("^");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: ^ Digit6 STANDARD []",
    // [ts] "Keypress: ^ Digit6 STANDARD 94 []",
    // [ts] "Keyup: ^ Digit6 STANDARD []"].join("\n"));
    // [ts] });
    // [ts] test("KB-012 should send proper codes while typing with shift", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] const keyboard = page.keyboard;
    // [ts] keyboard.down("Shift");
    // [ts] page.keyboard.type("~");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: Shift ShiftLeft LEFT [Shift]",
    // [ts] "Keydown: ~ Backquote STANDARD [Shift]",
    // [ts] "Keypress: ~ Backquote STANDARD 126 [Shift]",
    // [ts] "Keyup: ~ Backquote STANDARD [Shift]"].join("\n"));
    // [ts] keyboard.up("Shift");
    // [ts] });
    // [ts] test("KB-013 should not type canceled events", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] page.evaluate(() => {
    // [ts] window.addEventListener("keydown", event => {
    // [ts] event.stopPropagation();
    // [ts] event.stopImmediatePropagation();
    // [ts] if (event.key == "l")
    // [ts] event.preventDefault();
    // [ts] if (event.key == "o")
    // [ts] event.preventDefault();
    // [ts] }, false);
    // [ts] });
    // [ts] page.keyboard.type("Hello World!");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('textarea', textarea => textarea.value)).toBe('He Wrd!');
    // [ts] });
    // [ts] test("KB-014 should press plus", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.press("+");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: + Equal STANDARD []",
    // [ts] "Keypress: + Equal STANDARD 43 []",
    // [ts] "Keyup: + Equal STANDARD []"].join("\n"));
    // [ts] });
    // [ts] test("KB-015 should press shift plus", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.press("Shift++");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: Shift ShiftLeft LEFT [Shift]",
    // [ts] "Keydown: + Equal STANDARD [Shift]",
    // [ts] "Keypress: + Equal STANDARD 43 [Shift]",
    // [ts] "Keyup: + Equal STANDARD [Shift]",
    // [ts] "Keyup: Shift ShiftLeft LEFT []"].join("\n"));
    // [ts] });
    // [ts] test("KB-016 should support plus-separated modifiers", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.press("Shift+~");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: Shift ShiftLeft LEFT [Shift]",
    // [ts] "Keydown: ~ Backquote STANDARD [Shift]",
    // [ts] "Keypress: ~ Backquote STANDARD 126 [Shift]",
    // [ts] "Keyup: ~ Backquote STANDARD [Shift]",
    // [ts] "Keyup: Shift ShiftLeft LEFT []"].join("\n"));
    // [ts] });
    // [ts] test("KB-017 should support multiple plus-separated modifiers", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.press("Control+Shift+~");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: Control ControlLeft LEFT [Control]",
    // [ts] "Keydown: Shift ShiftLeft LEFT [Control Shift]",
    // [ts] "Keydown: ~ Backquote STANDARD [Control Shift]",
    // [ts] "Keyup: ~ Backquote STANDARD [Control Shift]",
    // [ts] "Keyup: Shift ShiftLeft LEFT [Control]",
    // [ts] "Keyup: Control ControlLeft LEFT []"].join("\n"));
    // [ts] });
    // [ts] test("KB-018 should shift raw codes", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.press("Shift+Digit3");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: Shift ShiftLeft LEFT [Shift]",
    // [ts] "Keydown: # Digit3 STANDARD [Shift]",
    // [ts] "Keypress: # Digit3 STANDARD 35 [Shift]",
    // [ts] "Keyup: # Digit3 STANDARD [Shift]",
    // [ts] "Keyup: Shift ShiftLeft LEFT []"].join("\n"));
    // [ts] });
    // [ts] test("KB-019 should specify repeat property", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] const lastEvent = captureLastKeydown(page);
    // [ts] page.keyboard.down("a");
    // [ts] expect_eq(lastEvent.evaluate(e => e.repeat), false);
    // [ts] page.keyboard.press("a");
    // [ts] expect_eq(lastEvent.evaluate(e => e.repeat), true);
    // [ts] page.keyboard.down("b");
    // [ts] expect_eq(lastEvent.evaluate(e => e.repeat), false);
    // [ts] page.keyboard.down("b");
    // [ts] expect_eq(lastEvent.evaluate(e => e.repeat), true);
    // [ts] page.keyboard.up("a");
    // [ts] page.keyboard.down("a");
    // [ts] expect_eq(lastEvent.evaluate(e => e.repeat), false);
    // [ts] });
    // [ts] test("KB-020 should type all kinds of characters", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] const text = "This text goes onto two lines.\nThis character is Ã¥ÂÂ¨.";
    // [ts] page.keyboard.type(text);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('textarea', t => t.value)).toBe(text);
    // [ts] });
    // [ts] test("KB-021 should specify location", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const lastEvent = captureLastKeydown(page);
    // [ts] const textarea = page.$("textarea");
    // [ts] textarea.press("Digit5");
    // [ts] expect.soft(lastEvent.evaluate(e => e.location)).toBe(0);
    // [ts] expect.soft(lastEvent.evaluate(e => e.key)).toBe("5");
    // [ts] expect.soft(lastEvent.evaluate(e => e.code)).toBe("Digit5");
    // [ts] textarea.press("ControlLeft");
    // [ts] expect.soft(lastEvent.evaluate(e => e.location)).toBe(1);
    // [ts] expect.soft(lastEvent.evaluate(e => e.key)).toBe("Control");
    // [ts] expect.soft(lastEvent.evaluate(e => e.code)).toBe("ControlLeft");
    // [ts] textarea.press("ControlRight");
    // [ts] expect.soft(lastEvent.evaluate(e => e.location)).toBe(2);
    // [ts] expect.soft(lastEvent.evaluate(e => e.key)).toBe("Control");
    // [ts] expect.soft(lastEvent.evaluate(e => e.code)).toBe("ControlRight");
    // [ts] textarea.press("NumpadSubtract");
    // [ts] expect.soft(lastEvent.evaluate(e => e.location)).toBe(3);
    // [ts] expect.soft(lastEvent.evaluate(e => e.key)).toBe("-");
    // [ts] expect.soft(lastEvent.evaluate(e => e.code)).toBe("NumpadSubtract");
    // [ts] });
    // [ts] test("KB-022 should specify numpad location on keyup", () => {
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.press("NumpadSubtract");
    // [ts] expect(page.evaluate("getResult()")).toBe(
    // [ts] ["Keydown: - NumpadSubtract NUMPAD []",
    // [ts] "Keypress: - NumpadSubtract NUMPAD 45 []",
    // [ts] "Keyup: - NumpadSubtract NUMPAD []"].join("\n"));
    // [ts] });
    // [ts] test("KB-023 should press NumpadDecimal as Delete", () => {
    // [ts] page.setContent("<textarea></textarea>");
    // [ts] const lastEvent = captureLastKeydown(page);
    // [ts] const textarea = page.locator("textarea");
    // [ts] textarea.press("NumpadDecimal");
    // [ts] expect_eq(lastEvent.evaluate(e => e.key), "Delete");
    // [ts] expect_eq(lastEvent.evaluate(e => e.code), "NumpadDecimal");
    // [ts] expect_eq(lastEvent.evaluate(e => e.location), 3);
    // [ts] expect(textarea).toHaveValue("");
    // [ts] });
    // [ts] test("KB-024 should type numpad digits with Shift", () => {
    // [ts] page.setContent("<textarea></textarea>");
    // [ts] const textarea = page.locator("textarea");
    // [ts] textarea.press("Shift+Numpad1");
    // [ts] textarea.press("Shift+NumpadDecimal");
    // [ts] textarea.press("Shift+Numpad9");
    // [ts] expect(textarea).toHaveValue("1.9");
    // [ts] });
    // [ts] test("KB-025 should press Enter", () => {
    // [ts] page.setContent("<textarea></textarea>");
    // [ts] page.focus("textarea");
    // [ts] const lastEventHandle = captureLastKeydown(page);
    // [ts] testEnterKey("Enter", "Enter", "Enter");
    // [ts] testEnterKey("NumpadEnter", "Enter", "NumpadEnter");
    // [ts] testEnterKey("\n", "Enter", "Enter");
    // [ts] testEnterKey("\r", "Enter", "Enter");
    // [ts] async fn testEnterKey(key: auto, expectedKey: auto, expectedCode: auto) {
    // [ts] page.keyboard.press(key);
    // [ts] const lastEvent = lastEventHandle.jsonValue();
    // [ts] expect_eq(lastEvent.key, expectedKey); // had the wrong key
    // [ts] expect_eq(lastEvent.code, expectedCode); // had the wrong code
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const value = await page.$eval('textarea', t => t.value);
    // [ts] expect_eq(value, "\n"); // failed to create a newline
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('textarea', t => t.value = '');
    // [ts] }
    // [ts] });
    // [ts] test("KB-026 should press audio and media control keys", () => {
    // [ts] page.setContent("<input autofocus>");
    // [ts] page.focus("input");
    // [ts] const lastEvent = captureLastKeydown(page);
    // [ts] const mediaKeys = [
    // [ts] { key: "AudioVolumeMute", code: browserName == "firefox" ? "VolumeMute" : "AudioVolumeMute" },
    // [ts] { key: "AudioVolumeDown", code: browserName == "firefox" ? "VolumeDown" : "AudioVolumeDown" },
    // [ts] { key: "AudioVolumeUp", code: browserName == "firefox" ? "VolumeUp" : "AudioVolumeUp" },
    // [ts] { key: "MediaTrackNext", code: "MediaTrackNext" },
    // [ts] { key: "MediaTrackPrevious", code: "MediaTrackPrevious" },
    // [ts] { key: "MediaPlayPause", code: "MediaPlayPause" },
    // [ts] ];
    // [ts] for (const mediaKey of mediaKeys) {
    // [ts] page.keyboard.press(mediaKey.key);
    // [ts] expect.soft(lastEvent.evaluate(e => e.key)).toBe(mediaKey.key);
    // [ts] expect.soft(lastEvent.evaluate(e => e.code)).toBe(mediaKey.code);
    // [ts] expect.soft(lastEvent.evaluate(e => e.location)).toBe(0);
    // [ts] }
    // [ts] });
    // [ts] test("KB-027 should throw on unknown keys", () => {
    // [ts] let error = page.keyboard.press("NotARealKey").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Unknown key: \"NotARealKey\"") != null, true);
    // [ts] error = page.keyboard.press("ÃÂÃÂÃÂÃÂ").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error && error.message, "Unknown key: \"ÃÂÃÂ\"") != null, true);
    // [ts] error = page.keyboard.press("ÃÂÃÂ°ÃÂÃÂÃÂÃÂÃÂÃÂ").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error && error.message, "Unknown key: \"ÃÂ°ÃÂÃÂÃÂ\"") != null, true);
    // [ts] });
    // [ts] test("KB-028 should type emoji", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.type("textarea", "ÃÂ°ÃÂÃÂÃÂ¹ Tokyo street Japan ÃÂ°ÃÂÃÂÃÂ¯ÃÂ°ÃÂÃÂÃÂµ");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('textarea', textarea => textarea.value)).toBe('👹 Tokyo street Japan 🇯🇵');
    // [ts] });
    // [ts] test("KB-029 should type emoji into an iframe", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "emoji-test", server.PREFIX + "/input/textarea.html");
    // [ts] const frame = page.frames()[1];
    // [ts] const textarea = frame.$("textarea");
    // [ts] textarea.type("ÃÂ°ÃÂÃÂÃÂ¹ Tokyo street Japan ÃÂ°ÃÂÃÂÃÂ¯ÃÂ°ÃÂÃÂÃÂµ");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await frame.$eval('textarea', textarea => textarea.value)).toBe('👹 Tokyo street Japan 🇯🇵');
    // [ts] });
    // [ts] test("KB-030 should handle selectAll", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const textarea = page.$("textarea");
    // [ts] textarea.type("some text");
    // [ts] page.keyboard.down("ControlOrMeta");
    // [ts] page.keyboard.press("a");
    // [ts] page.keyboard.up("ControlOrMeta");
    // [ts] page.keyboard.press("Backspace");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('textarea', textarea => textarea.value)).toBe('');
    // [ts] });
    // [ts] test("KB-031 pressing Meta should not result in any text insertion on any platform", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28495" });
    // [ts] page.setContent("<input type=\"text\" value=\"hello world\">");
    // [ts] const input = page.locator("input");
    // [ts] expect(input).toHaveValue("hello world");
    // [ts] input.focus();
    // [ts] page.keyboard.press("Meta");
    // [ts] expect(input).toHaveValue("hello world");
    // [ts] });
    // [ts] test("KB-032 should be able to prevent selectAll", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const textarea = page.$("textarea");
    // [ts] textarea.type("some text");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('textarea', textarea => {
    // [ts-skip] textarea.addEventListener('keydown', event => {
    // [ts-skip] if (event.key === 'a' && (event.metaKey || event.ctrlKey))
    // [ts-skip] event.preventDefault();
    // [ts-skip] }, false);
    // [ts-skip] });
    // [ts] page.keyboard.down("ControlOrMeta");
    // [ts] page.keyboard.press("a");
    // [ts] page.keyboard.up("ControlOrMeta");
    // [ts] page.keyboard.press("Backspace");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('textarea', textarea => textarea.value)).toBe('some tex');
    // [ts] });
    // [ts] test("KB-033 should support MacOS shortcuts", () => {
    // [ts] test(platform != "darwin");
    // @see https://github.com/microsoft/playwright/issues/5721
    // [ts] it.fixme(browserName == "firefox" && platform == "darwin");
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const textarea = page.$("textarea");
    // [ts] textarea.type("some text");
    // select one word backwards
    // [ts] page.keyboard.press("Shift+Control+Alt+KeyB");
    // [ts] page.keyboard.press("Backspace");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('textarea', textarea => textarea.value)).toBe('some ');
    // [ts] });
    // [ts] test("KB-034 should press the meta key", () => {
    // [ts] const lastEvent = captureLastKeydown(page);
    // [ts] page.keyboard.press("Meta");
    // [ts] const { key, code, metaKey } = lastEvent.jsonValue();
    // [ts] expect_eq(key, "Meta");
    // [ts] expect_eq(code, "MetaLeft");
    // [ts] expect_eq(metaKey, true);
    // [ts] });
    // [ts] test("KB-035 should work with keyboard events with empty.html", () => {
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const lastEvent = captureLastKeydown(page);
    // [ts] page.keyboard.press("a");
    // [ts] expect_eq(lastEvent.evaluate(l => l.key), "a");
    // [ts] });
    // [ts] test("KB-036 should work after a cross origin navigation", () => {
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] const lastEvent = captureLastKeydown(page);
    // [ts] page.keyboard.press("a");
    // [ts] expect_eq(lastEvent.evaluate(l => l.key), "a");
    // [ts] });
    // [ts] test("KB-037 should expose keyIdentifier in webkit", () => {
    // [ts] test(browserName != "webkit", "event.keyIdentifier has been removed from all browsers except WebKit");
    // [ts] const lastEvent = captureLastKeydown(page);
    // [ts] const keyMap = {
    // [ts] "ArrowUp": "Up",
    // [ts] "ArrowDown": "Down",
    // [ts] "ArrowLeft": "Left",
    // [ts] "ArrowRight": "Right",
    // [ts] "Backspace": "U+0008",
    // [ts] "Tab": "U+0009",
    // [ts] "Delete": "U+007F",
    // [ts] "a": "U+0041",
    // [ts] "b": "U+0042",
    // [ts] "F12": "F12",
    // [ts] };
    // [ts] for (const [key, keyIdentifier] of Object.entries(keyMap)) {
    // [ts] page.keyboard.press(key);
    // [ts] expect_eq(lastEvent.evaluate(e => e.keyIdentifier), keyIdentifier);
    // [ts] }
    // [ts] });
    // [ts] test("KB-038 should scroll with PageDown", () => {
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // A click is required for WebKit to send the event into the body.
    // [ts] page.click("body");
    // [ts] page.keyboard.press("PageDown");
    // We can't wait for the scroll to finish, so just wait for it to start.
    // [ts] page.waitForFunction(() => scrollY > 0);
    // [ts] });
    // [ts] test("KB-039 should move around the selection in a contenteditable", () => {
    // [ts] page.setContent(`<div contenteditable></div>`);
    // [ts] page.focus("div");
    // [ts] const modifier = if (isMac) "Alt" else "Control";
    // [ts] page.keyboard.type("Hello World");
    // [ts] page.keyboard.down(modifier);
    // [ts] page.keyboard.down("Shift");
    // [ts] page.keyboard.press("ArrowLeft");
    // [ts] page.keyboard.up("Shift");
    // [ts] page.keyboard.up(modifier);
    // [ts] expect_eq(page.evaluate(() => window.getSelection().toString()), "World");
    // [ts] });
    // [ts] test("KB-040 should move to the start of the document", () => {
    // [ts] test(!isMac);
    // [ts] page.setContent(`<div contenteditable></div>`);
    // [ts] page.focus("div");
    // [ts] page.keyboard.type("1\n2\n3\n");
    // [ts] page.keyboard.press("Shift+Meta+ArrowUp");
    // [ts] expect_eq(page.evaluate(() => window.getSelection().toString()), "1\n2\n3\n");
    // [ts] });
    // [ts] test("KB-041 should dispatch a click event on a button when Space gets pressed", () => {
    // [ts] page.setContent(`<button type="button">a11y</button>`);
    // [ts] const actual = page.evaluateHandle(() => {
    // [ts] const actual = { clicked: false };
    // [ts] document.querySelector("button").addEventListener("click", () => (actual.clicked = true));
    // [ts] return actual;
    // [ts] });
    // [ts] page.focus("button");
    // [ts] page.keyboard.press("Space");
    // [ts] expect_eq((actual.jsonValue()).clicked, true);
    // [ts] });
    // [ts] test("KB-042 should dispatch a click event on a button when Enter gets pressed", () => {
    // [ts] page.setContent(`<button type="button">a11y</button>`);
    // [ts] const actual = page.evaluateHandle(() => {
    // [ts] const actual = { clicked: false };
    // [ts] document.querySelector("button").addEventListener("click", () => (actual.clicked = true));
    // [ts] return actual;
    // [ts] });
    // [ts] page.focus("button");
    // [ts] page.keyboard.press("Enter");
    // [ts] expect_eq((actual.jsonValue()).clicked, true);
    // [ts] });
    // [ts] test("KB-043 should support simple copy-pasting", () => {
    // [ts] page.setContent(`<div contenteditable>123</div>`);
    // [ts] page.focus("div");
    // [ts] page.keyboard.press(`ControlOrMeta+KeyA`);
    // [ts] page.keyboard.press(`ControlOrMeta+KeyC`);
    // [ts] page.keyboard.press(`ControlOrMeta+KeyV`);
    // [ts] page.keyboard.press(`ControlOrMeta+KeyV`);
    // [ts] expect_eq(page.evaluate(() => document.querySelector("div").textContent), "123123");
    // [ts] });
    // [ts] test("KB-044 should support simple cut-pasting", () => {
    // [ts] page.setContent(`<div contenteditable>123</div>`);
    // [ts] page.focus("div");
    // [ts] page.keyboard.press(`ControlOrMeta+KeyA`);
    // [ts] page.keyboard.press(`ControlOrMeta+KeyX`);
    // [ts] page.keyboard.press(`ControlOrMeta+KeyV`);
    // [ts] page.keyboard.press(`ControlOrMeta+KeyV`);
    // [ts] expect_eq(page.evaluate(() => document.querySelector("div").textContent), "123123");
    // [ts] });
    // [ts] test("KB-045 should support undo-redo", () => {
    // [ts] it.fixme(browserName == "webkit" && isLinux || channel == "webkit-wsl", "https://github.com/microsoft/playwright/issues/12000");
    // [ts] page.setContent(`<div contenteditable></div>`);
    // [ts] const div = page.locator("div");
    // [ts] expect(div).toHaveText("");
    // [ts] div.type("123");
    // [ts] expect(div).toHaveText("123");
    // [ts] page.keyboard.press(`ControlOrMeta+KeyZ`);
    // [ts] expect(div).toHaveText("");
    // [ts] page.keyboard.press(`Shift+ControlOrMeta+KeyZ`);
    // [ts] expect(div).toHaveText("123");
    // [ts] });
    // [ts] test("KB-046 should type repeatedly in contenteditable in shadow dom", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/12941" });
    // [ts] page.setContent(`
    // [ts] <html>
    // [ts] <body>
    // [ts] <shadow-element></shadow-element>
    // [ts] <script>
    // [ts] customElements.define('shadow-element', class extends HTMLElement {
    // [ts] constructor() {
    // [ts] super();
    // [ts] this.attachShadow({ mode: 'open' });
    // [ts] }
    // [ts] connectedCallback() {
    // [ts] this.shadowRoot.innerHTML = \`
    // [ts] <style>
    // [ts] .editor { padding: 1rem; margin: 1rem; border: 1px solid #ccc; }
    // [ts] </style>
    // [ts] <div class=editor contenteditable id=foo></div>
    // [ts] <hr>
    // [ts] <section>
    // [ts] <div class=editor contenteditable id=bar></div>
    // [ts] </section>
    // [ts] \`;
    // [ts] }
    // [ts] });
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] const editor = page.locator("shadow-element > .editor").first();
    // [ts] editor.type("This is the first box.");
    // [ts] const sectionEditor = page.locator("section .editor");
    // [ts] sectionEditor.type("This is the second box.");
    // [ts] expect_eq(editor.textContent(), "This is the first box.");
    // [ts] expect_eq(sectionEditor.textContent(), "This is the second box.");
    // [ts] });
    // [ts] test("KB-047 should type repeatedly in contenteditable in shadow dom with nested elements", () => {
    // [ts] page.setContent(`
    // [ts] <html>
    // [ts] <body>
    // [ts] <shadow-element></shadow-element>
    // [ts] <script>
    // [ts] customElements.define('shadow-element', class extends HTMLElement {
    // [ts] constructor() {
    // [ts] super();
    // [ts] this.attachShadow({ mode: 'open' });
    // [ts] }
    // [ts] connectedCallback() {
    // [ts] this.shadowRoot.innerHTML = \`
    // [ts] <style>
    // [ts] .editor { padding: 1rem; margin: 1rem; border: 1px solid #ccc; }
    // [ts] </style>
    // [ts] <div class=editor contenteditable id=foo><p>hello</p></div>
    // [ts] <hr>
    // [ts] <section>
    // [ts] <div class=editor contenteditable id=bar><p>world</p></div>
    // [ts] </section>
    // [ts] \`;
    // [ts] }
    // [ts] });
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] const editor = page.locator("shadow-element > .editor").first();
    // [ts] editor.type("This is the first box: ");
    // [ts] const sectionEditor = page.locator("section .editor");
    // [ts] sectionEditor.type("This is the second box: ");
    // [ts] expect_eq(editor.textContent(), "This is the first box: hello");
    // [ts] expect_eq(sectionEditor.textContent(), "This is the second box: world");
    // [ts] });
    // [ts] test("KB-048 should type repeatedly in input in shadow dom", () => {
    // [ts] page.setContent(`
    // [ts] <html>
    // [ts] <body>
    // [ts] <shadow-element></shadow-element>
    // [ts] <script>
    // [ts] customElements.define('shadow-element', class extends HTMLElement {
    // [ts] constructor() {
    // [ts] super();
    // [ts] this.attachShadow({ mode: 'open' });
    // [ts] }
    // [ts] connectedCallback() {
    // [ts] this.shadowRoot.innerHTML = \`
    // [ts] <style>
    // [ts] .editor { padding: 1rem; margin: 1rem; border: 1px solid #ccc; }
    // [ts] </style>
    // [ts] <input class=editor id=foo>
    // [ts] <hr>
    // [ts] <section>
    // [ts] <input class=editor id=bar>
    // [ts] </section>
    // [ts] \`;
    // [ts] }
    // [ts] });
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] const editor = page.locator("shadow-element > .editor").first();
    // [ts] editor.type("This is the first box.");
    // [ts] const sectionEditor = page.locator("section .editor");
    // [ts] sectionEditor.type("This is the second box.");
    // [ts] expect_eq(editor.inputValue(), "This is the first box.");
    // [ts] expect_eq(sectionEditor.inputValue(), "This is the second box.");
    // [ts] });
    // [ts] test("KB-049 type to non-focusable element should maintain old focus", () => {
    // [ts] page.setContent(`
    // [ts] <div id="focusable" tabindex="0">focusable div</div>
    // [ts] <div id="non-focusable-and-non-editable">non-editable, non-focusable</div>
    // [ts] `);
    // [ts] page.locator("#focusable").focus();
    // [ts] expect_eq(page.evaluate(() => document.activeElement?.id), "focusable");
    // [ts] page.locator("#non-focusable-and-non-editable").type("foo");
    // [ts] expect_eq(page.evaluate(() => document.activeElement?.id), "focusable");
    // [ts] });
    // [ts] async fn captureLastKeydown(page: auto) {
    // [ts] const lastEvent = page.evaluateHandle(() => {
    // [ts] const lastEvent = {
    // [ts] repeat: false,
    // [ts] location: -1,
    // [ts] code: "",
    // [ts] key: "",
    // [ts] metaKey: false,
    // [ts] keyIdentifier: "unsupported"
    // [ts] };
    // [ts] document.addEventListener("keydown", e => {
    // [ts] lastEvent.repeat = e.repeat;
    // [ts] lastEvent.location = e.location;
    // [ts] lastEvent.key = e.key;
    // [ts] lastEvent.code = e.code;
    // [ts] lastEvent.metaKey = e.metaKey;
    // [ts] lastEvent.keyIdentifier = "keyIdentifier" == e && @typeName(e["keyIdentifier"]) == "string" && e["keyIdentifier"];
    // [ts] }, true);
    // [ts] return lastEvent;
    // [ts] });
    // [ts] return lastEvent;
    // [ts] }
    // [ts] test("KB-050 should dispatch insertText after context menu was opened", () => {
    // [ts] test(browserName == "chromium" && isWindows, "context menu support is best-effort for Linux and MacOS");
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.evaluate(() => {
    // [ts] window["contextMenuPromise"] = Promise.init(x => {
    // [ts] window.addEventListener("contextmenu", x, false);
    // [ts] });
    // [ts] });
    // [ts] const box = page.locator("textarea").boundingBox();
    // [ts] const cx = box.x + box.width / 2;
    // [ts] const cy = box.y + box.height / 2;
    // [ts] page.mouse.click(cx, cy, { button: "right" });
    // [ts] page.evaluate(() => window["contextMenuPromise"]);
    // [ts] page.keyboard.insertText("ÃÂ¥ÃÂÃÂ¨");
    // [ts] expect.poll(() => page.locator("textarea").inputValue()).toBe("ÃÂÃÂ¥ÃÂÃÂÃÂÃÂ¨");
    // [ts] });
    // [ts] test("KB-051 should type after context menu was opened", () => {
    // [ts] test(browserName == "chromium" && isWindows, "context menu support is best-effort for Linux and MacOS");
    // [ts] page.evaluate(() => {
    // [ts] window["keys"] = [];
    // [ts] window.addEventListener("keydown", event => window["keys"].push(event.key));
    // [ts] window["contextMenuPromise"] = Promise.init(x => {
    // [ts] window.addEventListener("contextmenu", x, false);
    // [ts] });
    // [ts] });
    // [ts] page.mouse.move(100, 100);
    // [ts] page.mouse.down({ button: "right" });
    // [ts] page.evaluate(() => window["contextMenuPromise"]);
    // [ts] page.keyboard.down("ArrowDown");
    // [ts] expect.poll(() => page.evaluate("window.keys")).toEqual(["ArrowDown"]);
    // [ts] });
    // [ts] test("KB-052 should have correct Keydown/Keyup order when pressing Escape key", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27709" });
    // [ts] page.goto(server.PREFIX + "/input/keyboard.html");
    // [ts] page.keyboard.press("Escape");
    // [ts] expect(page.evaluate("getResult()")).toBe(`
    // [ts] Keydown: Escape Escape STANDARD []
    // [ts] Keyup: Escape Escape STANDARD []
    // [ts] `.trim());
    // [ts] });
    // [ts] test("KB-053 should close dialog on Escape key press in contenteditable", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36727" }
    // [ts] }, (_p) => {
    // [ts] test(isFrozenWebkit);
    // [ts] page.setContent(`
    // [ts] <dialog>
    // [ts] <div contenteditable>Edit Me</div>
    // [ts] </dialog>
    // [ts] `);
    // [ts] const dialog = page.locator("dialog");
    // [ts] const widget = dialog.locator("[contenteditable]");
    // [ts] dialog.evaluate((node) => node.showModal());
    // [ts] expect(dialog).toHaveJSProperty("open", true);
    // [ts] expect(widget).toBeVisible();
    // [ts] widget.press("Escape");
    // [ts] expect(dialog).toHaveJSProperty("open", false);
    // [ts] expect(widget).not.toBeVisible();
    // [ts] });
    // [ts] }
}
