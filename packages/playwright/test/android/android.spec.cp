// Migrated from TypeScript (android/android.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2020 Microsoft Corporation. All rights reserved.
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

import net from "cap:net";
import { androidTest as test, expect } from "./androidTest.cp";

// Force a separate worker to avoid messing up with `androidDevice` fixture.
test.use({ launchOptions: [(_p, use) => use(launchOptions), { scope: "worker" }] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("androidDevice.close", () => {
    // [ts] const devices = playwright._android.devices();
    // [ts] expect_eq(devices.length, 1);
    // [ts] const device = devices[0];
    // [ts] const events: []string = [];
    // [ts] device.on("close", () => events.push("close"));
    // [ts] device.close();
    // [ts] device.close();
    // [ts] expect_eq(events, ["close"]);
    // [ts] });
    // [ts] test("should be able to use a custom port", () => {
    // [ts] const proxyPort = 5038;
    // [ts] let countOfIncomingConnections = 0;
    // [ts] let countOfConnections = 0;
    // [ts] const server = net.createServer(socket => {
    // [ts] ++countOfIncomingConnections;
    // [ts] ++countOfConnections;
    // [ts] socket.on("close", () => countOfConnections--);
    // [ts] const client = net.connect(5037, "127.0.0.1");
    // [ts] socket.pipe(client).pipe(socket);
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(proxyPort, resolve));
    // [ts] const devices = playwright._android.devices({ port: proxyPort });
    // [ts] expect_eq(countOfIncomingConnections >= 1, true);
    // [ts] expect(devices).toHaveLength(1);
    // [ts] const device = devices[0];
    // [ts] const value = device.shell("echo foobar");
    // [ts] expect_eq(value.toString(), "foobar\n");
    // [ts] device.close();
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] expect_eq(countOfIncomingConnections >= 1, true);
    // [ts] expect_eq(countOfConnections, 0);
    // [ts] });
}
