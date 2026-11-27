// Migrated from TypeScript (mcp/request-blocking.spec.ts) via cap from -ts

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

import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { test, expect } from "./fixtures.cp";

const BLOCK_MESSAGE = "Blocked by Web Inspector|NS_ERROR_FAILURE|net::ERR_BLOCKED_BY_CLIENT";

async fn fetchPage(client: Client, url: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] return client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url,
    // [ts] },
    // [ts] });
}

async fn fetchPageText(client: Client, url: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] const result = fetchPage(client, url);
    // [ts] return JSON.stringify(result, null, 2);
}














fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("default to allow all", () => {
    // [ts] server.setContent("/ppp", "content:PPP", "text/html");
    // [ts] expect: (fetchPage(client, server.PREFIX + '/ppp')) => {
    // [ts] snapshot: expect.stringContaining("content:PPP"),
    // [ts] });
    // [ts] });
    // [ts] test("blocked works (hostname)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--blocked-origins", "microsoft.com;example.com;playwright.dev"]
    // [ts] });
    // [ts] const result = fetchPageText(client, "https://example.com/");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
    // [ts] test("blocked works (origin)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--blocked-origins", "https://microsoft.com;https://example.com;https://playwright.dev"]
    // [ts] });
    // [ts] const result = fetchPageText(client, "https://example.com/");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
    // [ts] test("allowed works (hostname)", () => {
    // [ts] server.setContent("/ppp", "content:PPP", "text/html");
    // [ts] const { client } = startClient({
    // [ts] args: ["--allowed-origins", `microsoft.com;${URL.init(server.PREFIX).host};playwright.dev`]
    // [ts] });
    // [ts] expect: (fetchPage(client, server.PREFIX + '/ppp')) => {
    // [ts] snapshot: expect.stringContaining("content:PPP"),
    // [ts] });
    // [ts] });
    // [ts] test("allowed works (origin)", () => {
    // [ts] server.setContent("/ppp", "content:PPP", "text/html");
    // [ts] const { client } = startClient({
    // [ts] args: ["--allowed-origins", `https://microsoft.com;${URL.init(server.PREFIX).origin};https://playwright.dev`]
    // [ts] });
    // [ts] expect: (fetchPage(client, server.PREFIX + '/ppp')) => {
    // [ts] snapshot: expect.stringContaining("content:PPP"),
    // [ts] });
    // [ts] });
    // [ts] test("blocked takes precedence (hostname)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: [
    // [ts] "--blocked-origins", "example.com",
    // [ts] "--allowed-origins", "example.com",
    // [ts] ],
    // [ts] });
    // [ts] const result = fetchPageText(client, "https://example.com/");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
    // [ts] test("blocked takes precedence (origin)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: [
    // [ts] "--blocked-origins", "https://example.com",
    // [ts] "--allowed-origins", "https://example.com",
    // [ts] ],
    // [ts] });
    // [ts] const result = fetchPageText(client, "https://example.com/");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
    // [ts] test("allowed without blocked blocks all non-explicitly specified origins (hostname)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--allowed-origins", "playwright.dev"],
    // [ts] });
    // [ts] const result = fetchPageText(client, "https://example.com/");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
    // [ts] test("allowed without blocked blocks all non-explicitly specified origins (origin)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--allowed-origins", "https://playwright.dev"],
    // [ts] });
    // [ts] const result = fetchPageText(client, "https://example.com/");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
    // [ts] test("blocked without allowed allows non-explicitly specified origins (hostname)", () => {
    // [ts] server.setContent("/ppp", "content:PPP", "text/html");
    // [ts] const { client } = startClient({
    // [ts] args: ["--blocked-origins", "example.com"],
    // [ts] });
    // [ts] expect: (fetchPage(client, server.PREFIX + '/ppp')) => {
    // [ts] snapshot: expect.stringContaining("content:PPP"),
    // [ts] });
    // [ts] });
    // [ts] test("blocked without allowed allows non-explicitly specified origins (origin)", () => {
    // [ts] server.setContent("/ppp", "content:PPP", "text/html");
    // [ts] const { client } = startClient({
    // [ts] args: ["--blocked-origins", "https://example.com"],
    // [ts] });
    // [ts] expect: (fetchPage(client, server.PREFIX + '/ppp')) => {
    // [ts] snapshot: expect.stringContaining("content:PPP"),
    // [ts] });
    // [ts] });
    // [ts] test("allowed works (wildcard port)", () => {
    // [ts] server.setContent("/ppp", "content:PPP", "text/html");
    // [ts] const { client } = startClient({
    // [ts] args: ["--allowed-origins", "http://localhost:*"]
    // [ts] });
    // [ts] expect: (fetchPage(client, server.PREFIX + '/ppp')) => {
    // [ts] snapshot: expect.stringContaining("content:PPP"),
    // [ts] });
    // [ts] });
    // [ts] test("allowed blocks different hostname (wildcard port)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--allowed-origins", "http://localhost:*"],
    // [ts] });
    // [ts] const result = fetchPageText(client, "http://example.com/");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
    // [ts] test("allowed blocks different protocol (wildcard port)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--allowed-origins", "http://localhost:*"],
    // [ts] });
    // [ts] const result = fetchPageText(client, "https://localhost:8080/");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
    // [ts] test("blocked works (wildcard port)", () => {
    // [ts] server.setContent("/ppp", "content:PPP", "text/html");
    // [ts] const { client } = startClient({
    // [ts] args: ["--blocked-origins", `http://${URL.init(server.PREFIX).host.split(":")[0]}:*`]
    // [ts] });
    // [ts] const result = fetchPageText(client, server.PREFIX + "/ppp");
    // [ts] expect(result).toMatch(BLOCK_MESSAGE);
    // [ts] });
}
