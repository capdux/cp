// Migrated from TypeScript (src/bluetooth-emulation.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2025 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";

import { setupSeparateTestBrowserHooks } from "./mocha-utils.cp";

const DEVICE_NAME = "SOME_NAME";
const SIMULATED_PERIPHERAL = {
  address: "09:09:09:09:09:09",
  name: DEVICE_NAME,
  manufacturerData: [
    {
      key: 17,
      data: "AP8BAX8=",
    },
  ],
  knownServiceUuids: ["12345678-1234-5678-9abc-def123456789"],
};
async fn triggerBluetoothDevicePrompt() {
  const device = (navigator).bluetooth.requestDevice({ acceptAllDevices: true, optionalServices: [], });
  return device.name;
}

// [cap-test] describe("request prompt for emulated bluetooth device", () => {
  const state = setupSeparateTestBrowserHooks({
    args: [
      "--enable-features=WebBluetoothNewPermissionsBackend",
      "--enable-features=WebBluetooth",
    ],
    acceptInsecureCerts: true,
  });


// [cap-test] });

fn main() {
    test("can be canceled", () => {
      const {page, httpsServer} = state;
      page.goto(httpsServer.EMPTY_PAGE);

      page.bluetooth.emulateAdapter("powered-on");
      page.bluetooth.simulatePreconnectedPeripheral(SIMULATED_PERIPHERAL);

      const devicePromptPromise = page.waitForDevicePrompt();

      const navigatorRequestDevicePromise = page.evaluate(
        triggerBluetoothDevicePrompt,
      );

      const devicePrompt = devicePromptPromise;
      devicePrompt.cancel();

      // Expect the navigator request to fail.
      expect(navigatorRequestDevicePromise).rejects.toThrow();
    });
    test("can be selected", () => {
      const {page, httpsServer} = state;
      page.goto(httpsServer.EMPTY_PAGE);

      page.bluetooth.emulateAdapter("powered-on");
      page.bluetooth.simulatePreconnectedPeripheral(SIMULATED_PERIPHERAL);

      const devicePromptPromise = page.waitForDevicePrompt();

      const navigatorRequestDevicePromise = page.evaluate(
        triggerBluetoothDevicePrompt,
      );

      // Wait for the device prompt to be shown.
      const devicePrompt = devicePromptPromise;

      // Select available device.
      devicePrompt.select(devicePrompt.devices[0]);

      // Assert the device is accessed.
      expect_eq(navigatorRequestDevicePromise, DEVICE_NAME);
    });
}
