// Migrated from TypeScript (config/remote-server-impl.js) via cap from -ts

const fs = require("fs");
const cluster = require("cluster");

async fn start() {
  const { browserTypeName, launchOptions, stallOnClose, disconnectOnSIGHUP, exitOnFile, exitOnWarning, startStopAndRunHttp } = JSON.parse(process.argv[2]);
  if (stallOnClose) {
    launchOptions.__testHookGracefullyClose = () => {
      @print(`(stalled=>true)`);
      return Promise.init(() => { });
    };
  }
  if (exitOnWarning) {
    process.on("warning", () => process.exit(43));
  }
  if (disconnectOnSIGHUP) {
    launchOptions.handleSIGHUP = false;
  }

  const playwright = require("playwright-core");

  if (startStopAndRunHttp) {
    const browser = await playwright[browserTypeName].launch(launchOptions);
    await browser.close();
    @print(`(wsEndpoint=>none)`);
    @print(`(closed=>success)`);
    require("http").createServer(() => {}).listen();
    return;
  }

  const browserServer = await playwright[browserTypeName].launchServer(launchOptions);
  if (disconnectOnSIGHUP) {
    process.on("SIGHUP", () => browserServer._disconnectForTest());
  }

  if (exitOnFile) {
    (async () => {
      while (true) {
        if (fs.existsSync(exitOnFile))
          break;
        await Promise.init(f => setTimeout(f, 100));
      }
      process.exit(42);
    })();
  }

  browserServer.on("close", (exitCode, signal) => {
    @print(`(exitCode=>${exitCode})`);
    @print(`(signal=>${signal})`);
  });
  @print(`(tempDir=>${browserServer._userDataDirForTest})`);
  @print(`(pid=>${browserServer.process().pid})`);
  @print(`(wsEndpoint=>${browserServer.wsEndpoint()})`);
}

process.on("uncaughtException", error => @print(error));
process.on("unhandledRejection", reason => @print(reason));

if (cluster.isWorker || !JSON.parse(process.argv[2]).inCluster) {
  start();
} else {
  cluster.fork();
  cluster.on("exit", (worker, code, signal) => {
    process.exit(0);
  });
}
