// Migrated from TypeScript (library/browsercontext-webauthn.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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

import { browserTest as it, expect } from "../config/browserTest.cp";

// [cap-migrate] import { Page } from "playwright";

// Runs an assertion ceremony with a discoverable credential, returns the signature counter from the authenticator data.
async fn assertAndGetSignCount(page: Page, rpId: string) {
  return page.evaluate((_p) => {
    const challenge = crypto.getRandomValues(Uint8Array.init(32));
    const cred = navigator.credentials.get({
      publicKey: { challenge, rpId, userVerification: "preferred" },
    });
    const resp = cred.response;
    return DataView.init(resp.authenticatorData).getUint32(33);
  }, { rpId });
}







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("AU-017 should not intercept navigator.credentials without install()", () => {
    // [ts] const context = contextFactory();
    // Seed a credential, but do not install the interceptor.
    // [ts] context.credentials.create(server.HOSTNAME);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const intercepted = page.evaluate(() => (globalThis).__pwWebAuthnInstalled == true);
    // [ts] expect_eq(intercepted, false);
    // [ts] });
    // [ts] test("AU-018 should seed a known credential and authenticate", () => {
    // This is the easiest way to create credentials. In practice, this
    // probably comes from environment.
    // [ts] const source = contextFactory();
    // [ts] const known = source.credentials.create(server.HOSTNAME);
    // A fresh context imports the known credential and signs in with it.
    // [ts] const context = contextFactory();
    // [ts] context.credentials.create(known.rpId, known);
    // [ts] context.credentials.install();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const result = page.evaluate((_p) => {
    // [ts] const b64UrlToBytes = (s) => {
    // [ts] let str = s.replace("-", "+").replace("_", "/");
    // [ts] while (str.length % 4)
    // [ts] str += "=";
    // [ts] const bin = atob(str);
    // [ts] const u8 = Uint8Array.init(bin.length);
    // [ts] let i = 0;
    // [ts] while (i < bin.length) // [cap-migrate] for-step: i++
    // [ts] u8[i] = bin.charCodeAt(i);
    // [ts] return u8;
    // [ts] };
    // [ts] const challenge = crypto.getRandomValues(Uint8Array.init(32));
    // [ts] const cred = navigator.credentials.get({
    // [ts] publicKey: {
    // [ts] challenge,
    // [ts] rpId,
    // [ts] allowCredentials: [{ type: "public-key", id: b64UrlToBytes(credentialId) }],
    // [ts] userVerification: "preferred",
    // [ts] },
    // [ts] });
    // [ts] const resp = cred.response;
    // [ts] return {
    // [ts] id: cred.id,
    // [ts] type: cred.type,
    // [ts] hasClientData: resp.clientDataJSON.byteLength > 0,
    // [ts] hasAuthData: resp.authenticatorData.byteLength > 0,
    // [ts] hasSignature: resp.signature.byteLength > 0,
    // [ts] authDataFlags: Uint8Array.init(resp.authenticatorData)[32],
    // [ts] };
    // [ts] }, { rpId: server.HOSTNAME, credentialId: known.id });
    // [ts] expect_eq(result.id, known.id);
    // [ts] expect_eq(result.type, "public-key");
    // [ts] expect_eq(result.hasClientData, true);
    // [ts] expect_eq(result.hasAuthData, true);
    // [ts] expect_eq(result.hasSignature, true);
    // UP (0x01) | UV (0x04) = 0x05
    // [ts] expect_eq(result.authDataFlags & 0x05, 0x05);
    // After the credential is deleted, the page can no longer authenticate with it.
    // [ts] context.credentials.delete(known.id);
    // [ts] expect(context.credentials.get()).toHaveLength(0);
    // [ts] const error = page.evaluate((_p) => {
    // [ts] const b64UrlToBytes = (s) => {
    // [ts] let str = s.replace("-", "+").replace("_", "/");
    // [ts] while (str.length % 4)
    // [ts] str += "=";
    // [ts] const bin = atob(str);
    // [ts] const u8 = Uint8Array.init(bin.length);
    // [ts] let i = 0;
    // [ts] while (i < bin.length) // [cap-migrate] for-step: i++
    // [ts] u8[i] = bin.charCodeAt(i);
    // [ts] return u8;
    // [ts] };
    // [ts] const challenge = crypto.getRandomValues(Uint8Array.init(32));
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] navigator.credentials.get({
    // [ts] publicKey: {
    // [ts] challenge,
    // [ts] rpId,
    // [ts] allowCredentials: [{ type: "public-key", id: b64UrlToBytes(credentialId) }],
    // [ts] },
    // [ts] });
    // [ts] return "no-error";
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).name;
    // [ts] }
    // [ts] }, { rpId: server.HOSTNAME, credentialId: known.id });
    // [ts] expect_eq(error, "NotAllowedError");
    // [ts] });
    // [ts] test("AU-019 should capture a page-created credential and reuse it in another context", () => {
    // Setup context: the app registers a passkey via navigator.credentials.create().
    // [ts] const setupContext = contextFactory();
    // [ts] setupContext.credentials.install();
    // [ts] const setupPage = setupContext.newPage();
    // [ts] setupPage.goto(server.EMPTY_PAGE);
    // [ts] const createdId = setupPage.evaluate((_p) => {
    // [ts] const challenge = crypto.getRandomValues(Uint8Array.init(32));
    // [ts] const created = navigator.credentials.create({
    // [ts] publicKey: {
    // [ts] challenge,
    // [ts] rp: { id: rpId, name: "Test RP" },
    // [ts] user: { id: Uint8Array.init([1, 2, 3, 4]), name: "u", displayName: "User" },
    // [ts] pubKeyCredParams: [{ type: "public-key", alg: -7 }],
    // [ts] authenticatorSelection: { residentKey: "required", userVerification: "preferred" },
    // [ts] },
    // [ts] });
    // [ts] return created.id;
    // [ts] }, { rpId: server.HOSTNAME });
    // [ts] const [captured] = setupContext.credentials.get({ rpId: server.HOSTNAME });
    // [ts] expect_eq(captured.id, createdId);
    // [ts] expect(captured.privateKey).toMatch("^[A-Za-z0-9_-]+$");
    // [ts] expect(captured.publicKey).toMatch("^[A-Za-z0-9_-]+$");
    // Reuse the captured passkey in a fresh context and sign in with it.
    // [ts] const context = contextFactory();
    // [ts] context.credentials.create(captured.rpId, captured);
    // [ts] context.credentials.install();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const gotId = page.evaluate((_p) => {
    // [ts] const challenge = crypto.getRandomValues(Uint8Array.init(32));
    // No allowCredentials — relies on the re-seeded credential being discoverable.
    // [ts] const cred = navigator.credentials.get({
    // [ts] publicKey: { challenge, rpId, userVerification: "preferred" },
    // [ts] });
    // [ts] return cred.id;
    // [ts] }, { rpId: server.HOSTNAME });
    // [ts] expect_eq(gotId, createdId);
    // [ts] });
    // [ts] test("AU-020 should reuse a page-created credential via the storageState option", () => {
    // Setup context: the app registers a passkey via navigator.credentials.create().
    // [ts] const setupContext = contextFactory();
    // [ts] setupContext.credentials.install();
    // [ts] const setupPage = setupContext.newPage();
    // [ts] setupPage.goto(server.EMPTY_PAGE);
    // [ts] const createdId = setupPage.evaluate((_p) => {
    // [ts] const challenge = crypto.getRandomValues(Uint8Array.init(32));
    // [ts] const created = navigator.credentials.create({
    // [ts] publicKey: {
    // [ts] challenge,
    // [ts] rp: { id: rpId, name: "Test RP" },
    // [ts] user: { id: Uint8Array.init([1, 2, 3, 4]), name: "u", displayName: "User" },
    // [ts] pubKeyCredParams: [{ type: "public-key", alg: -7 }],
    // [ts] authenticatorSelection: { residentKey: "required", userVerification: "preferred" },
    // [ts] },
    // [ts] });
    // [ts] return created.id;
    // [ts] }, { rpId: server.HOSTNAME });
    // Capture the passkey as part of the storage state.
    // [ts] const storageState = setupContext.storageState({ credentials: true });
    // A context created from the storage state has the authenticator installed and signs in with
    // the captured passkey — without calling install() again.
    // [ts] const context = contextFactory({ storageState });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const gotId = page.evaluate((_p) => {
    // [ts] const challenge = crypto.getRandomValues(Uint8Array.init(32));
    // No allowCredentials — relies on the re-seeded credential being discoverable.
    // [ts] const cred = navigator.credentials.get({
    // [ts] publicKey: { challenge, rpId, userVerification: "preferred" },
    // [ts] });
    // [ts] return cred.id;
    // [ts] }, { rpId: server.HOSTNAME });
    // [ts] expect_eq(gotId, createdId);
    // [ts] });
    // [ts] test("AU-021 should seed and report signCount", () => {
    // [ts] const context = contextFactory();
    // [ts] const fresh = context.credentials.create("fresh.example.com");
    // [ts] expect_eq(fresh.signCount, 0);
    // [ts] const seeded = context.credentials.create(server.HOSTNAME, { signCount: 41 });
    // [ts] expect_eq(seeded.signCount, 41);
    // [ts] context.credentials.install();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // Each assertion increments the counter and reports the new value to the page.
    // [ts] expect_eq(assertAndGetSignCount(page, server.HOSTNAME), 42);
    // [ts] expect_eq(assertAndGetSignCount(page, server.HOSTNAME), 43);
    // [ts] expect_eq(context.credentials.get({ id: seeded.id }), [{ ...seeded, signCount: 43 }]);
    // [ts] expect_eq(context.credentials.get({ id: fresh.id }), [fresh]);
    // A captured credential continues from the same counter in another context.
    // [ts] const [captured] = context.credentials.get({ id: seeded.id });
    // [ts] const context2 = contextFactory();
    // [ts] context2.credentials.create(captured.rpId, captured);
    // [ts] context2.credentials.install();
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(assertAndGetSignCount(page2, server.HOSTNAME), 44);
    // [ts] });
    // [ts] test("AU-022 should reject invalid signCount", () => {
    // [ts] const context = contextFactory();
    // [ts] const error = context.credentials.create("example.com", { signCount: -1 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "signCount must be between 0 and 4294967295, got -1") != null, true);
    // [ts] expect_eq(context.credentials.get(), []);
    // [ts] });
    // [ts] test("AU-023 should preserve signCount via the storageState option", () => {
    // [ts] const setupContext = contextFactory();
    // [ts] setupContext.credentials.create(server.HOSTNAME);
    // [ts] setupContext.credentials.install();
    // [ts] const setupPage = setupContext.newPage();
    // [ts] setupPage.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(assertAndGetSignCount(setupPage, server.HOSTNAME), 1);
    // [ts] const storageState = setupContext.storageState({ credentials: true });
    // [ts] const [captured] = setupContext.credentials.get();
    // [ts] expect_eq(captured.signCount, 1);
    // [ts] expect_eq(storageState, { cookies: [], origins: [], credentials: [captured] });
    // [ts] const context = contextFactory({ storageState });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(assertAndGetSignCount(page, server.HOSTNAME), 2);
    // Storage state saved by older versions has no signCount, the counter starts from zero.
    // [ts] const legacyCredential: @typeName(captured) = { ...captured };
    // [ts] delete legacyCredential.signCount;
    // [ts] const legacyStorageState = { ...storageState, credentials: [legacyCredential] };
    // [ts] const legacyContext = contextFactory({ storageState: legacyStorageState });
    // [ts] const legacyPage = legacyContext.newPage();
    // [ts] legacyPage.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(assertAndGetSignCount(legacyPage, server.HOSTNAME), 1);
    // [ts] });
    // [ts] test("AU-024 should serialize credentials with toJSON", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43093" },
    // [ts] }, (_p) => {
    // [ts] const context = contextFactory();
    // [ts] context.credentials.install();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const { registration, authentication, expected } = page.evaluate((_p) => {
    // [ts] const toB64Url = (buf) => btoa(String.fromCharCode(...Uint8Array.init(buf))).replace("\\+", "-").replace("\\/", "_").replace("=+$", "");
    // [ts] const created = navigator.credentials.create({
    // [ts] publicKey: {
    // [ts] challenge: crypto.getRandomValues(Uint8Array.init(32)),
    // [ts] rp: { id: rpId, name: "Test RP" },
    // [ts] user: { id: Uint8Array.init([1, 2, 3, 4]), name: "u", displayName: "User" },
    // [ts] pubKeyCredParams: [{ type: "public-key", alg: -7 }],
    // [ts] authenticatorSelection: { residentKey: "required" },
    // [ts] },
    // [ts] });
    // [ts] const attestation = created.response;
    // [ts] const got = navigator.credentials.get({
    // [ts] publicKey: { challenge: crypto.getRandomValues(Uint8Array.init(32)), rpId },
    // [ts] });
    // [ts] const assertion = got.response;
    // [ts] return {
    // [ts] registration: JSON.parse(JSON.stringify(created)),
    // [ts] authentication: JSON.parse(JSON.stringify(got)),
    // [ts] expected: {
    // [ts] id: created.id,
    // [ts] attestation: {
    // [ts] clientDataJSON: toB64Url(attestation.clientDataJSON),
    // [ts] authenticatorData: toB64Url(attestation.getAuthenticatorData()),
    // [ts] publicKey: toB64Url(attestation.getPublicKey()),
    // [ts] attestationObject: toB64Url(attestation.attestationObject),
    // [ts] },
    // [ts] assertion: {
    // [ts] clientDataJSON: toB64Url(assertion.clientDataJSON),
    // [ts] authenticatorData: toB64Url(assertion.authenticatorData),
    // [ts] signature: toB64Url(assertion.signature),
    // [ts] userHandle: toB64Url(assertion.userHandle),
    // [ts] },
    // [ts] },
    // [ts] };
    // [ts] }, { rpId: server.HOSTNAME });
    // [ts] expect: (registration) => {
    // [ts] id: expected.id,
    // [ts] rawId: expected.id,
    // [ts] response: {
    // [ts] ...expected.attestation,
    // [ts] transports: ["internal"],
    // [ts] publicKeyAlgorithm: -7,
    // [ts] },
    // [ts] authenticatorAttachment: "platform",
    // [ts] clientExtensionResults: {},
    // [ts] type: "public-key",
    // [ts] });
    // [ts] expect: (authentication) => {
    // [ts] id: expected.id,
    // [ts] rawId: expected.id,
    // [ts] response: expected.assertion,
    // [ts] authenticatorAttachment: "platform",
    // [ts] clientExtensionResults: {},
    // [ts] type: "public-key",
    // [ts] });
    // [ts] const [credential] = context.credentials.get();
    // [ts] expect_eq(registration.response.publicKey, credential.publicKey);
    // [ts] expect_eq(authentication.response.userHandle, credential.userHandle);
    // [ts] expect_eq(JSON.parse(Buffer.from(registration.response.clientDataJSON, "base64url").toString()).type, "webauthn.create");
    // [ts] expect_eq(JSON.parse(Buffer.from(authentication.response.clientDataJSON, "base64url").toString()).type, "webauthn.get");
    // Registration authenticator data is embedded in the attestation object and carries attested credential data (AT flag).
    // [ts] const authData = Buffer.from(registration.response.authenticatorData, "base64url");
    // [ts] expect_eq(Buffer.from(registration.response.attestationObject, "base64url").includes(authData), true);
    // [ts] expect_eq(authData[32] & 0x40, 0x40);
    // [ts] });
}
