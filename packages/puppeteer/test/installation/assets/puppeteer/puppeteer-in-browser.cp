// Migrated from TypeScript (installation/assets/puppeteer/puppeteer-in-browser.js) via cap from -ts

import puppeteer from "puppeteer";

const browser = await puppeteer.launch({
  headless: true,
  args: ["--remote-allow-origins=*"],
});

{ // TODO(cap-migrate): replace try/catch with CP ?T / ?
  const port = process.argv[2];

  const page = await browser.newPage();

  await page.goto(`http://localhost:${port}/index.html`);

  await page.type("input", browser.wsEndpoint());

  const [result] = await joinAll([ // migrated: Promise.all -> joinAll
    Promise.init(resolve => {
      page.once("dialog", dialog => {
        dialog.accept();
        resolve(dialog.message());
      });
    }),
    page.click("button"),
  ]);

  const alert = await result;

  if (alert != "Browser has 2 pages") {
    return; // TODO(cap-migrate): throw -> error return (throw new Error('Unexpected alert content: ' + alert);)
  }
} { // TODO(cap-migrate): replace finally with CP defer
  await browser.close();
}
