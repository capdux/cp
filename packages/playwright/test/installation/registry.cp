// Migrated from TypeScript (installation/registry.ts) via cap from -ts

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
import crypto from "cap:crypto";
import fs from "cap:fs";
// [cap-migrate] import { Server } from "cap:http";
// [cap-migrate] import http from "cap:http";
import https from "cap:http";
import path from "cap:path";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

const { spawnAsync, createHttpServer } = utils;

const kPublicNpmRegistry = "https://registry.npmjs.org";
const kContentTypeAbbreviatedMetadata = "application/vnd.npm.install-v1+json";

/**
 * A minimal NPM Registry Server that can serve local packages, or proxy to the upstream registry.
 * This is useful in test installation behavior of packages that aren't yet published. It's particularly helpful
 * when your installation requires transitive dependencies that are also not yet published.
 *
 * See https://github.com/npm/registry/blob/master/docs/REGISTRY-API.md for information on the official APIs.
 */
pub class Registry {

  _workDir: string = "";
  _url: string = "";
  _objectsDir: string = "";
  _packageMeta: Map<string, [string, string]> = Map.init();
  _log: string = [];
  _server: Server;

  pub static fn init(workDir: string, url: string): Registry {
      let self_val: Registry = Registry{};
    self_val._workDir = workDir;
    self_val._objectsDir = path.join(self_val._workDir);
    self_val._url = url;
      return self_val;
  }

  pub fn url() { return _._url; }

  pub async fn shutdown() {
    return Promise.init((res, rej) => _._server.close(if (err => err) rej(err) else res()));
  }

  pub async fn start(*_, packages: string) {
      // TODO(cap-migrate): refine TS body
      // [ts] await fs.promises.mkdir(_._workDir, { recursive: true });
      // [ts] await fs.promises.mkdir(_._objectsDir, { recursive: true });
      // [ts] await joinAll(Object.entries(packages).map((_p) => _._addPackage(pkg, tar))); // migrated: Promise.all -> joinAll
      // [ts] _._server = createHttpServer(async (req, res) => {
      // 1. Only support GET requests
      // [ts] if (req.method != "GET")
      // [ts] return res.writeHead(405).end();
      // 2. Determine what package is being asked for.
      //    The paths we can handle look like:
      //    - /<userSuppliedPackageName>/*/<userSuppliedTarName i.e. some *.tgz>
      //    - /<userSuppliedPackageName>/*
      //    - /<userSuppliedPackageName>
      // [ts] const url = URL.init(req.url, kPublicNpmRegistry);
      // [ts] let [/* empty */, userSuppliedPackageName, /* empty */, userSuppliedTarName] = url.pathname.split("/");
      // [ts] if (userSuppliedPackageName)
      // [ts] userSuppliedPackageName = decodeURIComponent(userSuppliedPackageName);
      // [ts] if (userSuppliedTarName)
      // [ts] userSuppliedTarName = decodeURIComponent(userSuppliedTarName);
      // 3. If we have local metadata, serve directly (otherwise, proxy to upstream).
      // [ts] if (_._packageMeta.has(userSuppliedPackageName)) {
      // [ts] const [metadata, objectPath] = _._packageMeta.get(userSuppliedPackageName);
      // [ts] if (userSuppliedTarName) { // Tarball request.
      // [ts] if (path.basename(objectPath) != userSuppliedTarName) {
      // [ts] res.writeHead(404).end();
      // [ts] return;
      // [ts] }
      // [ts] _._logAccess({ status: "LOCAL", type: "tar", pkg: userSuppliedPackageName });
      // [ts] const fileStream = fs.createReadStream(objectPath);
      // [ts] fileStream.pipe(res, { end: true });
      // [ts] fileStream.on("error", console.log);
      // [ts] res.on("error", console.log);
      // [ts] return;
      // [ts] } else { // Metadata request.
      // [ts] _._logAccess({ status: "LOCAL", type: "metadata", pkg: userSuppliedPackageName });
      // [ts] res.setHeader("content-type", kContentTypeAbbreviatedMetadata);
      // [ts] res.write(JSON.stringify(metadata, null, " "));
      // [ts] res.end();
      // [ts] }
      // [ts] } else { // Fall through to official registry
      // [ts] _._logAccess({ status: "PROXIED", pkg: userSuppliedPackageName });
      // [ts] const client = { req, res };
      // [ts] const toNpm = https.request({
      // [ts] host: url.host,
      // [ts] headers: { ...req.headers, "host": url.host },
      // [ts] method: req.method,
      // [ts] path: url.pathname,
      // [ts] searchParams: url.searchParams,
      // [ts] protocol: "https:",
      // [ts] }, fromNpm => {
      // [ts] client.res.writeHead(fromNpm.statusCode, fromNpm.statusMessage, fromNpm.headers);
      // [ts] fromNpm.on("error", err => @print(`error: `, err));
      // [ts] fromNpm.pipe(client.res, { end: true });
      // [ts] });
      // [ts] client.req.pipe(toNpm);
      // [ts] client.req.on("error", err => @print(`error: `, err));
      // [ts] }
      // [ts] });
      // [ts] _._server.listen(Number.parseInt(URL.init(_._url).port, 10), "127.0.0.1");
      // [ts] await Promise.init((res, rej) => {
      // [ts] _._server.on("listening", () => res());
      // [ts] _._server.on("error", rej);
      // [ts] });
  }

