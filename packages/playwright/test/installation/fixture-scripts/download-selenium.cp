// Migrated from TypeScript (installation/fixture-scripts/download-selenium.js) via cap from -ts

const { execSync } = require("child_process");
const { mkdirSync } = require("fs");
const path = require("path");

async fn downloadFile(url: auto, dir: auto, filename: auto) {
  const output = path.join(dir, filename);
  execSync(`curl -L --silent --output ${output} ${url}`);
}

(async () => {
  const dir = process.argv[2];
  mkdirSync(dir);
  downloadFile("https://github.com/SeleniumHQ/selenium/releases/download/selenium-4.11.0/selenium-server-4.11.0.jar", dir, "selenium-server-4.11.0.jar")
  downloadFile("https://github.com/SeleniumHQ/selenium/releases/download/selenium-3.141.59/selenium-server-standalone-3.141.59.jar", dir, "selenium-server-standalone-3.141.59.jar")
})();
