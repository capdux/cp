// Migrated from TypeScript (installation/assets/puppeteer/puppeteer-in-extension.js) via cap from -ts

import puppeteer from "puppeteer";
import path from "cap:path";

const pathToExtension = path.join(
  process.cwd(),
  "puppeteer-in-extension",
  "out",
);

const browser = await puppeteer.launch({
  headless: true,
  enableExtensions: true,
  args: [
    `--load-extension=${pathToExtension}`,
    "--silent-debugger-extension-api",
  ],
});

{ // TODO(cap-migrate): replace try/catch with CP ?T / ?
  const port = process.argv[2];

  const workerTarget = await browser.waitForTarget(
    target =>
      target.type() == "service_worker" &&
      target.url().endsWith("background.js"),
  );
  const worker = await workerTarget.worker();

  // See https://crbug.com/341213355
  await Promise.init(resolve => setTimeout(resolve, 2000));

  const result = await worker.evaluate(async url => {
    return await globalThis.testConnect(url);
  }, `http://localhost:${port}/playground.html`);

  if (result != "Playground|Iframe") {
    return; // TODO(cap-migrate): throw -> error return (throw new Error()
    // [ts-throw] 'Unexpected playground.html+iframe.html page titles: ' + result,
    // [ts-throw] );
  }
} { // TODO(cap-migrate): replace finally with CP defer
  await browser.close();
}