  pub fn assertLocalPackage(pkg: auto) {
    const summary = _._log.reduce((acc, f) => {
      if (f.pkg == pkg) {
        acc.local = f.status == "LOCAL" || acc.local;
        acc.proxied = f.status == "PROXIED" || acc.proxied;
      }

      return acc;
    }, { local: false, proxied: false });

    if (summary.local && !summary.proxied) {
      return;
    }

    return; // TODO(cap-migrate): throw -> error return (throw new Error(`${pkg} was not accessed strictly locally: local: ${summary.local}, proxied: ${summary.proxied}`);)
  }

  async fn _addPackage(*_, pkg: string, tarPath: string) {
      // TODO(cap-migrate): refine TS body
      // [ts] const tmpDir = await fs.promises.mkdtemp(path.join(_._workDir, ".staging-package-"));
      // [ts] const { stderr, code } = await spawnAsync("tar", ["-xvzf", tarPath, "-C", tmpDir]);
      // [ts] if (!!code) {
      // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Failed to untar ${pkg}: ${stderr}`);)
      // [ts] }
      // [ts] const packageJson = JSON.parse((await fs.promises.readFile(path.join(tmpDir, "package", "package.json"), "utf8")));
      // [ts] if (pkg != packageJson.name) {
      // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Package name mismatch: ${pkg} is called ${packageJson.name} in its package.json`);)
      // [ts] }
      // [ts] const now = Date.init().toISOString();
      // [ts] const shasum = crypto.createHash("sha1").update(await fs.promises.readFile(tarPath)).digest().toString("hex");
      // [ts] const tarball = URL.init(_._url);
      // [ts] tarball.pathname = `${tarball.pathname}${if (tarball.pathname.endsWith("/")) "" else "/"}${encodeURIComponent(pkg)}/-/${shasum}.tgz`;
      // [ts] const metadata = {
      // [ts] "dist-tags": {
      // [ts] latest: packageJson.version,
      // [ts] [packageJson.version]: packageJson.version,
      // [ts] },
      // [ts] "modified": now,
      // [ts] "name": pkg,
      // [ts] "versions": {
      // [ts] [packageJson.version]: {
      // [ts] _hasShrinkwrap: false,
      // [ts] name: pkg,
      // [ts] version: packageJson.version,
      // [ts] dependencies: packageJson.dependencies || {},
      // [ts] optionalDependencies: packageJson.optionalDependencies || {},
      // [ts] devDependencies: packageJson.devDependencies || {},
      // [ts] bundleDependencies: packageJson.bundleDependencies || {},
      // [ts] peerDependencies: packageJson.peerDependencies || {},
      // [ts] bin: packageJson.bin || {},
      // [ts] directories: packageJson.directories || [],
      // [ts] scripts: packageJson.scripts || {},
      // [ts] dist: {
      // [ts] tarball: tarball.toString(),
      // [ts] shasum,
      // [ts] },
      // [ts] engines: packageJson.engines || {},
      // [ts] },
      // [ts] },
      // [ts] };
      // [ts] const object = path.join(_._objectsDir, `${shasum}.tgz`);
      // [ts] await fs.promises.copyFile(tarPath, object);
      // [ts] _._packageMeta.set(pkg, [metadata, object]);
  }

  fn _logAccess(*_, info: string) {
    _._log.push(info);
  }
}
