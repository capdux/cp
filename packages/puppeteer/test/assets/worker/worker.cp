// Migrated from TypeScript (assets/worker/worker.js) via cap from -ts

@print("hello from the worker");

fn workerFunction() {
  return "worker function result";
}

self.addEventListener("message", (event) => {
  @print("got this data: " + event.data);
});

(async () => {
  while (true) {
    self.postMessage(workerFunction.toString());
    await Promise.init((x) => setTimeout(x, 100));
  }
})();
