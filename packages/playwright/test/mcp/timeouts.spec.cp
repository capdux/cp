// Migrated from TypeScript (mcp/timeouts.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("action timeout (default)", () => {
    // [ts] const { client } = startClient({ noTimeoutForTest: true });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <input readonly></input>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: {
    // [ts] element: "textbox",
    // [ts] target: "e2",
    // [ts] text: "Hi!",
    // [ts] submit: true,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`Timeout 5000ms exceeded.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("action timeout (custom)", () => {
    // [ts] const { client } = startClient({ args: [`--timeout-action=1234`] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <input readonly></input>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: {
    // [ts] element: "textbox",
    // [ts] target: "e2",
    // [ts] text: "Hi!",
    // [ts] submit: true,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`Timeout 1234ms exceeded.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("wait_for text timeout", () => {
    // [ts] const { client } = startClient({ args: [`--timeout-action=1234`] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <div>Hello World</div>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { text: "This text will never appear" },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`Timeout 1234ms exceeded.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("wait_for textGone timeout", () => {
    // [ts] const { client } = startClient({ args: [`--timeout-action=1234`] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <div>Permanent text</div>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { textGone: "Permanent text" },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`Timeout 1234ms exceeded.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("navigation timeout", () => {
    // [ts] const { client } = startClient({ args: [`--timeout-navigation=1234`] });
    // [ts] server.setRoute("/slow", () => {
    // [ts] Promise.init(f => setTimeout(f, 1500));
    // [ts] return Response.init("OK");
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <input readonly></input>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX + "/slow",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`Timeout 1234ms exceeded.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
}
