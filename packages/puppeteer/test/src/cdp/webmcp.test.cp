// Migrated from TypeScript (src/cdp/webmcp.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2026 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/// <reference types="webmcp-types" />

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {Issue} from "puppeteer";
// [cap-migrate] import { WebMCPTool, WebMCPToolCall, WebMCPToolCallResult, } from "puppeteer-core/internal/cdp/WebMCP.js";

import { setupSeparateTestBrowserHooks } from "../mocha-utils.cp";
import { html, waitEvent } from "../utils.cp";

// [cap-test] describe("Page.webmcp", () => {
  const state = setupSeparateTestBrowserHooks({
    args: ["--enable-features=WebMCP"],
    acceptInsecureCerts: true,
  });














// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should list tools", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // [ts] const toolsAddedPromise = Promise.init(resolve => {
    // [ts] let count = 0;
    // [ts] page.webmcp.on("toolsadded", () => {
    // [ts] count++;
    // [ts] if (count == 2) {
    // [ts] resolve();
    // [ts] }
    // [ts] });
    // [ts] });
    // Register an imperative WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: (_p) => {
    // [ts] return `hello ${text}`;
    // [ts] },
    // [ts] annotations: {readOnlyHint: true, untrustedContentHint: true},
    // [ts] });
    // [ts] });
    // Register a declarative WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.setAttribute("toolname", "declarative-tool-name");
    // [ts] form.setAttribute("tooldescription", "tool description");
    // [ts] form.setAttribute("toolautosubmit", "");
    // [ts] document.body.appendChild(form);
    // [ts] });
    // [ts] toolsAddedPromise;
    // [ts] const tools = page.webmcp.tools();
    // [ts] expect_eq(tools.length, 2);
    // [ts] expect_eq(tools[0].name, "test-tool-1");
    // [ts] expect_eq(tools[0].description, "A test tool 1");
    // [ts] expect: (tools[0]!.inputSchema) => {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] });
    // [ts] expect_eq(tools[0].annotations != null, true);
    // [ts] expect_eq(tools[0].annotations.readOnly, true);
    // [ts] expect_eq(tools[0].annotations.untrustedContent, true);
    // [ts] expect_eq(tools[0].frame, page.mainFrame());
    // [ts] expect_eq(tools[0].formElement == null, true);
    // [ts] expect_eq(tools[0].location != null, true);
    // [ts] expect_eq(tools[1].name, "declarative-tool-name");
    // [ts] expect_eq(tools[1].description, "tool description");
    // [ts] expect: (tools[1]!.inputSchema) => {
    // [ts] type: "object",
    // [ts] properties: {},
    // [ts] required: [],
    // [ts] });
    // [ts] expect_eq(tools[1].annotations != null, true);
    // [ts] expect_eq(tools[1].annotations.autosubmit, true);
    // [ts] expect_eq(tools[1].frame, page.mainFrame());
    // [ts] expect_eq(tools[1].formElement != null, true);
    // [ts] expect_eq(tools[1].location == null, true);
    // [ts] });
    // [ts] test("should fire toolsadded events", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // [ts] const imperativeToolAdded = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsadded", event => {
    // [ts] return resolve(event.tools);
    // [ts] });
    // [ts] });
    // Register an imperative WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: () => {},
    // [ts] });
    // [ts] });
    // [ts] let addedTools = imperativeToolAdded;
    // [ts] expect_eq(addedTools.length, 1);
    // [ts] expect_eq(addedTools[0].name, "test-tool-1");
    // [ts] expect_eq(addedTools[0].description, "A test tool 1");
    // [ts] expect: (addedTools[0]!.inputSchema) => {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] });
    // [ts] expect_eq(addedTools[0].annotations == null, true);
    // [ts] expect_eq(addedTools[0].frame, page.mainFrame());
    // [ts] expect_eq(addedTools[0].formElement == null, true);
    // [ts] expect_eq(addedTools[0].location != null, true);
    // Register a declarative WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.setAttribute("toolname", "declarative-tool-name");
    // [ts] form.setAttribute("tooldescription", "tool description");
    // [ts] document.body.appendChild(form);
    // [ts] });
    // [ts] const declarativeToolAdded = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsadded", event => {
    // [ts] return resolve(event.tools);
    // [ts] });
    // [ts] });
    // [ts] addedTools = declarativeToolAdded;
    // [ts] expect_eq(addedTools.length, 1);
    // [ts] expect_eq(addedTools[0].name, "declarative-tool-name");
    // [ts] expect_eq(addedTools[0].description, "tool description");
    // [ts] expect_eq(addedTools[0].annotations == null, true);
    // [ts] expect: (addedTools[0]!.inputSchema) => {
    // [ts] type: "object",
    // [ts] properties: {},
    // [ts] required: [],
    // [ts] });
    // [ts] expect_eq(addedTools[0].frame, page.mainFrame());
    // [ts] expect_eq(addedTools[0].formElement != null, true);
    // [ts] expect_eq(addedTools[0].location == null, true);
    // [ts] });
    // [ts] test("should fire toolsremoved events", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // Register an imperative WebMCP tool.
    // [ts] using controllerHandle = page.evaluateHandle(() => {
    // [ts] const controller = AbortController.init();
    // [ts] document.modelContext?.registerTool(
    // [ts] {
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: () => {},
    // [ts] },
    // [ts] {signal: controller.signal},
    // [ts] );
    // [ts] return controller;
    // [ts] });
    // [ts] const imperativeToolRemoved = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsremoved", event => {
    // [ts] return resolve(event.tools);
    // [ts] });
    // [ts] });
    // Unregister imperative WebMCP tool.
    // [ts] controllerHandle.evaluate(el => {
    // [ts] el.abort();
    // [ts] });
    // [ts] let removedTools = imperativeToolRemoved;
    // [ts] expect_eq(removedTools.length, 1);
    // [ts] expect_eq(removedTools[0].name, "test-tool-1");
    // [ts] expect_eq(removedTools[0].description, "A test tool 1");
    // [ts] expect: (removedTools[0]!.inputSchema) => {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] });
    // [ts] expect_eq(removedTools[0].annotations == null, true);
    // [ts] expect_eq(removedTools[0].frame, page.mainFrame());
    // [ts] expect_eq(removedTools[0].formElement == null, true);
    // [ts] expect_eq(removedTools[0].location != null, true);
    // Register a declarative WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.setAttribute("toolname", "declarative-tool-name");
    // [ts] form.setAttribute("tooldescription", "tool description");
    // [ts] document.body.appendChild(form);
    // [ts] });
    // [ts] Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsadded", event => {
    // [ts] return resolve(event.tools);
    // [ts] });
    // [ts] });
    // [ts] const declarativeToolRemoved = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsremoved", event => {
    // [ts] return resolve(event.tools);
    // [ts] });
    // [ts] });
    // Unregister declarative WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("form").remove();
    // [ts] });
    // [ts] removedTools = declarativeToolRemoved;
    // [ts] expect_eq(removedTools.length, 1);
    // [ts] expect_eq(removedTools[0].name, "declarative-tool-name");
    // [ts] expect_eq(removedTools[0].description, "tool description");
    // [ts] expect: (removedTools[0]!.inputSchema) => {
    // [ts] type: "object",
    // [ts] properties: {},
    // [ts] required: [],
    // [ts] });
    // [ts] expect_eq(removedTools[0].annotations == null, true);
    // [ts] expect_eq(removedTools[0].frame, page.mainFrame());
    // [ts] expect_eq(removedTools[0].formElement != null, true);
    // [ts] expect_eq(removedTools[0].location == null, true);
    // [ts] });
    // [ts] test("should remove tools on frame navigation", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] const toolsAddedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsadded", () => {
    // [ts] resolve();
    // [ts] });
    // [ts] });
    // Register a declarative WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.setAttribute("toolname", "declarative-tool-name");
    // [ts] form.setAttribute("tooldescription", "tool description");
    // [ts] document.body.appendChild(form);
    // [ts] });
    // [ts] toolsAddedPromise;
    // [ts] const toolsRemovedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsremoved", event => {
    // [ts] return resolve(event.tools);
    // [ts] });
    // [ts] });
    // Reload page forces frame navigation.
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] const removedTools = toolsRemovedPromise;
    // [ts] expect_eq(removedTools.length, 1);
    // [ts] expect_eq(removedTools[0].name, "declarative-tool-name");
    // [ts] expect_eq(page.webmcp.tools().length, 0);
    // [ts] });
    // [ts] test("should handle multiple navigations and report tools correctly", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // 1. Register tool on C1
    // [ts] let toolsAddedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsadded", () => {
    // [ts] return resolve();
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.setAttribute("toolname", "tool-1");
    // [ts] form.setAttribute("tooldescription", "desc-1");
    // [ts] document.body.appendChild(form);
    // [ts] });
    // [ts] toolsAddedPromise;
    // [ts] expect_eq(page.webmcp.tools().length, 1);
    // [ts] expect_eq(page.webmcp.tools()[0].name, "tool-1");
    // 2. Navigate to C2
    // [ts] let toolsRemovedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsremoved", () => {
    // [ts] return resolve();
    // [ts] });
    // [ts] });
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] toolsRemovedPromise;
    // [ts] expect_eq(page.webmcp.tools().length, 0);
    // 3. Register tool on C2
    // [ts] toolsAddedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsadded", () => {
    // [ts] return resolve();
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.setAttribute("toolname", "tool-2");
    // [ts] form.setAttribute("tooldescription", "desc-2");
    // [ts] document.body.appendChild(form);
    // [ts] });
    // [ts] toolsAddedPromise;
    // [ts] expect_eq(page.webmcp.tools().length, 1);
    // [ts] expect_eq(page.webmcp.tools()[0].name, "tool-2");
    // 4. Navigate to C3
    // [ts] toolsRemovedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsremoved", () => {
    // [ts] return resolve();
    // [ts] });
    // [ts] });
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] toolsRemovedPromise;
    // [ts] expect_eq(page.webmcp.tools().length, 0);
    // [ts] });
    // [ts] test("should not reset tools on same-document navigation", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] const toolsAddedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsadded", () => {
    // [ts] return resolve();
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.setAttribute("toolname", "declarative-tool-name");
    // [ts] form.setAttribute("tooldescription", "tool description");
    // [ts] document.body.appendChild(form);
    // [ts] });
    // [ts] toolsAddedPromise;
    // [ts] expect_eq(page.webmcp.tools().length, 1);
    // Same document/hash navigation should not reset tools.
    // [ts] page.goto(httpsServer.EMPTY_PAGE + "#hash");
    // Tools should still be present because context was not destroyed.
    // [ts] expect_eq(page.webmcp.tools().length, 1);
    // [ts] expect_eq(page.webmcp.tools()[0].name, "declarative-tool-name");
    // [ts] });
    // [ts] test("should fire toolinvoked events", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // [ts] const toolAdded = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolsadded", event => {
    // [ts] return resolve(event.tools);
    // [ts] });
    // [ts] });
    // Register a WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: () => {},
    // [ts] });
    // [ts] });
    // [ts] const [addedTool] = toolAdded;
    // [ts] const addedToolCalled = Promise.init(resolve => {
    // [ts] addedTool.once("toolinvoked", resolve);
    // [ts] });
    // [ts] const toolCalled = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolinvoked", resolve);
    // [ts] });
    // Execute WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] const [tool] = document.modelContext.getTools();
    // [ts] void document.modelContext.executeTool(tool, {text: "test"});
    // [ts] });
    // [ts] const [addedToolCall, toolCall] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] addedToolCalled,
    // [ts] toolCalled,
    // [ts] ]);
    // [ts] async fn expectToolCall(call: WebMCPToolCall) {
    // [ts] expect_eq(call.id != null, true);
    // [ts] expect_eq(call.tool != null, true);
    // [ts] expect_eq(call.tool.name, "test-tool-1");
    // [ts] expect_eq(call.tool.description, "A test tool 1");
    // [ts] expect: (call.tool.inputSchema) => {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] });
    // [ts] expect_eq(call.tool.frame, page.mainFrame());
    // [ts] expect_eq(call.tool.formElement == null, true);
    // [ts] expect_eq(call.tool.location != null, true);
    // [ts] expect_eq(call.input, {text: "test"});
    // [ts] }
    // [ts] expectToolCall(addedToolCall);
    // [ts] expectToolCall(toolCall);
    // [ts] });
    // [ts] test("should fire toolresponded event with success", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // Register a WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: (_p) => {
    // [ts] return `hello ${text}`;
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] const toolCalled = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolinvoked", resolve);
    // [ts] });
    // [ts] const toolResponded = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolresponded", resolve);
    // [ts] });
    // Execute WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] const [tool] = document.modelContext.getTools();
    // [ts] void document.modelContext.executeTool(tool, {text: "world"});
    // [ts] });
    // [ts] const call = toolCalled;
    // [ts] const response = toolResponded;
    // [ts] expect_eq(response.id, call.id);
    // [ts] expect_eq(response.call, call);
    // [ts] expect_eq(response.status, "Completed");
    // [ts] expect_eq(response.output, "hello world");
    // [ts] expect_eq(response.errorText == null, true);
    // [ts] expect_eq(response.exception == null, true);
    // [ts] });
    // [ts] test("should fire toolresponded event with exception", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // Register a WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "raise-exception-tool",
    // [ts] description: "A tool that raises JS exception",
    // [ts] execute: () => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('sorry!');)
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] const toolCalled = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolinvoked", resolve);
    // [ts] });
    // [ts] const toolResponded = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolresponded", resolve);
    // [ts] });
    // Execute WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] const [tool] = document.modelContext.getTools();
    // [ts] void document.modelContext.executeTool(tool);
    // [ts] });
    // [ts] const call = toolCalled;
    // [ts] const response = toolResponded;
    // [ts] expect_eq(response.id, call.id);
    // [ts] expect_eq(response.call, call);
    // [ts] expect_eq(response.status, "Error");
    // [ts] expect_eq(response.output == null, true);
    // [ts] expect_eq(response.errorText, "");
    // [ts] expect_eq(response.exception != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, response.exception?.description, "sorry") != null, true);
    // [ts] });
    // [ts] test("should fire toolresponded event with errorText", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // Register a WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: () => {},
    // [ts] });
    // [ts] });
    // [ts] const toolCalled = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolinvoked", resolve);
    // [ts] });
    // [ts] const toolResponded = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolresponded", resolve);
    // [ts] });
    // Execute unknown WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] const [tool] = document.modelContext.getTools();
    // [ts] void document.modelContext.executeTool(tool, {
    // [ts] toJSON: () => {
    // [ts] return "invalid json";
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] const call = toolCalled;
    // [ts] const response = toolResponded;
    // [ts] expect_eq(response.id, call.id);
    // [ts] expect_eq(response.call, call);
    // [ts] expect_eq(response.status, "Error");
    // [ts] expect_eq(response.output == null, true);
    // [ts] expect_eq(response.errorText, "Failed to parse input arguments");
    // [ts] expect_eq(response.exception == null, true);
    // [ts] });
    // [ts] test("should invoke tool", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // [ts] const toolAddedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.on("toolsadded", resolve);
    // [ts] });
    // Register an imperative WebMCP tool.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: (_p) => {
    // [ts] return `hello ${text}`;
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] toolAddedPromise;
    // [ts] const [tool] = page.webmcp.tools();
    // [ts] const toolCalled = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolinvoked", resolve);
    // [ts] });
    // Invoke WebMCP tool.
    // [ts] const response = tool.execute({text: "world"});
    // [ts] const call = toolCalled;
    // [ts] expect_eq(response.id, call.id);
    // [ts] expect_eq(response.call, call);
    // [ts] expect_eq(response.status, "Completed");
    // [ts] expect_eq(response.output, "hello world");
    // [ts] expect_eq(response.errorText == null, true);
    // [ts] expect_eq(response.exception == null, true);
    // [ts] });
    // [ts] test("should cancel tool execution", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // [ts] const toolAddedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.on("toolsadded", resolve);
    // [ts] });
    // Register an imperative WebMCP tool with a delayed response.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: () => {
    // [ts] return Promise.init(resolve => {
    // [ts] setTimeout(() => {
    // [ts] resolve("done");
    // [ts] }, 5000);
    // [ts] });
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] toolAddedPromise;
    // [ts] const [tool] = page.webmcp.tools();
    // [ts] const toolCalled = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolinvoked", resolve);
    // [ts] });
    // [ts] const controller = AbortController.init();
    // [ts] const executePromise = tool.execute(
    // [ts] {text: "world"},
    // [ts] {signal: controller.signal},
    // [ts] );
    // [ts] const call = toolCalled;
    // [ts] controller.abort();
    // [ts] const response = executePromise;
    // [ts] expect_eq(response.id, call.id);
    // [ts] expect_eq(response.call, call);
    // [ts] expect_eq(response.status, "Canceled");
    // [ts] expect_eq(response.output == null, true);
    // [ts] expect_eq(response.errorText, "");
    // [ts] expect_eq(response.exception == null, true);
    // [ts] });
    // [ts] test("should cancel tool execution with already aborted signal", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(page.webmcp != null, true);
    // [ts] const toolAddedPromise = Promise.init(resolve => {
    // [ts] page.webmcp.on("toolsadded", resolve);
    // [ts] });
    // Register an imperative WebMCP tool with a delayed response.
    // [ts] page.evaluate(() => {
    // [ts] document.modelContext?.registerTool({
    // [ts] name: "test-tool-1",
    // [ts] description: "A test tool 1",
    // [ts] inputSchema: {
    // [ts] type: "object",
    // [ts] properties: {
    // [ts] text: {type: "string", description: "Some text"},
    // [ts] },
    // [ts] required: ["text"],
    // [ts] },
    // [ts] execute: () => {
    // [ts] return Promise.init(resolve => {
    // [ts] setTimeout(() => {
    // [ts] resolve("done");
    // [ts] }, 5000);
    // [ts] });
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] toolAddedPromise;
    // [ts] const [tool] = page.webmcp.tools();
    // [ts] const toolCalled = Promise.init(resolve => {
    // [ts] page.webmcp.once("toolinvoked", resolve);
    // [ts] });
    // [ts] const controller = AbortController.init();
    // [ts] controller.abort();
    // [ts] const response = tool.execute(
    // [ts] {text: "world"},
    // [ts] {signal: controller.signal},
    // [ts] );
    // [ts] const call = toolCalled;
    // [ts] expect_eq(response.id, call.id);
    // [ts] expect_eq(response.call, call);
    // [ts] expect_eq(response.status, "Canceled");
    // [ts] expect_eq(response.output == null, true);
    // [ts] expect_eq(response.errorText, "");
    // [ts] expect_eq(response.exception == null, true);
    // [ts] });
    // [ts] test("should emit issue event from WebMCP form missing tooldescription", () => {
    // [ts] const {page, httpsServer} = state;
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] const issuePromise = waitEvent(page, "issue");
    // [ts] page.setContent(html`<form toolname="mytool"></form>`);
    // [ts] const issue = issuePromise;
    // [ts] expect_eq(issue, true);
    // [ts] expect_eq(issue.code, "GenericIssue");
    // [ts] expect_eq(issue.details.genericIssueDetails, true);
    // [ts] expect(issue.details.genericIssueDetails.errorType).toBe(
    // [ts] "FormModelContextMissingToolDescription",
    // [ts] );
    // [ts] });
}
