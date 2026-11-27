// Migrated from TypeScript (assets/reading-list/react-dom_16.14.0.js) via cap from -ts

/** @license React v16.14.0
 * react-dom.development.js
 *
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

"use strict";

// TODO(cap-migrate): refine TS statement
// [ts] ((global, factory) => {
// [ts] if (@typeName(exports) == "object" && @typeName(module) != "undefined") { factory(exports, require("react")) else; }
// [ts] if (@typeName(define) == "function" && define.amd) { define(["exports", "react"], factory) else; }
// [ts] (global = global || self, factory(global.ReactDOM = {}, global.React));
// [ts] }(this, ((exports, React) => { "use strict";
// [ts] let ReactSharedInternals = React.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED; // Prevent newer renderers from RTE when used with older react package versions.
// Current owner and dispatcher used to share the same ref,
// but PR #14548 split them out to better support the react-debug-tools package.
// [ts] if (!ReactSharedInternals.hasOwnProperty("ReactCurrentDispatcher")) {
// [ts] ReactSharedInternals.ReactCurrentDispatcher = {
// [ts] current: null
// [ts] };
// [ts] }
// [ts] if (!ReactSharedInternals.hasOwnProperty("ReactCurrentBatchConfig")) {
// [ts] ReactSharedInternals.ReactCurrentBatchConfig = {
// [ts] suspense: null
// [ts] };
// [ts] }
// by calls to these methods by a Babel plugin.
//
// In PROD (or in packages without access to React internals),
// they are left as they are instead.
// [ts] fn warn(format: auto) {
// [ts] {
// [ts] let _len = arguments.length, args = Array.init(if (_len > 1) _len - 1 else 0), _key = 1;
// [ts] while (_key < _len) { // [cap-migrate] for-step: _key++
// [ts] args[_key - 1] = arguments[_key];
// [ts] }
// [ts] printWarning("warn", format, args);
// [ts] }
// [ts] }
// [ts] fn error(format: auto) {
// [ts] {
// [ts] let _len2 = arguments.length, args = Array.init(if (_len2 > 1) _len2 - 1 else 0), _key2 = 1;
// [ts] while (_key2 < _len2) { // [cap-migrate] for-step: _key2++
// [ts] args[_key2 - 1] = arguments[_key2];
// [ts] }
// [ts] printWarning("error", format, args);
// [ts] }
// [ts] }
// [ts] fn printWarning(level: auto, format: auto, args: auto) {
// When changing this logic, you might want to also
// update consoleWithStackDev.www.js as well.
// [ts] {
// [ts] let hasExistingStack = args.length > 0 && @typeName(args[args.length - 1]) == "string" && args[args.length - 1].indexOf("\n    in") == 0;
// [ts] if (!hasExistingStack) {
// [ts] let ReactDebugCurrentFrame = ReactSharedInternals.ReactDebugCurrentFrame;
// [ts] let stack = ReactDebugCurrentFrame.getStackAddendum();
// [ts] if (stack != "") {
// [ts] format += "%s";
// [ts] args = args.concat([stack]);
// [ts] }
// [ts] }
// [ts] let argsWithFormat = args.map((item) => {
// [ts] return "" + item;
// [ts] }); // Careful: RN currently depends on this prefix
// [ts] argsWithFormat.unshift("Warning: " + format); // We intentionally don't use spread (or .apply) directly because it
// breaks IE9: https://github.com/facebook/react/issues/13610
// eslint-disable-next-line react-internal/no-production-logging
// [ts] Function.prototype.apply.call(console[level], console, argsWithFormat);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// --- Welcome to debugging React ---
// This error was thrown as a convenience so that you can use this stack
// to find the callsite that caused this warning to fire.
// [ts] let argIndex = 0;
// [ts] let message = "Warning: " + format.replace("%s", () => {
// [ts] return args[argIndex++];
// [ts] });
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(message);)
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
// [ts] }
// [ts] }
// [ts] if (!React) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "ReactDOM was loaded before React. Make sure you load the React package before loading ReactDOM." );)
// [ts] }
// [ts] }
// [ts] let invokeGuardedCallbackImpl = (name, func, context, a, b, c, d, e, f) => {
// [ts] let funcArgs = Array.prototype.slice.call(arguments, 3);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] func.apply(context, funcArgs);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] _.onError(error);
// [ts] }
// [ts] };
// [ts] {
// In DEV mode, we swap out invokeGuardedCallback for a special version
// that plays more nicely with the browser's DevTools. The idea is to preserve
// "Pause on exceptions" behavior. Because React wraps all user-provided
// functions in invokeGuardedCallback, and the production version of
// invokeGuardedCallback uses a try-catch, all user exceptions are treated
// like caught exceptions, and the DevTools won't pause unless the developer
// takes the extra step of enabling pause on caught exceptions. This is
// unintuitive, though, because even though React has caught the error, from
// the developer's perspective, the error is uncaught.
//
// To preserve the expected "Pause on exceptions" behavior, we don't use a
// try-catch in DEV. Instead, we synchronously dispatch a fake event to a fake
// DOM node, and call the user-provided callback from inside an event handler
// for that fake event. If the callback throws, the error is "captured" using
// a global event handler. But because the error happens in a different
// event loop context, it does not interrupt the normal program flow.
// Effectively, this gives us try-catch behavior without actually using
// try-catch. Neat!
// Check that the browser supports the APIs we need to implement our special
// DEV version of invokeGuardedCallback
// [ts] if (@typeName(window) != "undefined" && @typeName(window.dispatchEvent) == "function" && @typeName(document) != "undefined" && @typeName(document.createEvent) == "function") {
// [ts] let fakeNode = document.createElement("react");
// [ts] let invokeGuardedCallbackDev = (name, func, context, a, b, c, d, e, f) => {
// If document doesn't exist we know for sure we will crash in this method
// when we call document.createEvent(). However this can cause confusing
// errors: https://github.com/facebookincubator/create-react-app/issues/3482
// So we preemptively throw with a better message instead.
// [ts] if (!(@typeName(document) != "undefined")) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "The `document` global was defined when React was initialized, but is not defined anymore. This can happen in a test environment if a component schedules an update from an asynchronous callback, but the test has already finished running. To solve this, you can either unmount the component at the end of your test (and ensure that any asynchronous operations get canceled in `componentWillUnmount`), or you can change the test itself to be asynchronous." );)
// [ts] }
// [ts] }
// [ts] let evt = document.createEvent("Event"); // Keeps track of whether the user-provided callback threw an error. We
// set this to true at the beginning, then set it to false right after
// calling the function. If the function errors, `didError` will never be
// set to false. This strategy works even if the browser is flaky and
// fails to call our global error handler, because it doesn't rely on
// the error event at all.
// [ts] let didError = true; // Keeps track of the value of window.event so that we can reset it
// during the callback to let user code access window.event in the
// browsers that support it.
// [ts] let windowEvent = window.event; // Keeps track of the descriptor of window.event to restore it after event
// dispatching: https://github.com/facebook/react/issues/13688
// [ts] let windowEventDescriptor = Object.getOwnPropertyDescriptor(window, "event"); // Create an event handler for our fake event. We will synchronously
// dispatch our fake event using `dispatchEvent`. Inside the handler, we
// call the user-provided callback.
// [ts] let funcArgs = Array.prototype.slice.call(arguments, 3);
// [ts] fn callCallback() {
// We immediately remove the callback from event listeners so that
// nested `invokeGuardedCallback` calls do not clash. Otherwise, a
// nested call would trigger the fake event handlers of any call higher
// in the stack.
// [ts] fakeNode.removeEventListener(evtType, callCallback, false); // We check for window.hasOwnProperty('event') to prevent the
// window.event assignment in both IE <= 10 as they throw an error
// "Member not found" in strict mode, and in Firefox which does not
// support window.event.
// [ts] if (@typeName(window.event) != "undefined" && window.hasOwnProperty("event")) {
// [ts] window.event = windowEvent;
// [ts] }
// [ts] func.apply(context, funcArgs);
// [ts] didError = false;
// [ts] } // Create a global error event handler. We use this to capture the value
// that was thrown. It's possible that this error handler will fire more
// than once; for example, if non-React code also calls `dispatchEvent`
// and a handler for that event throws. We should be resilient to most of
// those cases. Even if our error event handler fires more than once, the
// last error event is always used. If the callback actually does error,
// we know that the last error event is the correct one, because it's not
// possible for anything else to have happened in between our callback
// erroring and the code that follows the `dispatchEvent` call below. If
// the callback doesn't error, but the error event was fired, we know to
// ignore it because `didError` will be false, as described above.
// [ts] let error; // Use this to track whether the error event is ever called.
// [ts] let didSetError = false;
// [ts] let isCrossOriginError = false;
// [ts] fn handleWindowError(event: auto) {
// [ts] error = event.error;
// [ts] didSetError = true;
// [ts] if (error == null && event.colno == 0 && event.lineno == 0) {
// [ts] isCrossOriginError = true;
// [ts] }
// [ts] if (event.defaultPrevented) {
// Some other error handler has prevented default.
// Browsers silence the error report if this happens.
// We'll remember this to later decide whether to log it or not.
// [ts] if (error != null && @typeName(error) == "object") {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] error._suppressLogging = true;
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (inner) {// Ignore.)
// [ts] }
// [ts] }
// [ts] }
// [ts] } // Create a fake event type.
// [ts] let evtType = "react-" + (if (name) name else "invokeguardedcallback"); // Attach our event handlers
// [ts] window.addEventListener("error", handleWindowError);
// [ts] fakeNode.addEventListener(evtType, callCallback, false); // Synchronously dispatch our fake event. If the user-provided function
// errors, it will trigger our global error handler.
// [ts] evt.initEvent(evtType, false, false);
// [ts] fakeNode.dispatchEvent(evt);
// [ts] if (windowEventDescriptor) {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(window, 'event', windowEventDescriptor);
// [ts] }
// [ts] if (didError) {
// [ts] if (!didSetError) {
// The callback errored, but the error event never fired.
// [ts] error = Error.init("An error was thrown inside one of your components, but React " + "doesn't know what it was. This is likely due to browser " + "flakiness. React does its best to preserve the \"Pause on " + "exceptions\" behavior of the DevTools, which requires some " + "DEV-mode only tricks. It's possible that these don't work in " + "your browser. Try triggering the error in production mode, " + "or switching to a modern browser. If you suspect that this is " + "actually an issue with React, please file an issue.");
// [ts] } else if (isCrossOriginError) {
// [ts] error = Error.init("A cross-origin error was thrown. React doesn't have access to " + "the actual error object in development. " + "See https://fb.me/react-crossorigin-error for more information.");
// [ts] }
// [ts] _.onError(error);
// [ts] } // Remove our event listeners
// [ts] window.removeEventListener("error", handleWindowError);
// [ts] };
// [ts] invokeGuardedCallbackImpl = invokeGuardedCallbackDev;
// [ts] }
// [ts] }
// [ts] let invokeGuardedCallbackImpl$1 = invokeGuardedCallbackImpl;
// [ts] let hasError = false;
// [ts] let caughtError = null; // Used by event system to capture/rethrow the first error.
// [ts] let hasRethrowError = false;
// [ts] let rethrowError = null;
// [ts] let reporter = {
// [ts] onError: (error) => {
// [ts] hasError = true;
// [ts] caughtError = error;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Call a function while guarding against errors that happens within it.
// [ts] * Returns an error if it throws, otherwise null.
// [ts] *
// [ts] * In production, this is implemented using a try-catch. The reason we don't
// [ts] * use a try-catch directly is so that we can swap out a different
// [ts] * implementation in DEV mode.
// [ts] *
// [ts] * @param {String} name of the guard to use for logging or debugging
// [ts] * @param {Function} func The function to invoke
// [ts] * @param {*} context The context to use when calling the function
// [ts] * @param {...*} args Arguments for function
// [ts] */
// [ts] fn invokeGuardedCallback(name: auto, func: auto, context: auto, a: auto, b: auto, c: auto, d: auto, e: auto, f: auto) {
// [ts] hasError = false;
// [ts] caughtError = null;
// [ts] invokeGuardedCallbackImpl$1.apply(reporter, arguments);
// [ts] }
// [ts] /**
// [ts] * Same as invokeGuardedCallback, but instead of returning an error, it stores
// [ts] * it in a global so it can be rethrown by `rethrowCaughtError` later.
// [ts] * TODO: See if caughtError and rethrowError can be unified.
// [ts] *
// [ts] * @param {String} name of the guard to use for logging or debugging
// [ts] * @param {Function} func The function to invoke
// [ts] * @param {*} context The context to use when calling the function
// [ts] * @param {...*} args Arguments for function
// [ts] */
// [ts] fn invokeGuardedCallbackAndCatchFirstError(name: auto, func: auto, context: auto, a: auto, b: auto, c: auto, d: auto, e: auto, f: auto) {
// [ts] invokeGuardedCallback.apply(this, arguments);
// [ts] if (hasError) {
// [ts] let error = clearCaughtError();
// [ts] if (!hasRethrowError) {
// [ts] hasRethrowError = true;
// [ts] rethrowError = error;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * During execution of guarded functions we will capture the first error which
// [ts] * we will rethrow to be handled by the top level error handler.
// [ts] */
// [ts] fn rethrowCaughtError() {
// [ts] if (hasRethrowError) {
// [ts] let error = rethrowError;
// [ts] hasRethrowError = false;
// [ts] rethrowError = null;
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] }
// [ts] }
// [ts] fn hasCaughtError() {
// [ts] return hasError;
// [ts] }
// [ts] fn clearCaughtError() {
// [ts] if (hasError) {
// [ts] let error = caughtError;
// [ts] hasError = false;
// [ts] caughtError = null;
// [ts] return error;
// [ts] } else {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "clearCaughtError was called but no error was captured. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let getFiberCurrentPropsFromNode = null;
// [ts] let getInstanceFromNode = null;
// [ts] let getNodeFromInstance = null;
// [ts] fn setComponentTree(getFiberCurrentPropsFromNodeImpl: auto, getInstanceFromNodeImpl: auto, getNodeFromInstanceImpl: auto) {
// [ts] getFiberCurrentPropsFromNode = getFiberCurrentPropsFromNodeImpl;
// [ts] getInstanceFromNode = getInstanceFromNodeImpl;
// [ts] getNodeFromInstance = getNodeFromInstanceImpl;
// [ts] {
// [ts] if (!getNodeFromInstance || !getInstanceFromNode) {
// [ts] error("EventPluginUtils.setComponentTree(...): Injected " + "module is missing getNodeFromInstance or getInstanceFromNode.");
// [ts] }
// [ts] }
// [ts] }
// [ts] let validateEventDispatches;
// [ts] {
// [ts] validateEventDispatches = (event) => {
// [ts] let dispatchListeners = event._dispatchListeners;
// [ts] let dispatchInstances = event._dispatchInstances;
// [ts] let listenersIsArr = Array.isArray(dispatchListeners);
// [ts] let listenersLen = if (listenersIsArr) dispatchListeners.length else if (dispatchListeners) 1 else 0;
// [ts] let instancesIsArr = Array.isArray(dispatchInstances);
// [ts] let instancesLen = if (instancesIsArr) dispatchInstances.length else if (dispatchInstances) 1 else 0;
// [ts] if (instancesIsArr != listenersIsArr || instancesLen != listenersLen) {
// [ts] error("EventPluginUtils: Invalid `event`.");
// [ts] }
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Dispatch the event to the listener.
// [ts] * @param {SyntheticEvent} event SyntheticEvent to handle
// [ts] * @param {function} listener Application-level callback
// [ts] * @param {*} inst Internal component instance
// [ts] */
// [ts] fn executeDispatch(event: auto, listener: auto, inst: auto) {
// [ts] let type = event.type || "unknown-event";
// [ts] event.currentTarget = getNodeFromInstance(inst);
// [ts] invokeGuardedCallbackAndCatchFirstError(type, listener, null, event);
// [ts] event.currentTarget = null;
// [ts] }
// [ts] /**
// [ts] * Standard/simple iteration through an event's collected dispatches.
// [ts] */
// [ts] fn executeDispatchesInOrder(event: auto) {
// [ts] let dispatchListeners = event._dispatchListeners;
// [ts] let dispatchInstances = event._dispatchInstances;
// [ts] {
// [ts] validateEventDispatches(event);
// [ts] }
// [ts] if (Array.isArray(dispatchListeners)) {
// [ts] let i = 0;
// [ts] while (i < dispatchListeners.length) { // [cap-migrate] for-step: i++
// [ts] if (event.isPropagationStopped()) {
// [ts] break;
// [ts] } // Listeners and Instances are two parallel arrays that are always in sync.
// [ts] executeDispatch(event, dispatchListeners[i], dispatchInstances[i]);
// [ts] }
// [ts] } else if (dispatchListeners) {
// [ts] executeDispatch(event, dispatchListeners, dispatchInstances);
// [ts] }
// [ts] event._dispatchListeners = null;
// [ts] event._dispatchInstances = null;
// [ts] }
// [ts] let FunctionComponent = 0;
// [ts] let ClassComponent = 1;
// [ts] let IndeterminateComponent = 2; // Before we know whether it is function or class
// [ts] let HostRoot = 3; // Root of a host tree. Could be nested inside another node.
// [ts] let HostPortal = 4; // A subtree. Could be an entry point to a different renderer.
// [ts] let HostComponent = 5;
// [ts] let HostText = 6;
// [ts] let Fragment = 7;
// [ts] let Mode = 8;
// [ts] let ContextConsumer = 9;
// [ts] let ContextProvider = 10;
// [ts] let ForwardRef = 11;
// [ts] let Profiler = 12;
// [ts] let SuspenseComponent = 13;
// [ts] let MemoComponent = 14;
// [ts] let SimpleMemoComponent = 15;
// [ts] let LazyComponent = 16;
// [ts] let IncompleteClassComponent = 17;
// [ts] let DehydratedFragment = 18;
// [ts] let SuspenseListComponent = 19;
// [ts] let FundamentalComponent = 20;
// [ts] let ScopeComponent = 21;
// [ts] let Block = 22;
// [ts] /**
// [ts] * Injectable ordering of event plugins.
// [ts] */
// [ts] let eventPluginOrder = null;
// [ts] /**
// [ts] * Injectable mapping from names to event plugin modules.
// [ts] */
// [ts] let namesToPlugins = {};
// [ts] /**
// [ts] * Recomputes the plugin list using the injected plugins and plugin ordering.
// [ts] *
// [ts] * @private
// [ts] */
// [ts] fn recomputePluginOrdering() {
// [ts] if (!eventPluginOrder) {
// Wait until an `eventPluginOrder` is injected.
// [ts] return;
// [ts] }
// [ts] for (let pluginName in namesToPlugins) {
// [ts] let pluginModule = namesToPlugins[pluginName];
// [ts] let pluginIndex = eventPluginOrder.indexOf(pluginName);
// [ts] if (!(pluginIndex > -1)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "EventPluginRegistry: Cannot inject event plugins that do not exist in the plugin ordering, `" + pluginName + "`." );)
// [ts] }
// [ts] }
// [ts] if (plugins[pluginIndex]) {
// [ts] continue;
// [ts] }
// [ts] if (!pluginModule.extractEvents) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "EventPluginRegistry: Event plugins must implement an `extractEvents` method, but `" + pluginName + "` does not." );)
// [ts] }
// [ts] }
// [ts] plugins[pluginIndex] = pluginModule;
// [ts] let publishedEvents = pluginModule.eventTypes;
// [ts] for (let eventName in publishedEvents) {
// [ts] if (!publishEventForPlugin(publishedEvents[eventName], pluginModule, eventName)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "EventPluginRegistry: Failed to publish event `" + eventName + "` for plugin `" + pluginName + "`." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Publishes an event so that it can be dispatched by the supplied plugin.
// [ts] *
// [ts] * @param {object} dispatchConfig Dispatch configuration for the event.
// [ts] * @param {object} PluginModule Plugin publishing the event.
// [ts] * @return {boolean} True if the event was successfully published.
// [ts] * @private
// [ts] */
// [ts] fn publishEventForPlugin(dispatchConfig: auto, pluginModule: auto, eventName: auto) {
// [ts] if (!!eventNameDispatchConfigs.hasOwnProperty(eventName)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "EventPluginRegistry: More than one plugin attempted to publish the same event name, `" + eventName + "`." );)
// [ts] }
// [ts] }
// [ts] eventNameDispatchConfigs[eventName] = dispatchConfig;
// [ts] let phasedRegistrationNames = dispatchConfig.phasedRegistrationNames;
// [ts] if (phasedRegistrationNames) {
// [ts] for (let phaseName in phasedRegistrationNames) {
// [ts] if (phasedRegistrationNames.hasOwnProperty(phaseName)) {
// [ts] let phasedRegistrationName = phasedRegistrationNames[phaseName];
// [ts] publishRegistrationName(phasedRegistrationName, pluginModule, eventName);
// [ts] }
// [ts] }
// [ts] return true;
// [ts] } else if (dispatchConfig.registrationName) {
// [ts] publishRegistrationName(dispatchConfig.registrationName, pluginModule, eventName);
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] /**
// [ts] * Publishes a registration name that is used to identify dispatched events.
// [ts] *
// [ts] * @param {string} registrationName Registration name to add.
// [ts] * @param {object} PluginModule Plugin publishing the event.
// [ts] * @private
// [ts] */
// [ts] fn publishRegistrationName(registrationName: auto, pluginModule: auto, eventName: auto) {
// [ts] if (!!registrationNameModules[registrationName]) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "EventPluginRegistry: More than one plugin attempted to publish the same registration name, `" + registrationName + "`." );)
// [ts] }
// [ts] }
// [ts] registrationNameModules[registrationName] = pluginModule;
// [ts] registrationNameDependencies[registrationName] = pluginModule.eventTypes[eventName].dependencies;
// [ts] {
// [ts] let lowerCasedName = registrationName.toLowerCase();
// [ts] possibleRegistrationNames[lowerCasedName] = registrationName;
// [ts] if (registrationName == "onDoubleClick") {
// [ts] possibleRegistrationNames.ondblclick = registrationName;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Registers plugins so that they can extract and dispatch events.
// [ts] */
// [ts] /**
// [ts] * Ordered list of injected plugins.
// [ts] */
// [ts] let plugins = [];
// [ts] /**
// [ts] * Mapping from event name to dispatch config
// [ts] */
// [ts] let eventNameDispatchConfigs = {};
// [ts] /**
// [ts] * Mapping from registration name to plugin module
// [ts] */
// [ts] let registrationNameModules = {};
// [ts] /**
// [ts] * Mapping from registration name to event name
// [ts] */
// [ts] let registrationNameDependencies = {};
// [ts] /**
// [ts] * Mapping from lowercase registration names to the properly cased version,
// [ts] * used to warn in the case of missing event handlers. Available
// [ts] * only in true.
// [ts] * @type {Object}
// [ts] */
// [ts] let possibleRegistrationNames =  {} ; // Trust the developer to only use possibleRegistrationNames in true
// [ts] /**
// [ts] * Injects an ordering of plugins (by plugin name). This allows the ordering
// [ts] * to be decoupled from injection of the actual plugins so that ordering is
// [ts] * always deterministic regardless of packaging, on-the-fly injection, etc.
// [ts] *
// [ts] * @param {array} InjectedEventPluginOrder
// [ts] * @internal
// [ts] */
// [ts] fn injectEventPluginOrder(injectedEventPluginOrder: auto) {
// [ts] if (!!eventPluginOrder) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "EventPluginRegistry: Cannot inject event plugin ordering more than once. You are likely trying to load more than one copy of React." );)
// [ts] }
// [ts] } // Clone the ordering so it cannot be dynamically mutated.
// [ts] eventPluginOrder = Array.prototype.slice.call(injectedEventPluginOrder);
// [ts] recomputePluginOrdering();
// [ts] }
// [ts] /**
// [ts] * Injects plugins to be used by plugin event system. The plugin names must be
// [ts] * in the ordering injected by `injectEventPluginOrder`.
// [ts] *
// [ts] * Plugins can be injected as part of page initialization or on-the-fly.
// [ts] *
// [ts] * @param {object} injectedNamesToPlugins Map from names to plugin modules.
// [ts] * @internal
// [ts] */
// [ts] fn injectEventPluginsByName(injectedNamesToPlugins: auto) {
// [ts] let isOrderingDirty = false;
// [ts] for (let pluginName in injectedNamesToPlugins) {
// [ts] if (!injectedNamesToPlugins.hasOwnProperty(pluginName)) {
// [ts] continue;
// [ts] }
// [ts] let pluginModule = injectedNamesToPlugins[pluginName];
// [ts] if (!namesToPlugins.hasOwnProperty(pluginName) || namesToPlugins[pluginName] != pluginModule) {
// [ts] if (!!namesToPlugins[pluginName]) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "EventPluginRegistry: Cannot inject two different event plugins using the same name, `" + pluginName + "`." );)
// [ts] }
// [ts] }
// [ts] namesToPlugins[pluginName] = pluginModule;
// [ts] isOrderingDirty = true;
// [ts] }
// [ts] }
// [ts] if (isOrderingDirty) {
// [ts] recomputePluginOrdering();
// [ts] }
// [ts] }
// [ts] let canUseDOM = !!(@typeName(window) != "undefined" && @typeName(window.document) != "undefined" && @typeName(window.document.createElement) != "undefined");
// [ts] let ReactInternals = React.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED;
// [ts] let _assign = ReactInternals.assign;
// [ts] let PLUGIN_EVENT_SYSTEM = 1;
// [ts] let IS_REPLAYED = 1 << 5;
// [ts] let IS_FIRST_ANCESTOR = 1 << 6;
// [ts] let restoreImpl = null;
// [ts] let restoreTarget = null;
// [ts] let restoreQueue = null;
// [ts] fn restoreStateOfTarget(target: auto) {
// We perform this translation at the end of the event loop so that we
// always receive the correct fiber here
// [ts] let internalInstance = getInstanceFromNode(target);
// [ts] if (!internalInstance) {
// Unmounted
// [ts] return;
// [ts] }
// [ts] if (!(@typeName(restoreImpl) == "function")) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "setRestoreImplementation() needs to be called to handle a target for controlled events. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] let stateNode = internalInstance.stateNode; // Guard against Fiber being unmounted.
// [ts] if (stateNode) {
// [ts] let _props = getFiberCurrentPropsFromNode(stateNode);
// [ts] restoreImpl(internalInstance.stateNode, internalInstance.type, _props);
// [ts] }
// [ts] }
// [ts] fn setRestoreImplementation(impl: auto) {
// [ts] restoreImpl = impl;
// [ts] }
// [ts] fn enqueueStateRestore(target: auto) {
// [ts] if (restoreTarget) {
// [ts] if (restoreQueue) {
// [ts] restoreQueue.push(target);
// [ts] } else {
// [ts] restoreQueue = [target];
// [ts] }
// [ts] } else {
// [ts] restoreTarget = target;
// [ts] }
// [ts] }
// [ts] fn needsStateRestore() {
// [ts] return restoreTarget != null || restoreQueue != null;
// [ts] }
// [ts] fn restoreStateIfNeeded() {
// [ts] if (!restoreTarget) {
// [ts] return;
// [ts] }
// [ts] let target = restoreTarget;
// [ts] let queuedTargets = restoreQueue;
// [ts] restoreTarget = null;
// [ts] restoreQueue = null;
// [ts] restoreStateOfTarget(target);
// [ts] if (queuedTargets) {
// [ts] let i = 0;
// [ts] while (i < queuedTargets.length) { // [cap-migrate] for-step: i++
// [ts] restoreStateOfTarget(queuedTargets[i]);
// [ts] }
// [ts] }
// [ts] }
// [ts] let enableProfilerTimer = true; // Trace which interactions trigger each commit.
// [ts] let enableDeprecatedFlareAPI = false; // Experimental Host Component support.
// [ts] let enableFundamentalAPI = false; // Experimental Scope support.
// [ts] let warnAboutStringRefs = false;
// the renderer. Such as when we're dispatching events or if third party
// libraries need to call batchedUpdates. Eventually, this API will go away when
// everything is batched by default. We'll then have a similar API to opt-out of
// scheduled work and instead do synchronous work.
// Defaults
// [ts] let batchedUpdatesImpl = (fn, bookkeeping) => {
// [ts] return fn(bookkeeping);
// [ts] };
// [ts] let discreteUpdatesImpl = (fn, a, b, c, d) => {
// [ts] return fn(a, b, c, d);
// [ts] };
// [ts] let flushDiscreteUpdatesImpl = () => {};
// [ts] let batchedEventUpdatesImpl = batchedUpdatesImpl;
// [ts] let isInsideEventHandler = false;
// [ts] let isBatchingEventUpdates = false;
// [ts] fn finishEventHandler() {
// Here we wait until all updates have propagated, which is important
// when using controlled components within layers:
// https://github.com/facebook/react/issues/1698
// Then we restore state of any controlled component.
// [ts] let controlledComponentsHavePendingUpdates = needsStateRestore();
// [ts] if (controlledComponentsHavePendingUpdates) {
// If a controlled event was fired, we may need to restore the state of
// the DOM node back to the controlled value. This is necessary when React
// bails out of the update without touching the DOM.
// [ts] flushDiscreteUpdatesImpl();
// [ts] restoreStateIfNeeded();
// [ts] }
// [ts] }
// [ts] fn batchedUpdates(fn: auto, bookkeeping: auto) {
// [ts] if (isInsideEventHandler) {
// If we are currently inside another batch, we need to wait until it
// fully completes before restoring state.
// [ts] return fn(bookkeeping);
// [ts] }
// [ts] isInsideEventHandler = true;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return batchedUpdatesImpl(fn, bookkeeping);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] isInsideEventHandler = false;
// [ts] finishEventHandler();
// [ts] }
// [ts] }
// [ts] fn batchedEventUpdates(fn: auto, a: auto, b: auto) {
// [ts] if (isBatchingEventUpdates) {
// If we are currently inside another batch, we need to wait until it
// fully completes before restoring state.
// [ts] return fn(a, b);
// [ts] }
// [ts] isBatchingEventUpdates = true;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return batchedEventUpdatesImpl(fn, a, b);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] isBatchingEventUpdates = false;
// [ts] finishEventHandler();
// [ts] }
// [ts] } // This is for the React Flare event system
// [ts] fn discreteUpdates(fn: auto, a: auto, b: auto, c: auto, d: auto) {
// [ts] let prevIsInsideEventHandler = isInsideEventHandler;
// [ts] isInsideEventHandler = true;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return discreteUpdatesImpl(fn, a, b, c, d);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] isInsideEventHandler = prevIsInsideEventHandler;
// [ts] if (!isInsideEventHandler) {
// [ts] finishEventHandler();
// [ts] }
// [ts] }
// [ts] }
// [ts] fn flushDiscreteUpdatesIfNeeded(timeStamp: auto) {
// event.timeStamp isn't overly reliable due to inconsistencies in
// how different browsers have historically provided the time stamp.
// Some browsers provide high-resolution time stamps for all events,
// some provide low-resolution time stamps for all events. FF < 52
// even mixes both time stamps together. Some browsers even report
// negative time stamps or time stamps that are 0 (iOS9) in some cases.
// Given we are only comparing two time stamps with equality (!==),
// we are safe from the resolution differences. If the time stamp is 0
// we bail-out of preventing the flush, which can affect semantics,
// such as if an earlier flush removes or adds event listeners that
// are fired in the subsequent flush. However, this is the same
// behaviour as we had before this change, so the risks are low.
// [ts] if (!isInsideEventHandler && (!enableDeprecatedFlareAPI  )) {
// [ts] flushDiscreteUpdatesImpl();
// [ts] }
// [ts] }
// [ts] fn setBatchingImplementation(_batchedUpdatesImpl: auto, _discreteUpdatesImpl: auto, _flushDiscreteUpdatesImpl: auto, _batchedEventUpdatesImpl: auto) {
// [ts] batchedUpdatesImpl = _batchedUpdatesImpl;
// [ts] discreteUpdatesImpl = _discreteUpdatesImpl;
// [ts] flushDiscreteUpdatesImpl = _flushDiscreteUpdatesImpl;
// [ts] batchedEventUpdatesImpl = _batchedEventUpdatesImpl;
// [ts] }
// [ts] let DiscreteEvent = 0;
// [ts] let UserBlockingEvent = 1;
// [ts] let ContinuousEvent = 2;
// [ts] let ReactInternals$1 = React.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED;
// [ts] let _ReactInternals$Sched = ReactInternals$1.Scheduler,
// [ts] unstable_cancelCallback = _ReactInternals$Sched.unstable_cancelCallback,
// [ts] unstable_now = _ReactInternals$Sched.unstable_now,
// [ts] unstable_scheduleCallback = _ReactInternals$Sched.unstable_scheduleCallback,
// [ts] unstable_shouldYield = _ReactInternals$Sched.unstable_shouldYield,
// [ts] unstable_requestPaint = _ReactInternals$Sched.unstable_requestPaint,
// [ts] unstable_getFirstCallbackNode = _ReactInternals$Sched.unstable_getFirstCallbackNode,
// [ts] unstable_runWithPriority = _ReactInternals$Sched.unstable_runWithPriority,
// [ts] unstable_next = _ReactInternals$Sched.unstable_next,
// [ts] unstable_continueExecution = _ReactInternals$Sched.unstable_continueExecution,
// [ts] unstable_pauseExecution = _ReactInternals$Sched.unstable_pauseExecution,
// [ts] unstable_getCurrentPriorityLevel = _ReactInternals$Sched.unstable_getCurrentPriorityLevel,
// [ts] unstable_ImmediatePriority = _ReactInternals$Sched.unstable_ImmediatePriority,
// [ts] unstable_UserBlockingPriority = _ReactInternals$Sched.unstable_UserBlockingPriority,
// [ts] unstable_NormalPriority = _ReactInternals$Sched.unstable_NormalPriority,
// [ts] unstable_LowPriority = _ReactInternals$Sched.unstable_LowPriority,
// [ts] unstable_IdlePriority = _ReactInternals$Sched.unstable_IdlePriority,
// [ts] unstable_forceFrameRate = _ReactInternals$Sched.unstable_forceFrameRate,
// [ts] unstable_flushAllWithoutAsserting = _ReactInternals$Sched.unstable_flushAllWithoutAsserting;
// A reserved attribute.
// It is handled by React separately and shouldn't be written to the DOM.
// [ts] let RESERVED = 0; // A simple string attribute.
// Attributes that aren't in the whitelist are presumed to have this type.
// [ts] let STRING = 1; // A string attribute that accepts booleans in React. In HTML, these are called
// "enumerated" attributes with "true" and "false" as possible values.
// When true, it should be set to a "true" string.
// When false, it should be set to a "false" string.
// [ts] let BOOLEANISH_STRING = 2; // A real boolean attribute.
// When true, it should be present (set either to an empty string or its name).
// When false, it should be omitted.
// [ts] let BOOLEAN = 3; // An attribute that can be used as a flag as well as with a value.
// When true, it should be present (set either to an empty string or its name).
// When false, it should be omitted.
// For any other value, should be present with that value.
// [ts] let OVERLOADED_BOOLEAN = 4; // An attribute that must be numeric or parse as a numeric.
// When falsy, it should be removed.
// [ts] let NUMERIC = 5; // An attribute that must be positive numeric or parse as a positive numeric.
// When falsy, it should be removed.
// [ts] let POSITIVE_NUMERIC = 6;
// [ts] /* eslint-disable max-len */
// [ts] let ATTRIBUTE_NAME_START_CHAR = ":A-Z_a-z\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD";
// [ts] /* eslint-enable max-len */
// [ts] let ATTRIBUTE_NAME_CHAR = ATTRIBUTE_NAME_START_CHAR + "\\-.0-9\\u00B7\\u0300-\\u036F\\u203F-\\u2040";
// [ts] let ROOT_ATTRIBUTE_NAME = "data-reactroot";
// [ts] let VALID_ATTRIBUTE_NAME_REGEX = RegExp.init("^[" + ATTRIBUTE_NAME_START_CHAR + "][" + ATTRIBUTE_NAME_CHAR + "]*$");
// [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
// [ts] let illegalAttributeNameCache = {};
// [ts] let validatedAttributeNameCache = {};
// [ts] fn isAttributeNameSafe(attributeName: auto) {
// [ts] if (hasOwnProperty.call(validatedAttributeNameCache, attributeName)) {
// [ts] return true;
// [ts] }
// [ts] if (hasOwnProperty.call(illegalAttributeNameCache, attributeName)) {
// [ts] return false;
// [ts] }
// [ts] if (VALID_ATTRIBUTE_NAME_REGEX.test(attributeName)) {
// [ts] validatedAttributeNameCache[attributeName] = true;
// [ts] return true;
// [ts] }
// [ts] illegalAttributeNameCache[attributeName] = true;
// [ts] {
// [ts] error("Invalid attribute name: `%s`", attributeName);
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn shouldIgnoreAttribute(name: auto, propertyInfo: auto, isCustomComponentTag: auto) {
// [ts] if (propertyInfo != null) {
// [ts] return propertyInfo.type == RESERVED;
// [ts] }
// [ts] if (isCustomComponentTag) {
// [ts] return false;
// [ts] }
// [ts] if (name.length > 2 && (name[0] == "o" || name[0] == "O") && (name[1] == "n" || name[1] == "N")) {
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn shouldRemoveAttributeWithWarning(name: auto, value: auto, propertyInfo: auto, isCustomComponentTag: auto) {
// [ts] if (propertyInfo != null && propertyInfo.type == RESERVED) {
// [ts] return false;
// [ts] }
// [ts] switch (@typeName(value)) {
// [ts] "function" => { // $FlowIssue symbol is perfectly valid here; },
// eslint-disable-line
// [ts] "symbol" => {
// [ts] return true;
// [ts] },
// [ts] "boolean" => {
// [ts] {
// [ts] if (isCustomComponentTag) {
// [ts] return false;
// [ts] }
// [ts] if (propertyInfo != null) {
// [ts] return !propertyInfo.acceptsBooleans;
// [ts] } else {
// [ts] let prefix = name.toLowerCase().slice(0, 5);
// [ts] return prefix != "data-" && prefix != "aria-";
// [ts] }
// [ts] }
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn shouldRemoveAttribute(name: auto, value: auto, propertyInfo: auto, isCustomComponentTag: auto) {
// [ts] if (value == null || @typeName(value) == "undefined") {
// [ts] return true;
// [ts] }
// [ts] if (shouldRemoveAttributeWithWarning(name, value, propertyInfo, isCustomComponentTag)) {
// [ts] return true;
// [ts] }
// [ts] if (isCustomComponentTag) {
// [ts] return false;
// [ts] }
// [ts] if (propertyInfo != null) {
// [ts] switch (propertyInfo.type) {
// [ts] BOOLEAN => {
// [ts] return !value;
// [ts] },
// [ts] OVERLOADED_BOOLEAN => {
// [ts] return value == false;
// [ts] },
// [ts] NUMERIC => {
// [ts] return isNaN(value);
// [ts] },
// [ts] POSITIVE_NUMERIC => {
// [ts] return isNaN(value) || value < 1;
// [ts] }
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn getPropertyInfo(name: auto) {
// [ts] return if (properties.hasOwnProperty(name)) properties[name] else null;
// [ts] }
// [ts] fn PropertyInfoRecord(name: auto, type: auto, mustUseProperty: auto, attributeName: auto, attributeNamespace: auto, sanitizeURL: auto) {
// [ts] _.acceptsBooleans = type == BOOLEANISH_STRING || type == BOOLEAN || type == OVERLOADED_BOOLEAN;
// [ts] _.attributeName = attributeName;
// [ts] _.attributeNamespace = attributeNamespace;
// [ts] _.mustUseProperty = mustUseProperty;
// [ts] _.propertyName = name;
// [ts] _.type = type;
// [ts] _.sanitizeURL = sanitizeURL;
// [ts] } // When adding attributes to this list, be sure to also add them to
// the `possibleStandardNames` module to ensure casing and incorrect
// name warnings.
// [ts] let properties = {}; // These props are reserved by React. They shouldn't be written to the DOM.
// [ts] let reservedProps = ["children", "dangerouslySetInnerHTML", // TODO: This prevents the assignment of defaultValue to regular
// elements (not just inputs). Now that ReactDOMInput assigns to the
// defaultValue property -- do we need this?
// [ts] "defaultValue", "defaultChecked", "innerHTML", "suppressContentEditableWarning", "suppressHydrationWarning", "style"];
// [ts] reservedProps.forEach((name) => {
// [ts] properties[name] = PropertyInfoRecord.init(name, RESERVED, false, // mustUseProperty
// [ts] name, // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // A few React string attributes have a different name.
// This is a mapping from React prop names to the attribute names.
// [ts] [["acceptCharset", "accept-charset"], ["className", "class"], ["htmlFor", "for"], ["httpEquiv", "http-equiv"]].forEach((_ref) => {
// [ts] let name = _ref[0],
// [ts] attributeName = _ref[1];
// [ts] properties[name] = PropertyInfoRecord.init(name, STRING, false, // mustUseProperty
// [ts] attributeName, // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // These are "enumerated" HTML attributes that accept "true" and "false".
// In React, we let users pass `true` and `false` even though technically
// these aren't boolean attributes (they are coerced to strings).
// [ts] ["contentEditable", "draggable", "spellCheck", "value"].forEach((name) => {
// [ts] properties[name] = PropertyInfoRecord.init(name, BOOLEANISH_STRING, false, // mustUseProperty
// [ts] name.toLowerCase(), // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // These are "enumerated" SVG attributes that accept "true" and "false".
// In React, we let users pass `true` and `false` even though technically
// these aren't boolean attributes (they are coerced to strings).
// Since these are SVG attributes, their attribute names are case-sensitive.
// [ts] ["autoReverse", "externalResourcesRequired", "focusable", "preserveAlpha"].forEach((name) => {
// [ts] properties[name] = PropertyInfoRecord.init(name, BOOLEANISH_STRING, false, // mustUseProperty
// [ts] name, // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // These are HTML boolean attributes.
// [ts] ["allowFullScreen", "async", // Note: there is a special case that prevents it from being written to the DOM
// on the client side because the browsers are inconsistent. Instead we call focus().
// [ts] "autoFocus", "autoPlay", "controls", "default", "defer", "disabled", "disablePictureInPicture", "formNoValidate", "hidden", "loop", "noModule", "noValidate", "open", "playsInline", "readOnly", "required", "reversed", "scoped", "seamless", // Microdata
// [ts] "itemScope"].forEach((name) => {
// [ts] properties[name] = PropertyInfoRecord.init(name, BOOLEAN, false, // mustUseProperty
// [ts] name.toLowerCase(), // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // These are the few React props that we set as DOM properties
// rather than attributes. These are all booleans.
// [ts] ["checked", // Note: `option.selected` is not updated if `select.multiple` is
// disabled with `removeAttribute`. We have special logic for handling this.
// [ts] "multiple", "muted", "selected" // NOTE: if you add a camelCased prop to this list,
// you'll need to set attributeName to name.toLowerCase()
// instead in the assignment below.
// [ts] ].forEach((name) => {
// [ts] properties[name] = PropertyInfoRecord.init(name, BOOLEAN, true, // mustUseProperty
// [ts] name, // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // These are HTML attributes that are "overloaded booleans": they behave like
// booleans, but can also accept a string value.
// [ts] ["capture", "download" // NOTE: if you add a camelCased prop to this list,
// you'll need to set attributeName to name.toLowerCase()
// instead in the assignment below.
// [ts] ].forEach((name) => {
// [ts] properties[name] = PropertyInfoRecord.init(name, OVERLOADED_BOOLEAN, false, // mustUseProperty
// [ts] name, // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // These are HTML attributes that must be positive numbers.
// [ts] ["cols", "rows", "size", "span" // NOTE: if you add a camelCased prop to this list,
// you'll need to set attributeName to name.toLowerCase()
// instead in the assignment below.
// [ts] ].forEach((name) => {
// [ts] properties[name] = PropertyInfoRecord.init(name, POSITIVE_NUMERIC, false, // mustUseProperty
// [ts] name, // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // These are HTML attributes that must be numbers.
// [ts] ["rowSpan", "start"].forEach((name) => {
// [ts] properties[name] = PropertyInfoRecord.init(name, NUMERIC, false, // mustUseProperty
// [ts] name.toLowerCase(), // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] });
// [ts] let CAMELIZE = "[\\-\\:]([a-z])";
// [ts] let capitalize = (token) => {
// [ts] return token[1].toUpperCase();
// [ts] }; // This is a list of all SVG attributes that need special casing, namespacing,
// or boolean value assignment. Regular attributes that just accept strings
// and have the same names are omitted, just like in the HTML whitelist.
// Some of these attributes can be hard to find. This list was created by
// scraping the MDN documentation.
// [ts] ["accent-height", "alignment-baseline", "arabic-form", "baseline-shift", "cap-height", "clip-path", "clip-rule", "color-interpolation", "color-interpolation-filters", "color-profile", "color-rendering", "dominant-baseline", "enable-background", "fill-opacity", "fill-rule", "flood-color", "flood-opacity", "font-family", "font-size", "font-size-adjust", "font-stretch", "font-style", "font-variant", "font-weight", "glyph-name", "glyph-orientation-horizontal", "glyph-orientation-vertical", "horiz-adv-x", "horiz-origin-x", "image-rendering", "letter-spacing", "lighting-color", "marker-end", "marker-mid", "marker-start", "overline-position", "overline-thickness", "paint-order", "panose-1", "pointer-events", "rendering-intent", "shape-rendering", "stop-color", "stop-opacity", "strikethrough-position", "strikethrough-thickness", "stroke-dasharray", "stroke-dashoffset", "stroke-linecap", "stroke-linejoin", "stroke-miterlimit", "stroke-opacity", "stroke-width", "text-anchor", "text-decoration", "text-rendering", "underline-position", "underline-thickness", "unicode-bidi", "unicode-range", "units-per-em", "v-alphabetic", "v-hanging", "v-ideographic", "v-mathematical", "vector-effect", "vert-adv-y", "vert-origin-x", "vert-origin-y", "word-spacing", "writing-mode", "xmlns:xlink", "x-height" // NOTE: if you add a camelCased prop to this list,
// you'll need to set attributeName to name.toLowerCase()
// instead in the assignment below.
// [ts] ].forEach((attributeName) => {
// [ts] let name = attributeName.replace(CAMELIZE, capitalize);
// [ts] properties[name] = PropertyInfoRecord.init(name, STRING, false, // mustUseProperty
// [ts] attributeName, null, // attributeNamespace
// [ts] false);
// [ts] }); // String SVG attributes with the xlink namespace.
// [ts] ["xlink:actuate", "xlink:arcrole", "xlink:role", "xlink:show", "xlink:title", "xlink:type" // NOTE: if you add a camelCased prop to this list,
// you'll need to set attributeName to name.toLowerCase()
// instead in the assignment below.
// [ts] ].forEach((attributeName) => {
// [ts] let name = attributeName.replace(CAMELIZE, capitalize);
// [ts] properties[name] = PropertyInfoRecord.init(name, STRING, false, // mustUseProperty
// [ts] attributeName, "http://www.w3.org/1999/xlink", false);
// [ts] }); // String SVG attributes with the xml namespace.
// [ts] ["xml:base", "xml:lang", "xml:space" // NOTE: if you add a camelCased prop to this list,
// you'll need to set attributeName to name.toLowerCase()
// instead in the assignment below.
// [ts] ].forEach((attributeName) => {
// [ts] let name = attributeName.replace(CAMELIZE, capitalize);
// [ts] properties[name] = PropertyInfoRecord.init(name, STRING, false, // mustUseProperty
// [ts] attributeName, "http://www.w3.org/XML/1998/namespace", false);
// [ts] }); // These attribute exists both in HTML and SVG.
// The attribute name is case-sensitive in SVG so we can't just use
// the React name like we do for attributes that exist only in HTML.
// [ts] ["tabIndex", "crossOrigin"].forEach((attributeName) => {
// [ts] properties[attributeName] = PropertyInfoRecord.init(attributeName, STRING, false, // mustUseProperty
// [ts] attributeName.toLowerCase(), // attributeName
// [ts] null, // attributeNamespace
// [ts] false);
// [ts] }); // These attributes accept URLs. These must not allow javascript: URLS.
// These will also need to accept Trusted Types object in the future.
// [ts] let xlinkHref = "xlinkHref";
// [ts] properties[xlinkHref] = PropertyInfoRecord.init("xlinkHref", STRING, false, // mustUseProperty
// [ts] "xlink:href", "http://www.w3.org/1999/xlink", true);
// [ts] ["src", "href", "action", "formAction"].forEach((attributeName) => {
// [ts] properties[attributeName] = PropertyInfoRecord.init(attributeName, STRING, false, // mustUseProperty
// [ts] attributeName.toLowerCase(), // attributeName
// [ts] null, // attributeNamespace
// [ts] true);
// [ts] });
// [ts] let ReactDebugCurrentFrame = null;
// [ts] {
// [ts] ReactDebugCurrentFrame = ReactSharedInternals.ReactDebugCurrentFrame;
// [ts] } // A javascript: URL can contain leading C0 control or \u0020 SPACE,
// and any newline or tab are filtered out as if they're not part of the URL.
// https://url.spec.whatwg.org/#url-parsing
// Tab or newline are defined as \r\n\t:
// https://infra.spec.whatwg.org/#ascii-tab-or-newline
// A C0 control is a code point in the range \u0000 NULL to \u001F
// INFORMATION SEPARATOR ONE, inclusive:
// https://infra.spec.whatwg.org/#c0-control-or-space
// [ts] /* eslint-disable max-len */
// [ts] let isJavaScriptProtocol = "^[\\u0000-\\u001F ]*j[\\r\\n\\t]*a[\\r\\n\\t]*v[\\r\\n\\t]*a[\\r\\n\\t]*s[\\r\\n\\t]*c[\\r\\n\\t]*r[\\r\\n\\t]*i[\\r\\n\\t]*p[\\r\\n\\t]*t[\\r\\n\\t]*\\:";
// [ts] let didWarn = false;
// [ts] fn sanitizeURL(url: auto) {
// [ts] {
// [ts] if (!didWarn && isJavaScriptProtocol.test(url)) {
// [ts] didWarn = true;
// [ts] error("A future version of React will block javascript: URLs as a security precaution. " + "Use event handlers instead if you can. If you need to generate unsafe HTML try " + "using dangerouslySetInnerHTML instead. React was passed %s.", JSON.stringify(url));
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Get the value for a property on a node. Only used in DEV for SSR validation.
// [ts] * The "expected" argument is used as a hint of what the expected value is.
// [ts] * Some properties have multiple equivalent values.
// [ts] */
// [ts] fn getValueForProperty(node: auto, name: auto, expected: auto, propertyInfo: auto) {
// [ts] {
// [ts] if (propertyInfo.mustUseProperty) {
// [ts] let propertyName = propertyInfo.propertyName;
// [ts] return node[propertyName];
// [ts] } else {
// [ts] if ( propertyInfo.sanitizeURL) {
// If we haven't fully disabled javascript: URLs, and if
// the hydration is successful of a javascript: URL, we
// still want to warn on the client.
// [ts] sanitizeURL("" + expected);
// [ts] }
// [ts] let attributeName = propertyInfo.attributeName;
// [ts] let stringValue = null;
// [ts] if (propertyInfo.type == OVERLOADED_BOOLEAN) {
// [ts] if (node.hasAttribute(attributeName)) {
// [ts] let value = node.getAttribute(attributeName);
// [ts] if (value == "") {
// [ts] return true;
// [ts] }
// [ts] if (shouldRemoveAttribute(name, expected, propertyInfo, false)) {
// [ts] return value;
// [ts] }
// [ts] if (value == "" + expected) {
// [ts] return expected;
// [ts] }
// [ts] return value;
// [ts] }
// [ts] } else if (node.hasAttribute(attributeName)) {
// [ts] if (shouldRemoveAttribute(name, expected, propertyInfo, false)) {
// We had an attribute but shouldn't have had one, so read it
// for the error message.
// [ts] return node.getAttribute(attributeName);
// [ts] }
// [ts] if (propertyInfo.type == BOOLEAN) {
// If this was a boolean, it doesn't matter what the value is
// the fact that we have it is the same as the expected.
// [ts] return expected;
// [ts] } // Even if this property uses a namespace we use getAttribute
// because we assume its namespaced name is the same as our config.
// To use getAttributeNS we need the local name which we don't have
// in our config atm.
// [ts] stringValue = node.getAttribute(attributeName);
// [ts] }
// [ts] if (shouldRemoveAttribute(name, expected, propertyInfo, false)) {
// [ts] return if (stringValue == null) expected else stringValue;
// [ts] } else if (stringValue == "" + expected) {
// [ts] return expected;
// [ts] } else {
// [ts] return stringValue;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Get the value for a attribute on a node. Only used in DEV for SSR validation.
// [ts] * The third argument is used as a hint of what the expected value is. Some
// [ts] * attributes have multiple equivalent values.
// [ts] */
// [ts] fn getValueForAttribute(node: auto, name: auto, expected: auto) {
// [ts] {
// [ts] if (!isAttributeNameSafe(name)) {
// [ts] return;
// [ts] }
// [ts] if (!node.hasAttribute(name)) {
// [ts] return if (expected == null) null else null;
// [ts] }
// [ts] let value = node.getAttribute(name);
// [ts] if (value == "" + expected) {
// [ts] return expected;
// [ts] }
// [ts] return value;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Sets the value for a property on a node.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {string} name
// [ts] * @param {*} value
// [ts] */
// [ts] fn setValueForProperty(node: auto, name: auto, value: auto, isCustomComponentTag: auto) {
// [ts] let propertyInfo = getPropertyInfo(name);
// [ts] if (shouldIgnoreAttribute(name, propertyInfo, isCustomComponentTag)) {
// [ts] return;
// [ts] }
// [ts] if (shouldRemoveAttribute(name, value, propertyInfo, isCustomComponentTag)) {
// [ts] value = null;
// [ts] } // If the prop isn't in the special list, treat it as a simple attribute.
// [ts] if (isCustomComponentTag || propertyInfo == null) {
// [ts] if (isAttributeNameSafe(name)) {
// [ts] let _attributeName = name;
// [ts] if (value == null) {
// [ts] node.removeAttribute(_attributeName);
// [ts] } else {
// [ts] node.setAttribute(_attributeName,  "" + value);
// [ts] }
// [ts] }
// [ts] return;
// [ts] }
// [ts] let mustUseProperty = propertyInfo.mustUseProperty;
// [ts] if (mustUseProperty) {
// [ts] let propertyName = propertyInfo.propertyName;
// [ts] if (value == null) {
// [ts] let type = propertyInfo.type;
// [ts] node[propertyName] = if (type == BOOLEAN) false else "";
// [ts] } else {
// Contrary to `setAttribute`, object properties are properly
// `toString`ed by IE8/9.
// [ts] node[propertyName] = value;
// [ts] }
// [ts] return;
// [ts] } // The rest are treated as attributes with special cases.
// [ts] let attributeName = propertyInfo.attributeName,
// [ts] attributeNamespace = propertyInfo.attributeNamespace;
// [ts] if (value == null) {
// [ts] node.removeAttribute(attributeName);
// [ts] } else {
// [ts] let _type = propertyInfo.type;
// [ts] let attributeValue;
// [ts] if (_type == BOOLEAN || _type == OVERLOADED_BOOLEAN && value == true) {
// If attribute type is boolean, we know for sure it won't be an execution sink
// and we won't require Trusted Type here.
// [ts] attributeValue = "";
// [ts] } else {
// `setAttribute` with objects becomes only `[object]` in IE8/9,
// ('' + value) makes it output the correct toString()-value.
// [ts] {
// [ts] attributeValue = "" + value;
// [ts] }
// [ts] if (propertyInfo.sanitizeURL) {
// [ts] sanitizeURL(attributeValue.toString());
// [ts] }
// [ts] }
// [ts] if (attributeNamespace) {
// [ts] node.setAttributeNS(attributeNamespace, attributeName, attributeValue);
// [ts] } else {
// [ts] node.setAttribute(attributeName, attributeValue);
// [ts] }
// [ts] }
// [ts] }
// [ts] let BEFORE_SLASH_RE = "^(.*)[\\\\\\/]";
// [ts] fn describeComponentFrame(name: auto, source: auto, ownerName: auto) {
// [ts] let sourceInfo = "";
// [ts] if (source) {
// [ts] let path = source.fileName;
// [ts] let fileName = path.replace(BEFORE_SLASH_RE, "");
// [ts] {
// In DEV, include code for a common special case:
// prefer "folder/index.js" instead of just "index.js".
// [ts] if ("^index\\.".test(fileName)) {
// [ts] let match_val = path.match(BEFORE_SLASH_RE);
// [ts] if (match_val) {
// [ts] let pathBeforeSlash = match_val[1];
// [ts] if (pathBeforeSlash) {
// [ts] let folderName = pathBeforeSlash.replace(BEFORE_SLASH_RE, "");
// [ts] fileName = folderName + "/" + fileName;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] sourceInfo = " (at " + fileName + ":" + source.lineNumber + ")";
// [ts] } else if (ownerName) {
// [ts] sourceInfo = " (created by " + ownerName + ")";
// [ts] }
// [ts] return "\n    in " + (name || "Unknown") + sourceInfo;
// [ts] }
// The Symbol used to tag the ReactElement-like types. If there is no native Symbol
// nor polyfill, then a plain number is used for performance.
// [ts] let hasSymbol = @typeName(Symbol) == "function" && "__symbol_for";
// [ts] let REACT_ELEMENT_TYPE = if (hasSymbol) "__symbol_react.element" else 0xeac7;
// [ts] let REACT_PORTAL_TYPE = if (hasSymbol) "__symbol_react.portal" else 0xeaca;
// [ts] let REACT_FRAGMENT_TYPE = if (hasSymbol) "__symbol_react.fragment" else 0xeacb;
// [ts] let REACT_STRICT_MODE_TYPE = if (hasSymbol) "__symbol_react.strict_mode" else 0xeacc;
// [ts] let REACT_PROFILER_TYPE = if (hasSymbol) "__symbol_react.profiler" else 0xead2;
// [ts] let REACT_PROVIDER_TYPE = if (hasSymbol) "__symbol_react.provider" else 0xeacd;
// [ts] let REACT_CONTEXT_TYPE = if (hasSymbol) "__symbol_react.context" else 0xeace; // TODO: We don't use AsyncMode or ConcurrentMode anymore. They were temporary
// [ts] let REACT_CONCURRENT_MODE_TYPE = if (hasSymbol) "__symbol_react.concurrent_mode" else 0xeacf;
// [ts] let REACT_FORWARD_REF_TYPE = if (hasSymbol) "__symbol_react.forward_ref" else 0xead0;
// [ts] let REACT_SUSPENSE_TYPE = if (hasSymbol) "__symbol_react.suspense" else 0xead1;
// [ts] let REACT_SUSPENSE_LIST_TYPE = if (hasSymbol) "__symbol_react.suspense_list" else 0xead8;
// [ts] let REACT_MEMO_TYPE = if (hasSymbol) "__symbol_react.memo" else 0xead3;
// [ts] let REACT_LAZY_TYPE = if (hasSymbol) "__symbol_react.lazy" else 0xead4;
// [ts] let REACT_BLOCK_TYPE = if (hasSymbol) "__symbol_react.block" else 0xead9;
// [ts] let MAYBE_ITERATOR_SYMBOL = @typeName(Symbol) == "function" && "__symbol_iterator";
// [ts] let FAUX_ITERATOR_SYMBOL = "@@iterator";
// [ts] fn getIteratorFn(maybeIterable: auto) {
// [ts] if (maybeIterable == null || @typeName(maybeIterable) != "object") {
// [ts] return null;
// [ts] }
// [ts] let maybeIterator = MAYBE_ITERATOR_SYMBOL && maybeIterable[MAYBE_ITERATOR_SYMBOL] || maybeIterable[FAUX_ITERATOR_SYMBOL];
// [ts] if (@typeName(maybeIterator) == "function") {
// [ts] return maybeIterator;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] let Uninitialized = -1;
// [ts] let Pending = 0;
// [ts] let Resolved = 1;
// [ts] let Rejected = 2;
// [ts] fn refineResolvedLazyComponent(lazyComponent: auto) {
// [ts] return if (lazyComponent._status == Resolved) lazyComponent._result else null;
// [ts] }
// [ts] fn initializeLazyComponentType(lazyComponent: auto) {
// [ts] if (lazyComponent._status == Uninitialized) {
// [ts] lazyComponent._status = Pending;
// [ts] let ctor = lazyComponent._ctor;
// [ts] let thenable = ctor();
// [ts] lazyComponent._result = thenable;
// [ts] thenable.then((moduleObject) => {
// [ts] if (lazyComponent._status == Pending) {
// [ts] let defaultExport = moduleObject.default;
// [ts] {
// [ts] if (defaultExport == null) {
// [ts] error("lazy: Expected the result of a dynamic import() call. " + "Instead received: %s\n\nYour code should look like: \n  " + "const MyComponent = lazy(() => import('./MyComponent'))", moduleObject);
// [ts] }
// [ts] }
// [ts] lazyComponent._status = Resolved;
// [ts] lazyComponent._result = defaultExport;
// [ts] }
// [ts] }, (error) => {
// [ts] if (lazyComponent._status == Pending) {
// [ts] lazyComponent._status = Rejected;
// [ts] lazyComponent._result = error;
// [ts] }
// [ts] });
// [ts] }
// [ts] }
// [ts] fn getWrappedName(outerType: auto, innerType: auto, wrapperName: auto) {
// [ts] let functionName = innerType.displayName || innerType.name || "";
// [ts] return outerType.displayName || (if (functionName != "") wrapperName + "(" + functionName + ")" else wrapperName);
// [ts] }
// [ts] fn getComponentName(type: auto) {
// [ts] if (type == null) {
// Host root, text node or just invalid type.
// [ts] return null;
// [ts] }
// [ts] {
// [ts] if (@typeName(type.tag) == "number") {
// [ts] error("Received an unexpected object in getComponentName(). " + "This is likely a bug in React. Please file an issue.");
// [ts] }
// [ts] }
// [ts] if (@typeName(type) == "function") {
// [ts] return type.displayName || type.name || null;
// [ts] }
// [ts] if (@typeName(type) == "string") {
// [ts] return type;
// [ts] }
// [ts] switch (type) {
// [ts] REACT_FRAGMENT_TYPE => {
// [ts] return "Fragment";
// [ts] },
// [ts] REACT_PORTAL_TYPE => {
// [ts] return "Portal";
// [ts] },
// [ts] REACT_PROFILER_TYPE => {
// [ts] return "Profiler";
// [ts] },
// [ts] REACT_STRICT_MODE_TYPE => {
// [ts] return "StrictMode";
// [ts] },
// [ts] REACT_SUSPENSE_TYPE => {
// [ts] return "Suspense";
// [ts] },
// [ts] REACT_SUSPENSE_LIST_TYPE => {
// [ts] return "SuspenseList";
// [ts] }
// [ts] }
// [ts] if (@typeName(type) == "object") {
// [ts] switch (type.$$typeof) {
// [ts] REACT_CONTEXT_TYPE => {
// [ts] return "Context.Consumer";
// [ts] },
// [ts] REACT_PROVIDER_TYPE => {
// [ts] return "Context.Provider";
// [ts] },
// [ts] REACT_FORWARD_REF_TYPE => {
// [ts] return getWrappedName(type, type.render, "ForwardRef");
// [ts] },
// [ts] REACT_MEMO_TYPE => {
// [ts] return getComponentName(type.type);
// [ts] },
// [ts] REACT_BLOCK_TYPE => {
// [ts] return getComponentName(type.render);
// [ts] },
// [ts] REACT_LAZY_TYPE => {
// [ts] {
// [ts] let thenable = type;
// [ts] let resolvedThenable = refineResolvedLazyComponent(thenable);
// [ts] if (resolvedThenable) {
// [ts] return getComponentName(resolvedThenable);
// [ts] }
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] let ReactDebugCurrentFrame$1 = ReactSharedInternals.ReactDebugCurrentFrame;
// [ts] fn describeFiber(fiber: auto) {
// [ts] switch (fiber.tag) {
// [ts] HostRoot, HostPortal, HostText, Fragment, ContextProvider, ContextConsumer => {
// [ts] return "";
// [ts] },
// [ts] else => {
// [ts] let owner = fiber._debugOwner;
// [ts] let source = fiber._debugSource;
// [ts] let name = getComponentName(fiber.type);
// [ts] let ownerName = null;
// [ts] if (owner) {
// [ts] ownerName = getComponentName(owner.type);
// [ts] }
// [ts] return describeComponentFrame(name, source, ownerName);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getStackByFiberInDevAndProd(workInProgress: auto) {
// [ts] let info = "";
// [ts] let node = workInProgress;
// [ts] do {
// [ts] info += describeFiber(node);
// [ts] node = node.return;
// [ts] } while (node);
// [ts] return info;
// [ts] }
// [ts] let current = null;
// [ts] let isRendering = false;
// [ts] fn getCurrentFiberOwnerNameInDevOrNull() {
// [ts] {
// [ts] if (current == null) {
// [ts] return null;
// [ts] }
// [ts] let owner = current._debugOwner;
// [ts] if (owner != null && @typeName(owner) != "undefined") {
// [ts] return getComponentName(owner.type);
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn getCurrentFiberStackInDev() {
// [ts] {
// [ts] if (current == null) {
// [ts] return "";
// [ts] } // Safe because if current fiber exists, we are reconciling,
// and it is guaranteed to be the work-in-progress version.
// [ts] return getStackByFiberInDevAndProd(current);
// [ts] }
// [ts] }
// [ts] fn resetCurrentFiber() {
// [ts] {
// [ts] ReactDebugCurrentFrame$1.getCurrentStack = null;
// [ts] current = null;
// [ts] isRendering = false;
// [ts] }
// [ts] }
// [ts] fn setCurrentFiber(fiber: auto) {
// [ts] {
// [ts] ReactDebugCurrentFrame$1.getCurrentStack = getCurrentFiberStackInDev;
// [ts] current = fiber;
// [ts] isRendering = false;
// [ts] }
// [ts] }
// [ts] fn setIsRendering(rendering: auto) {
// [ts] {
// [ts] isRendering = rendering;
// [ts] }
// [ts] }
// Flow does not allow string concatenation of most non-string types. To work
// around this limitation, we use an opaque type that can only be obtained by
// passing the value through getToStringValue first.
// [ts] fn toString(value: auto) {
// [ts] return "" + value;
// [ts] }
// [ts] fn getToStringValue(value: auto) {
// [ts] switch (@typeName(value)) {
// [ts] "boolean", "number", "object", "string", "undefined" => {
// [ts] return value;
// [ts] },
// [ts] else => {
// function, symbol are assigned as empty strings
// [ts] return "";
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] */
// [ts] let ReactPropTypesSecret = "SECRET_DO_NOT_PASS_THIS_OR_YOU_WILL_BE_FIRED";
// [ts] let ReactPropTypesSecret_1 = ReactPropTypesSecret;
// [ts] let printWarning$1 = () => {};
// [ts] {
// [ts] let ReactPropTypesSecret$1 = ReactPropTypesSecret_1;
// [ts] let loggedTypeFailures = {};
// [ts] let has = Function.call.bind(Object.prototype.hasOwnProperty);
// [ts] printWarning$1 = (text) => {
// [ts] let message = "Warning: " + text;
// [ts] if (@typeName(console) != "undefined") {
// [ts] @print(message);
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// --- Welcome to debugging React ---
// This error was thrown as a convenience so that you can use this stack
// to find the callsite that caused this warning to fire.
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(message);)
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Assert that the values match with the type specs.
// [ts] * Error messages are memorized and will only be shown once.
// [ts] *
// [ts] * @param {object} typeSpecs Map of name to a ReactPropType
// [ts] * @param {object} values Runtime values that need to be type-checked
// [ts] * @param {string} location e.g. "prop", "context", "child context"
// [ts] * @param {string} componentName Name of the component for error messages.
// [ts] * @param {?Function} getStack Returns the component stack.
// [ts] * @private
// [ts] */
// [ts] fn checkPropTypes(typeSpecs: auto, values: auto, location: auto, componentName: auto, getStack: auto) {
// [ts] {
// [ts] for (let typeSpecName in typeSpecs) {
// [ts] if (has(typeSpecs, typeSpecName)) {
// [ts] let error;
// Prop type validation may throw. In case they do, we don't want to
// fail the render phase where it didn't fail before. So we log it.
// After these have been cleaned up, we'll let them throw.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// This is intentionally an invariant that gets caught. It's the same
// behavior as without this statement except with a better message.
// [ts] if (@typeName(typeSpecs[typeSpecName]) != "function") {
// [ts] let err = Error(
// [ts] (componentName || "React class") + ": " + location + " type `" + typeSpecName + "` is invalid; " +
// [ts] "it must be a function, usually from the `prop-types` package, but received `" + @typeName(typeSpecs[typeSpecName]) + "`."
// [ts] );
// [ts] err.name = "Invariant Violation";
// [ts] return; // TODO(cap-migrate): throw -> error return (throw err;)
// [ts] }
// [ts] error = typeSpecs[typeSpecName](values, typeSpecName, componentName, location, null, ReactPropTypesSecret$1);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (ex) {)
// [ts] error = ex;
// [ts] }
// [ts] if (error && !(error == Error)) {
// [ts] printWarning$1(
// [ts] (componentName || "React class") + ": type specification of " +
// [ts] location + " `" + typeSpecName + "` is invalid; the type checker " +
// [ts] "function must return `null` or an `Error` but returned a " + @typeName(error) + ". " +
// [ts] "You may have forgotten to pass an argument to the type checker " +
// [ts] "creator (arrayOf, instanceOf, objectOf, oneOf, oneOfType, and " +
// [ts] "shape all require an argument)."
// [ts] );
// [ts] }
// [ts] if (error == Error && !(error.message == loggedTypeFailures)) {
// Only monitor this failure once because there tends to be a lot of the
// same error.
// [ts] loggedTypeFailures[error.message] = true;
// [ts] let stack = if (getStack) getStack() else "";
// [ts] printWarning$1(
// [ts] "Failed " + location + " type: " + error.message + (if (stack != null) stack else "")
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Resets warning cache when testing.
// [ts] *
// [ts] * @private
// [ts] */
// [ts] checkPropTypes.resetWarningCache = () => {
// [ts] {
// [ts] loggedTypeFailures = {};
// [ts] }
// [ts] };
// [ts] let checkPropTypes_1 = checkPropTypes;
// [ts] let ReactDebugCurrentFrame$2 = null;
// [ts] let ReactControlledValuePropTypes = {
// [ts] checkPropTypes: null
// [ts] };
// [ts] {
// [ts] ReactDebugCurrentFrame$2 = ReactSharedInternals.ReactDebugCurrentFrame;
// [ts] let hasReadOnlyValue = {
// [ts] button: true,
// [ts] checkbox: true,
// [ts] image: true,
// [ts] hidden: true,
// [ts] radio: true,
// [ts] reset: true,
// [ts] submit: true
// [ts] };
// [ts] let propTypes = {
// [ts] value: (props, propName, componentName) => {
// [ts] if (hasReadOnlyValue[props.type] || props.onChange || props.readOnly || props.disabled || props[propName] == null || enableDeprecatedFlareAPI ) {
// [ts] return null;
// [ts] }
// [ts] return Error.init("You provided a `value` prop to a form field without an " + "`onChange` handler. This will render a read-only field. If " + "the field should be mutable use `defaultValue`. Otherwise, " + "set either `onChange` or `readOnly`.");
// [ts] },
// [ts] checked: (props, propName, componentName) => {
// [ts] if (props.onChange || props.readOnly || props.disabled || props[propName] == null || enableDeprecatedFlareAPI ) {
// [ts] return null;
// [ts] }
// [ts] return Error.init("You provided a `checked` prop to a form field without an " + "`onChange` handler. This will render a read-only field. If " + "the field should be mutable use `defaultChecked`. Otherwise, " + "set either `onChange` or `readOnly`.");
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Provide a linked `value` attribute for controlled forms. You should not use
// [ts] * this outside of the ReactDOM controlled form components.
// [ts] */
// [ts] ReactControlledValuePropTypes.checkPropTypes = (tagName, props) => {
// [ts] checkPropTypes_1(propTypes, props, "prop", tagName, ReactDebugCurrentFrame$2.getStackAddendum);
// [ts] };
// [ts] }
// [ts] fn isCheckable(elem: auto) {
// [ts] let type = elem.type;
// [ts] let nodeName = elem.nodeName;
// [ts] return nodeName && nodeName.toLowerCase() == "input" && (type == "checkbox" || type == "radio");
// [ts] }
// [ts] fn getTracker(node: auto) {
// [ts] return node._valueTracker;
// [ts] }
// [ts] fn detachTracker(node: auto) {
// [ts] node._valueTracker = null;
// [ts] }
// [ts] fn getValueFromNode(node: auto) {
// [ts] let value = "";
// [ts] if (!node) {
// [ts] return value;
// [ts] }
// [ts] if (isCheckable(node)) {
// [ts] value = if (node.checked) "true" else "false";
// [ts] } else {
// [ts] value = node.value;
// [ts] }
// [ts] return value;
// [ts] }
// [ts] fn trackValueOnNode(node: auto) {
// [ts] let valueField = if (isCheckable(node)) "checked" else "value";
// [ts] let descriptor = Object.getOwnPropertyDescriptor(node.constructor.prototype, valueField);
// [ts] let currentValue = "" + node[valueField]; // if someone has already defined a value or Safari, then bail
// and don't track value will cause over reporting of changes,
// but it's better then a hard failure
// (needed for certain tests that spyOn input values and Safari)
// [ts] if (node.hasOwnProperty(valueField) || @typeName(descriptor) == "undefined" || @typeName(descriptor.get) != "function" || @typeName(descriptor.set) != "function") {
// [ts] return;
// [ts] }
// [ts] let get = descriptor.get,
// [ts] set = descriptor.set;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(node, valueField, {
// [ts-skip] configurable: true,
// [ts-skip] get: function () {
// [ts-skip] return get.call(this);
// [ts-skip] },
// [ts-skip] set: function (value) {
// [ts-skip] currentValue = '' + value;
// [ts-skip] set.call(this, value);
// [ts-skip] }
// [ts-skip] }); // We could've passed this the first time
// but it triggers a bug in IE11 and Edge 14/15.
// Calling defineProperty() again should be equivalent.
// https://github.com/facebook/react/issues/11768
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(node, valueField, {
// [ts-skip] enumerable: descriptor.enumerable
// [ts-skip] });
// [ts] let tracker = {
// [ts] getValue: () => {
// [ts] return currentValue;
// [ts] },
// [ts] setValue: (value) => {
// [ts] currentValue = "" + value;
// [ts] },
// [ts] stopTracking: () => {
// [ts] detachTracker(node);
// [ts] delete node[valueField];
// [ts] }
// [ts] };
// [ts] return tracker;
// [ts] }
// [ts] fn track(node: auto) {
// [ts] if (getTracker(node)) {
// [ts] return;
// [ts] } // TODO: Once it's just Fiber we can move this to node._wrapperState
// [ts] node._valueTracker = trackValueOnNode(node);
// [ts] }
// [ts] fn updateValueIfChanged(node: auto) {
// [ts] if (!node) {
// [ts] return false;
// [ts] }
// [ts] let tracker = getTracker(node); // if there is no tracker at this point it's unlikely
// that trying again will succeed
// [ts] if (!tracker) {
// [ts] return true;
// [ts] }
// [ts] let lastValue = tracker.getValue();
// [ts] let nextValue = getValueFromNode(node);
// [ts] if (nextValue != lastValue) {
// [ts] tracker.setValue(nextValue);
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] let didWarnValueDefaultValue = false;
// [ts] let didWarnCheckedDefaultChecked = false;
// [ts] let didWarnControlledToUncontrolled = false;
// [ts] let didWarnUncontrolledToControlled = false;
// [ts] fn isControlled(props: auto) {
// [ts] let usesChecked = props.type == "checkbox" || props.type == "radio";
// [ts] return if (usesChecked) props.checked != null else props.value != null;
// [ts] }
// [ts] /**
// [ts] * Implements an <input> host component that allows setting these optional
// [ts] * props: `checked`, `value`, `defaultChecked`, and `defaultValue`.
// [ts] *
// [ts] * If `checked` or `value` are not supplied (or null/undefined), user actions
// [ts] * that affect the checked state or value will trigger updates to the element.
// [ts] *
// [ts] * If they are supplied (and not null/undefined), the rendered element will not
// [ts] * trigger updates to the element. Instead, the props must change in order for
// [ts] * the rendered element to be updated.
// [ts] *
// [ts] * The rendered element will be initialized as unchecked (or `defaultChecked`)
// [ts] * with an empty value (or `defaultValue`).
// [ts] *
// [ts] * See http://www.w3.org/TR/2012/WD-html5-20121025/the-input-element.html
// [ts] */
// [ts] fn getHostProps(element: auto, props: auto) {
// [ts] let node = element;
// [ts] let checked = props.checked;
// [ts] let hostProps = _assign({}, props, {
// [ts] defaultChecked: null,
// [ts] defaultValue: null,
// [ts] value: null,
// [ts] checked: if (checked != null) checked else node._wrapperState.initialChecked
// [ts] });
// [ts] return hostProps;
// [ts] }
// [ts] fn initWrapperState(element: auto, props: auto) {
// [ts] {
// [ts] ReactControlledValuePropTypes.checkPropTypes("input", props);
// [ts] if (props.checked != null && props.defaultChecked != null && !didWarnCheckedDefaultChecked) {
// [ts] error("%s contains an input of type %s with both checked and defaultChecked props. " + "Input elements must be either controlled or uncontrolled " + "(specify either the checked prop, or the defaultChecked prop, but not " + "both). Decide between using a controlled or uncontrolled input " + "element and remove one of these props. More info: " + "https://fb.me/react-controlled-components", getCurrentFiberOwnerNameInDevOrNull() || "A component", props.type);
// [ts] didWarnCheckedDefaultChecked = true;
// [ts] }
// [ts] if (props.value != null && props.defaultValue != null && !didWarnValueDefaultValue) {
// [ts] error("%s contains an input of type %s with both value and defaultValue props. " + "Input elements must be either controlled or uncontrolled " + "(specify either the value prop, or the defaultValue prop, but not " + "both). Decide between using a controlled or uncontrolled input " + "element and remove one of these props. More info: " + "https://fb.me/react-controlled-components", getCurrentFiberOwnerNameInDevOrNull() || "A component", props.type);
// [ts] didWarnValueDefaultValue = true;
// [ts] }
// [ts] }
// [ts] let node = element;
// [ts] let defaultValue = if (props.defaultValue == null) "" else props.defaultValue;
// [ts] node._wrapperState = {
// [ts] initialChecked: if (props.checked != null) props.checked else props.defaultChecked,
// [ts] initialValue: getToStringValue(if (props.value != null) props.value else defaultValue),
// [ts] controlled: isControlled(props)
// [ts] };
// [ts] }
// [ts] fn updateChecked(element: auto, props: auto) {
// [ts] let node = element;
// [ts] let checked = props.checked;
// [ts] if (checked != null) {
// [ts] setValueForProperty(node, "checked", checked, false);
// [ts] }
// [ts] }
// [ts] fn updateWrapper(element: auto, props: auto) {
// [ts] let node = element;
// [ts] {
// [ts] let controlled = isControlled(props);
// [ts] if (!node._wrapperState.controlled && controlled && !didWarnUncontrolledToControlled) {
// [ts] error("A component is changing an uncontrolled input of type %s to be controlled. " + "Input elements should not switch from uncontrolled to controlled (or vice versa). " + "Decide between using a controlled or uncontrolled input " + "element for the lifetime of the component. More info: https://fb.me/react-controlled-components", props.type);
// [ts] didWarnUncontrolledToControlled = true;
// [ts] }
// [ts] if (node._wrapperState.controlled && !controlled && !didWarnControlledToUncontrolled) {
// [ts] error("A component is changing a controlled input of type %s to be uncontrolled. " + "Input elements should not switch from controlled to uncontrolled (or vice versa). " + "Decide between using a controlled or uncontrolled input " + "element for the lifetime of the component. More info: https://fb.me/react-controlled-components", props.type);
// [ts] didWarnControlledToUncontrolled = true;
// [ts] }
// [ts] }
// [ts] updateChecked(element, props);
// [ts] let value = getToStringValue(props.value);
// [ts] let type = props.type;
// [ts] if (value != null) {
// [ts] if (type == "number") {
// [ts] if (value == 0 && node.value == "" || // We explicitly want to coerce to number here if possible.
// eslint-disable-next-line
// [ts] node.value != value) {
// [ts] node.value = toString(value);
// [ts] }
// [ts] } else if (node.value != toString(value)) {
// [ts] node.value = toString(value);
// [ts] }
// [ts] } else if (type == "submit" || type == "reset") {
// Submit/reset inputs need the attribute removed completely to avoid
// blank-text buttons.
// [ts] node.removeAttribute("value");
// [ts] return;
// [ts] }
// [ts] {
// When syncing the value attribute, the value comes from a cascade of
// properties:
//  1. The value React property
//  2. The defaultValue React property
//  3. Otherwise there should be no change
// [ts] if (props.hasOwnProperty("value")) {
// [ts] setDefaultValue(node, props.type, value);
// [ts] } else if (props.hasOwnProperty("defaultValue")) {
// [ts] setDefaultValue(node, props.type, getToStringValue(props.defaultValue));
// [ts] }
// [ts] }
// [ts] {
// When syncing the checked attribute, it only changes when it needs
// to be removed, such as transitioning from a checkbox into a text input
// [ts] if (props.checked == null && props.defaultChecked != null) {
// [ts] node.defaultChecked = !!props.defaultChecked;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn postMountWrapper(element: auto, props: auto, isHydrating: auto) {
// [ts] let node = element; // Do not assign value if it is already set. This prevents user text input
// from being lost during SSR hydration.
// [ts] if (props.hasOwnProperty("value") || props.hasOwnProperty("defaultValue")) {
// [ts] let type = props.type;
// [ts] let isButton = type == "submit" || type == "reset"; // Avoid setting value attribute on submit/reset inputs as it overrides the
// default value provided by the browser. See: #12872
// [ts] if (isButton && (props.value == null || props.value == null)) {
// [ts] return;
// [ts] }
// [ts] let initialValue = toString(node._wrapperState.initialValue); // Do not assign value if it is already set. This prevents user text input
// from being lost during SSR hydration.
// [ts] if (!isHydrating) {
// [ts] {
// When syncing the value attribute, the value property should use
// the wrapperState._initialValue property. This uses:
//
//   1. The value React property when present
//   2. The defaultValue React property when present
//   3. An empty string
// [ts] if (initialValue != node.value) {
// [ts] node.value = initialValue;
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// Otherwise, the value attribute is synchronized to the property,
// so we assign defaultValue to the same thing as the value property
// assignment step above.
// [ts] node.defaultValue = initialValue;
// [ts] }
// [ts] } // Normally, we'd just do `node.checked = node.checked` upon initial mount, less this bug
// this is needed to work around a chrome bug where setting defaultChecked
// will sometimes influence the value of checked (even after detachment).
// Reference: https://bugs.chromium.org/p/chromium/issues/detail?id=608416
// We need to temporarily unset name to avoid disrupting radio button groups.
// [ts] let name = node.name;
// [ts] if (name != "") {
// [ts] node.name = "";
// [ts] }
// [ts] {
// When syncing the checked attribute, both the checked property and
// attribute are assigned at the same time using defaultChecked. This uses:
//
//   1. The checked React property when present
//   2. The defaultChecked React property when present
//   3. Otherwise, false
// [ts] node.defaultChecked = !node.defaultChecked;
// [ts] node.defaultChecked = !!node._wrapperState.initialChecked;
// [ts] }
// [ts] if (name != "") {
// [ts] node.name = name;
// [ts] }
// [ts] }
// [ts] fn restoreControlledState(element: auto, props: auto) {
// [ts] let node = element;
// [ts] updateWrapper(node, props);
// [ts] updateNamedCousins(node, props);
// [ts] }
// [ts] fn updateNamedCousins(rootNode: auto, props: auto) {
// [ts] let name = props.name;
// [ts] if (props.type == "radio" && name != null) {
// [ts] let queryRoot = rootNode;
// [ts] while (queryRoot.parentNode) {
// [ts] queryRoot = queryRoot.parentNode;
// [ts] } // If `rootNode.form` was non-null, then we could try `form.elements`,
// but that sometimes behaves strangely in IE8. We could also try using
// `form.getElementsByName`, but that will only return direct children
// and won't include inputs that use the HTML5 `form=` attribute. Since
// the input might not even be in a form. It might not even be in the
// document. Let's just use the local `querySelectorAll` to ensure we don't
// miss anything.
// [ts] let group = queryRoot.querySelectorAll("input[name=" + JSON.stringify("" + name) + "][type=\"radio\"]");
// [ts] let i = 0;
// [ts] while (i < group.length) { // [cap-migrate] for-step: i++
// [ts] let otherNode = group[i];
// [ts] if (otherNode == rootNode || otherNode.form != rootNode.form) {
// [ts] continue;
// [ts] } // This will throw if radio buttons rendered by different copies of React
// and the same name are rendered into the same form (same as #1939).
// That's probably okay; we don't support it just as we don't support
// mixing React radio buttons with non-React ones.
// [ts] let otherProps = getFiberCurrentPropsFromNode$1(otherNode);
// [ts] if (!otherProps) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "ReactDOMInput: Mixing React and non-React radio inputs with the same `name` is not supported." );)
// [ts] }
// [ts] } // We need update the tracked value on the named cousin since the value
// was changed but the input saw no event or value set
// [ts] updateValueIfChanged(otherNode); // If this is a controlled radio button group, forcing the input that
// was previously checked to update will cause it to be come re-checked
// as appropriate.
// [ts] updateWrapper(otherNode, otherProps);
// [ts] }
// [ts] }
// [ts] } // In Chrome, assigning defaultValue to certain input types triggers input validation.
// For number inputs, the display value loses trailing decimal points. For email inputs,
// Chrome raises "The specified value <x> is not a valid email address".
//
// Here we check to see if the defaultValue has actually changed, avoiding these problems
// when the user is inputting text
//
// https://github.com/facebook/react/issues/7253
// [ts] fn setDefaultValue(node: auto, type: auto, value: auto) {
// [ts] if ( // Focused number inputs synchronize on blur. See ChangeEventPlugin.js
// [ts] type != "number" || node.ownerDocument.activeElement != node) {
// [ts] if (value == null) {
// [ts] node.defaultValue = toString(node._wrapperState.initialValue);
// [ts] } else if (node.defaultValue != toString(value)) {
// [ts] node.defaultValue = toString(value);
// [ts] }
// [ts] }
// [ts] }
// [ts] let didWarnSelectedSetOnOption = false;
// [ts] let didWarnInvalidChild = false;
// [ts] fn flattenChildren(children: auto) {
// [ts] let content = ""; // Flatten children. We'll warn if they are invalid
// during validateProps() which runs for hydration too.
// Note that this would throw on non-element objects.
// Elements are stringified (which is normally irrelevant
// but matters for <fbt>).
// [ts] React.Children.forEach(children, (child) => {
// [ts] if (child == null) {
// [ts] return;
// [ts] }
// [ts] content += child; // Note: we don't warn about invalid children here.
// Instead, this is done separately below so that
// it happens during the hydration codepath too.
// [ts] });
// [ts] return content;
// [ts] }
// [ts] /**
// [ts] * Implements an <option> host component that warns when `selected` is set.
// [ts] */
// [ts] fn validateProps(element: auto, props: auto) {
// [ts] {
// This mirrors the codepath above, but runs for hydration too.
// Warn about invalid children here so that client and hydration are consistent.
// TODO: this seems like it could cause a DEV-only throw for hydration
// if children contains a non-element object. We should try to avoid that.
// [ts] if (@typeName(props.children) == "object" && props.children != null) {
// [ts] React.Children.forEach(props.children, (child) => {
// [ts] if (child == null) {
// [ts] return;
// [ts] }
// [ts] if (@typeName(child) == "string" || @typeName(child) == "number") {
// [ts] return;
// [ts] }
// [ts] if (@typeName(child.type) != "string") {
// [ts] return;
// [ts] }
// [ts] if (!didWarnInvalidChild) {
// [ts] didWarnInvalidChild = true;
// [ts] error("Only strings and numbers are supported as <option> children.");
// [ts] }
// [ts] });
// [ts] } // TODO: Remove support for `selected` in <option>.
// [ts] if (props.selected != null && !didWarnSelectedSetOnOption) {
// [ts] error("Use the `defaultValue` or `value` props on <select> instead of " + "setting `selected` on <option>.");
// [ts] didWarnSelectedSetOnOption = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] (element, props) => {
// value="" should make a value attribute (#6219)
// [ts] if (props.value != null) {
// [ts] element.setAttribute("value", toString(getToStringValue(props.value)));
// [ts] }
// [ts] }
// [ts] (element, props) => {
// [ts] let hostProps = _assign({
// [ts] children: null
// [ts] }, props);
// [ts] let content = flattenChildren(props.children);
// [ts] if (content) {
// [ts] hostProps.children = content;
// [ts] }
// [ts] return hostProps;
// [ts] }
// [ts] let didWarnValueDefaultValue$1;
// [ts] {
// [ts] didWarnValueDefaultValue$1 = false;
// [ts] }
// [ts] fn getDeclarationErrorAddendum() {
// [ts] let ownerName = getCurrentFiberOwnerNameInDevOrNull();
// [ts] if (ownerName) {
// [ts] return "\n\nCheck the render method of `" + ownerName + "`.";
// [ts] }
// [ts] return "";
// [ts] }
// [ts] let valuePropNames = ["value", "defaultValue"];
// [ts] /**
// [ts] * Validation function for `value` and `defaultValue`.
// [ts] */
// [ts] fn checkSelectPropTypes(props: auto) {
// [ts] {
// [ts] ReactControlledValuePropTypes.checkPropTypes("select", props);
// [ts] let i = 0;
// [ts] while (i < valuePropNames.length) { // [cap-migrate] for-step: i++
// [ts] let propName = valuePropNames[i];
// [ts] if (props[propName] == null) {
// [ts] continue;
// [ts] }
// [ts] let isArray = Array.isArray(props[propName]);
// [ts] if (props.multiple && !isArray) {
// [ts] error("The `%s` prop supplied to <select> must be an array if " + "`multiple` is true.%s", propName, getDeclarationErrorAddendum());
// [ts] } else if (!props.multiple && isArray) {
// [ts] error("The `%s` prop supplied to <select> must be a scalar " + "value if `multiple` is false.%s", propName, getDeclarationErrorAddendum());
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn updateOptions(node: auto, multiple: auto, propValue: auto, setDefaultSelected: auto) {
// [ts] let options = node.options;
// [ts] if (multiple) {
// [ts] let selectedValues = propValue;
// [ts] let selectedValue = {};
// [ts] let i = 0;
// [ts] while (i < selectedValues.length) { // [cap-migrate] for-step: i++
// Prefix to avoid chaos with special keys.
// [ts] selectedValue["$" + selectedValues[i]] = true;
// [ts] }
// [ts] let _i = 0;
// [ts] while (_i < options.length) { // [cap-migrate] for-step: _i++
// [ts] let selected = selectedValue.hasOwnProperty("$" + options[_i].value);
// [ts] if (options[_i].selected != selected) {
// [ts] options[_i].selected = selected;
// [ts] }
// [ts] if (selected && setDefaultSelected) {
// [ts] options[_i].defaultSelected = true;
// [ts] }
// [ts] }
// [ts] } else {
// Do not set `select.value` as exact behavior isn't consistent across all
// browsers for all cases.
// [ts] let _selectedValue = toString(getToStringValue(propValue));
// [ts] let defaultSelected = null;
// [ts] let _i2 = 0;
// [ts] while (_i2 < options.length) { // [cap-migrate] for-step: _i2++
// [ts] if (options[_i2].value == _selectedValue) {
// [ts] options[_i2].selected = true;
// [ts] if (setDefaultSelected) {
// [ts] options[_i2].defaultSelected = true;
// [ts] }
// [ts] return;
// [ts] }
// [ts] if (defaultSelected == null && !options[_i2].disabled) {
// [ts] defaultSelected = options[_i2];
// [ts] }
// [ts] }
// [ts] if (defaultSelected != null) {
// [ts] defaultSelected.selected = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Implements a <select> host component that allows optionally setting the
// [ts] * props `value` and `defaultValue`. If `multiple` is false, the prop must be a
// [ts] * stringable. If `multiple` is true, the prop must be an array of stringables.
// [ts] *
// [ts] * If `value` is not supplied (or null/undefined), user actions that change the
// [ts] * selected option will trigger updates to the rendered options.
// [ts] *
// [ts] * If it is supplied (and not null/undefined), the rendered options will not
// [ts] * update in response to user actions. Instead, the `value` prop must change in
// [ts] * order for the rendered options to update.
// [ts] *
// [ts] * If `defaultValue` is provided, any options with the supplied values will be
// [ts] * selected.
// [ts] */
// [ts] (element, props) => {
// [ts] return _assign({}, props, {
// [ts] value: null
// [ts] });
// [ts] }
// [ts] (element, props) => {
// [ts] let node = element;
// [ts] {
// [ts] checkSelectPropTypes(props);
// [ts] }
// [ts] node._wrapperState = {
// [ts] wasMultiple: !!props.multiple
// [ts] };
// [ts] {
// [ts] if (props.value != null && props.defaultValue != null && !didWarnValueDefaultValue$1) {
// [ts] error("Select elements must be either controlled or uncontrolled " + "(specify either the value prop, or the defaultValue prop, but not " + "both). Decide between using a controlled or uncontrolled select " + "element and remove one of these props. More info: " + "https://fb.me/react-controlled-components");
// [ts] didWarnValueDefaultValue$1 = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] (element, props) => {
// [ts] let node = element;
// [ts] node.multiple = !!props.multiple;
// [ts] let value = props.value;
// [ts] if (value != null) {
// [ts] updateOptions(node, !!props.multiple, value, false);
// [ts] } else if (props.defaultValue != null) {
// [ts] updateOptions(node, !!props.multiple, props.defaultValue, true);
// [ts] }
// [ts] }
// [ts] fn postUpdateWrapper(element: auto, props: auto) {
// [ts] let node = element;
// [ts] let wasMultiple = node._wrapperState.wasMultiple;
// [ts] node._wrapperState.wasMultiple = !!props.multiple;
// [ts] let value = props.value;
// [ts] if (value != null) {
// [ts] updateOptions(node, !!props.multiple, value, false);
// [ts] } else if (wasMultiple != !!props.multiple) {
// For simplicity, reapply `defaultValue` if `multiple` is toggled.
// [ts] if (props.defaultValue != null) {
// [ts] updateOptions(node, !!props.multiple, props.defaultValue, true);
// [ts] } else {
// Revert the select back to its default unselected state.
// [ts] updateOptions(if (node, !!props.multiple, props.multiple) [] else "", false);
// [ts] }
// [ts] }
// [ts] }
// [ts] (element, props) => {
// [ts] let node = element;
// [ts] let value = props.value;
// [ts] if (value != null) {
// [ts] updateOptions(node, !!props.multiple, value, false);
// [ts] }
// [ts] }
// [ts] let didWarnValDefaultVal = false;
// [ts] /**
// [ts] * Implements a <textarea> host component that allows setting `value`, and
// [ts] * `defaultValue`. This differs from the traditional DOM API because value is
// [ts] * usually set as PCDATA children.
// [ts] *
// [ts] * If `value` is not supplied (or null/undefined), user actions that affect the
// [ts] * value will trigger updates to the element.
// [ts] *
// [ts] * If `value` is supplied (and not null/undefined), the rendered element will
// [ts] * not trigger updates to the element. Instead, the `value` prop must change in
// [ts] * order for the rendered element to be updated.
// [ts] *
// [ts] * The rendered element will be initialized with an empty value, the prop
// [ts] * `defaultValue` if specified, or the children content (deprecated).
// [ts] */
// [ts] (element, props) => {
// [ts] let node = element;
// [ts] if (!(props.dangerouslySetInnerHTML == null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "`dangerouslySetInnerHTML` does not make sense on <textarea>." );)
// [ts] }
// [ts] } // Always set children to the same thing. In IE9, the selection range will
// get reset if `textContent` is mutated.  We could add a check in setTextContent
// to only set the value if/when the value differs from the node value (which would
// completely solve this IE9 bug), but Sebastian+Sophie seemed to like this
// solution. The value can be a boolean or object so that's why it's forced
// to be a string.
// [ts] let hostProps = _assign({}, props, {
// [ts] value: null,
// [ts] defaultValue: null,
// [ts] children: toString(node._wrapperState.initialValue)
// [ts] });
// [ts] return hostProps;
// [ts] }
// [ts] (element, props) => {
// [ts] let node = element;
// [ts] {
// [ts] ReactControlledValuePropTypes.checkPropTypes("textarea", props);
// [ts] if (props.value != null && props.defaultValue != null && !didWarnValDefaultVal) {
// [ts] error("%s contains a textarea with both value and defaultValue props. " + "Textarea elements must be either controlled or uncontrolled " + "(specify either the value prop, or the defaultValue prop, but not " + "both). Decide between using a controlled or uncontrolled textarea " + "and remove one of these props. More info: " + "https://fb.me/react-controlled-components", getCurrentFiberOwnerNameInDevOrNull() || "A component");
// [ts] didWarnValDefaultVal = true;
// [ts] }
// [ts] }
// [ts] let initialValue = props.value; // Only bother fetching default value if we're going to use it
// [ts] if (initialValue == null) {
// [ts] let children = props.children,
// [ts] defaultValue = props.defaultValue;
// [ts] if (children != null) {
// [ts] {
// [ts] error("Use the `defaultValue` or `value` props instead of setting " + "children on <textarea>.");
// [ts] }
// [ts] {
// [ts] if (!(defaultValue == null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "If you supply `defaultValue` on a <textarea>, do not pass children." );)
// [ts] }
// [ts] }
// [ts] if (Array.isArray(children)) {
// [ts] if (!(children.length <= 1)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "<textarea> can only have at most one child." );)
// [ts] }
// [ts] }
// [ts] children = children[0];
// [ts] }
// [ts] defaultValue = children;
// [ts] }
// [ts] }
// [ts] if (defaultValue == null) {
// [ts] defaultValue = "";
// [ts] }
// [ts] initialValue = defaultValue;
// [ts] }
// [ts] node._wrapperState = {
// [ts] initialValue: getToStringValue(initialValue)
// [ts] };
// [ts] }
// [ts] (element, props) => {
// [ts] let node = element;
// [ts] let value = getToStringValue(props.value);
// [ts] let defaultValue = getToStringValue(props.defaultValue);
// [ts] if (value != null) {
// Cast `value` to a string to ensure the value is set correctly. While
// browsers typically do this as necessary, jsdom doesn't.
// [ts] let newValue = toString(value); // To avoid side effects (such as losing text selection), only set value if changed
// [ts] if (newValue != node.value) {
// [ts] node.value = newValue;
// [ts] }
// [ts] if (props.defaultValue == null && node.defaultValue != newValue) {
// [ts] node.defaultValue = newValue;
// [ts] }
// [ts] }
// [ts] if (defaultValue != null) {
// [ts] node.defaultValue = toString(defaultValue);
// [ts] }
// [ts] }
// [ts] (element, props) => {
// [ts] let node = element; // This is in postMount because we need access to the DOM node, which is not
// available until after the component has mounted.
// [ts] let textContent = node.textContent; // Only set node.value if textContent is equal to the expected
// initial value. In IE10/IE11 there is a bug where the placeholder attribute
// will populate textContent as well.
// https://developer.microsoft.com/microsoft-edge/platform/issues/101525/
// [ts] if (textContent == node._wrapperState.initialValue) {
// [ts] if (textContent != "" && textContent != null) {
// [ts] node.value = textContent;
// [ts] }
// [ts] }
// [ts] }
// [ts] (element, props) => {
// DOM component is still mounted; update
// [ts] updateWrapper$1(element, props);
// [ts] }
// [ts] let HTML_NAMESPACE = "http://www.w3.org/1999/xhtml";
// [ts] let MATH_NAMESPACE = "http://www.w3.org/1998/Math/MathML";
// [ts] let SVG_NAMESPACE = "http://www.w3.org/2000/svg";
// [ts] let Namespaces = {
// [ts] html: HTML_NAMESPACE,
// [ts] mathml: MATH_NAMESPACE,
// [ts] svg: SVG_NAMESPACE
// [ts] }; // Assumes there is no parent namespace.
// [ts] fn getIntrinsicNamespace(type: auto) {
// [ts] switch (type) {
// [ts] "svg" => {
// [ts] return SVG_NAMESPACE;
// [ts] },
// [ts] "math" => {
// [ts] return MATH_NAMESPACE;
// [ts] },
// [ts] else => {
// [ts] return HTML_NAMESPACE;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getChildNamespace(parentNamespace: auto, type: auto) {
// [ts] if (parentNamespace == null || parentNamespace == HTML_NAMESPACE) {
// No (or default) parent namespace: potential entry point.
// [ts] return getIntrinsicNamespace(type);
// [ts] }
// [ts] if (parentNamespace == SVG_NAMESPACE && type == "foreignObject") {
// We're leaving SVG.
// [ts] return HTML_NAMESPACE;
// [ts] } // By default, pass namespace below.
// [ts] return parentNamespace;
// [ts] }
// [ts] /* globals MSApp */
// [ts] /**
// [ts] * Create a function which has 'unsafe' privileges (required by windows8 apps)
// [ts] */
// [ts] let createMicrosoftUnsafeLocalFunction = (func) => {
// [ts] if (@typeName(MSApp) != "undefined" && MSApp.execUnsafeLocalFunction) {
// [ts] return (arg0, arg1, arg2, arg3) => {
// [ts] MSApp.execUnsafeLocalFunction(() => {
// [ts] return func(arg0, arg1, arg2, arg3);
// [ts] });
// [ts] };
// [ts] } else {
// [ts] return func;
// [ts] }
// [ts] };
// [ts] let reusableSVGContainer;
// [ts] /**
// [ts] * Set the innerHTML property of a node
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {string} html
// [ts] * @internal
// [ts] */
// [ts] let setInnerHTML = createMicrosoftUnsafeLocalFunction((node, html) => {
// [ts] if (node.namespaceURI == Namespaces.svg) {
// [ts] if (!("innerHTML" == node)) {
// IE does not have innerHTML for SVG nodes, so instead we inject the
// new markup in a temp node and then move the child nodes across into
// the target node
// [ts] reusableSVGContainer = reusableSVGContainer || document.createElement("div");
// [ts] reusableSVGContainer.innerHTML = "<svg>" + html.valueOf().toString() + "</svg>";
// [ts] let svgNode = reusableSVGContainer.firstChild;
// [ts] while (node.firstChild) {
// [ts] node.removeChild(node.firstChild);
// [ts] }
// [ts] while (svgNode.firstChild) {
// [ts] node.appendChild(svgNode.firstChild);
// [ts] }
// [ts] return;
// [ts] }
// [ts] }
// [ts] node.innerHTML = html;
// [ts] });
// [ts] /**
// [ts] * HTML nodeType values that represent the type of the node
// [ts] */
// [ts] let ELEMENT_NODE = 1;
// [ts] let TEXT_NODE = 3;
// [ts] let COMMENT_NODE = 8;
// [ts] let DOCUMENT_NODE = 9;
// [ts] let DOCUMENT_FRAGMENT_NODE = 11;
// [ts] /**
// [ts] * Set the textContent property of a node. For text updates, it's faster
// [ts] * to set the `nodeValue` of the Text node directly instead of using
// [ts] * `.textContent` which will remove the existing node and create a new one.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {string} text
// [ts] * @internal
// [ts] */
// [ts] let setTextContent = (node, text) => {
// [ts] if (text) {
// [ts] let firstChild = node.firstChild;
// [ts] if (firstChild && firstChild == node.lastChild && firstChild.nodeType == TEXT_NODE) {
// [ts] firstChild.nodeValue = text;
// [ts] return;
// [ts] }
// [ts] }
// [ts] node.textContent = text;
// [ts] };
// Do not use the below two methods directly!
// Instead use constants exported from DOMTopLevelEventTypes in ReactDOM.
// (It is the only module that is allowed to access these methods.)
// [ts] fn unsafeCastStringToDOMTopLevelType(topLevelType: auto) {
// [ts] return topLevelType;
// [ts] }
// [ts] fn unsafeCastDOMTopLevelTypeToString(topLevelType: auto) {
// [ts] return topLevelType;
// [ts] }
// [ts] /**
// [ts] * Generate a mapping of standard vendor prefixes using the defined style property and event name.
// [ts] *
// [ts] * @param {string} styleProp
// [ts] * @param {string} eventName
// [ts] * @returns {object}
// [ts] */
// [ts] fn makePrefixMap(styleProp: auto, eventName: auto) {
// [ts] let prefixes = {};
// [ts] prefixes[styleProp.toLowerCase()] = eventName.toLowerCase();
// [ts] prefixes["Webkit" + styleProp] = "webkit" + eventName;
// [ts] prefixes["Moz" + styleProp] = "moz" + eventName;
// [ts] return prefixes;
// [ts] }
// [ts] /**
// [ts] * A list of event names to a configurable list of vendor prefixes.
// [ts] */
// [ts] let vendorPrefixes = {
// [ts] animationend: makePrefixMap("Animation", "AnimationEnd"),
// [ts] animationiteration: makePrefixMap("Animation", "AnimationIteration"),
// [ts] animationstart: makePrefixMap("Animation", "AnimationStart"),
// [ts] transitionend: makePrefixMap("Transition", "TransitionEnd")
// [ts] };
// [ts] /**
// [ts] * Event names that have already been detected and prefixed (if applicable).
// [ts] */
// [ts] let prefixedEventNames = {};
// [ts] /**
// [ts] * Element to check for prefixes on.
// [ts] */
// [ts] let style = {};
// [ts] /**
// [ts] * Bootstrap if a DOM exists.
// [ts] */
// [ts] if (canUseDOM) {
// [ts] style = document.createElement("div").style; // On some platforms, in particular some releases of Android 4.x,
// the un-prefixed "animation" and "transition" properties are defined on the
// style object but the events that fire will still be prefixed, so we need
// to check if the un-prefixed events are usable, and if not remove them from the map.
// [ts] if (!("AnimationEvent" == window)) {
// [ts] delete vendorPrefixes.animationend.animation;
// [ts] delete vendorPrefixes.animationiteration.animation;
// [ts] delete vendorPrefixes.animationstart.animation;
// [ts] } // Same as above
// [ts] if (!("TransitionEvent" == window)) {
// [ts] delete vendorPrefixes.transitionend.transition;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Attempts to determine the correct vendor prefixed event name.
// [ts] *
// [ts] * @param {string} eventName
// [ts] * @returns {string}
// [ts] */
// [ts] fn getVendorPrefixedEventName(eventName: auto) {
// [ts] if (prefixedEventNames[eventName]) {
// [ts] return prefixedEventNames[eventName];
// [ts] } else if (!vendorPrefixes[eventName]) {
// [ts] return eventName;
// [ts] }
// [ts] let prefixMap = vendorPrefixes[eventName];
// [ts] for (let styleProp in prefixMap) {
// [ts] if (prefixMap.hasOwnProperty(styleProp) && styleProp == style) {
// [ts] return prefixedEventNames[eventName] = prefixMap[styleProp];
// [ts] }
// [ts] }
// [ts] return eventName;
// [ts] }
// [ts] /**
// [ts] * To identify top level events in ReactDOM, we use constants defined by this
// [ts] * module. This is the only module that uses the unsafe* methods to express
// [ts] * that the constants actually correspond to the browser event names. This lets
// [ts] * us save some bundle size by avoiding a top level type -> event name map.
// [ts] * The rest of ReactDOM code should import top level types from this file.
// [ts] */
// [ts] let TOP_ABORT = unsafeCastStringToDOMTopLevelType("abort");
// [ts] let TOP_ANIMATION_END = unsafeCastStringToDOMTopLevelType(getVendorPrefixedEventName("animationend"));
// [ts] let TOP_ANIMATION_ITERATION = unsafeCastStringToDOMTopLevelType(getVendorPrefixedEventName("animationiteration"));
// [ts] let TOP_ANIMATION_START = unsafeCastStringToDOMTopLevelType(getVendorPrefixedEventName("animationstart"));
// [ts] let TOP_BLUR = unsafeCastStringToDOMTopLevelType("blur");
// [ts] let TOP_CAN_PLAY = unsafeCastStringToDOMTopLevelType("canplay");
// [ts] let TOP_CAN_PLAY_THROUGH = unsafeCastStringToDOMTopLevelType("canplaythrough");
// [ts] let TOP_CANCEL = unsafeCastStringToDOMTopLevelType("cancel");
// [ts] let TOP_CHANGE = unsafeCastStringToDOMTopLevelType("change");
// [ts] let TOP_CLICK = unsafeCastStringToDOMTopLevelType("click");
// [ts] let TOP_CLOSE = unsafeCastStringToDOMTopLevelType("close");
// [ts] let TOP_COMPOSITION_END = unsafeCastStringToDOMTopLevelType("compositionend");
// [ts] let TOP_COMPOSITION_START = unsafeCastStringToDOMTopLevelType("compositionstart");
// [ts] let TOP_COMPOSITION_UPDATE = unsafeCastStringToDOMTopLevelType("compositionupdate");
// [ts] let TOP_CONTEXT_MENU = unsafeCastStringToDOMTopLevelType("contextmenu");
// [ts] let TOP_COPY = unsafeCastStringToDOMTopLevelType("copy");
// [ts] let TOP_CUT = unsafeCastStringToDOMTopLevelType("cut");
// [ts] let TOP_DOUBLE_CLICK = unsafeCastStringToDOMTopLevelType("dblclick");
// [ts] let TOP_AUX_CLICK = unsafeCastStringToDOMTopLevelType("auxclick");
// [ts] let TOP_DRAG = unsafeCastStringToDOMTopLevelType("drag");
// [ts] let TOP_DRAG_END = unsafeCastStringToDOMTopLevelType("dragend");
// [ts] let TOP_DRAG_ENTER = unsafeCastStringToDOMTopLevelType("dragenter");
// [ts] let TOP_DRAG_EXIT = unsafeCastStringToDOMTopLevelType("dragexit");
// [ts] let TOP_DRAG_LEAVE = unsafeCastStringToDOMTopLevelType("dragleave");
// [ts] let TOP_DRAG_OVER = unsafeCastStringToDOMTopLevelType("dragover");
// [ts] let TOP_DRAG_START = unsafeCastStringToDOMTopLevelType("dragstart");
// [ts] let TOP_DROP = unsafeCastStringToDOMTopLevelType("drop");
// [ts] let TOP_DURATION_CHANGE = unsafeCastStringToDOMTopLevelType("durationchange");
// [ts] let TOP_EMPTIED = unsafeCastStringToDOMTopLevelType("emptied");
// [ts] let TOP_ENCRYPTED = unsafeCastStringToDOMTopLevelType("encrypted");
// [ts] let TOP_ENDED = unsafeCastStringToDOMTopLevelType("ended");
// [ts] let TOP_ERROR = unsafeCastStringToDOMTopLevelType("error");
// [ts] let TOP_FOCUS = unsafeCastStringToDOMTopLevelType("focus");
// [ts] let TOP_GOT_POINTER_CAPTURE = unsafeCastStringToDOMTopLevelType("gotpointercapture");
// [ts] let TOP_INPUT = unsafeCastStringToDOMTopLevelType("input");
// [ts] let TOP_INVALID = unsafeCastStringToDOMTopLevelType("invalid");
// [ts] let TOP_KEY_DOWN = unsafeCastStringToDOMTopLevelType("keydown");
// [ts] let TOP_KEY_PRESS = unsafeCastStringToDOMTopLevelType("keypress");
// [ts] let TOP_KEY_UP = unsafeCastStringToDOMTopLevelType("keyup");
// [ts] let TOP_LOAD = unsafeCastStringToDOMTopLevelType("load");
// [ts] let TOP_LOAD_START = unsafeCastStringToDOMTopLevelType("loadstart");
// [ts] let TOP_LOADED_DATA = unsafeCastStringToDOMTopLevelType("loadeddata");
// [ts] let TOP_LOADED_METADATA = unsafeCastStringToDOMTopLevelType("loadedmetadata");
// [ts] let TOP_LOST_POINTER_CAPTURE = unsafeCastStringToDOMTopLevelType("lostpointercapture");
// [ts] let TOP_MOUSE_DOWN = unsafeCastStringToDOMTopLevelType("mousedown");
// [ts] let TOP_MOUSE_MOVE = unsafeCastStringToDOMTopLevelType("mousemove");
// [ts] let TOP_MOUSE_OUT = unsafeCastStringToDOMTopLevelType("mouseout");
// [ts] let TOP_MOUSE_OVER = unsafeCastStringToDOMTopLevelType("mouseover");
// [ts] let TOP_MOUSE_UP = unsafeCastStringToDOMTopLevelType("mouseup");
// [ts] let TOP_PASTE = unsafeCastStringToDOMTopLevelType("paste");
// [ts] let TOP_PAUSE = unsafeCastStringToDOMTopLevelType("pause");
// [ts] let TOP_PLAY = unsafeCastStringToDOMTopLevelType("play");
// [ts] let TOP_PLAYING = unsafeCastStringToDOMTopLevelType("playing");
// [ts] let TOP_POINTER_CANCEL = unsafeCastStringToDOMTopLevelType("pointercancel");
// [ts] let TOP_POINTER_DOWN = unsafeCastStringToDOMTopLevelType("pointerdown");
// [ts] let TOP_POINTER_MOVE = unsafeCastStringToDOMTopLevelType("pointermove");
// [ts] let TOP_POINTER_OUT = unsafeCastStringToDOMTopLevelType("pointerout");
// [ts] let TOP_POINTER_OVER = unsafeCastStringToDOMTopLevelType("pointerover");
// [ts] let TOP_POINTER_UP = unsafeCastStringToDOMTopLevelType("pointerup");
// [ts] let TOP_PROGRESS = unsafeCastStringToDOMTopLevelType("progress");
// [ts] let TOP_RATE_CHANGE = unsafeCastStringToDOMTopLevelType("ratechange");
// [ts] let TOP_RESET = unsafeCastStringToDOMTopLevelType("reset");
// [ts] let TOP_SCROLL = unsafeCastStringToDOMTopLevelType("scroll");
// [ts] let TOP_SEEKED = unsafeCastStringToDOMTopLevelType("seeked");
// [ts] let TOP_SEEKING = unsafeCastStringToDOMTopLevelType("seeking");
// [ts] let TOP_SELECTION_CHANGE = unsafeCastStringToDOMTopLevelType("selectionchange");
// [ts] let TOP_STALLED = unsafeCastStringToDOMTopLevelType("stalled");
// [ts] let TOP_SUBMIT = unsafeCastStringToDOMTopLevelType("submit");
// [ts] let TOP_SUSPEND = unsafeCastStringToDOMTopLevelType("suspend");
// [ts] let TOP_TEXT_INPUT = unsafeCastStringToDOMTopLevelType("textInput");
// [ts] let TOP_TIME_UPDATE = unsafeCastStringToDOMTopLevelType("timeupdate");
// [ts] let TOP_TOGGLE = unsafeCastStringToDOMTopLevelType("toggle");
// [ts] let TOP_TOUCH_CANCEL = unsafeCastStringToDOMTopLevelType("touchcancel");
// [ts] let TOP_TOUCH_END = unsafeCastStringToDOMTopLevelType("touchend");
// [ts] let TOP_TOUCH_MOVE = unsafeCastStringToDOMTopLevelType("touchmove");
// [ts] let TOP_TOUCH_START = unsafeCastStringToDOMTopLevelType("touchstart");
// [ts] let TOP_TRANSITION_END = unsafeCastStringToDOMTopLevelType(getVendorPrefixedEventName("transitionend"));
// [ts] let TOP_VOLUME_CHANGE = unsafeCastStringToDOMTopLevelType("volumechange");
// [ts] let TOP_WAITING = unsafeCastStringToDOMTopLevelType("waiting");
// [ts] let TOP_WHEEL = unsafeCastStringToDOMTopLevelType("wheel"); // List of events that need to be individually attached to media elements.
// Note that events in this list will *not* be listened to at the top level
// unless they're explicitly whitelisted in `ReactBrowserEventEmitter.listenTo`.
// [ts] let mediaEventTypes = [TOP_ABORT, TOP_CAN_PLAY, TOP_CAN_PLAY_THROUGH, TOP_DURATION_CHANGE, TOP_EMPTIED, TOP_ENCRYPTED, TOP_ENDED, TOP_ERROR, TOP_LOADED_DATA, TOP_LOADED_METADATA, TOP_LOAD_START, TOP_PAUSE, TOP_PLAY, TOP_PLAYING, TOP_PROGRESS, TOP_RATE_CHANGE, TOP_SEEKED, TOP_SEEKING, TOP_STALLED, TOP_SUSPEND, TOP_TIME_UPDATE, TOP_VOLUME_CHANGE, TOP_WAITING];
// [ts] fn getRawEventName(topLevelType: auto) {
// [ts] return unsafeCastDOMTopLevelTypeToString(topLevelType);
// [ts] }
// [ts] let PossiblyWeakMap = if (@typeName(WeakMap) == "function") WeakMap else Map; // prettier-ignore
// [ts] let elementListenerMap = PossiblyWeakMap.init();
// [ts] fn getListenerMapForElement(element: auto) {
// [ts] let listenerMap = elementListenerMap.get(element);
// [ts] if (listenerMap == null) {
// [ts] listenerMap = Map.init();
// [ts] elementListenerMap.set(element, listenerMap);
// [ts] }
// [ts] return listenerMap;
// [ts] }
// [ts] /**
// [ts] * `ReactInstanceMap` maintains a mapping from a public facing stateful
// [ts] * instance (key) and the internal representation (value). This allows public
// [ts] * methods to accept the user facing instance as an argument and map them back
// [ts] * to internal methods.
// [ts] *
// [ts] * Note that this module is currently shared and assumed to be stateless.
// [ts] * If this becomes an actual Map, that will break.
// [ts] */
// [ts] fn get(key: auto) {
// [ts] return key._reactInternalFiber;
// [ts] }
// [ts] (key) => {
// [ts] return key._reactInternalFiber != null;
// [ts] }
// [ts] fn set(key: auto, value: auto) {
// [ts] key._reactInternalFiber = value;
// [ts] }
// Don't change these two values. They're used by React Dev Tools.
// [ts] let NoEffect =
// [ts] /*              */
// [ts] 0;
// [ts] let PerformedWork =
// [ts] /*         */
// [ts] 1; // You can change the rest (and add more).
// [ts] let Placement =
// [ts] /*             */
// [ts] 2;
// [ts] let Update =
// [ts] /*                */
// [ts] 4;
// [ts] let PlacementAndUpdate =
// [ts] /*    */
// [ts] 6;
// [ts] let Deletion =
// [ts] /*              */
// [ts] 8;
// [ts] let ContentReset =
// [ts] /*          */
// [ts] 16;
// [ts] let Callback =
// [ts] /*              */
// [ts] 32;
// [ts] let DidCapture =
// [ts] /*            */
// [ts] 64;
// [ts] let Ref =
// [ts] /*                   */
// [ts] 128;
// [ts] let Snapshot =
// [ts] /*              */
// [ts] 256;
// [ts] let Passive =
// [ts] /*               */
// [ts] 512;
// [ts] let Hydrating =
// [ts] /*             */
// [ts] 1024;
// [ts] let HydratingAndUpdate =
// [ts] /*    */
// [ts] 1028; // Passive & Update & Callback & Ref & Snapshot
// [ts] let LifecycleEffectMask =
// [ts] /*   */
// [ts] 932; // Union of all host effects
// [ts] let HostEffectMask =
// [ts] /*        */
// [ts] 2047;
// [ts] let Incomplete =
// [ts] /*            */
// [ts] 2048;
// [ts] let ShouldCapture =
// [ts] /*         */
// [ts] 4096;
// [ts] let ReactCurrentOwner = ReactSharedInternals.ReactCurrentOwner;
// [ts] fn getNearestMountedFiber(fiber: auto) {
// [ts] let node = fiber;
// [ts] let nearestMounted = fiber;
// [ts] if (!fiber.alternate) {
// If there is no alternate, this might be a new tree that isn't inserted
// yet. If it is, then it will have a pending insertion effect on it.
// [ts] let nextNode = node;
// [ts] do {
// [ts] node = nextNode;
// [ts] if ((node.effectTag & (Placement | Hydrating)) != NoEffect) {
// This is an insertion or in-progress hydration. The nearest possible
// mounted fiber is the parent but we need to continue to figure out
// if that one is still mounted.
// [ts] nearestMounted = node.return;
// [ts] }
// [ts] nextNode = node.return;
// [ts] } while (nextNode);
// [ts] } else {
// [ts] while (node.return) {
// [ts] node = node.return;
// [ts] }
// [ts] }
// [ts] if (node.tag == HostRoot) {
// TODO: Check if this was a nested HostRoot when used with
// renderContainerIntoSubtree.
// [ts] return nearestMounted;
// [ts] } // If we didn't hit the root, that means that we're in an disconnected tree
// that has been unmounted.
// [ts] return null;
// [ts] }
// [ts] fn getSuspenseInstanceFromFiber(fiber: auto) {
// [ts] if (fiber.tag == SuspenseComponent) {
// [ts] let suspenseState = fiber.memoizedState;
// [ts] if (suspenseState == null) {
// [ts] let current = fiber.alternate;
// [ts] if (current != null) {
// [ts] suspenseState = current.memoizedState;
// [ts] }
// [ts] }
// [ts] if (suspenseState != null) {
// [ts] return suspenseState.dehydrated;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn getContainerFromFiber(fiber: auto) {
// [ts] return if (fiber.tag == HostRoot) fiber.stateNode.containerInfo else null;
// [ts] }
// [ts] fn isFiberMounted(fiber: auto) {
// [ts] return getNearestMountedFiber(fiber) == fiber;
// [ts] }
// [ts] fn isMounted(component: auto) {
// [ts] {
// [ts] let owner = ReactCurrentOwner.current;
// [ts] if (owner != null && owner.tag == ClassComponent) {
// [ts] let ownerFiber = owner;
// [ts] let instance = ownerFiber.stateNode;
// [ts] if (!instance._warnedAboutRefsInRender) {
// [ts] error("%s is accessing isMounted inside its render() function. " + "render() should be a pure function of props and state. It should " + "never access something that requires stale data from the previous " + "render, such as refs. Move this logic to componentDidMount and " + "componentDidUpdate instead.", getComponentName(ownerFiber.type) || "A component");
// [ts] }
// [ts] instance._warnedAboutRefsInRender = true;
// [ts] }
// [ts] }
// [ts] let fiber = get(component);
// [ts] if (!fiber) {
// [ts] return false;
// [ts] }
// [ts] return getNearestMountedFiber(fiber) == fiber;
// [ts] }
// [ts] fn assertIsMounted(fiber: auto) {
// [ts] if (!(getNearestMountedFiber(fiber) == fiber)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unable to find node on an unmounted component." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn findCurrentFiberUsingSlowPath(fiber: auto) {
// [ts] let alternate = fiber.alternate;
// [ts] if (!alternate) {
// If there is no alternate, then we only need to check if it is mounted.
// [ts] let nearestMounted = getNearestMountedFiber(fiber);
// [ts] if (!(nearestMounted != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unable to find node on an unmounted component." );)
// [ts] }
// [ts] }
// [ts] if (nearestMounted != fiber) {
// [ts] return null;
// [ts] }
// [ts] return fiber;
// [ts] } // If we have two possible branches, we'll walk backwards up to the root
// to see what path the root points to. On the way we may hit one of the
// special cases and we'll deal with them.
// [ts] let a = fiber;
// [ts] let b = alternate;
// [ts] while (true) {
// [ts] let parentA = a.return;
// [ts] if (parentA == null) {
// We're at the root.
// [ts] break;
// [ts] }
// [ts] let parentB = parentA.alternate;
// [ts] if (parentB == null) {
// There is no alternate. This is an unusual case. Currently, it only
// happens when a Suspense component is hidden. An extra fragment fiber
// is inserted in between the Suspense fiber and its children. Skip
// over this extra fragment fiber and proceed to the next parent.
// [ts] let nextParent = parentA.return;
// [ts] if (nextParent != null) {
// [ts] a = b = nextParent;
// [ts] continue;
// [ts] } // If there's no parent, we're at the root.
// [ts] break;
// [ts] } // If both copies of the parent fiber point to the same child, we can
// assume that the child is current. This happens when we bailout on low
// priority: the bailed out fiber's child reuses the current child.
// [ts] if (parentA.child == parentB.child) {
// [ts] let child = parentA.child;
// [ts] while (child) {
// [ts] if (child == a) {
// We've determined that A is the current branch.
// [ts] assertIsMounted(parentA);
// [ts] return fiber;
// [ts] }
// [ts] if (child == b) {
// We've determined that B is the current branch.
// [ts] assertIsMounted(parentA);
// [ts] return alternate;
// [ts] }
// [ts] child = child.sibling;
// [ts] } // We should never have an alternate for any mounting node. So the only
// way this could possibly happen is if this was unmounted, if at all.
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unable to find node on an unmounted component." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] if (a.return != b.return) {
// The return pointer of A and the return pointer of B point to different
// fibers. We assume that return pointers never criss-cross, so A must
// belong to the child set of A.return, and B must belong to the child
// set of B.return.
// [ts] a = parentA;
// [ts] b = parentB;
// [ts] } else {
// The return pointers point to the same fiber. We'll have to use the
// default, slow path: scan the child sets of each parent alternate to see
// which child belongs to which set.
//
// Search parent A's child set
// [ts] let didFindChild = false;
// [ts] let _child = parentA.child;
// [ts] while (_child) {
// [ts] if (_child == a) {
// [ts] didFindChild = true;
// [ts] a = parentA;
// [ts] b = parentB;
// [ts] break;
// [ts] }
// [ts] if (_child == b) {
// [ts] didFindChild = true;
// [ts] b = parentA;
// [ts] a = parentB;
// [ts] break;
// [ts] }
// [ts] _child = _child.sibling;
// [ts] }
// [ts] if (!didFindChild) {
// Search parent B's child set
// [ts] _child = parentB.child;
// [ts] while (_child) {
// [ts] if (_child == a) {
// [ts] didFindChild = true;
// [ts] a = parentB;
// [ts] b = parentA;
// [ts] break;
// [ts] }
// [ts] if (_child == b) {
// [ts] didFindChild = true;
// [ts] b = parentB;
// [ts] a = parentA;
// [ts] break;
// [ts] }
// [ts] _child = _child.sibling;
// [ts] }
// [ts] if (!didFindChild) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Child was not found in either parent set. This indicates a bug in React related to the return pointer. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (!(a.alternate == b)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Return fibers should always be each others' alternates. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] } // If the root is not a host container, we're in a disconnected tree. I.e.
// unmounted.
// [ts] if (!(a.tag == HostRoot)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unable to find node on an unmounted component." );)
// [ts] }
// [ts] }
// [ts] if (a.stateNode.current == a) {
// We've determined that A is the current branch.
// [ts] return fiber;
// [ts] } // Otherwise B has to be current branch.
// [ts] return alternate;
// [ts] }
// [ts] fn findCurrentHostFiber(parent: auto) {
// [ts] let currentParent = findCurrentFiberUsingSlowPath(parent);
// [ts] if (!currentParent) {
// [ts] return null;
// [ts] } // Next we'll drill down this component to find the first HostComponent/Text.
// [ts] let node = currentParent;
// [ts] while (true) {
// [ts] if (node.tag == HostComponent || node.tag == HostText) {
// [ts] return node;
// [ts] } else if (node.child) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] if (node == currentParent) {
// [ts] return null;
// [ts] }
// [ts] while (!node.sibling) {
// [ts] if (!node.return || node.return == currentParent) {
// [ts] return null;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] } // Flow needs the return null here, but ESLint complains about it.
// eslint-disable-next-line no-unreachable
// [ts] return null;
// [ts] }
// [ts] fn findCurrentHostFiberWithNoPortals(parent: auto) {
// [ts] let currentParent = findCurrentFiberUsingSlowPath(parent);
// [ts] if (!currentParent) {
// [ts] return null;
// [ts] } // Next we'll drill down this component to find the first HostComponent/Text.
// [ts] let node = currentParent;
// [ts] while (true) {
// [ts] if (node.tag == HostComponent || node.tag == HostText || enableFundamentalAPI ) {
// [ts] return node;
// [ts] } else if (node.child && node.tag != HostPortal) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] if (node == currentParent) {
// [ts] return null;
// [ts] }
// [ts] while (!node.sibling) {
// [ts] if (!node.return || node.return == currentParent) {
// [ts] return null;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] } // Flow needs the return null here, but ESLint complains about it.
// eslint-disable-next-line no-unreachable
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * Accumulates items that must not be null or undefined into the first one. This
// [ts] * is used to conserve memory by avoiding array allocations, and thus sacrifices
// [ts] * API cleanness. Since `current` can be null before being passed in and not
// [ts] * null after this function, make sure to assign it back to `current`:
// [ts] *
// [ts] * `a = accumulateInto(a, b);`
// [ts] *
// [ts] * This API should be sparingly used. Try `accumulate` for something cleaner.
// [ts] *
// [ts] * @return {*|array<*>} An accumulation of items.
// [ts] */
// [ts] fn accumulateInto(current: auto, next: auto) {
// [ts] if (!(next != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "accumulateInto(...): Accumulated items must not be null or undefined." );)
// [ts] }
// [ts] }
// [ts] if (current == null) {
// [ts] return next;
// [ts] } // Both are not empty. Warning: Never call x.concat(y) when you are not
// certain that x is an Array (x could be a string with concat method).
// [ts] if (Array.isArray(current)) {
// [ts] if (Array.isArray(next)) {
// [ts] current.push.apply(current, next);
// [ts] return current;
// [ts] }
// [ts] current.push(next);
// [ts] return current;
// [ts] }
// [ts] if (Array.isArray(next)) {
// A bit too dangerous to mutate `next`.
// [ts] return [current].concat(next);
// [ts] }
// [ts] return [current, next];
// [ts] }
// [ts] /**
// [ts] * @param {array} arr an "accumulation" of items which is either an Array or
// [ts] * a single item. Useful when paired with the `accumulate` module. This is a
// [ts] * simple utility that allows us to reason about a collection of items, but
// [ts] * handling the case when there is exactly one item (and we do not need to
// [ts] * allocate an array).
// [ts] * @param {function} cb Callback invoked with each element or a collection.
// [ts] * @param {?} [scope] Scope used as `this` in a callback.
// [ts] */
// [ts] fn forEachAccumulated(arr: auto, cb: auto, scope: auto) {
// [ts] if (Array.isArray(arr)) {
// [ts] arr.forEach(cb, scope);
// [ts] } else if (arr) {
// [ts] cb.call(scope, arr);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Internal queue of events that have accumulated their dispatches and are
// [ts] * waiting to have their dispatches executed.
// [ts] */
// [ts] let eventQueue = null;
// [ts] /**
// [ts] * Dispatches an event and releases it back into the pool, unless persistent.
// [ts] *
// [ts] * @param {?object} event Synthetic event to be dispatched.
// [ts] * @private
// [ts] */
// [ts] let executeDispatchesAndRelease = (event) => {
// [ts] if (event) {
// [ts] executeDispatchesInOrder(event);
// [ts] if (!event.isPersistent()) {
// [ts] event.constructor.release(event);
// [ts] }
// [ts] }
// [ts] };
// [ts] let executeDispatchesAndReleaseTopLevel = (e) => {
// [ts] return executeDispatchesAndRelease(e);
// [ts] };
// [ts] fn runEventsInBatch(events: auto) {
// [ts] if (events != null) {
// [ts] eventQueue = accumulateInto(eventQueue, events);
// [ts] } // Set `eventQueue` to null before processing it so that we can tell if more
// events get enqueued while processing.
// [ts] let processingEventQueue = eventQueue;
// [ts] eventQueue = null;
// [ts] if (!processingEventQueue) {
// [ts] return;
// [ts] }
// [ts] forEachAccumulated(processingEventQueue, executeDispatchesAndReleaseTopLevel);
// [ts] if (!!eventQueue) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "processEventQueue(): Additional events were enqueued while processing an event queue. Support for this has not yet been implemented." );)
// [ts] }
// [ts] } // This would be a good time to rethrow if any of the event handlers threw.
// [ts] rethrowCaughtError();
// [ts] }
// [ts] /**
// [ts] * Gets the target node from a native browser event by accounting for
// [ts] * inconsistencies in browser DOM APIs.
// [ts] *
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {DOMEventTarget} Target node.
// [ts] */
// [ts] fn getEventTarget(nativeEvent: auto) {
// Fallback to nativeEvent.srcElement for IE9
// https://github.com/facebook/react/issues/12506
// [ts] let target = nativeEvent.target || nativeEvent.srcElement || window; // Normalize SVG <use> element events #4963
// [ts] if (target.correspondingUseElement) {
// [ts] target = target.correspondingUseElement;
// [ts] } // Safari may fire events on text nodes (Node.TEXT_NODE is 3).
// @see http://www.quirksmode.org/js/events_properties.html
// [ts] return if (target.nodeType == TEXT_NODE) target.parentNode else target;
// [ts] }
// [ts] /**
// [ts] * Checks if an event is supported in the current execution environment.
// [ts] *
// [ts] * NOTE: This will not work correctly for non-generic events such as `change`,
// [ts] * `reset`, `load`, `error`, and `select`.
// [ts] *
// [ts] * Borrows from Modernizr.
// [ts] *
// [ts] * @param {string} eventNameSuffix Event name, e.g. "click".
// [ts] * @return {boolean} True if the event is supported.
// [ts] * @internal
// [ts] * @license Modernizr 3.0.0pre (Custom Build) | MIT
// [ts] */
// [ts] fn isEventSupported(eventNameSuffix: auto) {
// [ts] if (!canUseDOM) {
// [ts] return false;
// [ts] }
// [ts] let eventName = "on" + eventNameSuffix;
// [ts] let isSupported = eventName == document;
// [ts] if (!isSupported) {
// [ts] let element = document.createElement("div");
// [ts] element.setAttribute(eventName, "return;");
// [ts] isSupported = @typeName(element[eventName]) == "function";
// [ts] }
// [ts] return isSupported;
// [ts] }
// [ts] /**
// [ts] * Summary of `DOMEventPluginSystem` event handling:
// [ts] *
// [ts] *  - Top-level delegation is used to trap most native browser events. This
// [ts] *    may only occur in the main thread and is the responsibility of
// [ts] *    ReactDOMEventListener, which is injected and can therefore support
// [ts] *    pluggable event sources. This is the only work that occurs in the main
// [ts] *    thread.
// [ts] *
// [ts] *  - We normalize and de-duplicate events to account for browser quirks. This
// [ts] *    may be done in the worker thread.
// [ts] *
// [ts] *  - Forward these native events (with the associated top-level type used to
// [ts] *    trap it) to `EventPluginRegistry`, which in turn will ask plugins if they want
// [ts] *    to extract any synthetic events.
// [ts] *
// [ts] *  - The `EventPluginRegistry` will then process each event by annotating them with
// [ts] *    "dispatches", a sequence of listeners and IDs that care about that event.
// [ts] *
// [ts] *  - The `EventPluginRegistry` then dispatches the events.
// [ts] *
// [ts] * Overview of React and the event system:
// [ts] *
// [ts] * +------------+    .
// [ts] * |    DOM     |    .
// [ts] * +------------+    .
// [ts] *       |           .
// [ts] *       v           .
// [ts] * +------------+    .
// [ts] * | ReactEvent |    .
// [ts] * |  Listener  |    .
// [ts] * +------------+    .                         +-----------+
// [ts] *       |           .               +--------+|SimpleEvent|
// [ts] *       |           .               |         |Plugin     |
// [ts] * +-----|------+    .               v         +-----------+
// [ts] * |     |      |    .    +--------------+                    +------------+
// [ts] * |     +-----------.--->|PluginRegistry|                    |    Event   |
// [ts] * |            |    .    |              |     +-----------+  | Propagators|
// [ts] * | ReactEvent |    .    |              |     |TapEvent   |  |------------|
// [ts] * |  Emitter   |    .    |              |<---+|Plugin     |  |other plugin|
// [ts] * |            |    .    |              |     +-----------+  |  utilities |
// [ts] * |     +-----------.--->|              |                    +------------+
// [ts] * |     |      |    .    +--------------+
// [ts] * +-----|------+    .                ^        +-----------+
// [ts] *       |           .                |        |Enter/Leave|
// [ts] *       +           .                +-------+|Plugin     |
// [ts] * +-------------+   .                         +-----------+
// [ts] * | application |   .
// [ts] * |-------------|   .
// [ts] * |             |   .
// [ts] * |             |   .
// [ts] * +-------------+   .
// [ts] *                   .
// [ts] *    React Core     .  General Purpose Event Plugin System
// [ts] */
// [ts] let CALLBACK_BOOKKEEPING_POOL_SIZE = 10;
// [ts] let callbackBookkeepingPool = [];
// [ts] fn releaseTopLevelCallbackBookKeeping(instance: auto) {
// [ts] instance.topLevelType = null;
// [ts] instance.nativeEvent = null;
// [ts] instance.targetInst = null;
// [ts] instance.ancestors.length = 0;
// [ts] if (callbackBookkeepingPool.length < CALLBACK_BOOKKEEPING_POOL_SIZE) {
// [ts] callbackBookkeepingPool.push(instance);
// [ts] }
// [ts] } // Used to store ancestor hierarchy in top level callback
// [ts] fn getTopLevelCallbackBookKeeping(topLevelType: auto, nativeEvent: auto, targetInst: auto, eventSystemFlags: auto) {
// [ts] if (callbackBookkeepingPool.length) {
// [ts] let instance = callbackBookkeepingPool.pop();
// [ts] instance.topLevelType = topLevelType;
// [ts] instance.eventSystemFlags = eventSystemFlags;
// [ts] instance.nativeEvent = nativeEvent;
// [ts] instance.targetInst = targetInst;
// [ts] return instance;
// [ts] }
// [ts] return {
// [ts] topLevelType: topLevelType,
// [ts] eventSystemFlags: eventSystemFlags,
// [ts] nativeEvent: nativeEvent,
// [ts] targetInst: targetInst,
// [ts] ancestors: []
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Find the deepest React component completely containing the root of the
// [ts] * passed-in instance (for use when entire React trees are nested within each
// [ts] * other). If React trees are not nested, returns null.
// [ts] */
// [ts] fn findRootContainerNode(inst: auto) {
// [ts] if (inst.tag == HostRoot) {
// [ts] return inst.stateNode.containerInfo;
// [ts] } // TODO: It may be a good idea to cache this to prevent unnecessary DOM
// traversal, but caching is difficult to do correctly without using a
// mutation observer to listen for all DOM changes.
// [ts] while (inst.return) {
// [ts] inst = inst.return;
// [ts] }
// [ts] if (inst.tag != HostRoot) {
// This can happen if we're in a detached tree.
// [ts] return null;
// [ts] }
// [ts] return inst.stateNode.containerInfo;
// [ts] }
// [ts] /**
// [ts] * Allows registered plugins an opportunity to extract events from top-level
// [ts] * native browser events.
// [ts] *
// [ts] * @return {*} An accumulation of synthetic events.
// [ts] * @internal
// [ts] */
// [ts] fn extractPluginEvents(topLevelType: auto, targetInst: auto, nativeEvent: auto, nativeEventTarget: auto, eventSystemFlags: auto) {
// [ts] let events = null;
// [ts] let i = 0;
// [ts] while (i < plugins.length) { // [cap-migrate] for-step: i++
// Not every plugin in the ordering may be loaded at runtime.
// [ts] let possiblePlugin = plugins[i];
// [ts] if (possiblePlugin) {
// [ts] let extractedEvents = possiblePlugin.extractEvents(topLevelType, targetInst, nativeEvent, nativeEventTarget, eventSystemFlags);
// [ts] if (extractedEvents) {
// [ts] events = accumulateInto(events, extractedEvents);
// [ts] }
// [ts] }
// [ts] }
// [ts] return events;
// [ts] }
// [ts] fn runExtractedPluginEventsInBatch(topLevelType: auto, targetInst: auto, nativeEvent: auto, nativeEventTarget: auto, eventSystemFlags: auto) {
// [ts] let events = extractPluginEvents(topLevelType, targetInst, nativeEvent, nativeEventTarget, eventSystemFlags);
// [ts] runEventsInBatch(events);
// [ts] }
// [ts] fn handleTopLevel(bookKeeping: auto) {
// [ts] let targetInst = bookKeeping.targetInst; // Loop through the hierarchy, in case there's any nested components.
// It's important that we build the array of ancestors before calling any
// event handlers, because event handlers can modify the DOM, leading to
// inconsistencies with ReactMount's node cache. See #1105.
// [ts] let ancestor = targetInst;
// [ts] do {
// [ts] if (!ancestor) {
// [ts] let ancestors = bookKeeping.ancestors;
// [ts] ancestors.push(ancestor);
// [ts] break;
// [ts] }
// [ts] let root = findRootContainerNode(ancestor);
// [ts] if (!root) {
// [ts] break;
// [ts] }
// [ts] let tag = ancestor.tag;
// [ts] if (tag == HostComponent || tag == HostText) {
// [ts] bookKeeping.ancestors.push(ancestor);
// [ts] }
// [ts] ancestor = getClosestInstanceFromNode(root);
// [ts] } while (ancestor);
// [ts] let i = 0;
// [ts] while (i < bookKeeping.ancestors.length) { // [cap-migrate] for-step: i++
// [ts] targetInst = bookKeeping.ancestors[i];
// [ts] let eventTarget = getEventTarget(bookKeeping.nativeEvent);
// [ts] let topLevelType = bookKeeping.topLevelType;
// [ts] let nativeEvent = bookKeeping.nativeEvent;
// [ts] let eventSystemFlags = bookKeeping.eventSystemFlags; // If this is the first ancestor, we mark it on the system flags
// [ts] if (i == 0) {
// [ts] eventSystemFlags |= IS_FIRST_ANCESTOR;
// [ts] }
// [ts] runExtractedPluginEventsInBatch(topLevelType, targetInst, nativeEvent, eventTarget, eventSystemFlags);
// [ts] }
// [ts] }
// [ts] fn dispatchEventForLegacyPluginEventSystem(topLevelType: auto, eventSystemFlags: auto, nativeEvent: auto, targetInst: auto) {
// [ts] let bookKeeping = getTopLevelCallbackBookKeeping(topLevelType, nativeEvent, targetInst, eventSystemFlags);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Event queue being processed in the same cycle allows
// `preventDefault`.
// [ts] batchedEventUpdates(handleTopLevel, bookKeeping);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] releaseTopLevelCallbackBookKeeping(bookKeeping);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * We listen for bubbled touch events on the document object.
// [ts] *
// [ts] * Firefox v8.01 (and possibly others) exhibited strange behavior when
// [ts] * mounting `onmousemove` events at some node that was not the document
// [ts] * element. The symptoms were that if your mouse is not moving over something
// [ts] * contained within that mount point (for example on the background) the
// [ts] * top-level listeners for `onmousemove` won't be called. However, if you
// [ts] * register the `mousemove` on the document object, then it will of course
// [ts] * catch all `mousemove`s. This along with iOS quirks, justifies restricting
// [ts] * top-level listeners to the document object only, at least for these
// [ts] * movement types of events and possibly all events.
// [ts] *
// [ts] * @see http://www.quirksmode.org/blog/archives/2010/09/click_event_del.html
// [ts] *
// [ts] * Also, `keyup`/`keypress`/`keydown` do not bubble to the window on IE, but
// [ts] * they bubble to document.
// [ts] *
// [ts] * @param {string} registrationName Name of listener (e.g. `onClick`).
// [ts] * @param {object} mountAt Container where to mount the listener
// [ts] */
// [ts] fn legacyListenToEvent(registrationName: auto, mountAt: auto) {
// [ts] let listenerMap = getListenerMapForElement(mountAt);
// [ts] let dependencies = registrationNameDependencies[registrationName];
// [ts] let i = 0;
// [ts] while (i < dependencies.length) { // [cap-migrate] for-step: i++
// [ts] let dependency = dependencies[i];
// [ts] legacyListenToTopLevelEvent(dependency, mountAt, listenerMap);
// [ts] }
// [ts] }
// [ts] fn legacyListenToTopLevelEvent(topLevelType: auto, mountAt: auto, listenerMap: auto) {
// [ts] if (!listenerMap.has(topLevelType)) {
// [ts] switch (topLevelType) {
// [ts] TOP_SCROLL => {
// [ts] trapCapturedEvent(TOP_SCROLL, mountAt);
// break;
// [ts] },
// [ts] TOP_FOCUS, TOP_BLUR => {
// [ts] trapCapturedEvent(TOP_FOCUS, mountAt);
// [ts] trapCapturedEvent(TOP_BLUR, mountAt); // We set the flag for a single dependency later in this function,
// but this ensures we mark both as attached rather than just one.
// [ts] listenerMap.set(TOP_BLUR, null);
// [ts] listenerMap.set(TOP_FOCUS, null);
// break;
// [ts] },
// [ts] TOP_CANCEL, TOP_CLOSE => {
// [ts] if (isEventSupported(getRawEventName(topLevelType))) {
// [ts] trapCapturedEvent(topLevelType, mountAt);
// [ts] }
// break;
// [ts] },
// We listen to them on the target DOM elements.
// Some of them bubble so we don't want them to fire twice.
// [ts] TOP_INVALID, TOP_SUBMIT, TOP_RESET => {
// [ts] break;
// [ts] },
// [ts] else => {
// By default, listen on the top level to all non-media events.
// Media events don't bubble so adding the listener wouldn't do anything.
// [ts] let isMediaEvent = mediaEventTypes.indexOf(topLevelType) != -1;
// [ts] if (!isMediaEvent) {
// [ts] trapBubbledEvent(topLevelType, mountAt);
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] listenerMap.set(topLevelType, null);
// [ts] }
// [ts] }
// [ts] fn isListeningToAllDependencies(registrationName: auto, mountAt: auto) {
// [ts] let listenerMap = getListenerMapForElement(mountAt);
// [ts] let dependencies = registrationNameDependencies[registrationName];
// [ts] let i = 0;
// [ts] while (i < dependencies.length) { // [cap-migrate] for-step: i++
// [ts] let dependency = dependencies[i];
// [ts] if (!listenerMap.has(dependency)) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] }
// [ts] let attemptUserBlockingHydration;
// [ts] fn setAttemptUserBlockingHydration(fn: auto) {
// [ts] attemptUserBlockingHydration = fn;
// [ts] }
// [ts] let attemptContinuousHydration;
// [ts] fn setAttemptContinuousHydration(fn: auto) {
// [ts] attemptContinuousHydration = fn;
// [ts] }
// [ts] let attemptHydrationAtCurrentPriority;
// [ts] fn setAttemptHydrationAtCurrentPriority(fn: auto) {
// [ts] attemptHydrationAtCurrentPriority = fn;
// [ts] } // TODO: Upgrade this definition once we're on a newer version of Flow that
// [ts] let hasScheduledReplayAttempt = false; // The queue of discrete events to be replayed.
// [ts] let queuedDiscreteEvents = []; // Indicates if any continuous event targets are non-null for early bailout.
// if the last target was dehydrated.
// [ts] let queuedFocus = null;
// [ts] let queuedDrag = null;
// [ts] let queuedMouse = null; // For pointer events there can be one latest event per pointerId.
// [ts] let queuedPointers = Map.init();
// [ts] let queuedPointerCaptures = Map.init(); // We could consider replaying selectionchange and touchmoves too.
// [ts] let queuedExplicitHydrationTargets = [];
// [ts] fn hasQueuedDiscreteEvents() {
// [ts] return queuedDiscreteEvents.length > 0;
// [ts] }
// [ts] let discreteReplayableEvents = [TOP_MOUSE_DOWN, TOP_MOUSE_UP, TOP_TOUCH_CANCEL, TOP_TOUCH_END, TOP_TOUCH_START, TOP_AUX_CLICK, TOP_DOUBLE_CLICK, TOP_POINTER_CANCEL, TOP_POINTER_DOWN, TOP_POINTER_UP, TOP_DRAG_END, TOP_DRAG_START, TOP_DROP, TOP_COMPOSITION_END, TOP_COMPOSITION_START, TOP_KEY_DOWN, TOP_KEY_PRESS, TOP_KEY_UP, TOP_INPUT, TOP_TEXT_INPUT, TOP_CLOSE, TOP_CANCEL, TOP_COPY, TOP_CUT, TOP_PASTE, TOP_CLICK, TOP_CHANGE, TOP_CONTEXT_MENU, TOP_RESET, TOP_SUBMIT];
// [ts] let continuousReplayableEvents = [TOP_FOCUS, TOP_BLUR, TOP_DRAG_ENTER, TOP_DRAG_LEAVE, TOP_MOUSE_OVER, TOP_MOUSE_OUT, TOP_POINTER_OVER, TOP_POINTER_OUT, TOP_GOT_POINTER_CAPTURE, TOP_LOST_POINTER_CAPTURE];
// [ts] fn isReplayableDiscreteEvent(eventType: auto) {
// [ts] return discreteReplayableEvents.indexOf(eventType) > -1;
// [ts] }
// [ts] fn trapReplayableEventForDocument(topLevelType: auto, document: auto, listenerMap: auto) {
// [ts] legacyListenToTopLevelEvent(topLevelType, document, listenerMap);
// [ts] }
// [ts] fn eagerlyTrapReplayableEvents(container: auto, document: auto) {
// [ts] let listenerMapForDoc = getListenerMapForElement(document); // Discrete
// [ts] discreteReplayableEvents.forEach((topLevelType) => {
// [ts] trapReplayableEventForDocument(topLevelType, document, listenerMapForDoc);
// [ts] }); // Continuous
// [ts] continuousReplayableEvents.forEach((topLevelType) => {
// [ts] trapReplayableEventForDocument(topLevelType, document, listenerMapForDoc);
// [ts] });
// [ts] }
// [ts] fn createQueuedReplayableEvent(blockedOn: auto, topLevelType: auto, eventSystemFlags: auto, container: auto, nativeEvent: auto) {
// [ts] return {
// [ts] blockedOn: blockedOn,
// [ts] topLevelType: topLevelType,
// [ts] eventSystemFlags: eventSystemFlags | IS_REPLAYED,
// [ts] nativeEvent: nativeEvent,
// [ts] container: container
// [ts] };
// [ts] }
// [ts] fn queueDiscreteEvent(blockedOn: auto, topLevelType: auto, eventSystemFlags: auto, container: auto, nativeEvent: auto) {
// [ts] let queuedEvent = createQueuedReplayableEvent(blockedOn, topLevelType, eventSystemFlags, container, nativeEvent);
// [ts] queuedDiscreteEvents.push(queuedEvent);
// [ts] } // Resets the replaying for this type of continuous event to no event.
// [ts] fn clearIfContinuousEvent(topLevelType: auto, nativeEvent: auto) {
// [ts] switch (topLevelType) {
// [ts] TOP_FOCUS, TOP_BLUR => {
// [ts] queuedFocus = null;
// break;
// [ts] },
// [ts] TOP_DRAG_ENTER, TOP_DRAG_LEAVE => {
// [ts] queuedDrag = null;
// break;
// [ts] },
// [ts] TOP_MOUSE_OVER, TOP_MOUSE_OUT => {
// [ts] queuedMouse = null;
// break;
// [ts] },
// [ts] TOP_POINTER_OVER, TOP_POINTER_OUT => {
// [ts] {
// [ts] let pointerId = nativeEvent.pointerId;
// [ts] queuedPointers.delete(pointerId);
// [ts] break;
// [ts] }
// [ts] },
// [ts] TOP_GOT_POINTER_CAPTURE, TOP_LOST_POINTER_CAPTURE => {
// [ts] {
// [ts] let _pointerId = nativeEvent.pointerId;
// [ts] queuedPointerCaptures.delete(_pointerId);
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn accumulateOrCreateContinuousQueuedReplayableEvent(existingQueuedEvent: auto, blockedOn: auto, topLevelType: auto, eventSystemFlags: auto, container: auto, nativeEvent: auto) {
// [ts] if (existingQueuedEvent == null || existingQueuedEvent.nativeEvent != nativeEvent) {
// [ts] let queuedEvent = createQueuedReplayableEvent(blockedOn, topLevelType, eventSystemFlags, container, nativeEvent);
// [ts] if (blockedOn != null) {
// [ts] let _fiber2 = getInstanceFromNode$1(blockedOn);
// [ts] if (_fiber2 != null) {
// Attempt to increase the priority of this target.
// [ts] attemptContinuousHydration(_fiber2);
// [ts] }
// [ts] }
// [ts] return queuedEvent;
// [ts] } // If we have already queued this exact event, then it's because
// the different event systems have different DOM event listeners.
// We can accumulate the flags and store a single event to be
// replayed.
// [ts] existingQueuedEvent.eventSystemFlags |= eventSystemFlags;
// [ts] return existingQueuedEvent;
// [ts] }
// [ts] fn queueIfContinuousEvent(blockedOn: auto, topLevelType: auto, eventSystemFlags: auto, container: auto, nativeEvent: auto) {
// These set relatedTarget to null because the replayed event will be treated as if we
// moved from outside the window (no target) onto the target once it hydrates.
// Instead of mutating we could clone the event.
// [ts] switch (topLevelType) {
// [ts] TOP_FOCUS => {
// [ts] {
// [ts] let focusEvent = nativeEvent;
// [ts] queuedFocus = accumulateOrCreateContinuousQueuedReplayableEvent(queuedFocus, blockedOn, topLevelType, eventSystemFlags, container, focusEvent);
// [ts] return true;
// [ts] }
// [ts] },
// [ts] TOP_DRAG_ENTER => {
// [ts] {
// [ts] let dragEvent = nativeEvent;
// [ts] queuedDrag = accumulateOrCreateContinuousQueuedReplayableEvent(queuedDrag, blockedOn, topLevelType, eventSystemFlags, container, dragEvent);
// [ts] return true;
// [ts] }
// [ts] },
// [ts] TOP_MOUSE_OVER => {
// [ts] {
// [ts] let mouseEvent = nativeEvent;
// [ts] queuedMouse = accumulateOrCreateContinuousQueuedReplayableEvent(queuedMouse, blockedOn, topLevelType, eventSystemFlags, container, mouseEvent);
// [ts] return true;
// [ts] }
// [ts] },
// [ts] TOP_POINTER_OVER => {
// [ts] {
// [ts] let pointerEvent = nativeEvent;
// [ts] let pointerId = pointerEvent.pointerId;
// [ts] queuedPointers.set(pointerId, accumulateOrCreateContinuousQueuedReplayableEvent(queuedPointers.get(pointerId) || null, blockedOn, topLevelType, eventSystemFlags, container, pointerEvent));
// [ts] return true;
// [ts] }
// [ts] },
// [ts] TOP_GOT_POINTER_CAPTURE => {
// [ts] {
// [ts] let _pointerEvent = nativeEvent;
// [ts] let _pointerId2 = _pointerEvent.pointerId;
// [ts] queuedPointerCaptures.set(_pointerId2, accumulateOrCreateContinuousQueuedReplayableEvent(queuedPointerCaptures.get(_pointerId2) || null, blockedOn, topLevelType, eventSystemFlags, container, _pointerEvent));
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// [ts] return false;
// [ts] } // Check if this target is unblocked. Returns true if it's unblocked.
// [ts] fn attemptExplicitHydrationTarget(queuedTarget: auto) {
// TODO: This function shares a lot of logic with attemptToDispatchEvent.
// Try to unify them. It's a bit tricky since it would require two return
// values.
// [ts] let targetInst = getClosestInstanceFromNode(queuedTarget.target);
// [ts] if (targetInst != null) {
// [ts] let nearestMounted = getNearestMountedFiber(targetInst);
// [ts] if (nearestMounted != null) {
// [ts] let tag = nearestMounted.tag;
// [ts] if (tag == SuspenseComponent) {
// [ts] let instance = getSuspenseInstanceFromFiber(nearestMounted);
// [ts] if (instance != null) {
// We're blocked on hydrating this boundary.
// Increase its priority.
// [ts] queuedTarget.blockedOn = instance;
// [ts] unstable_runWithPriority(queuedTarget.priority, () => {
// [ts] attemptHydrationAtCurrentPriority(nearestMounted);
// [ts] });
// [ts] return;
// [ts] }
// [ts] } else if (tag == HostRoot) {
// [ts] let root = nearestMounted.stateNode;
// [ts] if (root.hydrate) {
// [ts] queuedTarget.blockedOn = getContainerFromFiber(nearestMounted); // We don't currently have a way to increase the priority of
// a root other than sync.
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] queuedTarget.blockedOn = null;
// [ts] }
// [ts] fn attemptReplayContinuousQueuedEvent(queuedEvent: auto) {
// [ts] if (queuedEvent.blockedOn != null) {
// [ts] return false;
// [ts] }
// [ts] let nextBlockedOn = attemptToDispatchEvent(queuedEvent.topLevelType, queuedEvent.eventSystemFlags, queuedEvent.container, queuedEvent.nativeEvent);
// [ts] if (nextBlockedOn != null) {
// We're still blocked. Try again later.
// [ts] let _fiber3 = getInstanceFromNode$1(nextBlockedOn);
// [ts] if (_fiber3 != null) {
// [ts] attemptContinuousHydration(_fiber3);
// [ts] }
// [ts] queuedEvent.blockedOn = nextBlockedOn;
// [ts] return false;
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn attemptReplayContinuousQueuedEventInMap(queuedEvent: auto, key: auto, map: auto) {
// [ts] if (attemptReplayContinuousQueuedEvent(queuedEvent)) {
// [ts] map.delete(key);
// [ts] }
// [ts] }
// [ts] fn replayUnblockedEvents() {
// [ts] hasScheduledReplayAttempt = false; // First replay discrete events.
// [ts] while (queuedDiscreteEvents.length > 0) {
// [ts] let nextDiscreteEvent = queuedDiscreteEvents[0];
// [ts] if (nextDiscreteEvent.blockedOn != null) {
// We're still blocked.
// Increase the priority of this boundary to unblock
// the next discrete event.
// [ts] let _fiber4 = getInstanceFromNode$1(nextDiscreteEvent.blockedOn);
// [ts] if (_fiber4 != null) {
// [ts] attemptUserBlockingHydration(_fiber4);
// [ts] }
// [ts] break;
// [ts] }
// [ts] let nextBlockedOn = attemptToDispatchEvent(nextDiscreteEvent.topLevelType, nextDiscreteEvent.eventSystemFlags, nextDiscreteEvent.container, nextDiscreteEvent.nativeEvent);
// [ts] if (nextBlockedOn != null) {
// We're still blocked. Try again later.
// [ts] nextDiscreteEvent.blockedOn = nextBlockedOn;
// [ts] } else {
// We've successfully replayed the first event. Let's try the next one.
// [ts] queuedDiscreteEvents.shift();
// [ts] }
// [ts] } // Next replay any continuous events.
// [ts] if (queuedFocus != null && attemptReplayContinuousQueuedEvent(queuedFocus)) {
// [ts] queuedFocus = null;
// [ts] }
// [ts] if (queuedDrag != null && attemptReplayContinuousQueuedEvent(queuedDrag)) {
// [ts] queuedDrag = null;
// [ts] }
// [ts] if (queuedMouse != null && attemptReplayContinuousQueuedEvent(queuedMouse)) {
// [ts] queuedMouse = null;
// [ts] }
// [ts] queuedPointers.forEach(attemptReplayContinuousQueuedEventInMap);
// [ts] queuedPointerCaptures.forEach(attemptReplayContinuousQueuedEventInMap);
// [ts] }
// [ts] fn scheduleCallbackIfUnblocked(queuedEvent: auto, unblocked: auto) {
// [ts] if (queuedEvent.blockedOn == unblocked) {
// [ts] queuedEvent.blockedOn = null;
// [ts] if (!hasScheduledReplayAttempt) {
// [ts] hasScheduledReplayAttempt = true; // Schedule a callback to attempt replaying as many events as are
// now unblocked. This first might not actually be unblocked yet.
// We could check it early to avoid scheduling an unnecessary callback.
// [ts] unstable_scheduleCallback(unstable_NormalPriority, replayUnblockedEvents);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn retryIfBlockedOn(unblocked: auto) {
// Mark anything that was blocked on this as no longer blocked
// and eligible for a replay.
// [ts] if (queuedDiscreteEvents.length > 0) {
// [ts] scheduleCallbackIfUnblocked(queuedDiscreteEvents[0], unblocked); // This is a exponential search for each boundary that commits. I think it's
// worth it because we expect very few discrete events to queue up and once
// we are actually fully unblocked it will be fast to replay them.
// [ts] let i = 1;
// [ts] while (i < queuedDiscreteEvents.length) { // [cap-migrate] for-step: i++
// [ts] let queuedEvent = queuedDiscreteEvents[i];
// [ts] if (queuedEvent.blockedOn == unblocked) {
// [ts] queuedEvent.blockedOn = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] if (queuedFocus != null) {
// [ts] scheduleCallbackIfUnblocked(queuedFocus, unblocked);
// [ts] }
// [ts] if (queuedDrag != null) {
// [ts] scheduleCallbackIfUnblocked(queuedDrag, unblocked);
// [ts] }
// [ts] if (queuedMouse != null) {
// [ts] scheduleCallbackIfUnblocked(queuedMouse, unblocked);
// [ts] }
// [ts] let unblock = (queuedEvent) => {
// [ts] return scheduleCallbackIfUnblocked(queuedEvent, unblocked);
// [ts] };
// [ts] queuedPointers.forEach(unblock);
// [ts] queuedPointerCaptures.forEach(unblock);
// [ts] let _i = 0;
// [ts] while (_i < queuedExplicitHydrationTargets.length) { // [cap-migrate] for-step: _i++
// [ts] let queuedTarget = queuedExplicitHydrationTargets[_i];
// [ts] if (queuedTarget.blockedOn == unblocked) {
// [ts] queuedTarget.blockedOn = null;
// [ts] }
// [ts] }
// [ts] while (queuedExplicitHydrationTargets.length > 0) {
// [ts] let nextExplicitTarget = queuedExplicitHydrationTargets[0];
// [ts] if (nextExplicitTarget.blockedOn != null) {
// We're still blocked.
// [ts] break;
// [ts] } else {
// [ts] attemptExplicitHydrationTarget(nextExplicitTarget);
// [ts] if (nextExplicitTarget.blockedOn == null) {
// We're unblocked.
// [ts] queuedExplicitHydrationTargets.shift();
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn addEventBubbleListener(element: auto, eventType: auto, listener: auto) {
// [ts] element.addEventListener(eventType, listener, false);
// [ts] }
// [ts] fn addEventCaptureListener(element: auto, eventType: auto, listener: auto) {
// [ts] element.addEventListener(eventType, listener, true);
// [ts] }
// do it in two places, which duplicates logic
// and increases the bundle size, we do it all
// here once. If we remove or refactor the
// SimpleEventPlugin, we should also remove or
// update the below line.
// [ts] let simpleEventPluginEventTypes = {};
// [ts] let topLevelEventsToDispatchConfig = Map.init();
// [ts] let eventPriorities = Map.init(); // We store most of the events in this module in pairs of two strings so we can re-use
// the code required to apply the same logic for event prioritization and that of the
// SimpleEventPlugin. This complicates things slightly, but the aim is to reduce code
// duplication (for which there would be quite a bit). For the events that are not needed
// for the SimpleEventPlugin (otherDiscreteEvents) we process them separately as an
// array of top level events.
// Lastly, we ignore prettier so we can keep the formatting sane.
// prettier-ignore
// [ts] let discreteEventPairsForSimpleEventPlugin = [TOP_BLUR, "blur", TOP_CANCEL, "cancel", TOP_CLICK, "click", TOP_CLOSE, "close", TOP_CONTEXT_MENU, "contextMenu", TOP_COPY, "copy", TOP_CUT, "cut", TOP_AUX_CLICK, "auxClick", TOP_DOUBLE_CLICK, "doubleClick", TOP_DRAG_END, "dragEnd", TOP_DRAG_START, "dragStart", TOP_DROP, "drop", TOP_FOCUS, "focus", TOP_INPUT, "input", TOP_INVALID, "invalid", TOP_KEY_DOWN, "keyDown", TOP_KEY_PRESS, "keyPress", TOP_KEY_UP, "keyUp", TOP_MOUSE_DOWN, "mouseDown", TOP_MOUSE_UP, "mouseUp", TOP_PASTE, "paste", TOP_PAUSE, "pause", TOP_PLAY, "play", TOP_POINTER_CANCEL, "pointerCancel", TOP_POINTER_DOWN, "pointerDown", TOP_POINTER_UP, "pointerUp", TOP_RATE_CHANGE, "rateChange", TOP_RESET, "reset", TOP_SEEKED, "seeked", TOP_SUBMIT, "submit", TOP_TOUCH_CANCEL, "touchCancel", TOP_TOUCH_END, "touchEnd", TOP_TOUCH_START, "touchStart", TOP_VOLUME_CHANGE, "volumeChange"];
// [ts] let otherDiscreteEvents = [TOP_CHANGE, TOP_SELECTION_CHANGE, TOP_TEXT_INPUT, TOP_COMPOSITION_START, TOP_COMPOSITION_END, TOP_COMPOSITION_UPDATE]; // prettier-ignore
// [ts] let userBlockingPairsForSimpleEventPlugin = [TOP_DRAG, "drag", TOP_DRAG_ENTER, "dragEnter", TOP_DRAG_EXIT, "dragExit", TOP_DRAG_LEAVE, "dragLeave", TOP_DRAG_OVER, "dragOver", TOP_MOUSE_MOVE, "mouseMove", TOP_MOUSE_OUT, "mouseOut", TOP_MOUSE_OVER, "mouseOver", TOP_POINTER_MOVE, "pointerMove", TOP_POINTER_OUT, "pointerOut", TOP_POINTER_OVER, "pointerOver", TOP_SCROLL, "scroll", TOP_TOGGLE, "toggle", TOP_TOUCH_MOVE, "touchMove", TOP_WHEEL, "wheel"]; // prettier-ignore
// [ts] let continuousPairsForSimpleEventPlugin = [TOP_ABORT, "abort", TOP_ANIMATION_END, "animationEnd", TOP_ANIMATION_ITERATION, "animationIteration", TOP_ANIMATION_START, "animationStart", TOP_CAN_PLAY, "canPlay", TOP_CAN_PLAY_THROUGH, "canPlayThrough", TOP_DURATION_CHANGE, "durationChange", TOP_EMPTIED, "emptied", TOP_ENCRYPTED, "encrypted", TOP_ENDED, "ended", TOP_ERROR, "error", TOP_GOT_POINTER_CAPTURE, "gotPointerCapture", TOP_LOAD, "load", TOP_LOADED_DATA, "loadedData", TOP_LOADED_METADATA, "loadedMetadata", TOP_LOAD_START, "loadStart", TOP_LOST_POINTER_CAPTURE, "lostPointerCapture", TOP_PLAYING, "playing", TOP_PROGRESS, "progress", TOP_SEEKING, "seeking", TOP_STALLED, "stalled", TOP_SUSPEND, "suspend", TOP_TIME_UPDATE, "timeUpdate", TOP_TRANSITION_END, "transitionEnd", TOP_WAITING, "waiting"];
// [ts] /**
// [ts] * Turns
// [ts] * ['abort', ...]
// [ts] * into
// [ts] * eventTypes = {
// [ts] *   'abort': {
// [ts] *     phasedRegistrationNames: {
// [ts] *       bubbled: 'onAbort',
// [ts] *       captured: 'onAbortCapture',
// [ts] *     },
// [ts] *     dependencies: [TOP_ABORT],
// [ts] *   },
// [ts] *   ...
// [ts] * };
// [ts] * topLevelEventsToDispatchConfig = new Map([
// [ts] *   [TOP_ABORT, { sameConfig }],
// [ts] * ]);
// [ts] */
// [ts] fn processSimpleEventPluginPairsByPriority(eventTypes: auto, priority: auto) {
// As the event types are in pairs of two, we need to iterate
// through in twos. The events are in pairs of two to save code
// and improve init perf of processing this array, as it will
// result in far fewer object allocations and property accesses
// if we only use three arrays to process all the categories of
// instead of tuples.
// [ts] let i = 0;
// [ts] while (i < eventTypes.length) { // [cap-migrate] for-step: i += 2
// [ts] let topEvent = eventTypes[i];
// [ts] let event = eventTypes[i + 1];
// [ts] let capitalizedEvent = event[0].toUpperCase() + event.slice(1);
// [ts] let onEvent = "on" + capitalizedEvent;
// [ts] let config = {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: onEvent,
// [ts] captured: onEvent + "Capture"
// [ts] },
// [ts] dependencies: [topEvent],
// [ts] eventPriority: priority
// [ts] };
// [ts] eventPriorities.set(topEvent, priority);
// [ts] topLevelEventsToDispatchConfig.set(topEvent, config);
// [ts] simpleEventPluginEventTypes[event] = config;
// [ts] }
// [ts] }
// [ts] fn processTopEventPairsByPriority(eventTypes: auto, priority: auto) {
// [ts] let i = 0;
// [ts] while (i < eventTypes.length) { // [cap-migrate] for-step: i++
// [ts] eventPriorities.set(eventTypes[i], priority);
// [ts] }
// [ts] } // SimpleEventPlugin
// [ts] processSimpleEventPluginPairsByPriority(discreteEventPairsForSimpleEventPlugin, DiscreteEvent);
// [ts] processSimpleEventPluginPairsByPriority(userBlockingPairsForSimpleEventPlugin, UserBlockingEvent);
// [ts] processSimpleEventPluginPairsByPriority(continuousPairsForSimpleEventPlugin, ContinuousEvent); // Not used by SimpleEventPlugin
// [ts] processTopEventPairsByPriority(otherDiscreteEvents, DiscreteEvent);
// [ts] fn getEventPriorityForPluginSystem(topLevelType: auto) {
// [ts] let priority = eventPriorities.get(topLevelType); // Default to a ContinuousEvent. Note: we might
// want to warn if we can't detect the priority
// for the event.
// [ts] return if (priority == null) ContinuousEvent else priority;
// [ts] }
// Intentionally not named imports because Rollup would use dynamic dispatch for
// [ts] let UserBlockingPriority = unstable_UserBlockingPriority,
// [ts] runWithPriority = unstable_runWithPriority; // TODO: can we stop exporting these?
// [ts] let _enabled = true;
// [ts] fn setEnabled(enabled: auto) {
// [ts] _enabled = !!enabled;
// [ts] }
// [ts] fn isEnabled() {
// [ts] return _enabled;
// [ts] }
// [ts] fn trapBubbledEvent(topLevelType: auto, element: auto) {
// [ts] trapEventForPluginEventSystem(element, topLevelType, false);
// [ts] }
// [ts] fn trapCapturedEvent(topLevelType: auto, element: auto) {
// [ts] trapEventForPluginEventSystem(element, topLevelType, true);
// [ts] }
// [ts] fn trapEventForPluginEventSystem(container: auto, topLevelType: auto, capture: auto) {
// [ts] let listener;
// [ts] switch (getEventPriorityForPluginSystem(topLevelType)) {
// [ts] DiscreteEvent => {
// [ts] listener = dispatchDiscreteEvent.bind(null, topLevelType, PLUGIN_EVENT_SYSTEM, container);
// break;
// [ts] },
// [ts] UserBlockingEvent => {
// [ts] listener = dispatchUserBlockingUpdate.bind(null, topLevelType, PLUGIN_EVENT_SYSTEM, container);
// break;
// [ts] },
// [ts] else => {
// [ts] listener = dispatchEvent.bind(null, topLevelType, PLUGIN_EVENT_SYSTEM, container);
// break;
// [ts] }
// [ts] }
// [ts] let rawEventName = getRawEventName(topLevelType);
// [ts] if (capture) {
// [ts] addEventCaptureListener(container, rawEventName, listener);
// [ts] } else {
// [ts] addEventBubbleListener(container, rawEventName, listener);
// [ts] }
// [ts] }
// [ts] fn dispatchDiscreteEvent(topLevelType: auto, eventSystemFlags: auto, container: auto, nativeEvent: auto) {
// [ts] flushDiscreteUpdatesIfNeeded(nativeEvent.timeStamp);
// [ts] discreteUpdates(dispatchEvent, topLevelType, eventSystemFlags, container, nativeEvent);
// [ts] }
// [ts] fn dispatchUserBlockingUpdate(topLevelType: auto, eventSystemFlags: auto, container: auto, nativeEvent: auto) {
// [ts] runWithPriority(UserBlockingPriority, dispatchEvent.bind(null, topLevelType, eventSystemFlags, container, nativeEvent));
// [ts] }
// [ts] fn dispatchEvent(topLevelType: auto, eventSystemFlags: auto, container: auto, nativeEvent: auto) {
// [ts] if (!_enabled) {
// [ts] return;
// [ts] }
// [ts] if (hasQueuedDiscreteEvents() && isReplayableDiscreteEvent(topLevelType)) {
// If we already have a queue of discrete events, and this is another discrete
// event, then we can't dispatch it regardless of its target, since they
// need to dispatch in order.
// [ts] queueDiscreteEvent(null, // Flags that we're not actually blocked on anything as far as we know.
// [ts] topLevelType, eventSystemFlags, container, nativeEvent);
// [ts] return;
// [ts] }
// [ts] let blockedOn = attemptToDispatchEvent(topLevelType, eventSystemFlags, container, nativeEvent);
// [ts] if (blockedOn == null) {
// We successfully dispatched this event.
// [ts] clearIfContinuousEvent(topLevelType, nativeEvent);
// [ts] return;
// [ts] }
// [ts] if (isReplayableDiscreteEvent(topLevelType)) {
// This this to be replayed later once the target is available.
// [ts] queueDiscreteEvent(blockedOn, topLevelType, eventSystemFlags, container, nativeEvent);
// [ts] return;
// [ts] }
// [ts] if (queueIfContinuousEvent(blockedOn, topLevelType, eventSystemFlags, container, nativeEvent)) {
// [ts] return;
// [ts] } // We need to clear only if we didn't queue because
// queueing is accummulative.
// [ts] clearIfContinuousEvent(topLevelType, nativeEvent); // This is not replayable so we'll invoke it but without a target,
// in case the event system needs to trace it.
// [ts] {
// [ts] dispatchEventForLegacyPluginEventSystem(topLevelType, eventSystemFlags, nativeEvent, null);
// [ts] }
// [ts] } // Attempt dispatching an event. Returns a SuspenseInstance or Container if it's blocked.
// [ts] fn attemptToDispatchEvent(topLevelType: auto, eventSystemFlags: auto, container: auto, nativeEvent: auto) {
// TODO: Warn if _enabled is false.
// [ts] let nativeEventTarget = getEventTarget(nativeEvent);
// [ts] let targetInst = getClosestInstanceFromNode(nativeEventTarget);
// [ts] if (targetInst != null) {
// [ts] let nearestMounted = getNearestMountedFiber(targetInst);
// [ts] if (nearestMounted == null) {
// This tree has been unmounted already. Dispatch without a target.
// [ts] targetInst = null;
// [ts] } else {
// [ts] let tag = nearestMounted.tag;
// [ts] if (tag == SuspenseComponent) {
// [ts] let instance = getSuspenseInstanceFromFiber(nearestMounted);
// [ts] if (instance != null) {
// Queue the event to be replayed later. Abort dispatching since we
// don't want this event dispatched twice through the event system.
// TODO: If this is the first discrete event in the queue. Schedule an increased
// priority for this boundary.
// [ts] return instance;
// [ts] } // This shouldn't happen, something went wrong but to avoid blocking
// the whole system, dispatch the event without a target.
// TODO: Warn.
// [ts] targetInst = null;
// [ts] } else if (tag == HostRoot) {
// [ts] let root = nearestMounted.stateNode;
// [ts] if (root.hydrate) {
// If this happens during a replay something went wrong and it might block
// the whole system.
// [ts] return getContainerFromFiber(nearestMounted);
// [ts] }
// [ts] targetInst = null;
// [ts] } else if (nearestMounted != targetInst) {
// If we get an event (ex: img onload) before committing that
// component's mount, ignore it for now (that is, treat it as if it was an
// event on a non-React tree). We might also consider queueing events and
// dispatching them after the mount.
// [ts] targetInst = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] dispatchEventForLegacyPluginEventSystem(topLevelType, eventSystemFlags, nativeEvent, targetInst);
// [ts] } // We're not blocked on anything.
// [ts] return null;
// [ts] }
// List derived from Gecko source code:
// https://github.com/mozilla/gecko-dev/blob/4e638efc71/layout/style/test/property_database.js
// [ts] let shorthandToLonghand = {
// [ts] animation: ["animationDelay", "animationDirection", "animationDuration", "animationFillMode", "animationIterationCount", "animationName", "animationPlayState", "animationTimingFunction"],
// [ts] background: ["backgroundAttachment", "backgroundClip", "backgroundColor", "backgroundImage", "backgroundOrigin", "backgroundPositionX", "backgroundPositionY", "backgroundRepeat", "backgroundSize"],
// [ts] backgroundPosition: ["backgroundPositionX", "backgroundPositionY"],
// [ts] border: ["borderBottomColor", "borderBottomStyle", "borderBottomWidth", "borderImageOutset", "borderImageRepeat", "borderImageSlice", "borderImageSource", "borderImageWidth", "borderLeftColor", "borderLeftStyle", "borderLeftWidth", "borderRightColor", "borderRightStyle", "borderRightWidth", "borderTopColor", "borderTopStyle", "borderTopWidth"],
// [ts] borderBlockEnd: ["borderBlockEndColor", "borderBlockEndStyle", "borderBlockEndWidth"],
// [ts] borderBlockStart: ["borderBlockStartColor", "borderBlockStartStyle", "borderBlockStartWidth"],
// [ts] borderBottom: ["borderBottomColor", "borderBottomStyle", "borderBottomWidth"],
// [ts] borderColor: ["borderBottomColor", "borderLeftColor", "borderRightColor", "borderTopColor"],
// [ts] borderImage: ["borderImageOutset", "borderImageRepeat", "borderImageSlice", "borderImageSource", "borderImageWidth"],
// [ts] borderInlineEnd: ["borderInlineEndColor", "borderInlineEndStyle", "borderInlineEndWidth"],
// [ts] borderInlineStart: ["borderInlineStartColor", "borderInlineStartStyle", "borderInlineStartWidth"],
// [ts] borderLeft: ["borderLeftColor", "borderLeftStyle", "borderLeftWidth"],
// [ts] borderRadius: ["borderBottomLeftRadius", "borderBottomRightRadius", "borderTopLeftRadius", "borderTopRightRadius"],
// [ts] borderRight: ["borderRightColor", "borderRightStyle", "borderRightWidth"],
// [ts] borderStyle: ["borderBottomStyle", "borderLeftStyle", "borderRightStyle", "borderTopStyle"],
// [ts] borderTop: ["borderTopColor", "borderTopStyle", "borderTopWidth"],
// [ts] borderWidth: ["borderBottomWidth", "borderLeftWidth", "borderRightWidth", "borderTopWidth"],
// [ts] columnRule: ["columnRuleColor", "columnRuleStyle", "columnRuleWidth"],
// [ts] columns: ["columnCount", "columnWidth"],
// [ts] flex: ["flexBasis", "flexGrow", "flexShrink"],
// [ts] flexFlow: ["flexDirection", "flexWrap"],
// [ts] font: ["fontFamily", "fontFeatureSettings", "fontKerning", "fontLanguageOverride", "fontSize", "fontSizeAdjust", "fontStretch", "fontStyle", "fontVariant", "fontVariantAlternates", "fontVariantCaps", "fontVariantEastAsian", "fontVariantLigatures", "fontVariantNumeric", "fontVariantPosition", "fontWeight", "lineHeight"],
// [ts] fontVariant: ["fontVariantAlternates", "fontVariantCaps", "fontVariantEastAsian", "fontVariantLigatures", "fontVariantNumeric", "fontVariantPosition"],
// [ts] gap: ["columnGap", "rowGap"],
// [ts] grid: ["gridAutoColumns", "gridAutoFlow", "gridAutoRows", "gridTemplateAreas", "gridTemplateColumns", "gridTemplateRows"],
// [ts] gridArea: ["gridColumnEnd", "gridColumnStart", "gridRowEnd", "gridRowStart"],
// [ts] gridColumn: ["gridColumnEnd", "gridColumnStart"],
// [ts] gridColumnGap: ["columnGap"],
// [ts] gridGap: ["columnGap", "rowGap"],
// [ts] gridRow: ["gridRowEnd", "gridRowStart"],
// [ts] gridRowGap: ["rowGap"],
// [ts] gridTemplate: ["gridTemplateAreas", "gridTemplateColumns", "gridTemplateRows"],
// [ts] listStyle: ["listStyleImage", "listStylePosition", "listStyleType"],
// [ts] margin: ["marginBottom", "marginLeft", "marginRight", "marginTop"],
// [ts] marker: ["markerEnd", "markerMid", "markerStart"],
// [ts] mask: ["maskClip", "maskComposite", "maskImage", "maskMode", "maskOrigin", "maskPositionX", "maskPositionY", "maskRepeat", "maskSize"],
// [ts] maskPosition: ["maskPositionX", "maskPositionY"],
// [ts] outline: ["outlineColor", "outlineStyle", "outlineWidth"],
// [ts] overflow: ["overflowX", "overflowY"],
// [ts] padding: ["paddingBottom", "paddingLeft", "paddingRight", "paddingTop"],
// [ts] placeContent: ["alignContent", "justifyContent"],
// [ts] placeItems: ["alignItems", "justifyItems"],
// [ts] placeSelf: ["alignSelf", "justifySelf"],
// [ts] textDecoration: ["textDecorationColor", "textDecorationLine", "textDecorationStyle"],
// [ts] textEmphasis: ["textEmphasisColor", "textEmphasisStyle"],
// [ts] transition: ["transitionDelay", "transitionDuration", "transitionProperty", "transitionTimingFunction"],
// [ts] wordWrap: ["overflowWrap"]
// [ts] };
// [ts] /**
// [ts] * CSS properties which accept numbers but are not in units of "px".
// [ts] */
// [ts] let isUnitlessNumber = {
// [ts] animationIterationCount: true,
// [ts] borderImageOutset: true,
// [ts] borderImageSlice: true,
// [ts] borderImageWidth: true,
// [ts] boxFlex: true,
// [ts] boxFlexGroup: true,
// [ts] boxOrdinalGroup: true,
// [ts] columnCount: true,
// [ts] columns: true,
// [ts] flex: true,
// [ts] flexGrow: true,
// [ts] flexPositive: true,
// [ts] flexShrink: true,
// [ts] flexNegative: true,
// [ts] flexOrder: true,
// [ts] gridArea: true,
// [ts] gridRow: true,
// [ts] gridRowEnd: true,
// [ts] gridRowSpan: true,
// [ts] gridRowStart: true,
// [ts] gridColumn: true,
// [ts] gridColumnEnd: true,
// [ts] gridColumnSpan: true,
// [ts] gridColumnStart: true,
// [ts] fontWeight: true,
// [ts] lineClamp: true,
// [ts] lineHeight: true,
// [ts] opacity: true,
// [ts] order: true,
// [ts] orphans: true,
// [ts] tabSize: true,
// [ts] widows: true,
// [ts] zIndex: true,
// [ts] zoom: true,
// SVG-related properties
// [ts] fillOpacity: true,
// [ts] floodOpacity: true,
// [ts] stopOpacity: true,
// [ts] strokeDasharray: true,
// [ts] strokeDashoffset: true,
// [ts] strokeMiterlimit: true,
// [ts] strokeOpacity: true,
// [ts] strokeWidth: true
// [ts] };
// [ts] /**
// [ts] * @param {string} prefix vendor-specific prefix, eg: Webkit
// [ts] * @param {string} key style name, eg: transitionDuration
// [ts] * @return {string} style name prefixed with `prefix`, properly camelCased, eg:
// [ts] * WebkitTransitionDuration
// [ts] */
// [ts] fn prefixKey(prefix: auto, key: auto) {
// [ts] return prefix + key.charAt(0).toUpperCase() + key.substring(1);
// [ts] }
// [ts] /**
// [ts] * Support style names that may come passed in prefixed by adding permutations
// [ts] * of vendor prefixes.
// [ts] */
// [ts] let prefixes = ["Webkit", "ms", "Moz", "O"]; // Using Object.keys here, or else the vanilla for-in loop makes IE8 go into an
// infinite loop, because it iterates over the newly added props too.
// [ts] Object.keys(isUnitlessNumber).forEach((prop) => {
// [ts] prefixes.forEach((prefix) => {
// [ts] isUnitlessNumber[prefixKey(prefix, prop)] = isUnitlessNumber[prop];
// [ts] });
// [ts] });
// [ts] /**
// [ts] * Convert a value into the proper css writable value. The style name `name`
// [ts] * should be logical (no hyphens), as specified
// [ts] * in `CSSProperty.isUnitlessNumber`.
// [ts] *
// [ts] * @param {string} name CSS property name such as `topMargin`.
// [ts] * @param {*} value CSS property value such as `10px`.
// [ts] * @return {string} Normalized style value with dimensions applied.
// [ts] */
// [ts] fn dangerousStyleValue(name: auto, value: auto, isCustomProperty: auto) {
// Note that we've removed escapeTextForBrowser() calls here since the
// whole string will be escaped when the attribute is injected into
// the markup. If you provide unsafe user data here they can inject
// arbitrary CSS which may be problematic (I couldn't repro this):
// https://www.owasp.org/index.php/XSS_Filter_Evasion_Cheat_Sheet
// http://www.thespanner.co.uk/2007/11/26/ultimate-xss-css-injection/
// This is not an XSS hole but instead a potential CSS injection issue
// which has lead to a greater discussion about how we're going to
// trust URLs moving forward. See #2115901
// [ts] let isEmpty = value == null || @typeName(value) == "boolean" || value == "";
// [ts] if (isEmpty) {
// [ts] return "";
// [ts] }
// [ts] if (!isCustomProperty && @typeName(value) == "number" && value != 0 && !(isUnitlessNumber.hasOwnProperty(name) && isUnitlessNumber[name])) {
// [ts] return value + "px"; // Presumes implicit 'px' suffix for unitless numbers
// [ts] }
// [ts] return ("" + value).trim();
// [ts] }
// [ts] let uppercasePattern = "([A-Z])";
// [ts] let msPattern = "^ms-";
// [ts] /**
// [ts] * Hyphenates a camelcased CSS property name, for example:
// [ts] *
// [ts] *   > hyphenateStyleName('backgroundColor')
// [ts] *   < "background-color"
// [ts] *   > hyphenateStyleName('MozTransition')
// [ts] *   < "-moz-transition"
// [ts] *   > hyphenateStyleName('msTransition')
// [ts] *   < "-ms-transition"
// [ts] *
// [ts] * As Modernizr suggests (http://modernizr.com/docs/#prefixed), an `ms` prefix
// [ts] * is converted to `-ms-`.
// [ts] */
// [ts] fn hyphenateStyleName(name: auto) {
// [ts] return name.replace(uppercasePattern, "-$1").toLowerCase().replace(msPattern, "-ms-");
// [ts] }
// [ts] let warnValidStyle = () => {};
// [ts] {
// 'msTransform' is correct, but the other prefixes should be capitalized
// [ts] let badVendoredStyleNamePattern = "^(?:webkit|moz|o)[A-Z]";
// [ts] let msPattern$1 = "^-ms-";
// [ts] let hyphenPattern = "-(.)"; // style values shouldn't contain a semicolon
// [ts] let badStyleValueWithSemicolonPattern = ";\\s*$";
// [ts] let warnedStyleNames = {};
// [ts] let warnedStyleValues = {};
// [ts] let warnedForNaNValue = false;
// [ts] let warnedForInfinityValue = false;
// [ts] let camelize = (string) => {
// [ts] return string.replace(hyphenPattern, (_unused, character) => {
// [ts] return character.toUpperCase();
// [ts] });
// [ts] };
// [ts] let warnHyphenatedStyleName = (name) => {
// [ts] if (warnedStyleNames.hasOwnProperty(name) && warnedStyleNames[name]) {
// [ts] return;
// [ts] }
// [ts] warnedStyleNames[name] = true;
// [ts] error("Unsupported style property %s. Did you mean %s?", name, // As Andi Smith suggests
// (http://www.andismith.com/blog/2012/02/modernizr-prefixed/), an `-ms` prefix
// is converted to lowercase `ms`.
// [ts] camelize(name.replace(msPattern$1, "ms-")));
// [ts] };
// [ts] let warnBadVendoredStyleName = (name) => {
// [ts] if (warnedStyleNames.hasOwnProperty(name) && warnedStyleNames[name]) {
// [ts] return;
// [ts] }
// [ts] warnedStyleNames[name] = true;
// [ts] error("Unsupported vendor-prefixed style property %s. Did you mean %s?", name, name.charAt(0).toUpperCase() + name.slice(1));
// [ts] };
// [ts] let warnStyleValueWithSemicolon = (name, value) => {
// [ts] if (warnedStyleValues.hasOwnProperty(value) && warnedStyleValues[value]) {
// [ts] return;
// [ts] }
// [ts] warnedStyleValues[value] = true;
// [ts] error("Style property values shouldn't contain a semicolon. " + "Try \"%s: %s\" instead.", name, value.replace(badStyleValueWithSemicolonPattern, ""));
// [ts] };
// [ts] let warnStyleValueIsNaN = (name, value) => {
// [ts] if (warnedForNaNValue) {
// [ts] return;
// [ts] }
// [ts] warnedForNaNValue = true;
// [ts] error("`NaN` is an invalid value for the `%s` css style property.", name);
// [ts] };
// [ts] let warnStyleValueIsInfinity = (name, value) => {
// [ts] if (warnedForInfinityValue) {
// [ts] return;
// [ts] }
// [ts] warnedForInfinityValue = true;
// [ts] error("`Infinity` is an invalid value for the `%s` css style property.", name);
// [ts] };
// [ts] warnValidStyle = (name, value) => {
// [ts] if (name.indexOf("-") > -1) {
// [ts] warnHyphenatedStyleName(name);
// [ts] } else if (badVendoredStyleNamePattern.test(name)) {
// [ts] warnBadVendoredStyleName(name);
// [ts] } else if (badStyleValueWithSemicolonPattern.test(value)) {
// [ts] warnStyleValueWithSemicolon(name, value);
// [ts] }
// [ts] if (@typeName(value) == "number") {
// [ts] if (isNaN(value)) {
// [ts] warnStyleValueIsNaN(name, value);
// [ts] } else if (!isFinite(value)) {
// [ts] warnStyleValueIsInfinity(name, value);
// [ts] }
// [ts] }
// [ts] };
// [ts] }
// [ts] let warnValidStyle$1 = warnValidStyle;
// [ts] /**
// [ts] * Operations for dealing with CSS properties.
// [ts] */
// [ts] /**
// [ts] * This creates a string that is expected to be equivalent to the style
// [ts] * attribute generated by server-side rendering. It by-passes warnings and
// [ts] * security checks so it's not safe to use this value for anything other than
// [ts] * comparison. It is only used in DEV for SSR validation.
// [ts] */
// [ts] fn createDangerousStringForStyles(styles: auto) {
// [ts] {
// [ts] let serialized = "";
// [ts] let delimiter = "";
// [ts] for (let styleName in styles) {
// [ts] if (!styles.hasOwnProperty(styleName)) {
// [ts] continue;
// [ts] }
// [ts] let styleValue = styles[styleName];
// [ts] if (styleValue != null) {
// [ts] let isCustomProperty = styleName.indexOf("--") == 0;
// [ts] serialized += delimiter + (if (isCustomProperty) styleName else hyphenateStyleName(styleName)) + ":";
// [ts] serialized += dangerousStyleValue(styleName, styleValue, isCustomProperty);
// [ts] delimiter = ";";
// [ts] }
// [ts] }
// [ts] return serialized || null;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Sets the value for multiple styles on a node.  If a value is specified as
// [ts] * '' (empty string), the corresponding style property will be unset.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {object} styles
// [ts] */
// [ts] fn setValueForStyles(node: auto, styles: auto) {
// [ts] let style = node.style;
// [ts] for (let styleName in styles) {
// [ts] if (!styles.hasOwnProperty(styleName)) {
// [ts] continue;
// [ts] }
// [ts] let isCustomProperty = styleName.indexOf("--") == 0;
// [ts] {
// [ts] if (!isCustomProperty) {
// [ts] warnValidStyle$1(styleName, styles[styleName]);
// [ts] }
// [ts] }
// [ts] let styleValue = dangerousStyleValue(styleName, styles[styleName], isCustomProperty);
// [ts] if (styleName == "float") {
// [ts] styleName = "cssFloat";
// [ts] }
// [ts] if (isCustomProperty) {
// [ts] style.setProperty(styleName, styleValue);
// [ts] } else {
// [ts] style[styleName] = styleValue;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn isValueEmpty(value: auto) {
// [ts] return value == null || @typeName(value) == "boolean" || value == "";
// [ts] }
// [ts] /**
// [ts] * Given {color: 'red', overflow: 'hidden'} returns {
// [ts] *   color: 'color',
// [ts] *   overflowX: 'overflow',
// [ts] *   overflowY: 'overflow',
// [ts] * }. This can be read as "the overflowY property was set by the overflow
// [ts] * shorthand". That is, the values are the property that each was derived from.
// [ts] */
// [ts] fn expandShorthandMap(styles: auto) {
// [ts] let expanded = {};
// [ts] for (let key in styles) {
// [ts] let longhands = shorthandToLonghand[key] || [key];
// [ts] let i = 0;
// [ts] while (i < longhands.length) { // [cap-migrate] for-step: i++
// [ts] expanded[longhands[i]] = key;
// [ts] }
// [ts] }
// [ts] return expanded;
// [ts] }
// [ts] /**
// [ts] * When mixing shorthand and longhand property names, we warn during updates if
// [ts] * we expect an incorrect result to occur. In particular, we warn for:
// [ts] *
// [ts] * Updating a shorthand property (longhand gets overwritten):
// [ts] *   {font: 'foo', fontVariant: 'bar'} -> {font: 'baz', fontVariant: 'bar'}
// [ts] *   becomes .style.font = 'baz'
// [ts] * Removing a shorthand property (longhand gets lost too):
// [ts] *   {font: 'foo', fontVariant: 'bar'} -> {fontVariant: 'bar'}
// [ts] *   becomes .style.font = ''
// [ts] * Removing a longhand property (should revert to shorthand; doesn't):
// [ts] *   {font: 'foo', fontVariant: 'bar'} -> {font: 'foo'}
// [ts] *   becomes .style.fontVariant = ''
// [ts] */
// [ts] fn validateShorthandPropertyCollisionInDev(styleUpdates: auto, nextStyles: auto) {
// [ts] {
// [ts] if (!nextStyles) {
// [ts] return;
// [ts] }
// [ts] let expandedUpdates = expandShorthandMap(styleUpdates);
// [ts] let expandedStyles = expandShorthandMap(nextStyles);
// [ts] let warnedAbout = {};
// [ts] for (let key in expandedUpdates) {
// [ts] let originalKey = expandedUpdates[key];
// [ts] let correctOriginalKey = expandedStyles[key];
// [ts] if (correctOriginalKey && originalKey != correctOriginalKey) {
// [ts] let warningKey = originalKey + "," + correctOriginalKey;
// [ts] if (warnedAbout[warningKey]) {
// [ts] continue;
// [ts] }
// [ts] warnedAbout[warningKey] = true;
// [ts] error(if ("%s a style property during rerender (%s) when a " + "conflicting property is set (%s) can lead to styling bugs. To " + "avoid this, don't mix shorthand and non-shorthand properties " + "for the same value; instead, replace the shorthand with " + "separate values.", isValueEmpty(styleUpdates[originalKey])) "Removing" else "Updating", originalKey, correctOriginalKey);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// For HTML, certain tags should omit their close tag. We keep a whitelist for
// those special-case tags.
// [ts] let omittedCloseTags = {
// [ts] area: true,
// [ts] base: true,
// [ts] br: true,
// [ts] col: true,
// [ts] embed: true,
// [ts] hr: true,
// [ts] img: true,
// [ts] input: true,
// [ts] keygen: true,
// [ts] link: true,
// [ts] meta: true,
// [ts] param: true,
// [ts] source: true,
// [ts] track: true,
// [ts] wbr: true // NOTE: menuitem's close tag should be omitted, but that causes problems.
// [ts] };
// `omittedCloseTags` except that `menuitem` should still have its closing tag.
// [ts] let voidElementTags = _assign({
// [ts] menuitem: true
// [ts] }, omittedCloseTags);
// [ts] let HTML = "__html";
// [ts] let ReactDebugCurrentFrame$3 = null;
// [ts] {
// [ts] ReactDebugCurrentFrame$3 = ReactSharedInternals.ReactDebugCurrentFrame;
// [ts] }
// [ts] fn assertValidProps(tag: auto, props: auto) {
// [ts] if (!props) {
// [ts] return;
// [ts] } // Note the use of `==` which checks for null or undefined.
// [ts] if (voidElementTags[tag]) {
// [ts] if (!(props.children == null && props.dangerouslySetInnerHTML == null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( tag + " is a void element tag and must neither have `children` nor use `dangerouslySetInnerHTML`." + ( ReactDebugCurrentFrame$3.getStackAddendum() ) );)
// [ts] }
// [ts] }
// [ts] }
// [ts] if (props.dangerouslySetInnerHTML != null) {
// [ts] if (!(props.children == null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Can only set one of `children` or `props.dangerouslySetInnerHTML`." );)
// [ts] }
// [ts] }
// [ts] if (!(@typeName(props.dangerouslySetInnerHTML) == "object" && HTML == props.dangerouslySetInnerHTML)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "`props.dangerouslySetInnerHTML` must be in the form `{__html: ...}`. Please visit https://fb.me/react-invariant-dangerously-set-inner-html for more information." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] if (!props.suppressContentEditableWarning && props.contentEditable && props.children != null) {
// [ts] error("A component is `contentEditable` and contains `children` managed by " + "React. It is now your responsibility to guarantee that none of " + "those nodes are unexpectedly modified or duplicated. This is " + "probably not intentional.");
// [ts] }
// [ts] }
// [ts] if (!(props.style == null || @typeName(props.style) == "object")) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "The `style` prop expects a mapping from style properties to values, not a string. For example, style={{marginRight: spacing + 'em'}} when using JSX." + ( ReactDebugCurrentFrame$3.getStackAddendum() ) );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn isCustomComponent(tagName: auto, props: auto) {
// [ts] if (tagName.indexOf("-") == -1) {
// [ts] return @typeName(props.is) == "string";
// [ts] }
// [ts] switch (tagName) {
// These are reserved SVG and MathML elements.
// We don't mind this whitelist too much because we expect it to never grow.
// The alternative is to track the namespace in a few places which is convoluted.
// https://w3c.github.io/webcomponents/spec/custom/#custom-elements-core-concepts
// [ts] "annotation-xml", "color-profile", "font-face", "font-face-src", "font-face-uri", "font-face-format", "font-face-name", "missing-glyph" => {
// [ts] return false;
// [ts] },
// [ts] else => {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// When adding attributes to the HTML or SVG whitelist, be sure to
// also add them to this module to ensure casing and incorrect name
// warnings.
// [ts] let possibleStandardNames = {
// HTML
// [ts] accept: "accept",
// [ts] acceptcharset: "acceptCharset",
// [ts] "accept-charset": "acceptCharset",
// [ts] accesskey: "accessKey",
// [ts] action: "action",
// [ts] allowfullscreen: "allowFullScreen",
// [ts] alt: "alt",
// [ts] as: "as",
// [ts] async: "async",
// [ts] autocapitalize: "autoCapitalize",
// [ts] autocomplete: "autoComplete",
// [ts] autocorrect: "autoCorrect",
// [ts] autofocus: "autoFocus",
// [ts] autoplay: "autoPlay",
// [ts] autosave: "autoSave",
// [ts] capture: "capture",
// [ts] cellpadding: "cellPadding",
// [ts] cellspacing: "cellSpacing",
// [ts] challenge: "challenge",
// [ts] charset: "charSet",
// [ts] checked: "checked",
// [ts] children: "children",
// [ts] cite: "cite",
// [ts] class: "className",
// [ts] classid: "classID",
// [ts] classname: "className",
// [ts] cols: "cols",
// [ts] colspan: "colSpan",
// [ts] content: "content",
// [ts] contenteditable: "contentEditable",
// [ts] contextmenu: "contextMenu",
// [ts] controls: "controls",
// [ts] controlslist: "controlsList",
// [ts] coords: "coords",
// [ts] crossorigin: "crossOrigin",
// [ts] dangerouslysetinnerhtml: "dangerouslySetInnerHTML",
// [ts] data: "data",
// [ts] datetime: "dateTime",
// [ts] default: "default",
// [ts] defaultchecked: "defaultChecked",
// [ts] defaultvalue: "defaultValue",
// [ts] defer: "defer",
// [ts] dir: "dir",
// [ts] disabled: "disabled",
// [ts] disablepictureinpicture: "disablePictureInPicture",
// [ts] download: "download",
// [ts] draggable: "draggable",
// [ts] enctype: "encType",
// [ts] for: "htmlFor",
// [ts] form: "form",
// [ts] formmethod: "formMethod",
// [ts] formaction: "formAction",
// [ts] formenctype: "formEncType",
// [ts] formnovalidate: "formNoValidate",
// [ts] formtarget: "formTarget",
// [ts] frameborder: "frameBorder",
// [ts] headers: "headers",
// [ts] height: "height",
// [ts] hidden: "hidden",
// [ts] high: "high",
// [ts] href: "href",
// [ts] hreflang: "hrefLang",
// [ts] htmlfor: "htmlFor",
// [ts] httpequiv: "httpEquiv",
// [ts] "http-equiv": "httpEquiv",
// [ts] icon: "icon",
// [ts] id: "id",
// [ts] innerhtml: "innerHTML",
// [ts] inputmode: "inputMode",
// [ts] integrity: "integrity",
// [ts] is: "is",
// [ts] itemid: "itemID",
// [ts] itemprop: "itemProp",
// [ts] itemref: "itemRef",
// [ts] itemscope: "itemScope",
// [ts] itemtype: "itemType",
// [ts] keyparams: "keyParams",
// [ts] keytype: "keyType",
// [ts] kind: "kind",
// [ts] label: "label",
// [ts] lang: "lang",
// [ts] list: "list",
// [ts] loop: "loop",
// [ts] low: "low",
// [ts] manifest: "manifest",
// [ts] marginwidth: "marginWidth",
// [ts] marginheight: "marginHeight",
// [ts] max: "max",
// [ts] maxlength: "maxLength",
// [ts] media: "media",
// [ts] mediagroup: "mediaGroup",
// [ts] method: "method",
// [ts] min: "min",
// [ts] minlength: "minLength",
// [ts] multiple: "multiple",
// [ts] muted: "muted",
// [ts] name: "name",
// [ts] nomodule: "noModule",
// [ts] nonce: "nonce",
// [ts] novalidate: "noValidate",
// [ts] open: "open",
// [ts] optimum: "optimum",
// [ts] pattern: "pattern",
// [ts] placeholder: "placeholder",
// [ts] playsinline: "playsInline",
// [ts] poster: "poster",
// [ts] preload: "preload",
// [ts] profile: "profile",
// [ts] radiogroup: "radioGroup",
// [ts] readonly: "readOnly",
// [ts] referrerpolicy: "referrerPolicy",
// [ts] rel: "rel",
// [ts] required: "required",
// [ts] reversed: "reversed",
// [ts] role: "role",
// [ts] rows: "rows",
// [ts] rowspan: "rowSpan",
// [ts] sandbox: "sandbox",
// [ts] scope: "scope",
// [ts] scoped: "scoped",
// [ts] scrolling: "scrolling",
// [ts] seamless: "seamless",
// [ts] selected: "selected",
// [ts] shape: "shape",
// [ts] size: "size",
// [ts] sizes: "sizes",
// [ts] span: "span",
// [ts] spellcheck: "spellCheck",
// [ts] src: "src",
// [ts] srcdoc: "srcDoc",
// [ts] srclang: "srcLang",
// [ts] srcset: "srcSet",
// [ts] start: "start",
// [ts] step: "step",
// [ts] style: "style",
// [ts] summary: "summary",
// [ts] tabindex: "tabIndex",
// [ts] target: "target",
// [ts] title: "title",
// [ts] type: "type",
// [ts] usemap: "useMap",
// [ts] value: "value",
// [ts] width: "width",
// [ts] wmode: "wmode",
// [ts] wrap: "wrap",
// SVG
// [ts] about: "about",
// [ts] accentheight: "accentHeight",
// [ts] "accent-height": "accentHeight",
// [ts] accumulate: "accumulate",
// [ts] additive: "additive",
// [ts] alignmentbaseline: "alignmentBaseline",
// [ts] "alignment-baseline": "alignmentBaseline",
// [ts] allowreorder: "allowReorder",
// [ts] alphabetic: "alphabetic",
// [ts] amplitude: "amplitude",
// [ts] arabicform: "arabicForm",
// [ts] "arabic-form": "arabicForm",
// [ts] ascent: "ascent",
// [ts] attributename: "attributeName",
// [ts] attributetype: "attributeType",
// [ts] autoreverse: "autoReverse",
// [ts] azimuth: "azimuth",
// [ts] basefrequency: "baseFrequency",
// [ts] baselineshift: "baselineShift",
// [ts] "baseline-shift": "baselineShift",
// [ts] baseprofile: "baseProfile",
// [ts] bbox: "bbox",
// [ts] begin: "begin",
// [ts] bias: "bias",
// [ts] by: "by",
// [ts] calcmode: "calcMode",
// [ts] capheight: "capHeight",
// [ts] "cap-height": "capHeight",
// [ts] clip: "clip",
// [ts] clippath: "clipPath",
// [ts] "clip-path": "clipPath",
// [ts] clippathunits: "clipPathUnits",
// [ts] cliprule: "clipRule",
// [ts] "clip-rule": "clipRule",
// [ts] color: "color",
// [ts] colorinterpolation: "colorInterpolation",
// [ts] "color-interpolation": "colorInterpolation",
// [ts] colorinterpolationfilters: "colorInterpolationFilters",
// [ts] "color-interpolation-filters": "colorInterpolationFilters",
// [ts] colorprofile: "colorProfile",
// [ts] "color-profile": "colorProfile",
// [ts] colorrendering: "colorRendering",
// [ts] "color-rendering": "colorRendering",
// [ts] contentscripttype: "contentScriptType",
// [ts] contentstyletype: "contentStyleType",
// [ts] cursor: "cursor",
// [ts] cx: "cx",
// [ts] cy: "cy",
// [ts] d: "d",
// [ts] datatype: "datatype",
// [ts] decelerate: "decelerate",
// [ts] descent: "descent",
// [ts] diffuseconstant: "diffuseConstant",
// [ts] direction: "direction",
// [ts] display: "display",
// [ts] divisor: "divisor",
// [ts] dominantbaseline: "dominantBaseline",
// [ts] "dominant-baseline": "dominantBaseline",
// [ts] dur: "dur",
// [ts] dx: "dx",
// [ts] dy: "dy",
// [ts] edgemode: "edgeMode",
// [ts] elevation: "elevation",
// [ts] enablebackground: "enableBackground",
// [ts] "enable-background": "enableBackground",
// [ts] end: "end",
// [ts] exponent: "exponent",
// [ts] externalresourcesrequired: "externalResourcesRequired",
// [ts] fill: "fill",
// [ts] fillopacity: "fillOpacity",
// [ts] "fill-opacity": "fillOpacity",
// [ts] fillrule: "fillRule",
// [ts] "fill-rule": "fillRule",
// [ts] filter: "filter",
// [ts] filterres: "filterRes",
// [ts] filterunits: "filterUnits",
// [ts] floodopacity: "floodOpacity",
// [ts] "flood-opacity": "floodOpacity",
// [ts] floodcolor: "floodColor",
// [ts] "flood-color": "floodColor",
// [ts] focusable: "focusable",
// [ts] fontfamily: "fontFamily",
// [ts] "font-family": "fontFamily",
// [ts] fontsize: "fontSize",
// [ts] "font-size": "fontSize",
// [ts] fontsizeadjust: "fontSizeAdjust",
// [ts] "font-size-adjust": "fontSizeAdjust",
// [ts] fontstretch: "fontStretch",
// [ts] "font-stretch": "fontStretch",
// [ts] fontstyle: "fontStyle",
// [ts] "font-style": "fontStyle",
// [ts] fontvariant: "fontVariant",
// [ts] "font-variant": "fontVariant",
// [ts] fontweight: "fontWeight",
// [ts] "font-weight": "fontWeight",
// [ts] format: "format",
// [ts] from: "from",
// [ts] fx: "fx",
// [ts] fy: "fy",
// [ts] g1: "g1",
// [ts] g2: "g2",
// [ts] glyphname: "glyphName",
// [ts] "glyph-name": "glyphName",
// [ts] glyphorientationhorizontal: "glyphOrientationHorizontal",
// [ts] "glyph-orientation-horizontal": "glyphOrientationHorizontal",
// [ts] glyphorientationvertical: "glyphOrientationVertical",
// [ts] "glyph-orientation-vertical": "glyphOrientationVertical",
// [ts] glyphref: "glyphRef",
// [ts] gradienttransform: "gradientTransform",
// [ts] gradientunits: "gradientUnits",
// [ts] hanging: "hanging",
// [ts] horizadvx: "horizAdvX",
// [ts] "horiz-adv-x": "horizAdvX",
// [ts] horizoriginx: "horizOriginX",
// [ts] "horiz-origin-x": "horizOriginX",
// [ts] ideographic: "ideographic",
// [ts] imagerendering: "imageRendering",
// [ts] "image-rendering": "imageRendering",
// [ts] in2: "in2",
// [ts] in: "in",
// [ts] inlist: "inlist",
// [ts] intercept: "intercept",
// [ts] k1: "k1",
// [ts] k2: "k2",
// [ts] k3: "k3",
// [ts] k4: "k4",
// [ts] k: "k",
// [ts] kernelmatrix: "kernelMatrix",
// [ts] kernelunitlength: "kernelUnitLength",
// [ts] kerning: "kerning",
// [ts] keypoints: "keyPoints",
// [ts] keysplines: "keySplines",
// [ts] keytimes: "keyTimes",
// [ts] lengthadjust: "lengthAdjust",
// [ts] letterspacing: "letterSpacing",
// [ts] "letter-spacing": "letterSpacing",
// [ts] lightingcolor: "lightingColor",
// [ts] "lighting-color": "lightingColor",
// [ts] limitingconeangle: "limitingConeAngle",
// [ts] local: "local",
// [ts] markerend: "markerEnd",
// [ts] "marker-end": "markerEnd",
// [ts] markerheight: "markerHeight",
// [ts] markermid: "markerMid",
// [ts] "marker-mid": "markerMid",
// [ts] markerstart: "markerStart",
// [ts] "marker-start": "markerStart",
// [ts] markerunits: "markerUnits",
// [ts] markerwidth: "markerWidth",
// [ts] mask: "mask",
// [ts] maskcontentunits: "maskContentUnits",
// [ts] maskunits: "maskUnits",
// [ts] mathematical: "mathematical",
// [ts] mode: "mode",
// [ts] numoctaves: "numOctaves",
// [ts] offset: "offset",
// [ts] opacity: "opacity",
// [ts] operator: "operator",
// [ts] order: "order",
// [ts] orient: "orient",
// [ts] orientation: "orientation",
// [ts] origin: "origin",
// [ts] overflow: "overflow",
// [ts] overlineposition: "overlinePosition",
// [ts] "overline-position": "overlinePosition",
// [ts] overlinethickness: "overlineThickness",
// [ts] "overline-thickness": "overlineThickness",
// [ts] paintorder: "paintOrder",
// [ts] "paint-order": "paintOrder",
// [ts] panose1: "panose1",
// [ts] "panose-1": "panose1",
// [ts] pathlength: "pathLength",
// [ts] patterncontentunits: "patternContentUnits",
// [ts] patterntransform: "patternTransform",
// [ts] patternunits: "patternUnits",
// [ts] pointerevents: "pointerEvents",
// [ts] "pointer-events": "pointerEvents",
// [ts] points: "points",
// [ts] pointsatx: "pointsAtX",
// [ts] pointsaty: "pointsAtY",
// [ts] pointsatz: "pointsAtZ",
// [ts] prefix: "prefix",
// [ts] preservealpha: "preserveAlpha",
// [ts] preserveaspectratio: "preserveAspectRatio",
// [ts] primitiveunits: "primitiveUnits",
// [ts] property: "property",
// [ts] r: "r",
// [ts] radius: "radius",
// [ts] refx: "refX",
// [ts] refy: "refY",
// [ts] renderingintent: "renderingIntent",
// [ts] "rendering-intent": "renderingIntent",
// [ts] repeatcount: "repeatCount",
// [ts] repeatdur: "repeatDur",
// [ts] requiredextensions: "requiredExtensions",
// [ts] requiredfeatures: "requiredFeatures",
// [ts] resource: "resource",
// [ts] restart: "restart",
// [ts] result: "result",
// [ts] results: "results",
// [ts] rotate: "rotate",
// [ts] rx: "rx",
// [ts] ry: "ry",
// [ts] scale: "scale",
// [ts] security: "security",
// [ts] seed: "seed",
// [ts] shaperendering: "shapeRendering",
// [ts] "shape-rendering": "shapeRendering",
// [ts] slope: "slope",
// [ts] spacing: "spacing",
// [ts] specularconstant: "specularConstant",
// [ts] specularexponent: "specularExponent",
// [ts] speed: "speed",
// [ts] spreadmethod: "spreadMethod",
// [ts] startoffset: "startOffset",
// [ts] stddeviation: "stdDeviation",
// [ts] stemh: "stemh",
// [ts] stemv: "stemv",
// [ts] stitchtiles: "stitchTiles",
// [ts] stopcolor: "stopColor",
// [ts] "stop-color": "stopColor",
// [ts] stopopacity: "stopOpacity",
// [ts] "stop-opacity": "stopOpacity",
// [ts] strikethroughposition: "strikethroughPosition",
// [ts] "strikethrough-position": "strikethroughPosition",
// [ts] strikethroughthickness: "strikethroughThickness",
// [ts] "strikethrough-thickness": "strikethroughThickness",
// [ts] string: "string",
// [ts] stroke: "stroke",
// [ts] strokedasharray: "strokeDasharray",
// [ts] "stroke-dasharray": "strokeDasharray",
// [ts] strokedashoffset: "strokeDashoffset",
// [ts] "stroke-dashoffset": "strokeDashoffset",
// [ts] strokelinecap: "strokeLinecap",
// [ts] "stroke-linecap": "strokeLinecap",
// [ts] strokelinejoin: "strokeLinejoin",
// [ts] "stroke-linejoin": "strokeLinejoin",
// [ts] strokemiterlimit: "strokeMiterlimit",
// [ts] "stroke-miterlimit": "strokeMiterlimit",
// [ts] strokewidth: "strokeWidth",
// [ts] "stroke-width": "strokeWidth",
// [ts] strokeopacity: "strokeOpacity",
// [ts] "stroke-opacity": "strokeOpacity",
// [ts] suppresscontenteditablewarning: "suppressContentEditableWarning",
// [ts] suppresshydrationwarning: "suppressHydrationWarning",
// [ts] surfacescale: "surfaceScale",
// [ts] systemlanguage: "systemLanguage",
// [ts] tablevalues: "tableValues",
// [ts] targetx: "targetX",
// [ts] targety: "targetY",
// [ts] textanchor: "textAnchor",
// [ts] "text-anchor": "textAnchor",
// [ts] textdecoration: "textDecoration",
// [ts] "text-decoration": "textDecoration",
// [ts] textlength: "textLength",
// [ts] textrendering: "textRendering",
// [ts] "text-rendering": "textRendering",
// [ts] to: "to",
// [ts] transform: "transform",
// [ts] typeof: "typeof",
// [ts] u1: "u1",
// [ts] u2: "u2",
// [ts] underlineposition: "underlinePosition",
// [ts] "underline-position": "underlinePosition",
// [ts] underlinethickness: "underlineThickness",
// [ts] "underline-thickness": "underlineThickness",
// [ts] unicode: "unicode",
// [ts] unicodebidi: "unicodeBidi",
// [ts] "unicode-bidi": "unicodeBidi",
// [ts] unicoderange: "unicodeRange",
// [ts] "unicode-range": "unicodeRange",
// [ts] unitsperem: "unitsPerEm",
// [ts] "units-per-em": "unitsPerEm",
// [ts] unselectable: "unselectable",
// [ts] valphabetic: "vAlphabetic",
// [ts] "v-alphabetic": "vAlphabetic",
// [ts] values: "values",
// [ts] vectoreffect: "vectorEffect",
// [ts] "vector-effect": "vectorEffect",
// [ts] version: "version",
// [ts] vertadvy: "vertAdvY",
// [ts] "vert-adv-y": "vertAdvY",
// [ts] vertoriginx: "vertOriginX",
// [ts] "vert-origin-x": "vertOriginX",
// [ts] vertoriginy: "vertOriginY",
// [ts] "vert-origin-y": "vertOriginY",
// [ts] vhanging: "vHanging",
// [ts] "v-hanging": "vHanging",
// [ts] videographic: "vIdeographic",
// [ts] "v-ideographic": "vIdeographic",
// [ts] viewbox: "viewBox",
// [ts] viewtarget: "viewTarget",
// [ts] visibility: "visibility",
// [ts] vmathematical: "vMathematical",
// [ts] "v-mathematical": "vMathematical",
// [ts] vocab: "vocab",
// [ts] widths: "widths",
// [ts] wordspacing: "wordSpacing",
// [ts] "word-spacing": "wordSpacing",
// [ts] writingmode: "writingMode",
// [ts] "writing-mode": "writingMode",
// [ts] x1: "x1",
// [ts] x2: "x2",
// [ts] x: "x",
// [ts] xchannelselector: "xChannelSelector",
// [ts] xheight: "xHeight",
// [ts] "x-height": "xHeight",
// [ts] xlinkactuate: "xlinkActuate",
// [ts] "xlink:actuate": "xlinkActuate",
// [ts] xlinkarcrole: "xlinkArcrole",
// [ts] "xlink:arcrole": "xlinkArcrole",
// [ts] xlinkhref: "xlinkHref",
// [ts] "xlink:href": "xlinkHref",
// [ts] xlinkrole: "xlinkRole",
// [ts] "xlink:role": "xlinkRole",
// [ts] xlinkshow: "xlinkShow",
// [ts] "xlink:show": "xlinkShow",
// [ts] xlinktitle: "xlinkTitle",
// [ts] "xlink:title": "xlinkTitle",
// [ts] xlinktype: "xlinkType",
// [ts] "xlink:type": "xlinkType",
// [ts] xmlbase: "xmlBase",
// [ts] "xml:base": "xmlBase",
// [ts] xmllang: "xmlLang",
// [ts] "xml:lang": "xmlLang",
// [ts] xmlns: "xmlns",
// [ts] "xml:space": "xmlSpace",
// [ts] xmlnsxlink: "xmlnsXlink",
// [ts] "xmlns:xlink": "xmlnsXlink",
// [ts] xmlspace: "xmlSpace",
// [ts] y1: "y1",
// [ts] y2: "y2",
// [ts] y: "y",
// [ts] ychannelselector: "yChannelSelector",
// [ts] z: "z",
// [ts] zoomandpan: "zoomAndPan"
// [ts] };
// [ts] let ariaProperties = {
// [ts] "aria-current": 0,
// state
// [ts] "aria-details": 0,
// [ts] "aria-disabled": 0,
// state
// [ts] "aria-hidden": 0,
// state
// [ts] "aria-invalid": 0,
// state
// [ts] "aria-keyshortcuts": 0,
// [ts] "aria-label": 0,
// [ts] "aria-roledescription": 0,
// Widget Attributes
// [ts] "aria-autocomplete": 0,
// [ts] "aria-checked": 0,
// [ts] "aria-expanded": 0,
// [ts] "aria-haspopup": 0,
// [ts] "aria-level": 0,
// [ts] "aria-modal": 0,
// [ts] "aria-multiline": 0,
// [ts] "aria-multiselectable": 0,
// [ts] "aria-orientation": 0,
// [ts] "aria-placeholder": 0,
// [ts] "aria-pressed": 0,
// [ts] "aria-readonly": 0,
// [ts] "aria-required": 0,
// [ts] "aria-selected": 0,
// [ts] "aria-sort": 0,
// [ts] "aria-valuemax": 0,
// [ts] "aria-valuemin": 0,
// [ts] "aria-valuenow": 0,
// [ts] "aria-valuetext": 0,
// Live Region Attributes
// [ts] "aria-atomic": 0,
// [ts] "aria-busy": 0,
// [ts] "aria-live": 0,
// [ts] "aria-relevant": 0,
// Drag-and-Drop Attributes
// [ts] "aria-dropeffect": 0,
// [ts] "aria-grabbed": 0,
// Relationship Attributes
// [ts] "aria-activedescendant": 0,
// [ts] "aria-colcount": 0,
// [ts] "aria-colindex": 0,
// [ts] "aria-colspan": 0,
// [ts] "aria-controls": 0,
// [ts] "aria-describedby": 0,
// [ts] "aria-errormessage": 0,
// [ts] "aria-flowto": 0,
// [ts] "aria-labelledby": 0,
// [ts] "aria-owns": 0,
// [ts] "aria-posinset": 0,
// [ts] "aria-rowcount": 0,
// [ts] "aria-rowindex": 0,
// [ts] "aria-rowspan": 0,
// [ts] "aria-setsize": 0
// [ts] };
// [ts] let warnedProperties = {};
// [ts] let rARIA = RegExp.init("^(aria)-[" + ATTRIBUTE_NAME_CHAR + "]*$");
// [ts] let rARIACamel = RegExp.init("^(aria)[A-Z][" + ATTRIBUTE_NAME_CHAR + "]*$");
// [ts] let hasOwnProperty$1 = Object.prototype.hasOwnProperty;
// [ts] fn validateProperty(tagName: auto, name: auto) {
// [ts] {
// [ts] if (hasOwnProperty$1.call(warnedProperties, name) && warnedProperties[name]) {
// [ts] return true;
// [ts] }
// [ts] if (rARIACamel.test(name)) {
// [ts] let ariaName = "aria-" + name.slice(4).toLowerCase();
// [ts] let correctName = if (ariaProperties.hasOwnProperty(ariaName)) ariaName else null; // If this is an aria-* attribute, but is not listed in the known DOM
// DOM properties, then it is an invalid aria-* attribute.
// [ts] if (correctName == null) {
// [ts] error("Invalid ARIA attribute `%s`. ARIA attributes follow the pattern aria-* and must be lowercase.", name);
// [ts] warnedProperties[name] = true;
// [ts] return true;
// [ts] } // aria-* attributes should be lowercase; suggest the lowercase version.
// [ts] if (name != correctName) {
// [ts] error("Invalid ARIA attribute `%s`. Did you mean `%s`?", name, correctName);
// [ts] warnedProperties[name] = true;
// [ts] return true;
// [ts] }
// [ts] }
// [ts] if (rARIA.test(name)) {
// [ts] let lowerCasedName = name.toLowerCase();
// [ts] let standardName = if (ariaProperties.hasOwnProperty(lowerCasedName)) lowerCasedName else null; // If this is an aria-* attribute, but is not listed in the known DOM
// DOM properties, then it is an invalid aria-* attribute.
// [ts] if (standardName == null) {
// [ts] warnedProperties[name] = true;
// [ts] return false;
// [ts] } // aria-* attributes should be lowercase; suggest the lowercase version.
// [ts] if (name != standardName) {
// [ts] error("Unknown ARIA attribute `%s`. Did you mean `%s`?", name, standardName);
// [ts] warnedProperties[name] = true;
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn warnInvalidARIAProps(type: auto, props: auto) {
// [ts] {
// [ts] let invalidProps = [];
// [ts] for (let key in props) {
// [ts] let isValid = validateProperty(type, key);
// [ts] if (!isValid) {
// [ts] invalidProps.push(key);
// [ts] }
// [ts] }
// [ts] let unknownPropString = invalidProps.map((prop) => {
// [ts] return "`" + prop + "`";
// [ts] }).join(", ");
// [ts] if (invalidProps.length == 1) {
// [ts] error("Invalid aria prop %s on <%s> tag. " + "For details, see https://fb.me/invalid-aria-prop", unknownPropString, type);
// [ts] } else if (invalidProps.length > 1) {
// [ts] error("Invalid aria props %s on <%s> tag. " + "For details, see https://fb.me/invalid-aria-prop", unknownPropString, type);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn validateProperties(type: auto, props: auto) {
// [ts] if (isCustomComponent(type, props)) {
// [ts] return;
// [ts] }
// [ts] warnInvalidARIAProps(type, props);
// [ts] }
// [ts] let didWarnValueNull = false;
// [ts] (type, props) => {
// [ts] {
// [ts] if (type != "input" && type != "textarea" && type != "select") {
// [ts] return;
// [ts] }
// [ts] if (props != null && props.value == null && !didWarnValueNull) {
// [ts] didWarnValueNull = true;
// [ts] if (type == "select" && props.multiple) {
// [ts] error("`value` prop on `%s` should not be null. " + "Consider using an empty array when `multiple` is set to `true` " + "to clear the component or `undefined` for uncontrolled components.", type);
// [ts] } else {
// [ts] error("`value` prop on `%s` should not be null. " + "Consider using an empty string to clear the component or `undefined` " + "for uncontrolled components.", type);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let validateProperty$1 = () => {};
// [ts] {
// [ts] let warnedProperties$1 = {};
// [ts] let _hasOwnProperty = Object.prototype.hasOwnProperty;
// [ts] let EVENT_NAME_REGEX = "^on.";
// [ts] let INVALID_EVENT_NAME_REGEX = "^on[^A-Z]";
// [ts] let rARIA$1 = RegExp.init("^(aria)-[" + ATTRIBUTE_NAME_CHAR + "]*$");
// [ts] let rARIACamel$1 = RegExp.init("^(aria)[A-Z][" + ATTRIBUTE_NAME_CHAR + "]*$");
// [ts] validateProperty$1 = (tagName, name, value, canUseEventSystem) => {
// [ts] if (_hasOwnProperty.call(warnedProperties$1, name) && warnedProperties$1[name]) {
// [ts] return true;
// [ts] }
// [ts] let lowerCasedName = name.toLowerCase();
// [ts] if (lowerCasedName == "onfocusin" || lowerCasedName == "onfocusout") {
// [ts] error("React uses onFocus and onBlur instead of onFocusIn and onFocusOut. " + "All React events are normalized to bubble, so onFocusIn and onFocusOut " + "are not needed/supported by React.");
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] } // We can't rely on the event system being injected on the server.
// [ts] if (canUseEventSystem) {
// [ts] if (registrationNameModules.hasOwnProperty(name)) {
// [ts] return true;
// [ts] }
// [ts] let registrationName = if (possibleRegistrationNames.hasOwnProperty(lowerCasedName)) possibleRegistrationNames[lowerCasedName] else null;
// [ts] if (registrationName != null) {
// [ts] error("Invalid event handler property `%s`. Did you mean `%s`?", name, registrationName);
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] if (EVENT_NAME_REGEX.test(name)) {
// [ts] error("Unknown event handler property `%s`. It will be ignored.", name);
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] } else if (EVENT_NAME_REGEX.test(name)) {
// If no event plugins have been injected, we are in a server environment.
// So we can't tell if the event name is correct for sure, but we can filter
// out known bad ones like `onclick`. We can't suggest a specific replacement though.
// [ts] if (INVALID_EVENT_NAME_REGEX.test(name)) {
// [ts] error("Invalid event handler property `%s`. " + "React events use the camelCase naming convention, for example `onClick`.", name);
// [ts] }
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] } // Let the ARIA attribute hook validate ARIA attributes
// [ts] if (rARIA$1.test(name) || rARIACamel$1.test(name)) {
// [ts] return true;
// [ts] }
// [ts] if (lowerCasedName == "innerhtml") {
// [ts] error("Directly setting property `innerHTML` is not permitted. " + "For more information, lookup documentation on `dangerouslySetInnerHTML`.");
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] if (lowerCasedName == "aria") {
// [ts] error("The `aria` attribute is reserved for future use in React. " + "Pass individual `aria-` attributes instead.");
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] if (lowerCasedName == "is" && value != null && value != null && @typeName(value) != "string") {
// [ts] error("Received a `%s` for a string attribute `is`. If this is expected, cast " + "the value to a string.", @typeName(value));
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] if (@typeName(value) == "number" && isNaN(value)) {
// [ts] error("Received NaN for the `%s` attribute. If this is expected, cast " + "the value to a string.", name);
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] let propertyInfo = getPropertyInfo(name);
// [ts] let isReserved = propertyInfo != null && propertyInfo.type == RESERVED; // Known attributes should match the casing specified in the property config.
// [ts] if (possibleStandardNames.hasOwnProperty(lowerCasedName)) {
// [ts] let standardName = possibleStandardNames[lowerCasedName];
// [ts] if (standardName != name) {
// [ts] error("Invalid DOM property `%s`. Did you mean `%s`?", name, standardName);
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] } else if (!isReserved && name != lowerCasedName) {
// Unknown attributes should have lowercase casing since that's how they
// will be cased anyway with server rendering.
// [ts] error("React does not recognize the `%s` prop on a DOM element. If you " + "intentionally want it to appear in the DOM as a custom " + "attribute, spell it as lowercase `%s` instead. " + "If you accidentally passed it from a parent component, remove " + "it from the DOM element.", name, lowerCasedName);
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] if (@typeName(value) == "boolean" && shouldRemoveAttributeWithWarning(name, value, propertyInfo, false)) {
// [ts] if (value) {
// [ts] error("Received `%s` for a non-boolean attribute `%s`.\n\n" + "If you want to write it to the DOM, pass a string instead: " + "%s=\"%s\" or %s={value.toString()}.", value, name, name, value, name);
// [ts] } else {
// [ts] error("Received `%s` for a non-boolean attribute `%s`.\n\n" + "If you want to write it to the DOM, pass a string instead: " + "%s=\"%s\" or %s={value.toString()}.\n\n" + "If you used to conditionally omit it with %s={condition && value}, " + "pass %s={condition ? value : undefined} instead.", value, name, name, value, name, name, name);
// [ts] }
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] } // Now that we've validated casing, do not validate
// data types for reserved props
// [ts] if (isReserved) {
// [ts] return true;
// [ts] } // Warn when a known attribute is a bad type
// [ts] if (shouldRemoveAttributeWithWarning(name, value, propertyInfo, false)) {
// [ts] warnedProperties$1[name] = true;
// [ts] return false;
// [ts] } // Warn when passing the strings 'false' or 'true' into a boolean prop
// [ts] if ((value == "false" || value == "true") && propertyInfo != null && propertyInfo.type == BOOLEAN) {
// [ts] error(if ("Received the string `%s` for the boolean attribute `%s`. " + "%s " + "Did you mean %s={%s}?", value, name, value == "false") "The browser will interpret it as a truthy value." else "Although this works, it will not work as expected if you pass the string \"false\".", name, value);
// [ts] warnedProperties$1[name] = true;
// [ts] return true;
// [ts] }
// [ts] return true;
// [ts] };
// [ts] }
// [ts] let warnUnknownProperties = (type, props, canUseEventSystem) => {
// [ts] {
// [ts] let unknownProps = [];
// [ts] for (let key in props) {
// [ts] let isValid = validateProperty$1(type, key, props[key], canUseEventSystem);
// [ts] if (!isValid) {
// [ts] unknownProps.push(key);
// [ts] }
// [ts] }
// [ts] let unknownPropString = unknownProps.map((prop) => {
// [ts] return "`" + prop + "`";
// [ts] }).join(", ");
// [ts] if (unknownProps.length == 1) {
// [ts] error("Invalid value for prop %s on <%s> tag. Either remove it from the element, " + "or pass a string or number value to keep it in the DOM. " + "For details, see https://fb.me/react-attribute-behavior", unknownPropString, type);
// [ts] } else if (unknownProps.length > 1) {
// [ts] error("Invalid values for props %s on <%s> tag. Either remove them from the element, " + "or pass a string or number value to keep them in the DOM. " + "For details, see https://fb.me/react-attribute-behavior", unknownPropString, type);
// [ts] }
// [ts] }
// [ts] };
// [ts] (type, props, canUseEventSystem) => {
// [ts] if (isCustomComponent(type, props)) {
// [ts] return;
// [ts] }
// [ts] warnUnknownProperties(type, props, canUseEventSystem);
// [ts] }
// [ts] let didWarnInvalidHydration = false;
// [ts] let DANGEROUSLY_SET_INNER_HTML = "dangerouslySetInnerHTML";
// [ts] let SUPPRESS_CONTENT_EDITABLE_WARNING = "suppressContentEditableWarning";
// [ts] let SUPPRESS_HYDRATION_WARNING = "suppressHydrationWarning";
// [ts] let AUTOFOCUS = "autoFocus";
// [ts] let CHILDREN = "children";
// [ts] let STYLE = "style";
// [ts] let HTML$1 = "__html";
// [ts] let HTML_NAMESPACE$1 = Namespaces.html;
// [ts] let warnedUnknownTags;
// [ts] let suppressHydrationWarning;
// [ts] let validatePropertiesInDevelopment;
// [ts] let warnForTextDifference;
// [ts] let warnForPropDifference;
// [ts] let warnForExtraAttributes;
// [ts] let warnForInvalidEventListener;
// [ts] let canDiffStyleForHydrationWarning;
// [ts] let normalizeMarkupForTextOrAttribute;
// [ts] let normalizeHTML;
// [ts] {
// [ts] warnedUnknownTags = {
// Chrome is the only major browser not shipping <time>. But as of July
// 2017 it intends to ship it due to widespread usage. We intentionally
// *don't* warn for <time> even if it's unrecognized by Chrome because
// it soon will be, and many apps have been using it anyway.
// [ts] time: true,
// There are working polyfills for <dialog>. Let people use it.
// [ts] dialog: true,
// Electron ships a custom <webview> tag to display external web content in
// an isolated frame and process.
// This tag is not present in non Electron environments such as JSDom which
// is often used for testing purposes.
// @see https://electronjs.org/docs/api/webview-tag
// [ts] webview: true
// [ts] };
// [ts] validatePropertiesInDevelopment = (type, props) => {
// [ts] validateProperties(type, props);
// [ts] validateProperties$1(type, props);
// [ts] validateProperties$2(type, props,
// [ts] /* canUseEventSystem */
// [ts] true);
// [ts] }; // IE 11 parses & normalizes the style attribute as opposed to other
// browsers. It adds spaces and sorts the properties in some
// non-alphabetical order. Handling that would require sorting CSS
// properties in the client & server versions or applying
// `expectedStyle` to a temporary DOM node to read its `style` attribute
// normalized. Since it only affects IE, we're skipping style warnings
// in that browser completely in favor of doing all that work.
// See https://github.com/facebook/react/issues/11807
// [ts] canDiffStyleForHydrationWarning = canUseDOM && !document.documentMode; // HTML parsing normalizes CR and CRLF to LF.
// It also can turn \u0000 into \uFFFD inside attributes.
// https://www.w3.org/TR/html5/single-page.html#preprocessing-the-input-stream
// If we have a mismatch, it might be caused by that.
// We will still patch up in this case but not fire the warning.
// [ts] let NORMALIZE_NEWLINES_REGEX = "\\r\\n?";
// [ts] let NORMALIZE_NULL_AND_REPLACEMENT_REGEX = "\\u0000|\\uFFFD";
// [ts] normalizeMarkupForTextOrAttribute = (markup) => {
// [ts] let markupString = if (@typeName(markup) == "string") markup else "" + markup;
// [ts] return markupString.replace(NORMALIZE_NEWLINES_REGEX, "\n").replace(NORMALIZE_NULL_AND_REPLACEMENT_REGEX, "");
// [ts] };
// [ts] warnForTextDifference = (serverText, clientText) => {
// [ts] if (didWarnInvalidHydration) {
// [ts] return;
// [ts] }
// [ts] let normalizedClientText = normalizeMarkupForTextOrAttribute(clientText);
// [ts] let normalizedServerText = normalizeMarkupForTextOrAttribute(serverText);
// [ts] if (normalizedServerText == normalizedClientText) {
// [ts] return;
// [ts] }
// [ts] didWarnInvalidHydration = true;
// [ts] error("Text content did not match. Server: \"%s\" Client: \"%s\"", normalizedServerText, normalizedClientText);
// [ts] };
// [ts] warnForPropDifference = (propName, serverValue, clientValue) => {
// [ts] if (didWarnInvalidHydration) {
// [ts] return;
// [ts] }
// [ts] let normalizedClientValue = normalizeMarkupForTextOrAttribute(clientValue);
// [ts] let normalizedServerValue = normalizeMarkupForTextOrAttribute(serverValue);
// [ts] if (normalizedServerValue == normalizedClientValue) {
// [ts] return;
// [ts] }
// [ts] didWarnInvalidHydration = true;
// [ts] error("Prop `%s` did not match. Server: %s Client: %s", propName, JSON.stringify(normalizedServerValue), JSON.stringify(normalizedClientValue));
// [ts] };
// [ts] warnForExtraAttributes = (attributeNames) => {
// [ts] if (didWarnInvalidHydration) {
// [ts] return;
// [ts] }
// [ts] didWarnInvalidHydration = true;
// [ts] let names = [];
// [ts] attributeNames.forEach((name) => {
// [ts] names.push(name);
// [ts] });
// [ts] error("Extra attributes from the server: %s", names);
// [ts] };
// [ts] warnForInvalidEventListener = (registrationName, listener) => {
// [ts] if (listener == false) {
// [ts] error("Expected `%s` listener to be a function, instead got `false`.\n\n" + "If you used to conditionally omit it with %s={condition && value}, " + "pass %s={condition ? value : undefined} instead.", registrationName, registrationName, registrationName);
// [ts] } else {
// [ts] error("Expected `%s` listener to be a function, instead got a value of `%s` type.", registrationName, @typeName(listener));
// [ts] }
// [ts] }; // Parse the HTML and read it back to normalize the HTML string so that it
// can be used for comparison.
// [ts] normalizeHTML = (parent, html) => {
// We could have created a separate document here to avoid
// re-initializing custom elements if they exist. But this breaks
// how <noscript> is being handled. So we use the same document.
// See the discussion in https://github.com/facebook/react/pull/11157.
// [ts] let testElement = if (parent.namespaceURI == HTML_NAMESPACE$1) parent.ownerDocument.createElement(parent.tagName) else parent.ownerDocument.createElementNS(parent.namespaceURI, parent.tagName);
// [ts] testElement.innerHTML = html;
// [ts] return testElement.innerHTML;
// [ts] };
// [ts] }
// [ts] fn ensureListeningTo(rootContainerElement: auto, registrationName: auto) {
// [ts] let isDocumentOrFragment = rootContainerElement.nodeType == DOCUMENT_NODE || rootContainerElement.nodeType == DOCUMENT_FRAGMENT_NODE;
// [ts] let doc = if (isDocumentOrFragment) rootContainerElement else rootContainerElement.ownerDocument;
// [ts] legacyListenToEvent(registrationName, doc);
// [ts] }
// [ts] fn getOwnerDocumentFromRootContainer(rootContainerElement: auto) {
// [ts] return if (rootContainerElement.nodeType == DOCUMENT_NODE) rootContainerElement else rootContainerElement.ownerDocument;
// [ts] }
// [ts] fn noop() {}
// [ts] fn trapClickOnNonInteractiveElement(node: auto) {
// Mobile Safari does not fire properly bubble click events on
// non-interactive elements, which means delegated click listeners do not
// fire. The workaround for this bug involves attaching an empty click
// listener on the target node.
// http://www.quirksmode.org/blog/archives/2010/09/click_event_del.html
// Just set it using the onclick property so that we don't have to manage any
// bookkeeping for it. Not sure if we need to clear it when the listener is
// removed.
// TODO: Only do this for the relevant Safaris maybe?
// [ts] node.onclick = noop;
// [ts] }
// [ts] fn setInitialDOMProperties(tag: auto, domElement: auto, rootContainerElement: auto, nextProps: auto, isCustomComponentTag: auto) {
// [ts] for (let propKey in nextProps) {
// [ts] if (!nextProps.hasOwnProperty(propKey)) {
// [ts] continue;
// [ts] }
// [ts] let nextProp = nextProps[propKey];
// [ts] if (propKey == STYLE) {
// [ts] {
// [ts] if (nextProp) {
// Freeze the next style object so that we can assume it won't be
// mutated. We have already warned for this in the past.
// [ts] Object.freeze(nextProp);
// [ts] }
// [ts] } // Relies on `updateStylesByID` not mutating `styleUpdates`.
// [ts] setValueForStyles(domElement, nextProp);
// [ts] } else if (propKey == DANGEROUSLY_SET_INNER_HTML) {
// [ts] let nextHtml = if (nextProp) nextProp[HTML$1] else null;
// [ts] if (nextHtml != null) {
// [ts] setInnerHTML(domElement, nextHtml);
// [ts] }
// [ts] } else if (propKey == CHILDREN) {
// [ts] if (@typeName(nextProp) == "string") {
// Avoid setting initial textContent when the text is empty. In IE11 setting
// textContent on a <textarea> will cause the placeholder to not
// show within the <textarea> until it has been focused and blurred again.
// https://github.com/facebook/react/issues/6731#issuecomment-254874553
// [ts] let canSetTextContent = tag != "textarea" || nextProp != "";
// [ts] if (canSetTextContent) {
// [ts] setTextContent(domElement, nextProp);
// [ts] }
// [ts] } else if (@typeName(nextProp) == "number") {
// [ts] setTextContent(domElement, "" + nextProp);
// [ts] }
// [ts] } else if ( propKey == SUPPRESS_CONTENT_EDITABLE_WARNING || propKey == SUPPRESS_HYDRATION_WARNING) ; else if (propKey == AUTOFOCUS) ; else if (registrationNameModules.hasOwnProperty(propKey)) {
// [ts] if (nextProp != null) {
// [ts] if ( @typeName(nextProp) != "function") {
// [ts] warnForInvalidEventListener(propKey, nextProp);
// [ts] }
// [ts] ensureListeningTo(rootContainerElement, propKey);
// [ts] }
// [ts] } else if (nextProp != null) {
// [ts] setValueForProperty(domElement, propKey, nextProp, isCustomComponentTag);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn updateDOMProperties(domElement: auto, updatePayload: auto, wasCustomComponentTag: auto, isCustomComponentTag: auto) {
// TODO: Handle wasCustomComponentTag
// [ts] let i = 0;
// [ts] while (i < updatePayload.length) { // [cap-migrate] for-step: i += 2
// [ts] let propKey = updatePayload[i];
// [ts] let propValue = updatePayload[i + 1];
// [ts] if (propKey == STYLE) {
// [ts] setValueForStyles(domElement, propValue);
// [ts] } else if (propKey == DANGEROUSLY_SET_INNER_HTML) {
// [ts] setInnerHTML(domElement, propValue);
// [ts] } else if (propKey == CHILDREN) {
// [ts] setTextContent(domElement, propValue);
// [ts] } else {
// [ts] setValueForProperty(domElement, propKey, propValue, isCustomComponentTag);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn createElement(type: auto, props: auto, rootContainerElement: auto, parentNamespace: auto) {
// [ts] let isCustomComponentTag; // We create tags in the namespace of their parent container, except HTML
// tags get no namespace.
// [ts] let ownerDocument = getOwnerDocumentFromRootContainer(rootContainerElement);
// [ts] let domElement;
// [ts] let namespaceURI = parentNamespace;
// [ts] if (namespaceURI == HTML_NAMESPACE$1) {
// [ts] namespaceURI = getIntrinsicNamespace(type);
// [ts] }
// [ts] if (namespaceURI == HTML_NAMESPACE$1) {
// [ts] {
// [ts] isCustomComponentTag = isCustomComponent(type, props); // Should this check be gated by parent namespace? Not sure we want to
// allow <SVG> or <mATH>.
// [ts] if (!isCustomComponentTag && type != type.toLowerCase()) {
// [ts] error("<%s /> is using incorrect casing. " + "Use PascalCase for React components, " + "or lowercase for HTML elements.", type);
// [ts] }
// [ts] }
// [ts] if (type == "script") {
// Create the script via .innerHTML so its "parser-inserted" flag is
// set to true and it does not execute
// [ts] let div = ownerDocument.createElement("div");
// [ts] div.innerHTML = "<script><" + "/script>"; // eslint-disable-line
// This is guaranteed to yield a script element.
// [ts] let firstChild = div.firstChild;
// [ts] domElement = div.removeChild(firstChild);
// [ts] } else if (@typeName(props.is) == "string") {
// $FlowIssue `createElement` should be updated for Web Components
// [ts] domElement = ownerDocument.createElement(type, {
// [ts] is: props.is
// [ts] });
// [ts] } else {
// Separate else branch instead of using `props.is || undefined` above because of a Firefox bug.
// See discussion in https://github.com/facebook/react/pull/6896
// and discussion in https://bugzilla.mozilla.org/show_bug.cgi?id=1276240
// [ts] domElement = ownerDocument.createElement(type); // Normally attributes are assigned in `setInitialDOMProperties`, however the `multiple` and `size`
// attributes on `select`s needs to be added before `option`s are inserted.
// This prevents:
// - a bug where the `select` does not scroll to the correct option because singular
//  `select` elements automatically pick the first item #13222
// - a bug where the `select` set the first item as selected despite the `size` attribute #14239
// See https://github.com/facebook/react/issues/13222
// and https://github.com/facebook/react/issues/14239
// [ts] if (type == "select") {
// [ts] let node = domElement;
// [ts] if (props.multiple) {
// [ts] node.multiple = true;
// [ts] } else if (props.size) {
// Setting a size greater than 1 causes a select to behave like `multiple=true`, where
// it is possible that no option is selected.
//
// This is only necessary when a select in "single selection mode".
// [ts] node.size = props.size;
// [ts] }
// [ts] }
// [ts] }
// [ts] } else {
// [ts] domElement = ownerDocument.createElementNS(namespaceURI, type);
// [ts] }
// [ts] {
// [ts] if (namespaceURI == HTML_NAMESPACE$1) {
// [ts] if (!isCustomComponentTag && Object.prototype.toString.call(domElement) == "[object HTMLUnknownElement]" && !Object.prototype.hasOwnProperty.call(warnedUnknownTags, type)) {
// [ts] warnedUnknownTags[type] = true;
// [ts] error("The tag <%s> is unrecognized in this browser. " + "If you meant to render a React component, start its name with " + "an uppercase letter.", type);
// [ts] }
// [ts] }
// [ts] }
// [ts] return domElement;
// [ts] }
// [ts] fn createTextNode(text: auto, rootContainerElement: auto) {
// [ts] return getOwnerDocumentFromRootContainer(rootContainerElement).createTextNode(text);
// [ts] }
// [ts] fn setInitialProperties(domElement: auto, tag: auto, rawProps: auto, rootContainerElement: auto) {
// [ts] let isCustomComponentTag = isCustomComponent(tag, rawProps);
// [ts] {
// [ts] validatePropertiesInDevelopment(tag, rawProps);
// [ts] } // TODO: Make sure that we check isMounted before firing any of these events.
// [ts] let props;
// [ts] switch (tag) {
// [ts] "iframe", "object", "embed" => {
// [ts] trapBubbledEvent(TOP_LOAD, domElement);
// [ts] props = rawProps;
// break;
// [ts] },
// Create listener for each media event
// [ts] let i = 0;
// [ts] while (i < mediaEventTypes.length) { // [cap-migrate] for-step: i++
// [ts] trapBubbledEvent(mediaEventTypes[i], domElement);
// [ts] }
// [ts] "video", "audio" => {
// [ts] props = rawProps;
// break;
// [ts] },
// [ts] "source" => {
// [ts] trapBubbledEvent(TOP_ERROR, domElement);
// [ts] props = rawProps;
// break;
// [ts] },
// [ts] "img", "image", "link" => {
// [ts] trapBubbledEvent(TOP_ERROR, domElement);
// [ts] trapBubbledEvent(TOP_LOAD, domElement);
// [ts] props = rawProps;
// break;
// [ts] },
// [ts] "form" => {
// [ts] trapBubbledEvent(TOP_RESET, domElement);
// [ts] trapBubbledEvent(TOP_SUBMIT, domElement);
// [ts] props = rawProps;
// break;
// [ts] },
// [ts] "details" => {
// [ts] trapBubbledEvent(TOP_TOGGLE, domElement);
// [ts] props = rawProps;
// break;
// [ts] },
// [ts] "input" => {
// [ts] initWrapperState(domElement, rawProps);
// [ts] props = getHostProps(domElement, rawProps);
// [ts] trapBubbledEvent(TOP_INVALID, domElement); // For controlled components we always need to ensure we're listening
// to onChange. Even if there is no listener.
// [ts] ensureListeningTo(rootContainerElement, "onChange");
// break;
// [ts] },
// [ts] "option" => {
// [ts] validateProps(domElement, rawProps);
// [ts] props = getHostProps$1(domElement, rawProps);
// break;
// [ts] },
// [ts] "select" => {
// [ts] initWrapperState$1(domElement, rawProps);
// [ts] props = getHostProps$2(domElement, rawProps);
// [ts] trapBubbledEvent(TOP_INVALID, domElement); // For controlled components we always need to ensure we're listening
// to onChange. Even if there is no listener.
// [ts] ensureListeningTo(rootContainerElement, "onChange");
// break;
// [ts] },
// [ts] "textarea" => {
// [ts] initWrapperState$2(domElement, rawProps);
// [ts] props = getHostProps$3(domElement, rawProps);
// [ts] trapBubbledEvent(TOP_INVALID, domElement); // For controlled components we always need to ensure we're listening
// to onChange. Even if there is no listener.
// [ts] ensureListeningTo(rootContainerElement, "onChange");
// break;
// [ts] },
// [ts] else => {
// [ts] props = rawProps;
// [ts] }
// [ts] }
// [ts] assertValidProps(tag, props);
// [ts] setInitialDOMProperties(tag, domElement, rootContainerElement, props, isCustomComponentTag);
// [ts] switch (tag) {
// TODO: Make sure we check if this is still unmounted or do any clean
// up necessary since we never stop tracking anymore.
// [ts] "input" => {
// [ts] track(domElement);
// [ts] postMountWrapper(domElement, rawProps, false);
// break;
// [ts] },
// TODO: Make sure we check if this is still unmounted or do any clean
// up necessary since we never stop tracking anymore.
// [ts] "textarea" => {
// [ts] track(domElement);
// [ts] postMountWrapper$3(domElement);
// break;
// [ts] },
// [ts] "option" => {
// [ts] postMountWrapper$1(domElement, rawProps);
// break;
// [ts] },
// [ts] "select" => {
// [ts] postMountWrapper$2(domElement, rawProps);
// break;
// [ts] },
// [ts] else => {
// [ts] if (@typeName(props.onClick) == "function") {
// TODO: This cast may not be sound for SVG, MathML or custom elements.
// [ts] trapClickOnNonInteractiveElement(domElement);
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] } // Calculate the diff between the two objects.
// [ts] fn diffProperties(domElement: auto, tag: auto, lastRawProps: auto, nextRawProps: auto, rootContainerElement: auto) {
// [ts] {
// [ts] validatePropertiesInDevelopment(tag, nextRawProps);
// [ts] }
// [ts] let updatePayload = null;
// [ts] let lastProps;
// [ts] let nextProps;
// [ts] switch (tag) {
// [ts] "input" => {
// [ts] lastProps = getHostProps(domElement, lastRawProps);
// [ts] nextProps = getHostProps(domElement, nextRawProps);
// [ts] updatePayload = [];
// break;
// [ts] },
// [ts] "option" => {
// [ts] lastProps = getHostProps$1(domElement, lastRawProps);
// [ts] nextProps = getHostProps$1(domElement, nextRawProps);
// [ts] updatePayload = [];
// break;
// [ts] },
// [ts] "select" => {
// [ts] lastProps = getHostProps$2(domElement, lastRawProps);
// [ts] nextProps = getHostProps$2(domElement, nextRawProps);
// [ts] updatePayload = [];
// break;
// [ts] },
// [ts] "textarea" => {
// [ts] lastProps = getHostProps$3(domElement, lastRawProps);
// [ts] nextProps = getHostProps$3(domElement, nextRawProps);
// [ts] updatePayload = [];
// break;
// [ts] },
// [ts] else => {
// [ts] lastProps = lastRawProps;
// [ts] nextProps = nextRawProps;
// [ts] if (@typeName(lastProps.onClick) != "function" && @typeName(nextProps.onClick) == "function") {
// TODO: This cast may not be sound for SVG, MathML or custom elements.
// [ts] trapClickOnNonInteractiveElement(domElement);
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] assertValidProps(tag, nextProps);
// [ts] let propKey;
// [ts] let styleName;
// [ts] let styleUpdates = null;
// [ts] for (propKey in lastProps) {
// [ts] if (nextProps.hasOwnProperty(propKey) || !lastProps.hasOwnProperty(propKey) || lastProps[propKey] == null) {
// [ts] continue;
// [ts] }
// [ts] if (propKey == STYLE) {
// [ts] let lastStyle = lastProps[propKey];
// [ts] for (styleName in lastStyle) {
// [ts] if (lastStyle.hasOwnProperty(styleName)) {
// [ts] if (!styleUpdates) {
// [ts] styleUpdates = {};
// [ts] }
// [ts] styleUpdates[styleName] = "";
// [ts] }
// [ts] }
// [ts] } else if (propKey == DANGEROUSLY_SET_INNER_HTML || propKey == CHILDREN) ; else if ( propKey == SUPPRESS_CONTENT_EDITABLE_WARNING || propKey == SUPPRESS_HYDRATION_WARNING) ; else if (propKey == AUTOFOCUS) ; else if (registrationNameModules.hasOwnProperty(propKey)) {
// This is a special case. If any listener updates we need to ensure
// that the "current" fiber pointer gets updated so we need a commit
// to update this element.
// [ts] if (!updatePayload) {
// [ts] updatePayload = [];
// [ts] }
// [ts] } else {
// For all other deleted properties we add it to the queue. We use
// the whitelist in the commit phase instead.
// [ts] (updatePayload = updatePayload || []).push(propKey, null);
// [ts] }
// [ts] }
// [ts] for (propKey in nextProps) {
// [ts] let nextProp = nextProps[propKey];
// [ts] let lastProp = if (lastProps != null) lastProps[propKey] else null;
// [ts] if (!nextProps.hasOwnProperty(propKey) || nextProp == lastProp || nextProp == null && lastProp == null) {
// [ts] continue;
// [ts] }
// [ts] if (propKey == STYLE) {
// [ts] {
// [ts] if (nextProp) {
// Freeze the next style object so that we can assume it won't be
// mutated. We have already warned for this in the past.
// [ts] Object.freeze(nextProp);
// [ts] }
// [ts] }
// [ts] if (lastProp) {
// Unset styles on `lastProp` but not on `nextProp`.
// [ts] for (styleName in lastProp) {
// [ts] if (lastProp.hasOwnProperty(styleName) && (!nextProp || !nextProp.hasOwnProperty(styleName))) {
// [ts] if (!styleUpdates) {
// [ts] styleUpdates = {};
// [ts] }
// [ts] styleUpdates[styleName] = "";
// [ts] }
// [ts] } // Update styles that changed since `lastProp`.
// [ts] for (styleName in nextProp) {
// [ts] if (nextProp.hasOwnProperty(styleName) && lastProp[styleName] != nextProp[styleName]) {
// [ts] if (!styleUpdates) {
// [ts] styleUpdates = {};
// [ts] }
// [ts] styleUpdates[styleName] = nextProp[styleName];
// [ts] }
// [ts] }
// [ts] } else {
// Relies on `updateStylesByID` not mutating `styleUpdates`.
// [ts] if (!styleUpdates) {
// [ts] if (!updatePayload) {
// [ts] updatePayload = [];
// [ts] }
// [ts] updatePayload.push(propKey, styleUpdates);
// [ts] }
// [ts] styleUpdates = nextProp;
// [ts] }
// [ts] } else if (propKey == DANGEROUSLY_SET_INNER_HTML) {
// [ts] let nextHtml = if (nextProp) nextProp[HTML$1] else null;
// [ts] let lastHtml = if (lastProp) lastProp[HTML$1] else null;
// [ts] if (nextHtml != null) {
// [ts] if (lastHtml != nextHtml) {
// [ts] (updatePayload = updatePayload || []).push(propKey, nextHtml);
// [ts] }
// [ts] }
// [ts] } else if (propKey == CHILDREN) {
// [ts] if (lastProp != nextProp && (@typeName(nextProp) == "string" || @typeName(nextProp) == "number")) {
// [ts] (updatePayload = updatePayload || []).push(propKey, "" + nextProp);
// [ts] }
// [ts] } else if ( propKey == SUPPRESS_CONTENT_EDITABLE_WARNING || propKey == SUPPRESS_HYDRATION_WARNING) ; else if (registrationNameModules.hasOwnProperty(propKey)) {
// [ts] if (nextProp != null) {
// We eagerly listen to this even though we haven't committed yet.
// [ts] if ( @typeName(nextProp) != "function") {
// [ts] warnForInvalidEventListener(propKey, nextProp);
// [ts] }
// [ts] ensureListeningTo(rootContainerElement, propKey);
// [ts] }
// [ts] if (!updatePayload && lastProp != nextProp) {
// This is a special case. If any listener updates we need to ensure
// that the "current" props pointer gets updated so we need a commit
// to update this element.
// [ts] updatePayload = [];
// [ts] }
// [ts] } else {
// For any other property we always add it to the queue and then we
// filter it out using the whitelist during the commit.
// [ts] (updatePayload = updatePayload || []).push(propKey, nextProp);
// [ts] }
// [ts] }
// [ts] if (styleUpdates) {
// [ts] {
// [ts] validateShorthandPropertyCollisionInDev(styleUpdates, nextProps[STYLE]);
// [ts] }
// [ts] (updatePayload = updatePayload || []).push(STYLE, styleUpdates);
// [ts] }
// [ts] return updatePayload;
// [ts] } // Apply the diff.
// [ts] fn updateProperties(domElement: auto, updatePayload: auto, tag: auto, lastRawProps: auto, nextRawProps: auto) {
// Update checked *before* name.
// In the middle of an update, it is possible to have multiple checked.
// When a checked radio tries to change name, browser makes another radio's checked false.
// [ts] if (tag == "input" && nextRawProps.type == "radio" && nextRawProps.name != null) {
// [ts] updateChecked(domElement, nextRawProps);
// [ts] }
// [ts] let wasCustomComponentTag = isCustomComponent(tag, lastRawProps);
// [ts] let isCustomComponentTag = isCustomComponent(tag, nextRawProps); // Apply the diff.
// [ts] updateDOMProperties(domElement, updatePayload, wasCustomComponentTag, isCustomComponentTag); // TODO: Ensure that an update gets scheduled if any of the special props
// changed.
// [ts] switch (tag) {
// Update the wrapper around inputs *after* updating props. This has to
// happen after `updateDOMProperties`. Otherwise HTML5 input validations
// raise warnings and prevent the new value from being assigned.
// [ts] "input" => {
// [ts] updateWrapper(domElement, nextRawProps);
// break;
// [ts] },
// [ts] "textarea" => {
// [ts] updateWrapper$1(domElement, nextRawProps);
// break;
// [ts] },
// <select> value update needs to occur after <option> children
// reconciliation
// [ts] "select" => {
// [ts] postUpdateWrapper(domElement, nextRawProps);
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getPossibleStandardName(propName: auto) {
// [ts] {
// [ts] let lowerCasedName = propName.toLowerCase();
// [ts] if (!possibleStandardNames.hasOwnProperty(lowerCasedName)) {
// [ts] return null;
// [ts] }
// [ts] return possibleStandardNames[lowerCasedName] || null;
// [ts] }
// [ts] }
// [ts] fn diffHydratedProperties(domElement: auto, tag: auto, rawProps: auto, parentNamespace: auto, rootContainerElement: auto) {
// [ts] let isCustomComponentTag;
// [ts] let extraAttributeNames;
// [ts] {
// [ts] suppressHydrationWarning = rawProps[SUPPRESS_HYDRATION_WARNING] == true;
// [ts] isCustomComponentTag = isCustomComponent(tag, rawProps);
// [ts] validatePropertiesInDevelopment(tag, rawProps);
// [ts] } // TODO: Make sure that we check isMounted before firing any of these events.
// [ts] switch (tag) {
// [ts] "iframe", "object", "embed" => {
// [ts] trapBubbledEvent(TOP_LOAD, domElement);
// break;
// [ts] },
// Create listener for each media event
// [ts] let i = 0;
// [ts] while (i < mediaEventTypes.length) { // [cap-migrate] for-step: i++
// [ts] trapBubbledEvent(mediaEventTypes[i], domElement);
// [ts] }
// [ts] "video", "audio" => {
// [ts] break;
// [ts] },
// [ts] "source" => {
// [ts] trapBubbledEvent(TOP_ERROR, domElement);
// break;
// [ts] },
// [ts] "img", "image", "link" => {
// [ts] trapBubbledEvent(TOP_ERROR, domElement);
// [ts] trapBubbledEvent(TOP_LOAD, domElement);
// break;
// [ts] },
// [ts] "form" => {
// [ts] trapBubbledEvent(TOP_RESET, domElement);
// [ts] trapBubbledEvent(TOP_SUBMIT, domElement);
// break;
// [ts] },
// [ts] "details" => {
// [ts] trapBubbledEvent(TOP_TOGGLE, domElement);
// break;
// [ts] },
// [ts] "input" => {
// [ts] initWrapperState(domElement, rawProps);
// [ts] trapBubbledEvent(TOP_INVALID, domElement); // For controlled components we always need to ensure we're listening
// to onChange. Even if there is no listener.
// [ts] ensureListeningTo(rootContainerElement, "onChange");
// break;
// [ts] },
// [ts] "option" => {
// [ts] validateProps(domElement, rawProps);
// break;
// [ts] },
// [ts] "select" => {
// [ts] initWrapperState$1(domElement, rawProps);
// [ts] trapBubbledEvent(TOP_INVALID, domElement); // For controlled components we always need to ensure we're listening
// to onChange. Even if there is no listener.
// [ts] ensureListeningTo(rootContainerElement, "onChange");
// break;
// [ts] },
// [ts] "textarea" => {
// [ts] initWrapperState$2(domElement, rawProps);
// [ts] trapBubbledEvent(TOP_INVALID, domElement); // For controlled components we always need to ensure we're listening
// to onChange. Even if there is no listener.
// [ts] ensureListeningTo(rootContainerElement, "onChange");
// break;
// [ts] }
// [ts] }
// [ts] assertValidProps(tag, rawProps);
// [ts] {
// [ts] extraAttributeNames = Set.init();
// [ts] let attributes = domElement.attributes;
// [ts] let _i = 0;
// [ts] while (_i < attributes.length) { // [cap-migrate] for-step: _i++
// [ts] let name = attributes[_i].name.toLowerCase();
// [ts] switch (name) {
// Built-in SSR attribute is whitelisted
// [ts] "data-reactroot" => {
// [ts] break;
// Controlled attributes are not validated
// TODO: Only ignore them on controlled tags.
// [ts] },
// [ts] "value" => {
// [ts] break;
// [ts] },
// [ts] "checked" => {
// [ts] break;
// [ts] },
// [ts] "selected" => {
// [ts] break;
// [ts] },
// [ts] else => {
// Intentionally use the original name.
// See discussion in https://github.com/facebook/react/pull/10676.
// [ts] extraAttributeNames.add(attributes[_i].name);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let updatePayload = null;
// [ts] for (let propKey in rawProps) {
// [ts] if (!rawProps.hasOwnProperty(propKey)) {
// [ts] continue;
// [ts] }
// [ts] let nextProp = rawProps[propKey];
// [ts] if (propKey == CHILDREN) {
// For text content children we compare against textContent. This
// might match additional HTML that is hidden when we read it using
// textContent. E.g. "foo" will match "f<span>oo</span>" but that still
// satisfies our requirement. Our requirement is not to produce perfect
// HTML and attributes. Ideally we should preserve structure but it's
// ok not to if the visible content is still enough to indicate what
// even listeners these nodes might be wired up to.
// TODO: Warn if there is more than a single textNode as a child.
// TODO: Should we use domElement.firstChild.nodeValue to compare?
// [ts] if (@typeName(nextProp) == "string") {
// [ts] if (domElement.textContent != nextProp) {
// [ts] if ( !suppressHydrationWarning) {
// [ts] warnForTextDifference(domElement.textContent, nextProp);
// [ts] }
// [ts] updatePayload = [CHILDREN, nextProp];
// [ts] }
// [ts] } else if (@typeName(nextProp) == "number") {
// [ts] if (domElement.textContent != "" + nextProp) {
// [ts] if ( !suppressHydrationWarning) {
// [ts] warnForTextDifference(domElement.textContent, nextProp);
// [ts] }
// [ts] updatePayload = [CHILDREN, "" + nextProp];
// [ts] }
// [ts] }
// [ts] } else if (registrationNameModules.hasOwnProperty(propKey)) {
// [ts] if (nextProp != null) {
// [ts] if ( @typeName(nextProp) != "function") {
// [ts] warnForInvalidEventListener(propKey, nextProp);
// [ts] }
// [ts] ensureListeningTo(rootContainerElement, propKey);
// [ts] }
// [ts] } else if ( // Convince Flow we've calculated it (it's DEV-only in this method.)
// [ts] @typeName(isCustomComponentTag) == "boolean") {
// Validate that the properties correspond to their expected values.
// [ts] let serverValue = void 0;
// [ts] let propertyInfo = getPropertyInfo(propKey);
// [ts] if (suppressHydrationWarning) ; else if ( propKey == SUPPRESS_CONTENT_EDITABLE_WARNING || propKey == SUPPRESS_HYDRATION_WARNING || // Controlled attributes are not validated
// TODO: Only ignore them on controlled tags.
// [ts] propKey == "value" || propKey == "checked" || propKey == "selected") ; else if (propKey == DANGEROUSLY_SET_INNER_HTML) {
// [ts] let serverHTML = domElement.innerHTML;
// [ts] let nextHtml = if (nextProp) nextProp[HTML$1] else null;
// [ts] let expectedHTML = normalizeHTML(if (domElement, nextHtml != null) nextHtml else "");
// [ts] if (expectedHTML != serverHTML) {
// [ts] warnForPropDifference(propKey, serverHTML, expectedHTML);
// [ts] }
// [ts] } else if (propKey == STYLE) {
// $FlowFixMe - Should be inferred as not undefined.
// [ts] extraAttributeNames.delete(propKey);
// [ts] if (canDiffStyleForHydrationWarning) {
// [ts] let expectedStyle = createDangerousStringForStyles(nextProp);
// [ts] serverValue = domElement.getAttribute("style");
// [ts] if (expectedStyle != serverValue) {
// [ts] warnForPropDifference(propKey, serverValue, expectedStyle);
// [ts] }
// [ts] }
// [ts] } else if (isCustomComponentTag) {
// $FlowFixMe - Should be inferred as not undefined.
// [ts] extraAttributeNames.delete(propKey.toLowerCase());
// [ts] serverValue = getValueForAttribute(domElement, propKey, nextProp);
// [ts] if (nextProp != serverValue) {
// [ts] warnForPropDifference(propKey, serverValue, nextProp);
// [ts] }
// [ts] } else if (!shouldIgnoreAttribute(propKey, propertyInfo, isCustomComponentTag) && !shouldRemoveAttribute(propKey, nextProp, propertyInfo, isCustomComponentTag)) {
// [ts] let isMismatchDueToBadCasing = false;
// [ts] if (propertyInfo != null) {
// $FlowFixMe - Should be inferred as not undefined.
// [ts] extraAttributeNames.delete(propertyInfo.attributeName);
// [ts] serverValue = getValueForProperty(domElement, propKey, nextProp, propertyInfo);
// [ts] } else {
// [ts] let ownNamespace = parentNamespace;
// [ts] if (ownNamespace == HTML_NAMESPACE$1) {
// [ts] ownNamespace = getIntrinsicNamespace(tag);
// [ts] }
// [ts] if (ownNamespace == HTML_NAMESPACE$1) {
// $FlowFixMe - Should be inferred as not undefined.
// [ts] extraAttributeNames.delete(propKey.toLowerCase());
// [ts] } else {
// [ts] let standardName = getPossibleStandardName(propKey);
// [ts] if (standardName != null && standardName != propKey) {
// If an SVG prop is supplied with bad casing, it will
// be successfully parsed from HTML, but will produce a mismatch
// (and would be incorrectly rendered on the client).
// However, we already warn about bad casing elsewhere.
// So we'll skip the misleading extra mismatch warning in this case.
// [ts] isMismatchDueToBadCasing = true; // $FlowFixMe - Should be inferred as not undefined.
// [ts] extraAttributeNames.delete(standardName);
// [ts] } // $FlowFixMe - Should be inferred as not undefined.
// [ts] extraAttributeNames.delete(propKey);
// [ts] }
// [ts] serverValue = getValueForAttribute(domElement, propKey, nextProp);
// [ts] }
// [ts] if (nextProp != serverValue && !isMismatchDueToBadCasing) {
// [ts] warnForPropDifference(propKey, serverValue, nextProp);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// $FlowFixMe - Should be inferred as not undefined.
// [ts] if (extraAttributeNames.size > 0 && !suppressHydrationWarning) {
// $FlowFixMe - Should be inferred as not undefined.
// [ts] warnForExtraAttributes(extraAttributeNames);
// [ts] }
// [ts] }
// [ts] switch (tag) {
// TODO: Make sure we check if this is still unmounted or do any clean
// up necessary since we never stop tracking anymore.
// [ts] "input" => {
// [ts] track(domElement);
// [ts] postMountWrapper(domElement, rawProps, true);
// break;
// [ts] },
// TODO: Make sure we check if this is still unmounted or do any clean
// up necessary since we never stop tracking anymore.
// [ts] "textarea" => {
// [ts] track(domElement);
// [ts] postMountWrapper$3(domElement);
// break;
// [ts] },
// For input and textarea we current always set the value property at
// post mount to force it to diverge from attributes. However, for
// option and select we don't quite do the same thing and select
// is not resilient to the DOM state changing so we don't do that here.
// TODO: Consider not doing this for input and textarea.
// [ts] "select", "option" => {
// [ts] break;
// [ts] },
// [ts] else => {
// [ts] if (@typeName(rawProps.onClick) == "function") {
// TODO: This cast may not be sound for SVG, MathML or custom elements.
// [ts] trapClickOnNonInteractiveElement(domElement);
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] return updatePayload;
// [ts] }
// [ts] fn diffHydratedText(textNode: auto, text: auto) {
// [ts] let isDifferent = textNode.nodeValue != text;
// [ts] return isDifferent;
// [ts] }
// [ts] fn warnForUnmatchedText(textNode: auto, text: auto) {
// [ts] {
// [ts] warnForTextDifference(textNode.nodeValue, text);
// [ts] }
// [ts] }
// [ts] fn warnForDeletedHydratableElement(parentNode: auto, child: auto) {
// [ts] {
// [ts] if (didWarnInvalidHydration) {
// [ts] return;
// [ts] }
// [ts] didWarnInvalidHydration = true;
// [ts] error("Did not expect server HTML to contain a <%s> in <%s>.", child.nodeName.toLowerCase(), parentNode.nodeName.toLowerCase());
// [ts] }
// [ts] }
// [ts] fn warnForDeletedHydratableText(parentNode: auto, child: auto) {
// [ts] {
// [ts] if (didWarnInvalidHydration) {
// [ts] return;
// [ts] }
// [ts] didWarnInvalidHydration = true;
// [ts] error("Did not expect server HTML to contain the text node \"%s\" in <%s>.", child.nodeValue, parentNode.nodeName.toLowerCase());
// [ts] }
// [ts] }
// [ts] fn warnForInsertedHydratedElement(parentNode: auto, tag: auto, props: auto) {
// [ts] {
// [ts] if (didWarnInvalidHydration) {
// [ts] return;
// [ts] }
// [ts] didWarnInvalidHydration = true;
// [ts] error("Expected server HTML to contain a matching <%s> in <%s>.", tag, parentNode.nodeName.toLowerCase());
// [ts] }
// [ts] }
// [ts] fn warnForInsertedHydratedText(parentNode: auto, text: auto) {
// [ts] {
// [ts] if (text == "") {
// We expect to insert empty text nodes since they're not represented in
// the HTML.
// TODO: Remove this special case if we can just avoid inserting empty
// text nodes.
// [ts] return;
// [ts] }
// [ts] if (didWarnInvalidHydration) {
// [ts] return;
// [ts] }
// [ts] didWarnInvalidHydration = true;
// [ts] error("Expected server HTML to contain a matching text node for \"%s\" in <%s>.", text, parentNode.nodeName.toLowerCase());
// [ts] }
// [ts] }
// [ts] (domElement, tag, props) => {
// [ts] switch (tag) {
// [ts] "input" => {
// [ts] restoreControlledState(domElement, props);
// [ts] return;
// [ts] },
// [ts] "textarea" => {
// [ts] restoreControlledState$2(domElement, props);
// [ts] return;
// [ts] },
// [ts] "select" => {
// [ts] restoreControlledState$1(domElement, props);
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getActiveElement(doc: auto) {
// [ts] doc = doc || (if (@typeName(document) != "undefined") document else null);
// [ts] if (@typeName(doc) == "undefined") {
// [ts] return null;
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return doc.activeElement || doc.body;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] return doc.body;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Given any node return the first leaf node without children.
// [ts] *
// [ts] * @param {DOMElement|DOMTextNode} node
// [ts] * @return {DOMElement|DOMTextNode}
// [ts] */
// [ts] fn getLeafNode(node: auto) {
// [ts] while (node && node.firstChild) {
// [ts] node = node.firstChild;
// [ts] }
// [ts] return node;
// [ts] }
// [ts] /**
// [ts] * Get the next sibling within a container. This will walk up the
// [ts] * DOM if a node's siblings have been exhausted.
// [ts] *
// [ts] * @param {DOMElement|DOMTextNode} node
// [ts] * @return {?DOMElement|DOMTextNode}
// [ts] */
// [ts] fn getSiblingNode(node: auto) {
// [ts] while (node) {
// [ts] if (node.nextSibling) {
// [ts] return node.nextSibling;
// [ts] }
// [ts] node = node.parentNode;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Get object describing the nodes which contain characters at offset.
// [ts] *
// [ts] * @param {DOMElement|DOMTextNode} root
// [ts] * @param {number} offset
// [ts] * @return {?object}
// [ts] */
// [ts] fn getNodeForCharacterOffset(root: auto, offset: auto) {
// [ts] let node = getLeafNode(root);
// [ts] let nodeStart = 0;
// [ts] let nodeEnd = 0;
// [ts] while (node) {
// [ts] if (node.nodeType == TEXT_NODE) {
// [ts] nodeEnd = nodeStart + node.textContent.length;
// [ts] if (nodeStart <= offset && nodeEnd >= offset) {
// [ts] return {
// [ts] node: node,
// [ts] offset: offset - nodeStart
// [ts] };
// [ts] }
// [ts] nodeStart = nodeEnd;
// [ts] }
// [ts] node = getLeafNode(getSiblingNode(node));
// [ts] }
// [ts] }
// [ts] /**
// [ts] * @param {DOMElement} outerNode
// [ts] * @return {?object}
// [ts] */
// [ts] fn getOffsets(outerNode: auto) {
// [ts] let ownerDocument = outerNode.ownerDocument;
// [ts] let win = ownerDocument && ownerDocument.defaultView || window;
// [ts] let selection = win.getSelection && win.getSelection();
// [ts] if (!selection || selection.rangeCount == 0) {
// [ts] return null;
// [ts] }
// [ts] let anchorNode = selection.anchorNode,
// [ts] anchorOffset = selection.anchorOffset,
// [ts] focusNode = selection.focusNode,
// [ts] focusOffset = selection.focusOffset; // In Firefox, anchorNode and focusNode can be "anonymous divs", e.g. the
// up/down buttons on an <input type="number">. Anonymous divs do not seem to
// expose properties, triggering a "Permission denied error" if any of its
// properties are accessed. The only seemingly possible way to avoid erroring
// is to access a property that typically works for non-anonymous divs and
// catch any error that may otherwise arise. See
// https://bugzilla.mozilla.org/show_bug.cgi?id=208427
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] /* eslint-disable no-unused-expressions */
// [ts] anchorNode.nodeType;
// [ts] focusNode.nodeType;
// [ts] /* eslint-enable no-unused-expressions */
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] return null;
// [ts] }
// [ts] return getModernOffsetsFromPoints(outerNode, anchorNode, anchorOffset, focusNode, focusOffset);
// [ts] }
// [ts] /**
// [ts] * Returns {start, end} where `start` is the character/codepoint index of
// [ts] * (anchorNode, anchorOffset) within the textContent of `outerNode`, and
// [ts] * `end` is the index of (focusNode, focusOffset).
// [ts] *
// [ts] * Returns null if you pass in garbage input but we should probably just crash.
// [ts] *
// [ts] * Exported only for testing.
// [ts] */
// [ts] fn getModernOffsetsFromPoints(outerNode: auto, anchorNode: auto, anchorOffset: auto, focusNode: auto, focusOffset: auto) {
// [ts] let length = 0;
// [ts] let start = -1;
// [ts] let end = -1;
// [ts] let indexWithinAnchor = 0;
// [ts] let indexWithinFocus = 0;
// [ts] let node = outerNode;
// [ts] let parentNode = null;
// [ts] outer: while (true) {
// [ts] let next = null;
// [ts] while (true) {
// [ts] if (node == anchorNode && (anchorOffset == 0 || node.nodeType == TEXT_NODE)) {
// [ts] start = length + anchorOffset;
// [ts] }
// [ts] if (node == focusNode && (focusOffset == 0 || node.nodeType == TEXT_NODE)) {
// [ts] end = length + focusOffset;
// [ts] }
// [ts] if (node.nodeType == TEXT_NODE) {
// [ts] length += node.nodeValue.length;
// [ts] }
// [ts] if ((next = node.firstChild) == null) {
// [ts] break;
// [ts] } // Moving from `node` to its first child `next`.
// [ts] parentNode = node;
// [ts] node = next;
// [ts] }
// [ts] while (true) {
// [ts] if (node == outerNode) {
// If `outerNode` has children, this is always the second time visiting
// it. If it has no children, this is still the first loop, and the only
// valid selection is anchorNode and focusNode both equal to this node
// and both offsets 0, in which case we will have handled above.
// [ts] break outer;
// [ts] }
// [ts] if (parentNode == anchorNode && ++indexWithinAnchor == anchorOffset) {
// [ts] start = length;
// [ts] }
// [ts] if (parentNode == focusNode && ++indexWithinFocus == focusOffset) {
// [ts] end = length;
// [ts] }
// [ts] if ((next = node.nextSibling) != null) {
// [ts] break;
// [ts] }
// [ts] node = parentNode;
// [ts] parentNode = node.parentNode;
// [ts] } // Moving from `node` to its next sibling `next`.
// [ts] node = next;
// [ts] }
// [ts] if (start == -1 || end == -1) {
// This should never happen. (Would happen if the anchor/focus nodes aren't
// actually inside the passed-in node.)
// [ts] return null;
// [ts] }
// [ts] return {
// [ts] start: start,
// [ts] end: end
// [ts] };
// [ts] }
// [ts] /**
// [ts] * In modern non-IE browsers, we can support both forward and backward
// [ts] * selections.
// [ts] *
// [ts] * Note: IE10+ supports the Selection object, but it does not support
// [ts] * the `extend` method, which means that even in modern IE, it's not possible
// [ts] * to programmatically create a backward selection. Thus, for all IE
// [ts] * versions, we use the old IE API to create our selections.
// [ts] *
// [ts] * @param {DOMElement|DOMTextNode} node
// [ts] * @param {object} offsets
// [ts] */
// [ts] fn setOffsets(node: auto, offsets: auto) {
// [ts] let doc = node.ownerDocument || document;
// [ts] let win = doc && doc.defaultView || window; // Edge fails with "Object expected" in some scenarios.
// (For instance: TinyMCE editor used in a list component that supports pasting to add more,
// fails when pasting 100+ items)
// [ts] if (!win.getSelection) {
// [ts] return;
// [ts] }
// [ts] let selection = win.getSelection();
// [ts] let length = node.textContent.length;
// [ts] let start = Math.min(offsets.start, length);
// [ts] let end = if (offsets.end == null) start else Math.min(offsets.end, length); // IE 11 uses modern selection, but doesn't support the extend method.
// Flip backward selections, so we can set with a single range.
// [ts] if (!selection.extend && start > end) {
// [ts] let temp = end;
// [ts] end = start;
// [ts] start = temp;
// [ts] }
// [ts] let startMarker = getNodeForCharacterOffset(node, start);
// [ts] let endMarker = getNodeForCharacterOffset(node, end);
// [ts] if (startMarker && endMarker) {
// [ts] if (selection.rangeCount == 1 && selection.anchorNode == startMarker.node && selection.anchorOffset == startMarker.offset && selection.focusNode == endMarker.node && selection.focusOffset == endMarker.offset) {
// [ts] return;
// [ts] }
// [ts] let range = doc.createRange();
// [ts] range.setStart(startMarker.node, startMarker.offset);
// [ts] selection.removeAllRanges();
// [ts] if (start > end) {
// [ts] selection.addRange(range);
// [ts] selection.extend(endMarker.node, endMarker.offset);
// [ts] } else {
// [ts] range.setEnd(endMarker.node, endMarker.offset);
// [ts] selection.addRange(range);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn isTextNode(node: auto) {
// [ts] return node && node.nodeType == TEXT_NODE;
// [ts] }
// [ts] fn containsNode(outerNode: auto, innerNode: auto) {
// [ts] if (!outerNode || !innerNode) {
// [ts] return false;
// [ts] } else if (outerNode == innerNode) {
// [ts] return true;
// [ts] } else if (isTextNode(outerNode)) {
// [ts] return false;
// [ts] } else if (isTextNode(innerNode)) {
// [ts] return containsNode(outerNode, innerNode.parentNode);
// [ts] } else if ("contains" == outerNode) {
// [ts] return outerNode.contains(innerNode);
// [ts] } else if (outerNode.compareDocumentPosition) {
// [ts] return !!(outerNode.compareDocumentPosition(innerNode) & 16);
// [ts] } else {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] fn isInDocument(node: auto) {
// [ts] return node && node.ownerDocument && containsNode(node.ownerDocument.documentElement, node);
// [ts] }
// [ts] fn isSameOriginFrame(iframe: auto) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Accessing the contentDocument of a HTMLIframeElement can cause the browser
// to throw, e.g. if it has a cross-origin src attribute.
// Safari will show an error in the console when the access results in "Blocked a frame with origin". e.g:
// iframe.contentDocument.defaultView;
// A safety way is to access one of the cross origin properties: Window or Location
// Which might result in "SecurityError" DOM Exception and it is compatible to Safari.
// https://html.spec.whatwg.org/multipage/browsers.html#integration-with-idl
// [ts] return @typeName(iframe.contentWindow.location.href) == "string";
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
// [ts] return false;
// [ts] }
// [ts] }
// [ts] fn getActiveElementDeep() {
// [ts] let win = window;
// [ts] let element = getActiveElement();
// [ts] while (element == win.HTMLIFrameElement) {
// [ts] if (isSameOriginFrame(element)) {
// [ts] win = element.contentWindow;
// [ts] } else {
// [ts] return element;
// [ts] }
// [ts] element = getActiveElement(win.document);
// [ts] }
// [ts] return element;
// [ts] }
// [ts] /**
// [ts] * @ReactInputSelection: React input selection module. Based on Selection.js,
// [ts] * but modified to be suitable for react and has a couple of bug fixes (doesn't
// [ts] * assume buttons have range selections allowed).
// [ts] * Input selection module for React.
// [ts] */
// [ts] /**
// [ts] * @hasSelectionCapabilities: we get the element types that support selection
// [ts] * from https://html.spec.whatwg.org/#do-not-apply, looking at `selectionStart`
// [ts] * and `selectionEnd` rows.
// [ts] */
// [ts] fn hasSelectionCapabilities(elem: auto) {
// [ts] let nodeName = elem && elem.nodeName && elem.nodeName.toLowerCase();
// [ts] return nodeName && (nodeName == "input" && (elem.type == "text" || elem.type == "search" || elem.type == "tel" || elem.type == "url" || elem.type == "password") || nodeName == "textarea" || elem.contentEditable == "true");
// [ts] }
// [ts] fn getSelectionInformation() {
// [ts] let focusedElem = getActiveElementDeep();
// [ts] return {
// Used by Flare
// [ts] activeElementDetached: null,
// [ts] focusedElem: focusedElem,
// [ts] selectionRange: if (hasSelectionCapabilities(focusedElem)) getSelection(focusedElem) else null
// [ts] };
// [ts] }
// [ts] /**
// [ts] * @restoreSelection: If any selection information was potentially lost,
// [ts] * restore it. This is useful when performing operations that could remove dom
// [ts] * nodes and place them back in, resulting in focus being lost.
// [ts] */
// [ts] fn restoreSelection(priorSelectionInformation: auto) {
// [ts] let curFocusedElem = getActiveElementDeep();
// [ts] let priorFocusedElem = priorSelectionInformation.focusedElem;
// [ts] let priorSelectionRange = priorSelectionInformation.selectionRange;
// [ts] if (curFocusedElem != priorFocusedElem && isInDocument(priorFocusedElem)) {
// [ts] if (priorSelectionRange != null && hasSelectionCapabilities(priorFocusedElem)) {
// [ts] setSelection(priorFocusedElem, priorSelectionRange);
// [ts] } // Focusing a node can change the scroll position, which is undesirable
// [ts] let ancestors = [];
// [ts] let ancestor = priorFocusedElem;
// [ts] while (ancestor = ancestor.parentNode) {
// [ts] if (ancestor.nodeType == ELEMENT_NODE) {
// [ts] ancestors.push({
// [ts] element: ancestor,
// [ts] left: ancestor.scrollLeft,
// [ts] top: ancestor.scrollTop
// [ts] });
// [ts] }
// [ts] }
// [ts] if (@typeName(priorFocusedElem.focus) == "function") {
// [ts] priorFocusedElem.focus();
// [ts] }
// [ts] let i = 0;
// [ts] while (i < ancestors.length) { // [cap-migrate] for-step: i++
// [ts] let info = ancestors[i];
// [ts] info.element.scrollLeft = info.left;
// [ts] info.element.scrollTop = info.top;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * @getSelection: Gets the selection bounds of a focused textarea, input or
// [ts] * contentEditable node.
// [ts] * -@input: Look up selection bounds of this input
// [ts] * -@return {start: selectionStart, end: selectionEnd}
// [ts] */
// [ts] fn getSelection(input: auto) {
// [ts] let selection;
// [ts] if ("selectionStart" == input) {
// Modern browser with input or textarea.
// [ts] selection = {
// [ts] start: input.selectionStart,
// [ts] end: input.selectionEnd
// [ts] };
// [ts] } else {
// Content editable or old IE textarea.
// [ts] selection = getOffsets(input);
// [ts] }
// [ts] return selection || {
// [ts] start: 0,
// [ts] end: 0
// [ts] };
// [ts] }
// [ts] /**
// [ts] * @setSelection: Sets the selection bounds of a textarea or input and focuses
// [ts] * the input.
// [ts] * -@input     Set selection bounds of this input or textarea
// [ts] * -@offsets   Object of same form that is returned from get*
// [ts] */
// [ts] fn setSelection(input: auto, offsets: auto) {
// [ts] let start = offsets.start,
// [ts] end = offsets.end;
// [ts] if (end == null) {
// [ts] end = start;
// [ts] }
// [ts] if ("selectionStart" == input) {
// [ts] input.selectionStart = start;
// [ts] input.selectionEnd = Math.min(end, input.value.length);
// [ts] } else {
// [ts] setOffsets(input, offsets);
// [ts] }
// [ts] }
// [ts] let validateDOMNesting = () => {};
// [ts] let updatedAncestorInfo = () => {};
// [ts] {
// This validation code was written based on the HTML5 parsing spec:
// https://html.spec.whatwg.org/multipage/syntax.html#has-an-element-in-scope
//
// Note: this does not catch all invalid nesting, nor does it try to (as it's
// not clear what practical benefit doing so provides); instead, we warn only
// for cases where the parser will give a parse tree differing from what React
// intended. For example, <b><div></div></b> is invalid but we don't warn
// because it still parses correctly; we do warn for other cases like nested
// <p> tags where the beginning of the second element implicitly closes the
// first, causing a confusing mess.
// https://html.spec.whatwg.org/multipage/syntax.html#special
// [ts] let specialTags = ["address", "applet", "area", "article", "aside", "base", "basefont", "bgsound", "blockquote", "body", "br", "button", "caption", "center", "col", "colgroup", "dd", "details", "dir", "div", "dl", "dt", "embed", "fieldset", "figcaption", "figure", "footer", "form", "frame", "frameset", "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hgroup", "hr", "html", "iframe", "img", "input", "isindex", "li", "link", "listing", "main", "marquee", "menu", "menuitem", "meta", "nav", "noembed", "noframes", "noscript", "object", "ol", "p", "param", "plaintext", "pre", "script", "section", "select", "source", "style", "summary", "table", "tbody", "td", "template", "textarea", "tfoot", "th", "thead", "title", "tr", "track", "ul", "wbr", "xmp"]; // https://html.spec.whatwg.org/multipage/syntax.html#has-an-element-in-scope
// [ts] let inScopeTags = ["applet", "caption", "html", "table", "td", "th", "marquee", "object", "template", // https://html.spec.whatwg.org/multipage/syntax.html#html-integration-point
// TODO: Distinguish by namespace here -- for <title>, including it here
// errs on the side of fewer warnings
// [ts] "foreignObject", "desc", "title"]; // https://html.spec.whatwg.org/multipage/syntax.html#has-an-element-in-button-scope
// [ts] let buttonScopeTags = inScopeTags.concat(["button"]); // https://html.spec.whatwg.org/multipage/syntax.html#generate-implied-end-tags
// [ts] let impliedEndTags = ["dd", "dt", "li", "option", "optgroup", "p", "rp", "rt"];
// [ts] let emptyAncestorInfo = {
// [ts] current: null,
// [ts] formTag: null,
// [ts] aTagInScope: null,
// [ts] buttonTagInScope: null,
// [ts] nobrTagInScope: null,
// [ts] pTagInButtonScope: null,
// [ts] listItemTagAutoclosing: null,
// [ts] dlItemTagAutoclosing: null
// [ts] };
// [ts] updatedAncestorInfo = (oldInfo, tag) => {
// [ts] let ancestorInfo = _assign({}, oldInfo || emptyAncestorInfo);
// [ts] let info = {
// [ts] tag: tag
// [ts] };
// [ts] if (inScopeTags.indexOf(tag) != -1) {
// [ts] ancestorInfo.aTagInScope = null;
// [ts] ancestorInfo.buttonTagInScope = null;
// [ts] ancestorInfo.nobrTagInScope = null;
// [ts] }
// [ts] if (buttonScopeTags.indexOf(tag) != -1) {
// [ts] ancestorInfo.pTagInButtonScope = null;
// [ts] } // See rules for 'li', 'dd', 'dt' start tags in
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-inbody
// [ts] if (specialTags.indexOf(tag) != -1 && tag != "address" && tag != "div" && tag != "p") {
// [ts] ancestorInfo.listItemTagAutoclosing = null;
// [ts] ancestorInfo.dlItemTagAutoclosing = null;
// [ts] }
// [ts] ancestorInfo.current = info;
// [ts] if (tag == "form") {
// [ts] ancestorInfo.formTag = info;
// [ts] }
// [ts] if (tag == "a") {
// [ts] ancestorInfo.aTagInScope = info;
// [ts] }
// [ts] if (tag == "button") {
// [ts] ancestorInfo.buttonTagInScope = info;
// [ts] }
// [ts] if (tag == "nobr") {
// [ts] ancestorInfo.nobrTagInScope = info;
// [ts] }
// [ts] if (tag == "p") {
// [ts] ancestorInfo.pTagInButtonScope = info;
// [ts] }
// [ts] if (tag == "li") {
// [ts] ancestorInfo.listItemTagAutoclosing = info;
// [ts] }
// [ts] if (tag == "dd" || tag == "dt") {
// [ts] ancestorInfo.dlItemTagAutoclosing = info;
// [ts] }
// [ts] return ancestorInfo;
// [ts] };
// [ts] /**
// [ts] * Returns whether
// [ts] */
// [ts] let isTagValidWithParent = (tag, parentTag) => {
// First, let's check if we're in an unusual parsing mode...
// [ts] switch (parentTag) {
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-inselect
// [ts] "select" => {
// [ts] return tag == "option" || tag == "optgroup" || tag == "#text";
// [ts] },
// [ts] "optgroup" => {
// [ts] return tag == "option" || tag == "#text";
// Strictly speaking, seeing an <option> doesn't mean we're in a <select>
// but
// [ts] },
// [ts] "option" => {
// [ts] return tag == "#text";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-intd
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-incaption
// No special behavior since these rules fall back to "in body" mode for
// all except special table nodes which cause bad parsing behavior anyway.
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-intr
// [ts] },
// [ts] "tr" => {
// [ts] return tag == "th" || tag == "td" || tag == "style" || tag == "script" || tag == "template";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-intbody
// [ts] },
// [ts] "tbody", "thead", "tfoot" => {
// [ts] return tag == "tr" || tag == "style" || tag == "script" || tag == "template";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-incolgroup
// [ts] },
// [ts] "colgroup" => {
// [ts] return tag == "col" || tag == "template";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-intable
// [ts] },
// [ts] "table" => {
// [ts] return tag == "caption" || tag == "colgroup" || tag == "tbody" || tag == "tfoot" || tag == "thead" || tag == "style" || tag == "script" || tag == "template";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-inhead
// [ts] },
// [ts] "head" => {
// [ts] return tag == "base" || tag == "basefont" || tag == "bgsound" || tag == "link" || tag == "meta" || tag == "title" || tag == "noscript" || tag == "noframes" || tag == "style" || tag == "script" || tag == "template";
// https://html.spec.whatwg.org/multipage/semantics.html#the-html-element
// [ts] },
// [ts] "html" => {
// [ts] return tag == "head" || tag == "body" || tag == "frameset";
// [ts] },
// [ts] "frameset" => {
// [ts] return tag == "frame";
// [ts] },
// [ts] "#document" => {
// [ts] return tag == "html";
// [ts] } // Probably in the "in body" parsing mode, so we outlaw only tag combos
// where the parsing rules cause implicit opens or closes to be added.
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-inbody
// [ts] switch (tag) {
// [ts] "h1", "h2", "h3", "h4", "h5", "h6" => {
// [ts] return parentTag != "h1" && parentTag != "h2" && parentTag != "h3" && parentTag != "h4" && parentTag != "h5" && parentTag != "h6";
// [ts] },
// [ts] "rp", "rt" => {
// [ts] return impliedEndTags.indexOf(parentTag) == -1;
// [ts] },
// These tags are only valid with a few parents that have special child
// parsing rules -- if we're down here, then none of those matched and
// so we allow it only if we don't know what the parent is, as all other
// cases are invalid.
// [ts] "body", "caption", "col", "colgroup", "frameset", "frame", "head", "html", "tbody", "td", "tfoot", "th", "thead", "tr" => {
// [ts] return parentTag == null;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] };
// [ts] /**
// [ts] * Returns whether
// [ts] */
// [ts] let findInvalidAncestorForTag = (tag, ancestorInfo) => {
// [ts] switch (tag) {
// [ts] "address", "article", "aside", "blockquote", "center", "details", "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "header", "hgroup", "main", "menu", "nav", "ol", "p", "section", "summary", "ul", "pre", "listing", "table", "hr", "xmp", "h1", "h2", "h3", "h4", "h5", "h6" => {
// [ts] return ancestorInfo.pTagInButtonScope;
// [ts] },
// [ts] "form" => {
// [ts] return ancestorInfo.formTag || ancestorInfo.pTagInButtonScope;
// [ts] },
// [ts] "li" => {
// [ts] return ancestorInfo.listItemTagAutoclosing;
// [ts] },
// [ts] "dd", "dt" => {
// [ts] return ancestorInfo.dlItemTagAutoclosing;
// [ts] },
// [ts] "button" => {
// [ts] return ancestorInfo.buttonTagInScope;
// [ts] },
// Spec says something about storing a list of markers, but it sounds
// equivalent to this check.
// [ts] "a" => {
// [ts] return ancestorInfo.aTagInScope;
// [ts] },
// [ts] "nobr" => {
// [ts] return ancestorInfo.nobrTagInScope;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] };
// [ts] let didWarn$1 = {};
// [ts] validateDOMNesting = (childTag, childText, ancestorInfo) => {
// [ts] ancestorInfo = ancestorInfo || emptyAncestorInfo;
// [ts] let parentInfo = ancestorInfo.current;
// [ts] let parentTag = parentInfo && parentInfo.tag;
// [ts] if (childText != null) {
// [ts] if (childTag != null) {
// [ts] error("validateDOMNesting: when childText is passed, childTag should be null");
// [ts] }
// [ts] childTag = "#text";
// [ts] }
// [ts] }
// [ts] let invalidParent = if (isTagValidWithParent(childTag, parentTag)) null else parentInfo;
// [ts] let invalidAncestor = if (invalidParent) null else findInvalidAncestorForTag(childTag, ancestorInfo);
// [ts] let invalidParentOrAncestor = invalidParent || invalidAncestor;
// [ts] if (!invalidParentOrAncestor) {
// [ts] return;
// [ts] }
// [ts] let ancestorTag = invalidParentOrAncestor.tag;
// [ts] let addendum = getCurrentFiberStackInDev();
// [ts] let warnKey = !!invalidParent + "|" + childTag + "|" + ancestorTag + "|" + addendum;
// [ts] if (didWarn$1[warnKey]) {
// [ts] return;
// [ts] }
// [ts] didWarn$1[warnKey] = true;
// [ts] let tagDisplayName = childTag;
// [ts] let whitespaceInfo = "";
// [ts] if (childTag == "#text") {
// [ts] if ("\\S".test(childText)) {
// [ts] tagDisplayName = "Text nodes";
// [ts] } else {
// [ts] tagDisplayName = "Whitespace text nodes";
// [ts] whitespaceInfo = " Make sure you don't have any extra whitespace between tags on " + "each line of your source code.";
// [ts] }
// [ts] } else {
// [ts] tagDisplayName = "<" + childTag + ">";
// [ts] }
// [ts] if (invalidParent) {
// [ts] let info = "";
// [ts] if (ancestorTag == "table" && childTag == "tr") {
// [ts] info += " Add a <tbody>, <thead> or <tfoot> to your code to match the DOM tree generated by " + "the browser.";
// [ts] }
// [ts] error("validateDOMNesting(...): %s cannot appear as a child of <%s>.%s%s", tagDisplayName, ancestorTag, whitespaceInfo, info);
// [ts] } else {
// [ts] error("validateDOMNesting(...): %s cannot appear as a descendant of " + "<%s>.", tagDisplayName, ancestorTag);
// [ts] }
// [ts] };
// [ts] }
// [ts] let SUPPRESS_HYDRATION_WARNING$1;
// [ts] {
// [ts] SUPPRESS_HYDRATION_WARNING$1 = "suppressHydrationWarning";
// [ts] }
// [ts] let SUSPENSE_START_DATA = "$";
// [ts] let SUSPENSE_END_DATA = "/$";
// [ts] let SUSPENSE_PENDING_START_DATA = "$?";
// [ts] let SUSPENSE_FALLBACK_START_DATA = "$!";
// [ts] let STYLE$1 = "style";
// [ts] let eventsEnabled = null;
// [ts] let selectionInformation = null;
// [ts] fn shouldAutoFocusHostComponent(type: auto, props: auto) {
// [ts] switch (type) {
// [ts] "button", "input", "select", "textarea" => {
// [ts] return !!props.autoFocus;
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn getRootHostContext(rootContainerInstance: auto) {
// [ts] let type;
// [ts] let namespace;
// [ts] let nodeType = rootContainerInstance.nodeType;
// [ts] switch (nodeType) {
// [ts] DOCUMENT_NODE, DOCUMENT_FRAGMENT_NODE => {
// [ts] {
// [ts] type = if (nodeType == DOCUMENT_NODE) "#document" else "#fragment";
// [ts] let root = rootContainerInstance.documentElement;
// [ts] namespace = if (root) root.namespaceURI else getChildNamespace(null, "");
// [ts] break;
// [ts] }
// [ts] },
// [ts] else => {
// [ts] {
// [ts] let container = if (nodeType == COMMENT_NODE) rootContainerInstance.parentNode else rootContainerInstance;
// [ts] let ownNamespace = container.namespaceURI || null;
// [ts] type = container.tagName;
// [ts] namespace = getChildNamespace(ownNamespace, type);
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] let validatedTag = type.toLowerCase();
// [ts] let ancestorInfo = updatedAncestorInfo(null, validatedTag);
// [ts] return {
// [ts] namespace: namespace,
// [ts] ancestorInfo: ancestorInfo
// [ts] };
// [ts] }
// [ts] }
// [ts] fn getChildHostContext(parentHostContext: auto, type: auto, rootContainerInstance: auto) {
// [ts] {
// [ts] let parentHostContextDev = parentHostContext;
// [ts] let namespace = getChildNamespace(parentHostContextDev.namespace, type);
// [ts] let ancestorInfo = updatedAncestorInfo(parentHostContextDev.ancestorInfo, type);
// [ts] return {
// [ts] namespace: namespace,
// [ts] ancestorInfo: ancestorInfo
// [ts] };
// [ts] }
// [ts] }
// [ts] fn getPublicInstance(instance: auto) {
// [ts] return instance;
// [ts] }
// [ts] fn prepareForCommit(containerInfo: auto) {
// [ts] eventsEnabled = isEnabled();
// [ts] selectionInformation = getSelectionInformation();
// [ts] setEnabled(false);
// [ts] }
// [ts] fn resetAfterCommit(containerInfo: auto) {
// [ts] restoreSelection(selectionInformation);
// [ts] setEnabled(eventsEnabled);
// [ts] eventsEnabled = null;
// [ts] selectionInformation = null;
// [ts] }
// [ts] fn createInstance(type: auto, props: auto, rootContainerInstance: auto, hostContext: auto, internalInstanceHandle: auto) {
// [ts] let parentNamespace;
// [ts] {
// TODO: take namespace into account when validating.
// [ts] let hostContextDev = hostContext;
// [ts] validateDOMNesting(type, null, hostContextDev.ancestorInfo);
// [ts] if (@typeName(props.children) == "string" || @typeName(props.children) == "number") {
// [ts] let string = "" + props.children;
// [ts] let ownAncestorInfo = updatedAncestorInfo(hostContextDev.ancestorInfo, type);
// [ts] validateDOMNesting(null, string, ownAncestorInfo);
// [ts] }
// [ts] parentNamespace = hostContextDev.namespace;
// [ts] }
// [ts] let domElement = createElement(type, props, rootContainerInstance, parentNamespace);
// [ts] precacheFiberNode(internalInstanceHandle, domElement);
// [ts] updateFiberProps(domElement, props);
// [ts] return domElement;
// [ts] }
// [ts] fn appendInitialChild(parentInstance: auto, child: auto) {
// [ts] parentInstance.appendChild(child);
// [ts] }
// [ts] fn finalizeInitialChildren(domElement: auto, type: auto, props: auto, rootContainerInstance: auto, hostContext: auto) {
// [ts] setInitialProperties(domElement, type, props, rootContainerInstance);
// [ts] return shouldAutoFocusHostComponent(type, props);
// [ts] }
// [ts] fn prepareUpdate(domElement: auto, type: auto, oldProps: auto, newProps: auto, rootContainerInstance: auto, hostContext: auto) {
// [ts] {
// [ts] let hostContextDev = hostContext;
// [ts] if (@typeName(newProps.children) != @typeName(oldProps.children) && (@typeName(newProps.children) == "string" || @typeName(newProps.children) == "number")) {
// [ts] let string = "" + newProps.children;
// [ts] let ownAncestorInfo = updatedAncestorInfo(hostContextDev.ancestorInfo, type);
// [ts] validateDOMNesting(null, string, ownAncestorInfo);
// [ts] }
// [ts] }
// [ts] return diffProperties(domElement, type, oldProps, newProps, rootContainerInstance);
// [ts] }
// [ts] fn shouldSetTextContent(type: auto, props: auto) {
// [ts] return type == "textarea" || type == "option" || type == "noscript" || @typeName(props.children) == "string" || @typeName(props.children) == "number" || @typeName(props.dangerouslySetInnerHTML) == "object" && props.dangerouslySetInnerHTML != null && props.dangerouslySetInnerHTML.__html != null;
// [ts] }
// [ts] fn shouldDeprioritizeSubtree(type: auto, props: auto) {
// [ts] return !!props.hidden;
// [ts] }
// [ts] fn createTextInstance(text: auto, rootContainerInstance: auto, hostContext: auto, internalInstanceHandle: auto) {
// [ts] {
// [ts] let hostContextDev = hostContext;
// [ts] validateDOMNesting(null, text, hostContextDev.ancestorInfo);
// [ts] }
// [ts] let textNode = createTextNode(text, rootContainerInstance);
// [ts] precacheFiberNode(internalInstanceHandle, textNode);
// [ts] return textNode;
// [ts] }
// if a component just imports ReactDOM (e.g. for findDOMNode).
// Some environments might not have setTimeout or clearTimeout.
// [ts] let scheduleTimeout = if (@typeName(setTimeout) == "function") setTimeout else null;
// [ts] let cancelTimeout = if (@typeName(clearTimeout) == "function") clearTimeout else null;
// [ts] let noTimeout = -1; // -------------------
// [ts] fn commitMount(domElement: auto, type: auto, newProps: auto, internalInstanceHandle: auto) {
// Despite the naming that might imply otherwise, this method only
// fires if there is an `Update` effect scheduled during mounting.
// This happens if `finalizeInitialChildren` returns `true` (which it
// does to implement the `autoFocus` attribute on the client). But
// there are also other cases when this might happen (such as patching
// up text content during hydration mismatch). So we'll check this again.
// [ts] if (shouldAutoFocusHostComponent(type, newProps)) {
// [ts] domElement.focus();
// [ts] }
// [ts] }
// [ts] fn commitUpdate(domElement: auto, updatePayload: auto, type: auto, oldProps: auto, newProps: auto, internalInstanceHandle: auto) {
// Update the props handle so that we know which props are the ones with
// with current event handlers.
// [ts] updateFiberProps(domElement, newProps); // Apply the diff to the DOM node.
// [ts] updateProperties(domElement, updatePayload, type, oldProps, newProps);
// [ts] }
// [ts] fn resetTextContent(domElement: auto) {
// [ts] setTextContent(domElement, "");
// [ts] }
// [ts] fn commitTextUpdate(textInstance: auto, oldText: auto, newText: auto) {
// [ts] textInstance.nodeValue = newText;
// [ts] }
// [ts] fn appendChild(parentInstance: auto, child: auto) {
// [ts] parentInstance.appendChild(child);
// [ts] }
// [ts] fn appendChildToContainer(container: auto, child: auto) {
// [ts] let parentNode;
// [ts] if (container.nodeType == COMMENT_NODE) {
// [ts] parentNode = container.parentNode;
// [ts] parentNode.insertBefore(child, container);
// [ts] } else {
// [ts] parentNode = container;
// [ts] parentNode.appendChild(child);
// [ts] } // This container might be used for a portal.
// If something inside a portal is clicked, that click should bubble
// through the React tree. However, on Mobile Safari the click would
// never bubble through the *DOM* tree unless an ancestor with onclick
// event exists. So we wouldn't see it and dispatch it.
// This is why we ensure that non React root containers have inline onclick
// defined.
// https://github.com/facebook/react/issues/11918
// [ts] let reactRootContainer = container._reactRootContainer;
// [ts] if ((reactRootContainer == null || reactRootContainer == null) && parentNode.onclick == null) {
// TODO: This cast may not be sound for SVG, MathML or custom elements.
// [ts] trapClickOnNonInteractiveElement(parentNode);
// [ts] }
// [ts] }
// [ts] fn insertBefore(parentInstance: auto, child: auto, beforeChild: auto) {
// [ts] parentInstance.insertBefore(child, beforeChild);
// [ts] }
// [ts] fn insertInContainerBefore(container: auto, child: auto, beforeChild: auto) {
// [ts] if (container.nodeType == COMMENT_NODE) {
// [ts] container.parentNode.insertBefore(child, beforeChild);
// [ts] } else {
// [ts] container.insertBefore(child, beforeChild);
// [ts] }
// [ts] }
// [ts] fn removeChild(parentInstance: auto, child: auto) {
// [ts] parentInstance.removeChild(child);
// [ts] }
// [ts] fn removeChildFromContainer(container: auto, child: auto) {
// [ts] if (container.nodeType == COMMENT_NODE) {
// [ts] container.parentNode.removeChild(child);
// [ts] } else {
// [ts] container.removeChild(child);
// [ts] }
// [ts] }
// [ts] fn hideInstance(instance: auto) {
// pass host context to this method?
// [ts] instance = instance;
// [ts] let style = instance.style;
// [ts] if (@typeName(style.setProperty) == "function") {
// [ts] style.setProperty("display", "none", "important");
// [ts] } else {
// [ts] style.display = "none";
// [ts] }
// [ts] }
// [ts] fn hideTextInstance(textInstance: auto) {
// [ts] textInstance.nodeValue = "";
// [ts] }
// [ts] fn unhideInstance(instance: auto, props: auto) {
// [ts] instance = instance;
// [ts] let styleProp = props[STYLE$1];
// [ts] let display = if (styleProp != null && styleProp != null && styleProp.hasOwnProperty("display")) styleProp.display else null;
// [ts] instance.style.display = dangerousStyleValue("display", display);
// [ts] }
// [ts] fn unhideTextInstance(textInstance: auto, text: auto) {
// [ts] textInstance.nodeValue = text;
// [ts] } // -------------------
// [ts] fn canHydrateInstance(instance: auto, type: auto, props: auto) {
// [ts] if (instance.nodeType != ELEMENT_NODE || type.toLowerCase() != instance.nodeName.toLowerCase()) {
// [ts] return null;
// [ts] } // This has now been refined to an element node.
// [ts] return instance;
// [ts] }
// [ts] fn canHydrateTextInstance(instance: auto, text: auto) {
// [ts] if (text == "" || instance.nodeType != TEXT_NODE) {
// Empty strings are not parsed by HTML so there won't be a correct match here.
// [ts] return null;
// [ts] } // This has now been refined to a text node.
// [ts] return instance;
// [ts] }
// [ts] fn isSuspenseInstancePending(instance: auto) {
// [ts] return instance.data == SUSPENSE_PENDING_START_DATA;
// [ts] }
// [ts] fn isSuspenseInstanceFallback(instance: auto) {
// [ts] return instance.data == SUSPENSE_FALLBACK_START_DATA;
// [ts] }
// [ts] fn getNextHydratable(node: auto) {
// Skip non-hydratable nodes.
// [ts] while (node != null) { // [cap-migrate] for-step: node = node.nextSibling
// [ts] let nodeType = node.nodeType;
// [ts] if (nodeType == ELEMENT_NODE || nodeType == TEXT_NODE) {
// [ts] break;
// [ts] }
// [ts] }
// [ts] return node;
// [ts] }
// [ts] fn getNextHydratableSibling(instance: auto) {
// [ts] return getNextHydratable(instance.nextSibling);
// [ts] }
// [ts] fn getFirstHydratableChild(parentInstance: auto) {
// [ts] return getNextHydratable(parentInstance.firstChild);
// [ts] }
// [ts] fn hydrateInstance(instance: auto, type: auto, props: auto, rootContainerInstance: auto, hostContext: auto, internalInstanceHandle: auto) {
// [ts] precacheFiberNode(internalInstanceHandle, instance); // TODO: Possibly defer this until the commit phase where all the events
// get attached.
// [ts] updateFiberProps(instance, props);
// [ts] let parentNamespace;
// [ts] {
// [ts] let hostContextDev = hostContext;
// [ts] parentNamespace = hostContextDev.namespace;
// [ts] }
// [ts] return diffHydratedProperties(instance, type, props, parentNamespace, rootContainerInstance);
// [ts] }
// [ts] fn hydrateTextInstance(textInstance: auto, text: auto, internalInstanceHandle: auto) {
// [ts] precacheFiberNode(internalInstanceHandle, textInstance);
// [ts] return diffHydratedText(textInstance, text);
// [ts] }
// [ts] fn getNextHydratableInstanceAfterSuspenseInstance(suspenseInstance: auto) {
// [ts] let node = suspenseInstance.nextSibling; // Skip past all nodes within this suspense boundary.
// There might be nested nodes so we need to keep track of how
// deep we are and only break out when we're back on top.
// [ts] let depth = 0;
// [ts] while (node) {
// [ts] if (node.nodeType == COMMENT_NODE) {
// [ts] let data = node.data;
// [ts] if (data == SUSPENSE_END_DATA) {
// [ts] if (depth == 0) {
// [ts] return getNextHydratableSibling(node);
// [ts] } else {
// [ts] depth--;
// [ts] }
// [ts] } else if (data == SUSPENSE_START_DATA || data == SUSPENSE_FALLBACK_START_DATA || data == SUSPENSE_PENDING_START_DATA) {
// [ts] depth++;
// [ts] }
// [ts] }
// [ts] node = node.nextSibling;
// [ts] } // TODO: Warn, we didn't find the end comment boundary.
// [ts] return null;
// [ts] } // Returns the SuspenseInstance if this node is a direct child of a
// SuspenseInstance. I.e. if its previous sibling is a Comment with
// SUSPENSE_x_START_DATA. Otherwise, null.
// [ts] fn getParentSuspenseInstance(targetInstance: auto) {
// [ts] let node = targetInstance.previousSibling; // Skip past all nodes within this suspense boundary.
// There might be nested nodes so we need to keep track of how
// deep we are and only break out when we're back on top.
// [ts] let depth = 0;
// [ts] while (node) {
// [ts] if (node.nodeType == COMMENT_NODE) {
// [ts] let data = node.data;
// [ts] if (data == SUSPENSE_START_DATA || data == SUSPENSE_FALLBACK_START_DATA || data == SUSPENSE_PENDING_START_DATA) {
// [ts] if (depth == 0) {
// [ts] return node;
// [ts] } else {
// [ts] depth--;
// [ts] }
// [ts] } else if (data == SUSPENSE_END_DATA) {
// [ts] depth++;
// [ts] }
// [ts] }
// [ts] node = node.previousSibling;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn commitHydratedContainer(container: auto) {
// Retry if any event replaying was blocked on this.
// [ts] retryIfBlockedOn(container);
// [ts] }
// [ts] fn commitHydratedSuspenseInstance(suspenseInstance: auto) {
// Retry if any event replaying was blocked on this.
// [ts] retryIfBlockedOn(suspenseInstance);
// [ts] }
// [ts] fn didNotMatchHydratedContainerTextInstance(parentContainer: auto, textInstance: auto, text: auto) {
// [ts] {
// [ts] warnForUnmatchedText(textInstance, text);
// [ts] }
// [ts] }
// [ts] fn didNotMatchHydratedTextInstance(parentType: auto, parentProps: auto, parentInstance: auto, textInstance: auto, text: auto) {
// [ts] if ( parentProps[SUPPRESS_HYDRATION_WARNING$1] != true) {
// [ts] warnForUnmatchedText(textInstance, text);
// [ts] }
// [ts] }
// [ts] fn didNotHydrateContainerInstance(parentContainer: auto, instance: auto) {
// [ts] {
// [ts] if (instance.nodeType == ELEMENT_NODE) {
// [ts] warnForDeletedHydratableElement(parentContainer, instance);
// [ts] } else if (instance.nodeType == COMMENT_NODE) ; else {
// [ts] warnForDeletedHydratableText(parentContainer, instance);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn didNotHydrateInstance(parentType: auto, parentProps: auto, parentInstance: auto, instance: auto) {
// [ts] if ( parentProps[SUPPRESS_HYDRATION_WARNING$1] != true) {
// [ts] if (instance.nodeType == ELEMENT_NODE) {
// [ts] warnForDeletedHydratableElement(parentInstance, instance);
// [ts] } else if (instance.nodeType == COMMENT_NODE) ; else {
// [ts] warnForDeletedHydratableText(parentInstance, instance);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn didNotFindHydratableContainerInstance(parentContainer: auto, type: auto, props: auto) {
// [ts] {
// [ts] warnForInsertedHydratedElement(parentContainer, type);
// [ts] }
// [ts] }
// [ts] fn didNotFindHydratableContainerTextInstance(parentContainer: auto, text: auto) {
// [ts] {
// [ts] warnForInsertedHydratedText(parentContainer, text);
// [ts] }
// [ts] }
// [ts] fn didNotFindHydratableInstance(parentType: auto, parentProps: auto, parentInstance: auto, type: auto, props: auto) {
// [ts] if ( parentProps[SUPPRESS_HYDRATION_WARNING$1] != true) {
// [ts] warnForInsertedHydratedElement(parentInstance, type);
// [ts] }
// [ts] }
// [ts] fn didNotFindHydratableTextInstance(parentType: auto, parentProps: auto, parentInstance: auto, text: auto) {
// [ts] if ( parentProps[SUPPRESS_HYDRATION_WARNING$1] != true) {
// [ts] warnForInsertedHydratedText(parentInstance, text);
// [ts] }
// [ts] }
// [ts] fn didNotFindHydratableSuspenseInstance(parentType: auto, parentProps: auto, parentInstance: auto) {
// [ts] if ( parentProps[SUPPRESS_HYDRATION_WARNING$1] != true) { ; }
// [ts] }
// [ts] let randomKey = Math.random().toString(36).slice(2);
// [ts] let internalInstanceKey = "__reactInternalInstance$" + randomKey;
// [ts] let internalEventHandlersKey = "__reactEventHandlers$" + randomKey;
// [ts] let internalContainerInstanceKey = "__reactContainere$" + randomKey;
// [ts] fn precacheFiberNode(hostInst: auto, node: auto) {
// [ts] node[internalInstanceKey] = hostInst;
// [ts] }
// [ts] fn markContainerAsRoot(hostRoot: auto, node: auto) {
// [ts] node[internalContainerInstanceKey] = hostRoot;
// [ts] }
// [ts] fn unmarkContainerAsRoot(node: auto) {
// [ts] node[internalContainerInstanceKey] = null;
// [ts] }
// [ts] fn isContainerMarkedAsRoot(node: auto) {
// [ts] return !!node[internalContainerInstanceKey];
// [ts] } // Given a DOM node, return the closest HostComponent or HostText fiber ancestor.
// If the target node is part of a hydrated or not yet rendered subtree, then
// this may also return a SuspenseComponent or HostRoot to indicate that.
// Conceptually the HostRoot fiber is a child of the Container node. So if you
// pass the Container node as the targetNode, you will not actually get the
// HostRoot back. To get to the HostRoot, you need to pass a child of it.
// The same thing applies to Suspense boundaries.
// [ts] fn getClosestInstanceFromNode(targetNode: auto) {
// [ts] let targetInst = targetNode[internalInstanceKey];
// [ts] if (targetInst) {
// Don't return HostRoot or SuspenseComponent here.
// [ts] return targetInst;
// [ts] } // If the direct event target isn't a React owned DOM node, we need to look
// to see if one of its parents is a React owned DOM node.
// [ts] let parentNode = targetNode.parentNode;
// [ts] while (parentNode) {
// We'll check if this is a container root that could include
// React nodes in the future. We need to check this first because
// if we're a child of a dehydrated container, we need to first
// find that inner container before moving on to finding the parent
// instance. Note that we don't check this field on  the targetNode
// itself because the fibers are conceptually between the container
// node and the first child. It isn't surrounding the container node.
// If it's not a container, we check if it's an instance.
// [ts] targetInst = parentNode[internalContainerInstanceKey] || parentNode[internalInstanceKey];
// [ts] if (targetInst) {
// Since this wasn't the direct target of the event, we might have
// stepped past dehydrated DOM nodes to get here. However they could
// also have been non-React nodes. We need to answer which one.
// If we the instance doesn't have any children, then there can't be
// a nested suspense boundary within it. So we can use this as a fast
// bailout. Most of the time, when people add non-React children to
// the tree, it is using a ref to a child-less DOM node.
// Normally we'd only need to check one of the fibers because if it
// has ever gone from having children to deleting them or vice versa
// it would have deleted the dehydrated boundary nested inside already.
// However, since the HostRoot starts out with an alternate it might
// have one on the alternate so we need to check in case this was a
// root.
// [ts] let alternate = targetInst.alternate;
// [ts] if (targetInst.child != null || alternate != null && alternate.child != null) {
// Next we need to figure out if the node that skipped past is
// nested within a dehydrated boundary and if so, which one.
// [ts] let suspenseInstance = getParentSuspenseInstance(targetNode);
// [ts] while (suspenseInstance != null) {
// We found a suspense instance. That means that we haven't
// hydrated it yet. Even though we leave the comments in the
// DOM after hydrating, and there are boundaries in the DOM
// that could already be hydrated, we wouldn't have found them
// through this pass since if the target is hydrated it would
// have had an internalInstanceKey on it.
// Let's get the fiber associated with the SuspenseComponent
// as the deepest instance.
// [ts] let targetSuspenseInst = suspenseInstance[internalInstanceKey];
// [ts] if (targetSuspenseInst) {
// [ts] return targetSuspenseInst;
// [ts] } // If we don't find a Fiber on the comment, it might be because
// we haven't gotten to hydrate it yet. There might still be a
// parent boundary that hasn't above this one so we need to find
// the outer most that is known.
// [ts] suspenseInstance = getParentSuspenseInstance(suspenseInstance); // If we don't find one, then that should mean that the parent
// host component also hasn't hydrated yet. We can return it
// below since it will bail out on the isMounted check later.
// [ts] }
// [ts] }
// [ts] return targetInst;
// [ts] }
// [ts] targetNode = parentNode;
// [ts] parentNode = targetNode.parentNode;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * Given a DOM node, return the ReactDOMComponent or ReactDOMTextComponent
// [ts] * instance, or null if the node was not rendered by this React.
// [ts] */
// [ts] (node) => {
// [ts] let inst = node[internalInstanceKey] || node[internalContainerInstanceKey];
// [ts] if (inst) {
// [ts] if (inst.tag == HostComponent || inst.tag == HostText || inst.tag == SuspenseComponent || inst.tag == HostRoot) {
// [ts] return inst;
// [ts] } else {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * Given a ReactDOMComponent or ReactDOMTextComponent, return the corresponding
// [ts] * DOM node.
// [ts] */
// [ts] (inst) => {
// [ts] if (inst.tag == HostComponent || inst.tag == HostText) {
// In Fiber this, is just the state node right now. We assume it will be
// a host component or host text.
// [ts] return inst.stateNode;
// [ts] } // Without this first invariant, passing a non-DOM-component triggers the next
// invariant for a missing parent, which is super confusing.
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "getNodeFromInstance: Invalid argument." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] (node) => {
// [ts] return node[internalEventHandlersKey] || null;
// [ts] }
// [ts] fn updateFiberProps(node: auto, props: auto) {
// [ts] node[internalEventHandlersKey] = props;
// [ts] }
// [ts] fn getParent(inst: auto) {
// [ts] do {
// [ts] inst = inst.return; // TODO: If this is a HostRoot we might want to bail out.
// That is depending on if we want nested subtrees (layers) to bubble
// events to their parent. We could also go through parentNode on the
// host node but that wouldn't work for React Native and doesn't let us
// do the portal feature.
// [ts] } while (inst && inst.tag != HostComponent);
// [ts] if (inst) {
// [ts] return inst;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * Return the lowest common ancestor of A and B, or null if they are in
// [ts] * different trees.
// [ts] */
// [ts] fn getLowestCommonAncestor(instA: auto, instB: auto) {
// [ts] let depthA = 0;
// [ts] let tempA = instA;
// [ts] while (tempA) { // [cap-migrate] for-step: tempA = getParent(tempA)
// [ts] depthA++;
// [ts] }
// [ts] let depthB = 0;
// [ts] let tempB = instB;
// [ts] while (tempB) { // [cap-migrate] for-step: tempB = getParent(tempB)
// [ts] depthB++;
// [ts] } // If A is deeper, crawl up.
// [ts] while (depthA - depthB > 0) {
// [ts] instA = getParent(instA);
// [ts] depthA--;
// [ts] } // If B is deeper, crawl up.
// [ts] while (depthB - depthA > 0) {
// [ts] instB = getParent(instB);
// [ts] depthB--;
// [ts] } // Walk in lockstep until we find a match.
// [ts] let depth = depthA;
// [ts] while (depth--) {
// [ts] if (instA == instB || instA == instB.alternate) {
// [ts] return instA;
// [ts] }
// [ts] instA = getParent(instA);
// [ts] instB = getParent(instB);
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * Simulates the traversal of a two-phase, capture/bubble event dispatch.
// [ts] */
// [ts] fn traverseTwoPhase(inst: auto, fn: auto, arg: auto) {
// [ts] let path = [];
// [ts] while (inst) {
// [ts] path.push(inst);
// [ts] inst = getParent(inst);
// [ts] }
// [ts] let i;
// [ts] for (i = path.length; i-- > 0;) {
// [ts] fn(path[i], "captured", arg);
// [ts] }
// [ts] i = 0;
// [ts] while (i < path.length) { // [cap-migrate] for-step: i++
// [ts] fn(path[i], "bubbled", arg);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Traverses the ID hierarchy and invokes the supplied `cb` on any IDs that
// [ts] * should would receive a `mouseEnter` or `mouseLeave` event.
// [ts] *
// [ts] * Does not invoke the callback on the nearest common ancestor because nothing
// [ts] * "entered" or "left" that element.
// [ts] */
// [ts] fn traverseEnterLeave(from: auto, to: auto, fn: auto, argFrom: auto, argTo: auto) {
// [ts] let common = if (from && to) getLowestCommonAncestor(from, to) else null;
// [ts] let pathFrom = [];
// [ts] while (true) {
// [ts] if (!from) {
// [ts] break;
// [ts] }
// [ts] if (from == common) {
// [ts] break;
// [ts] }
// [ts] let alternate = from.alternate;
// [ts] if (alternate != null && alternate == common) {
// [ts] break;
// [ts] }
// [ts] pathFrom.push(from);
// [ts] from = getParent(from);
// [ts] }
// [ts] let pathTo = [];
// [ts] while (true) {
// [ts] if (!to) {
// [ts] break;
// [ts] }
// [ts] if (to == common) {
// [ts] break;
// [ts] }
// [ts] let _alternate = to.alternate;
// [ts] if (_alternate != null && _alternate == common) {
// [ts] break;
// [ts] }
// [ts] pathTo.push(to);
// [ts] to = getParent(to);
// [ts] }
// [ts] let i = 0;
// [ts] while (i < pathFrom.length) { // [cap-migrate] for-step: i++
// [ts] fn(pathFrom[i], "bubbled", argFrom);
// [ts] }
// [ts] for (let _i = pathTo.length; _i-- > 0;) {
// [ts] fn(pathTo[_i], "captured", argTo);
// [ts] }
// [ts] }
// [ts] fn isInteractive(tag: auto) {
// [ts] return tag == "button" || tag == "input" || tag == "select" || tag == "textarea";
// [ts] }
// [ts] fn shouldPreventMouseEvent(name: auto, type: auto, props: auto) {
// [ts] switch (name) {
// [ts] "onClick", "onClickCapture", "onDoubleClick", "onDoubleClickCapture", "onMouseDown", "onMouseDownCapture", "onMouseMove", "onMouseMoveCapture", "onMouseUp", "onMouseUpCapture", "onMouseEnter" => {
// [ts] return !!(props.disabled && isInteractive(type));
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * @param {object} inst The instance, which is the source of events.
// [ts] * @param {string} registrationName Name of listener (e.g. `onClick`).
// [ts] * @return {?function} The stored callback.
// [ts] */
// [ts] fn getListener(inst: auto, registrationName: auto) {
// [ts] let listener; // TODO: shouldPreventMouseEvent is DOM-specific and definitely should not
// live here; needs to be moved to a better place soon
// [ts] let stateNode = inst.stateNode;
// [ts] if (!stateNode) {
// Work in progress (ex: onload events in incremental mode).
// [ts] return null;
// [ts] }
// [ts] let props = getFiberCurrentPropsFromNode(stateNode);
// [ts] if (!props) {
// Work in progress.
// [ts] return null;
// [ts] }
// [ts] listener = props[registrationName];
// [ts] if (shouldPreventMouseEvent(registrationName, inst.type, props)) {
// [ts] return null;
// [ts] }
// [ts] if (!(!listener || @typeName(listener) == "function")) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected `" + registrationName + "` listener to be a function, instead got a value of `" + typeof listener + "` type." );)
// [ts] }
// [ts] }
// [ts] return listener;
// [ts] }
// [ts] /**
// [ts] * Some event types have a notion of different registration names for different
// [ts] * "phases" of propagation. This finds listeners by a given phase.
// [ts] */
// [ts] fn listenerAtPhase(inst: auto, event: auto, propagationPhase: auto) {
// [ts] let registrationName = event.dispatchConfig.phasedRegistrationNames[propagationPhase];
// [ts] return getListener(inst, registrationName);
// [ts] }
// [ts] /**
// [ts] * A small set of propagation patterns, each of which will accept a small amount
// [ts] * of information, and generate a set of "dispatch ready event objects" - which
// [ts] * are sets of events that have already been annotated with a set of dispatched
// [ts] * listener functions/ids. The API is designed this way to discourage these
// [ts] * propagation strategies from actually executing the dispatches, since we
// [ts] * always want to collect the entire set of dispatches before executing even a
// [ts] * single one.
// [ts] */
// [ts] /**
// [ts] * Tags a `SyntheticEvent` with dispatched listeners. Creating this function
// [ts] * here, allows us to not have to bind or create functions for each event.
// [ts] * Mutating the event's members allows us to not have to create a wrapping
// [ts] * "dispatch" object that pairs the event with the listener.
// [ts] */
// [ts] fn accumulateDirectionalDispatches(inst: auto, phase: auto, event: auto) {
// [ts] {
// [ts] if (!inst) {
// [ts] error("Dispatching inst must not be null");
// [ts] }
// [ts] }
// [ts] let listener = listenerAtPhase(inst, event, phase);
// [ts] if (listener) {
// [ts] event._dispatchListeners = accumulateInto(event._dispatchListeners, listener);
// [ts] event._dispatchInstances = accumulateInto(event._dispatchInstances, inst);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Collect dispatches (must be entirely collected before dispatching - see unit
// [ts] * tests). Lazily allocate the array to conserve memory.  We must loop through
// [ts] * each event and perform the traversal for each one. We cannot perform a
// [ts] * single traversal for the entire collection of events because each event may
// [ts] * have a different target.
// [ts] */
// [ts] fn accumulateTwoPhaseDispatchesSingle(event: auto) {
// [ts] if (event && event.dispatchConfig.phasedRegistrationNames) {
// [ts] traverseTwoPhase(event._targetInst, accumulateDirectionalDispatches, event);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Accumulates without regard to direction, does not look for phased
// [ts] * registration names. Same as `accumulateDirectDispatchesSingle` but without
// [ts] * requiring that the `dispatchMarker` be the same as the dispatched ID.
// [ts] */
// [ts] fn accumulateDispatches(inst: auto, ignoredDirection: auto, event: auto) {
// [ts] if (inst && event && event.dispatchConfig.registrationName) {
// [ts] let registrationName = event.dispatchConfig.registrationName;
// [ts] let listener = getListener(inst, registrationName);
// [ts] if (listener) {
// [ts] event._dispatchListeners = accumulateInto(event._dispatchListeners, listener);
// [ts] event._dispatchInstances = accumulateInto(event._dispatchInstances, inst);
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Accumulates dispatches on an `SyntheticEvent`, but only for the
// [ts] * `dispatchMarker`.
// [ts] * @param {SyntheticEvent} event
// [ts] */
// [ts] fn accumulateDirectDispatchesSingle(event: auto) {
// [ts] if (event && event.dispatchConfig.registrationName) {
// [ts] accumulateDispatches(event._targetInst, null, event);
// [ts] }
// [ts] }
// [ts] fn accumulateTwoPhaseDispatches(events: auto) {
// [ts] forEachAccumulated(events, accumulateTwoPhaseDispatchesSingle);
// [ts] }
// [ts] fn accumulateEnterLeaveDispatches(leave: auto, enter: auto, from: auto, to: auto) {
// [ts] traverseEnterLeave(from, to, accumulateDispatches, leave, enter);
// [ts] }
// [ts] fn accumulateDirectDispatches(events: auto) {
// [ts] forEachAccumulated(events, accumulateDirectDispatchesSingle);
// [ts] }
// [ts] /**
// [ts] * These variables store information about text content of a target node,
// [ts] * allowing comparison of content before and after a given event.
// [ts] *
// [ts] * Identify the node where selection currently begins, then observe
// [ts] * both its text content and its current position in the DOM. Since the
// [ts] * browser may natively replace the target node during composition, we can
// [ts] * use its position to find its replacement.
// [ts] *
// [ts] *
// [ts] */
// [ts] let root = null;
// [ts] let startText = null;
// [ts] let fallbackText = null;
// [ts] fn initialize(nativeEventTarget: auto) {
// [ts] root = nativeEventTarget;
// [ts] startText = getText();
// [ts] return true;
// [ts] }
// [ts] fn reset() {
// [ts] root = null;
// [ts] startText = null;
// [ts] fallbackText = null;
// [ts] }
// [ts] fn getData() {
// [ts] if (fallbackText) {
// [ts] return fallbackText;
// [ts] }
// [ts] let start;
// [ts] let startValue = startText;
// [ts] let startLength = startValue.length;
// [ts] let end;
// [ts] let endValue = getText();
// [ts] let endLength = endValue.length;
// [ts] start = 0;
// [ts] while (start < startLength) { // [cap-migrate] for-step: start++
// [ts] if (startValue[start] != endValue[start]) {
// [ts] break;
// [ts] }
// [ts] }
// [ts] let minEnd = startLength - start;
// [ts] end = 1;
// [ts] while (end <= minEnd) { // [cap-migrate] for-step: end++
// [ts] if (startValue[startLength - end] != endValue[endLength - end]) {
// [ts] break;
// [ts] }
// [ts] }
// [ts] let sliceTail = if (end > 1) 1 - end else null;
// [ts] fallbackText = endValue.slice(start, sliceTail);
// [ts] return fallbackText;
// [ts] }
// [ts] fn getText() {
// [ts] if ("value" == root) {
// [ts] return root.value;
// [ts] }
// [ts] return root.textContent;
// [ts] }
// [ts] let EVENT_POOL_SIZE = 10;
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let EventInterface = {
// [ts] type: null,
// [ts] target: null,
// currentTarget is set when dispatching; no use in copying it here
// [ts] currentTarget: () => {
// [ts] return null;
// [ts] },
// [ts] eventPhase: null,
// [ts] bubbles: null,
// [ts] cancelable: null,
// [ts] timeStamp: (event) => {
// [ts] return event.timeStamp || Date.now();
// [ts] },
// [ts] defaultPrevented: null,
// [ts] isTrusted: null
// [ts] };
// [ts] fn functionThatReturnsTrue() {
// [ts] return true;
// [ts] }
// [ts] fn functionThatReturnsFalse() {
// [ts] return false;
// [ts] }
// [ts] /**
// [ts] * Synthetic events are dispatched by event plugins, typically in response to a
// [ts] * top-level event delegation handler.
// [ts] *
// [ts] * These systems should generally use pooling to reduce the frequency of garbage
// [ts] * collection. The system should check `isPersistent` to determine whether the
// [ts] * event should be released into the pool after being dispatched. Users that
// [ts] * need a persisted event should invoke `persist`.
// [ts] *
// [ts] * Synthetic events (and subclasses) implement the DOM Level 3 Events API by
// [ts] * normalizing browser quirks. Subclasses do not necessarily have to implement a
// [ts] * DOM interface; custom application-specific events can also subclass this.
// [ts] *
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {*} targetInst Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @param {DOMEventTarget} nativeEventTarget Target node.
// [ts] */
// [ts] fn SyntheticEvent(dispatchConfig: auto, targetInst: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] {
// these have a getter/setter for warnings
// [ts] delete _.nativeEvent;
// [ts] delete _.preventDefault;
// [ts] delete _.stopPropagation;
// [ts] delete _.isDefaultPrevented;
// [ts] delete _.isPropagationStopped;
// [ts] }
// [ts] _.dispatchConfig = dispatchConfig;
// [ts] _._targetInst = targetInst;
// [ts] _.nativeEvent = nativeEvent;
// [ts] let Interface = _.constructor.Interface;
// [ts] for (let propName in Interface) {
// [ts] if (!Interface.hasOwnProperty(propName)) {
// [ts] continue;
// [ts] }
// [ts] {
// [ts] delete this[propName]; // this has a getter/setter for warnings
// [ts] }
// [ts] let normalize = Interface[propName];
// [ts] if (normalize) {
// [ts] this[propName] = normalize(nativeEvent);
// [ts] } else {
// [ts] if (propName == "target") {
// [ts] _.target = nativeEventTarget;
// [ts] } else {
// [ts] this[propName] = nativeEvent[propName];
// [ts] }
// [ts] }
// [ts] }
// [ts] let defaultPrevented = if (nativeEvent.defaultPrevented != null) nativeEvent.defaultPrevented else nativeEvent.returnValue == false;
// [ts] if (defaultPrevented) {
// [ts] _.isDefaultPrevented = functionThatReturnsTrue;
// [ts] } else {
// [ts] _.isDefaultPrevented = functionThatReturnsFalse;
// [ts] }
// [ts] _.isPropagationStopped = functionThatReturnsFalse;
// [ts] return this;
// [ts] }
// [ts] _assign(SyntheticEvent.prototype, {
// [ts] preventDefault: () => {
// [ts] _.defaultPrevented = true;
// [ts] let event = _.nativeEvent;
// [ts] if (!event) {
// [ts] return;
// [ts] }
// [ts] if (event.preventDefault) {
// [ts] event.preventDefault();
// [ts] } else if (@typeName(event.returnValue) != "unknown") {
// [ts] event.returnValue = false;
// [ts] }
// [ts] _.isDefaultPrevented = functionThatReturnsTrue;
// [ts] },
// [ts] stopPropagation: () => {
// [ts] let event = _.nativeEvent;
// [ts] if (!event) {
// [ts] return;
// [ts] }
// [ts] if (event.stopPropagation) {
// [ts] event.stopPropagation();
// [ts] } else if (@typeName(event.cancelBubble) != "unknown") {
// The ChangeEventPlugin registers a "propertychange" event for
// IE. This event does not support bubbling or cancelling, and
// any references to cancelBubble throw "Member not found".  A
// typeof check of "unknown" circumvents this issue (and is also
// IE specific).
// [ts] event.cancelBubble = true;
// [ts] }
// [ts] _.isPropagationStopped = functionThatReturnsTrue;
// [ts] },
// [ts] /**
// [ts] * We release all dispatched `SyntheticEvent`s after each event loop, adding
// [ts] * them back into the pool. This allows a way to hold onto a reference that
// [ts] * won't be added back into the pool.
// [ts] */
// [ts] persist: () => {
// [ts] _.isPersistent = functionThatReturnsTrue;
// [ts] },
// [ts] /**
// [ts] * Checks if this event should be released back into the pool.
// [ts] *
// [ts] * @return {boolean} True if this should not be released, false otherwise.
// [ts] */
// [ts] isPersistent: functionThatReturnsFalse,
// [ts] /**
// [ts] * `PooledClass` looks for `destructor` on each instance it releases.
// [ts] */
// [ts] destructor: () => {
// [ts] let Interface = _.constructor.Interface;
// [ts] for (let propName in Interface) {
// [ts] {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, propName, getPooledWarningPropertyDefinition(propName, Interface[propName]));
// [ts] }
// [ts] }
// [ts] _.dispatchConfig = null;
// [ts] _._targetInst = null;
// [ts] _.nativeEvent = null;
// [ts] _.isDefaultPrevented = functionThatReturnsFalse;
// [ts] _.isPropagationStopped = functionThatReturnsFalse;
// [ts] _._dispatchListeners = null;
// [ts] _._dispatchInstances = null;
// [ts] {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'nativeEvent', getPooledWarningPropertyDefinition('nativeEvent', null));
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'isDefaultPrevented', getPooledWarningPropertyDefinition('isDefaultPrevented', functionThatReturnsFalse));
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'isPropagationStopped', getPooledWarningPropertyDefinition('isPropagationStopped', functionThatReturnsFalse));
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'preventDefault', getPooledWarningPropertyDefinition('preventDefault', function () {}));
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'stopPropagation', getPooledWarningPropertyDefinition('stopPropagation', function () {}));
// [ts] }
// [ts] }
// [ts] });
// [ts] SyntheticEvent.Interface = EventInterface;
// [ts] /**
// [ts] * Helper to reduce boilerplate when creating subclasses.
// [ts] */
// [ts] SyntheticEvent.extend = (Interface) => {
// [ts] let Super = this;
// [ts] let E = () => {};
// [ts] E.prototype = Super.prototype;
// [ts] let prototype = E.init();
// [ts] fn Class() {
// [ts] return Super.apply(this, arguments);
// [ts] }
// [ts] _assign(prototype, Class.prototype);
// [ts] Class.prototype = prototype;
// [ts] Class.prototype.constructor = Class;
// [ts] Class.Interface = _assign({}, Super.Interface, Interface);
// [ts] Class.extend = Super.extend;
// [ts] addEventPoolingTo(Class);
// [ts] return Class;
// [ts] };
// [ts] addEventPoolingTo(SyntheticEvent);
// [ts] /**
// [ts] * Helper to nullify syntheticEvent instance properties when destructing
// [ts] *
// [ts] * @param {String} propName
// [ts] * @param {?object} getVal
// [ts] * @return {object} defineProperty object
// [ts] */
// [ts] fn getPooledWarningPropertyDefinition(propName: auto, getVal: auto) {
// [ts] let isFunction = @typeName(getVal) == "function";
// [ts] return {
// [ts] configurable: true,
// [ts] set: set,
// [ts] get: get
// [ts] };
// [ts] fn set(val: auto) {
// [ts] let action = if (isFunction) "setting the method" else "setting the property";
// [ts] warn(action, "This is effectively a no-op");
// [ts] return val;
// [ts] }
// [ts] fn get() {
// [ts] let action = if (isFunction) "accessing the method" else "accessing the property";
// [ts] let result = if (isFunction) "This is a no-op function" else "This is set to null";
// [ts] warn(action, result);
// [ts] return getVal;
// [ts] }
// [ts] fn warn(action: auto, result: auto) {
// [ts] {
// [ts] error("This synthetic event is reused for performance reasons. If you're seeing this, " + "you're %s `%s` on a released/nullified synthetic event. %s. " + "If you must keep the original synthetic event around, use event.persist(). " + "See https://fb.me/react-event-pooling for more information.", action, propName, result);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getPooledEvent(dispatchConfig: auto, targetInst: auto, nativeEvent: auto, nativeInst: auto) {
// [ts] let EventConstructor = this;
// [ts] if (EventConstructor.eventPool.length) {
// [ts] let instance = EventConstructor.eventPool.pop();
// [ts] EventConstructor.call(instance, dispatchConfig, targetInst, nativeEvent, nativeInst);
// [ts] return instance;
// [ts] }
// [ts] return EventConstructor.init(dispatchConfig, targetInst, nativeEvent, nativeInst);
// [ts] }
// [ts] fn releasePooledEvent(event: auto) {
// [ts] let EventConstructor = this;
// [ts] if (!(event == EventConstructor)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Trying to release an event instance into a pool of a different type." );)
// [ts] }
// [ts] }
// [ts] event.destructor();
// [ts] if (EventConstructor.eventPool.length < EVENT_POOL_SIZE) {
// [ts] EventConstructor.eventPool.push(event);
// [ts] }
// [ts] }
// [ts] fn addEventPoolingTo(EventConstructor: auto) {
// [ts] EventConstructor.eventPool = [];
// [ts] EventConstructor.getPooled = getPooledEvent;
// [ts] EventConstructor.release = releasePooledEvent;
// [ts] }
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/#events-compositionevents
// [ts] */
// [ts] let SyntheticCompositionEvent = SyntheticEvent.extend({
// [ts] data: null
// [ts] });
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/2013/WD-DOM-Level-3-Events-20131105
// [ts] *      /#events-inputevents
// [ts] */
// [ts] let SyntheticInputEvent = SyntheticEvent.extend({
// [ts] data: null
// [ts] });
// [ts] let END_KEYCODES = [9, 13, 27, 32]; // Tab, Return, Esc, Space
// [ts] let START_KEYCODE = 229;
// [ts] let canUseCompositionEvent = canUseDOM && "CompositionEvent" == window;
// [ts] let documentMode = null;
// [ts] if (canUseDOM && "documentMode" == document) {
// [ts] documentMode = document.documentMode;
// [ts] } // Webkit offers a very useful `textInput` event that can be used to
// directly represent `beforeInput`. The IE `textinput` event is not as
// useful, so we don't use it.
// [ts] let canUseTextInputEvent = canUseDOM && "TextEvent" == window && !documentMode; // In IE9+, we have access to composition events, but the data supplied
// by the native compositionend event may be incorrect. Japanese ideographic
// spaces, for instance (\u3000) are not recorded correctly.
// [ts] let useFallbackCompositionData = canUseDOM && (!canUseCompositionEvent || documentMode && documentMode > 8 && documentMode <= 11);
// [ts] let SPACEBAR_CODE = 32;
// [ts] let SPACEBAR_CHAR = String.fromCharCode(SPACEBAR_CODE); // Events and their corresponding property names.
// [ts] let eventTypes = {
// [ts] beforeInput: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onBeforeInput",
// [ts] captured: "onBeforeInputCapture"
// [ts] },
// [ts] dependencies: [TOP_COMPOSITION_END, TOP_KEY_PRESS, TOP_TEXT_INPUT, TOP_PASTE]
// [ts] },
// [ts] compositionEnd: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onCompositionEnd",
// [ts] captured: "onCompositionEndCapture"
// [ts] },
// [ts] dependencies: [TOP_BLUR, TOP_COMPOSITION_END, TOP_KEY_DOWN, TOP_KEY_PRESS, TOP_KEY_UP, TOP_MOUSE_DOWN]
// [ts] },
// [ts] compositionStart: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onCompositionStart",
// [ts] captured: "onCompositionStartCapture"
// [ts] },
// [ts] dependencies: [TOP_BLUR, TOP_COMPOSITION_START, TOP_KEY_DOWN, TOP_KEY_PRESS, TOP_KEY_UP, TOP_MOUSE_DOWN]
// [ts] },
// [ts] compositionUpdate: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onCompositionUpdate",
// [ts] captured: "onCompositionUpdateCapture"
// [ts] },
// [ts] dependencies: [TOP_BLUR, TOP_COMPOSITION_UPDATE, TOP_KEY_DOWN, TOP_KEY_PRESS, TOP_KEY_UP, TOP_MOUSE_DOWN]
// [ts] }
// [ts] }; // Track whether we've ever handled a keypress on the space key.
// [ts] let hasSpaceKeypress = false;
// [ts] /**
// [ts] * Return whether a native keypress event is assumed to be a command.
// [ts] * This is required because Firefox fires `keypress` events for key commands
// [ts] * (cut, copy, select-all, etc.) even though no character is inserted.
// [ts] */
// [ts] fn isKeypressCommand(nativeEvent: auto) {
// [ts] return (nativeEvent.ctrlKey || nativeEvent.altKey || nativeEvent.metaKey) && // ctrlKey && altKey is equivalent to AltGr, and is not a command.
// [ts] !(nativeEvent.ctrlKey && nativeEvent.altKey);
// [ts] }
// [ts] /**
// [ts] * Translate native top level events into event types.
// [ts] *
// [ts] * @param {string} topLevelType
// [ts] * @return {object}
// [ts] */
// [ts] fn getCompositionEventType(topLevelType: auto) {
// [ts] switch (topLevelType) {
// [ts] TOP_COMPOSITION_START => {
// [ts] return eventTypes.compositionStart;
// [ts] },
// [ts] TOP_COMPOSITION_END => {
// [ts] return eventTypes.compositionEnd;
// [ts] },
// [ts] TOP_COMPOSITION_UPDATE => {
// [ts] return eventTypes.compositionUpdate;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Does our fallback best-guess model think this event signifies that
// [ts] * composition has begun?
// [ts] *
// [ts] * @param {string} topLevelType
// [ts] * @param {object} nativeEvent
// [ts] * @return {boolean}
// [ts] */
// [ts] fn isFallbackCompositionStart(topLevelType: auto, nativeEvent: auto) {
// [ts] return topLevelType == TOP_KEY_DOWN && nativeEvent.keyCode == START_KEYCODE;
// [ts] }
// [ts] /**
// [ts] * Does our fallback mode think that this event is the end of composition?
// [ts] *
// [ts] * @param {string} topLevelType
// [ts] * @param {object} nativeEvent
// [ts] * @return {boolean}
// [ts] */
// [ts] fn isFallbackCompositionEnd(topLevelType: auto, nativeEvent: auto) {
// [ts] switch (topLevelType) {
// Command keys insert or clear IME input.
// [ts] TOP_KEY_UP => {
// [ts] return END_KEYCODES.indexOf(nativeEvent.keyCode) != -1;
// [ts] },
// Expect IME keyCode on each keydown. If we get any other
// code we must have exited earlier.
// [ts] TOP_KEY_DOWN => {
// [ts] return nativeEvent.keyCode != START_KEYCODE;
// [ts] },
// Events are not possible without cancelling IME.
// [ts] TOP_KEY_PRESS, TOP_MOUSE_DOWN, TOP_BLUR => {
// [ts] return true;
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Google Input Tools provides composition data via a CustomEvent,
// [ts] * with the `data` property populated in the `detail` object. If this
// [ts] * is available on the event object, use it. If not, this is a plain
// [ts] * composition event and we have nothing special to extract.
// [ts] *
// [ts] * @param {object} nativeEvent
// [ts] * @return {?string}
// [ts] */
// [ts] fn getDataFromCustomEvent(nativeEvent: auto) {
// [ts] let detail = nativeEvent.detail;
// [ts] if (@typeName(detail) == "object" && "data" == detail) {
// [ts] return detail.data;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * Check if a composition event was triggered by Korean IME.
// [ts] * Our fallback mode does not work well with IE's Korean IME,
// [ts] * so just use native composition events when Korean IME is used.
// [ts] * Although CompositionEvent.locale property is deprecated,
// [ts] * it is available in IE, where our fallback mode is enabled.
// [ts] *
// [ts] * @param {object} nativeEvent
// [ts] * @return {boolean}
// [ts] */
// [ts] fn isUsingKoreanIME(nativeEvent: auto) {
// [ts] return nativeEvent.locale == "ko";
// [ts] } // Track the current IME composition status, if any.
// [ts] let isComposing = false;
// [ts] /**
// [ts] * @return {?object} A SyntheticCompositionEvent.
// [ts] */
// [ts] fn extractCompositionEvent(topLevelType: auto, targetInst: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] let eventType;
// [ts] let fallbackData;
// [ts] if (canUseCompositionEvent) {
// [ts] eventType = getCompositionEventType(topLevelType);
// [ts] } else if (!isComposing) {
// [ts] if (isFallbackCompositionStart(topLevelType, nativeEvent)) {
// [ts] eventType = eventTypes.compositionStart;
// [ts] }
// [ts] } else if (isFallbackCompositionEnd(topLevelType, nativeEvent)) {
// [ts] eventType = eventTypes.compositionEnd;
// [ts] }
// [ts] if (!eventType) {
// [ts] return null;
// [ts] }
// [ts] if (useFallbackCompositionData && !isUsingKoreanIME(nativeEvent)) {
// The current composition is stored statically and must not be
// overwritten while composition continues.
// [ts] if (!isComposing && eventType == eventTypes.compositionStart) {
// [ts] isComposing = initialize(nativeEventTarget);
// [ts] } else if (eventType == eventTypes.compositionEnd) {
// [ts] if (isComposing) {
// [ts] fallbackData = getData();
// [ts] }
// [ts] }
// [ts] }
// [ts] let event = SyntheticCompositionEvent.getPooled(eventType, targetInst, nativeEvent, nativeEventTarget);
// [ts] if (fallbackData) {
// Inject data generated from fallback path into the synthetic event.
// This matches the property of native CompositionEventInterface.
// [ts] event.data = fallbackData;
// [ts] } else {
// [ts] let customData = getDataFromCustomEvent(nativeEvent);
// [ts] if (customData != null) {
// [ts] event.data = customData;
// [ts] }
// [ts] }
// [ts] accumulateTwoPhaseDispatches(event);
// [ts] return event;
// [ts] }
// [ts] /**
// [ts] * @param {TopLevelType} topLevelType Number from `TopLevelType`.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {?string} The string corresponding to this `beforeInput` event.
// [ts] */
// [ts] fn getNativeBeforeInputChars(topLevelType: auto, nativeEvent: auto) {
// [ts] switch (topLevelType) {
// [ts] TOP_COMPOSITION_END => {
// [ts] return getDataFromCustomEvent(nativeEvent);
// [ts] },
// [ts] /**
// [ts] * If native `textInput` events are available, our goal is to make
// [ts] * use of them. However, there is a special case: the spacebar key.
// [ts] * In Webkit, preventing default on a spacebar `textInput` event
// [ts] * cancels character insertion, but it *also* causes the browser
// [ts] * to fall back to its default spacebar behavior of scrolling the
// [ts] * page.
// [ts] *
// [ts] * Tracking at:
// [ts] * https://code.google.com/p/chromium/issues/detail?id=355103
// [ts] *
// [ts] * To avoid this issue, use the keypress event as if no `textInput`
// [ts] * event is available.
// [ts] */
// [ts] TOP_KEY_PRESS => {
// [ts] let which = nativeEvent.which;
// [ts] if (which != SPACEBAR_CODE) {
// [ts] return null;
// [ts] }
// [ts] hasSpaceKeypress = true;
// [ts] return SPACEBAR_CHAR;
// [ts] },
// Record the characters to be added to the DOM.
// [ts] TOP_TEXT_INPUT => {
// [ts] let chars = nativeEvent.data; // If it's a spacebar character, assume that we have already handled
// it at the keypress level and bail immediately. Android Chrome
// doesn't give us keycodes, so we need to ignore it.
// [ts] if (chars == SPACEBAR_CHAR && hasSpaceKeypress) {
// [ts] return null;
// [ts] }
// [ts] return chars;
// [ts] },
// [ts] else => {
// For other native event types, do nothing.
// [ts] return null;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * For browsers that do not provide the `textInput` event, extract the
// [ts] * appropriate string to use for SyntheticInputEvent.
// [ts] *
// [ts] * @param {number} topLevelType Number from `TopLevelEventTypes`.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {?string} The fallback string for this `beforeInput` event.
// [ts] */
// [ts] fn getFallbackBeforeInputChars(topLevelType: auto, nativeEvent: auto) {
// If we are currently composing (IME) and using a fallback to do so,
// try to extract the composed characters from the fallback object.
// If composition event is available, we extract a string only at
// compositionevent, otherwise extract it at fallback events.
// [ts] if (isComposing) {
// [ts] if (topLevelType == TOP_COMPOSITION_END || !canUseCompositionEvent && isFallbackCompositionEnd(topLevelType, nativeEvent)) {
// [ts] let chars = getData();
// [ts] reset();
// [ts] isComposing = false;
// [ts] return chars;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] switch (topLevelType) {
// If a paste event occurs after a keypress, throw out the input
// chars. Paste events should not lead to BeforeInput events.
// [ts] TOP_PASTE => {
// [ts] return null;
// [ts] },
// [ts] /**
// [ts] * As of v27, Firefox may fire keypress events even when no character
// [ts] * will be inserted. A few possibilities:
// [ts] *
// [ts] * - `which` is `0`. Arrow keys, Esc key, etc.
// [ts] *
// [ts] * - `which` is the pressed key code, but no char is available.
// [ts] *   Ex: 'AltGr + d` in Polish. There is no modified character for
// [ts] *   this key combination and no character is inserted into the
// [ts] *   document, but FF fires the keypress for char code `100` anyway.
// [ts] *   No `input` event will occur.
// [ts] *
// [ts] * - `which` is the pressed key code, but a command combination is
// [ts] *   being used. Ex: `Cmd+C`. No character is inserted, and no
// [ts] *   `input` event will occur.
// [ts] */
// [ts] TOP_KEY_PRESS => {
// [ts] if (!isKeypressCommand(nativeEvent)) {
// IE fires the `keypress` event when a user types an emoji via
// Touch keyboard of Windows.  In such a case, the `char` property
// holds an emoji character like `\uD83D\uDE0A`.  Because its length
// is 2, the property `which` does not represent an emoji correctly.
// In such a case, we directly return the `char` property instead of
// using `which`.
// [ts] if (nativeEvent.char && nativeEvent.char.length > 1) {
// [ts] return nativeEvent.char;
// [ts] } else if (nativeEvent.which) {
// [ts] return String.fromCharCode(nativeEvent.which);
// [ts] }
// [ts] }
// [ts] return null;
// [ts] },
// [ts] TOP_COMPOSITION_END => {
// [ts] return if (useFallbackCompositionData && !isUsingKoreanIME(nativeEvent)) null else nativeEvent.data;
// [ts] },
// [ts] else => {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Extract a SyntheticInputEvent for `beforeInput`, based on either native
// [ts] * `textInput` or fallback behavior.
// [ts] *
// [ts] * @return {?object} A SyntheticInputEvent.
// [ts] */
// [ts] fn extractBeforeInputEvent(topLevelType: auto, targetInst: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] let chars;
// [ts] if (canUseTextInputEvent) {
// [ts] chars = getNativeBeforeInputChars(topLevelType, nativeEvent);
// [ts] } else {
// [ts] chars = getFallbackBeforeInputChars(topLevelType, nativeEvent);
// [ts] } // If no characters are being inserted, no BeforeInput event should
// be fired.
// [ts] if (!chars) {
// [ts] return null;
// [ts] }
// [ts] let event = SyntheticInputEvent.getPooled(eventTypes.beforeInput, targetInst, nativeEvent, nativeEventTarget);
// [ts] event.data = chars;
// [ts] accumulateTwoPhaseDispatches(event);
// [ts] return event;
// [ts] }
// [ts] /**
// [ts] * Create an `onBeforeInput` event to match
// [ts] * http://www.w3.org/TR/2013/WD-DOM-Level-3-Events-20131105/#events-inputevents.
// [ts] *
// [ts] * This event plugin is based on the native `textInput` event
// [ts] * available in Chrome, Safari, Opera, and IE. This event fires after
// [ts] * `onKeyPress` and `onCompositionEnd`, but before `onInput`.
// [ts] *
// [ts] * `beforeInput` is spec'd but not implemented in any browsers, and
// [ts] * the `input` event does not provide any useful information about what has
// [ts] * actually been added, contrary to the spec. Thus, `textInput` is the best
// [ts] * available event to identify the characters that have actually been inserted
// [ts] * into the target node.
// [ts] *
// [ts] * This plugin is also responsible for emitting `composition` events, thus
// [ts] * allowing us to share composition fallback code for both `beforeInput` and
// [ts] * `composition` event types.
// [ts] */
// [ts] let BeforeInputEventPlugin = {
// [ts] eventTypes: eventTypes,
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget, eventSystemFlags) => {
// [ts] let composition = extractCompositionEvent(topLevelType, targetInst, nativeEvent, nativeEventTarget);
// [ts] let beforeInput = extractBeforeInputEvent(topLevelType, targetInst, nativeEvent, nativeEventTarget);
// [ts] if (composition == null) {
// [ts] return beforeInput;
// [ts] }
// [ts] if (beforeInput == null) {
// [ts] return composition;
// [ts] }
// [ts] return [composition, beforeInput];
// [ts] }
// [ts] };
// [ts] /**
// [ts] * @see http://www.whatwg.org/specs/web-apps/current-work/multipage/the-input-element.html#input-type-attr-summary
// [ts] */
// [ts] let supportedInputTypes = {
// [ts] color: true,
// [ts] date: true,
// [ts] datetime: true,
// [ts] "datetime-local": true,
// [ts] email: true,
// [ts] month: true,
// [ts] number: true,
// [ts] password: true,
// [ts] range: true,
// [ts] search: true,
// [ts] tel: true,
// [ts] text: true,
// [ts] time: true,
// [ts] url: true,
// [ts] week: true
// [ts] };
// [ts] fn isTextInputElement(elem: auto) {
// [ts] let nodeName = elem && elem.nodeName && elem.nodeName.toLowerCase();
// [ts] if (nodeName == "input") {
// [ts] return !!supportedInputTypes[elem.type];
// [ts] }
// [ts] if (nodeName == "textarea") {
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] let eventTypes$1 = {
// [ts] change: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onChange",
// [ts] captured: "onChangeCapture"
// [ts] },
// [ts] dependencies: [TOP_BLUR, TOP_CHANGE, TOP_CLICK, TOP_FOCUS, TOP_INPUT, TOP_KEY_DOWN, TOP_KEY_UP, TOP_SELECTION_CHANGE]
// [ts] }
// [ts] };
// [ts] fn createAndAccumulateChangeEvent(inst: auto, nativeEvent: auto, target: auto) {
// [ts] let event = SyntheticEvent.getPooled(eventTypes$1.change, inst, nativeEvent, target);
// [ts] event.type = "change"; // Flag this event loop as needing state restore.
// [ts] enqueueStateRestore(target);
// [ts] accumulateTwoPhaseDispatches(event);
// [ts] return event;
// [ts] }
// [ts] /**
// [ts] * For IE shims
// [ts] */
// [ts] let activeElement = null;
// [ts] let activeElementInst = null;
// [ts] /**
// [ts] * SECTION: handle `change` event
// [ts] */
// [ts] fn shouldUseChangeEvent(elem: auto) {
// [ts] let nodeName = elem.nodeName && elem.nodeName.toLowerCase();
// [ts] return nodeName == "select" || nodeName == "input" && elem.type == "file";
// [ts] }
// [ts] fn manualDispatchChangeEvent(nativeEvent: auto) {
// [ts] let event = createAndAccumulateChangeEvent(activeElementInst, nativeEvent, getEventTarget(nativeEvent)); // If change and propertychange bubbled, we'd just bind to it like all the
// other events and have it go through ReactBrowserEventEmitter. Since it
// doesn't, we manually listen for the events and so we have to enqueue and
// process the abstract event manually.
//
// Batching is necessary here in order to ensure that all event handlers run
// before the next rerender (including event handlers attached to ancestor
// elements instead of directly on the input). Without this, controlled
// components don't work properly in conjunction with event bubbling because
// the component is rerendered and the value reverted before all the event
// handlers can run. See https://github.com/facebook/react/issues/708.
// [ts] batchedUpdates(runEventInBatch, event);
// [ts] }
// [ts] fn runEventInBatch(event: auto) {
// [ts] runEventsInBatch(event);
// [ts] }
// [ts] fn getInstIfValueChanged(targetInst: auto) {
// [ts] let targetNode = getNodeFromInstance$1(targetInst);
// [ts] if (updateValueIfChanged(targetNode)) {
// [ts] return targetInst;
// [ts] }
// [ts] }
// [ts] fn getTargetInstForChangeEvent(topLevelType: auto, targetInst: auto) {
// [ts] if (topLevelType == TOP_CHANGE) {
// [ts] return targetInst;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * SECTION: handle `input` event
// [ts] */
// [ts] let isInputEventSupported = false;
// [ts] if (canUseDOM) {
// IE9 claims to support the input event but fails to trigger it when
// deleting text, so we ignore its input events.
// [ts] isInputEventSupported = isEventSupported("input") && (!document.documentMode || document.documentMode > 9);
// [ts] }
// [ts] /**
// [ts] * (For IE <=9) Starts tracking propertychange events on the passed-in element
// [ts] * and override the value property so that we can distinguish user events from
// [ts] * value changes in JS.
// [ts] */
// [ts] fn startWatchingForValueChange(target: auto, targetInst: auto) {
// [ts] activeElement = target;
// [ts] activeElementInst = targetInst;
// [ts] activeElement.attachEvent("onpropertychange", handlePropertyChange);
// [ts] }
// [ts] /**
// [ts] * (For IE <=9) Removes the event listeners from the currently-tracked element,
// [ts] * if any exists.
// [ts] */
// [ts] fn stopWatchingForValueChange() {
// [ts] if (!activeElement) {
// [ts] return;
// [ts] }
// [ts] activeElement.detachEvent("onpropertychange", handlePropertyChange);
// [ts] activeElement = null;
// [ts] activeElementInst = null;
// [ts] }
// [ts] /**
// [ts] * (For IE <=9) Handles a propertychange event, sending a `change` event if
// [ts] * the value of the active element has changed.
// [ts] */
// [ts] fn handlePropertyChange(nativeEvent: auto) {
// [ts] if (nativeEvent.propertyName != "value") {
// [ts] return;
// [ts] }
// [ts] if (getInstIfValueChanged(activeElementInst)) {
// [ts] manualDispatchChangeEvent(nativeEvent);
// [ts] }
// [ts] }
// [ts] fn handleEventsForInputEventPolyfill(topLevelType: auto, target: auto, targetInst: auto) {
// [ts] if (topLevelType == TOP_FOCUS) {
// In IE9, propertychange fires for most input events but is buggy and
// doesn't fire when text is deleted, but conveniently, selectionchange
// appears to fire in all of the remaining cases so we catch those and
// forward the event if the value has changed
// In either case, we don't want to call the event handler if the value
// is changed from JS so we redefine a setter for `.value` that updates
// our activeElementValue variable, allowing us to ignore those changes
//
// stopWatching() should be a noop here but we call it just in case we
// missed a blur event somehow.
// [ts] stopWatchingForValueChange();
// [ts] startWatchingForValueChange(target, targetInst);
// [ts] } else if (topLevelType == TOP_BLUR) {
// [ts] stopWatchingForValueChange();
// [ts] }
// [ts] } // For IE8 and IE9.
// [ts] fn getTargetInstForInputEventPolyfill(topLevelType: auto, targetInst: auto) {
// [ts] if (topLevelType == TOP_SELECTION_CHANGE || topLevelType == TOP_KEY_UP || topLevelType == TOP_KEY_DOWN) {
// On the selectionchange event, the target is just document which isn't
// helpful for us so just check activeElement instead.
//
// 99% of the time, keydown and keyup aren't necessary. IE8 fails to fire
// propertychange on the first input event after setting `value` from a
// script and fires only keydown, keypress, keyup. Catching keyup usually
// gets it and catching keydown lets us fire an event for the first
// keystroke if user does a key repeat (it'll be a little delayed: right
// before the second keystroke). Other input methods (e.g., paste) seem to
// fire selectionchange normally.
// [ts] return getInstIfValueChanged(activeElementInst);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * SECTION: handle `click` event
// [ts] */
// [ts] fn shouldUseClickEvent(elem: auto) {
// Use the `click` event to detect changes to checkbox and radio inputs.
// This approach works across all browsers, whereas `change` does not fire
// until `blur` in IE8.
// [ts] let nodeName = elem.nodeName;
// [ts] return nodeName && nodeName.toLowerCase() == "input" && (elem.type == "checkbox" || elem.type == "radio");
// [ts] }
// [ts] fn getTargetInstForClickEvent(topLevelType: auto, targetInst: auto) {
// [ts] if (topLevelType == TOP_CLICK) {
// [ts] return getInstIfValueChanged(targetInst);
// [ts] }
// [ts] }
// [ts] fn getTargetInstForInputOrChangeEvent(topLevelType: auto, targetInst: auto) {
// [ts] if (topLevelType == TOP_INPUT || topLevelType == TOP_CHANGE) {
// [ts] return getInstIfValueChanged(targetInst);
// [ts] }
// [ts] }
// [ts] fn handleControlledInputBlur(node: auto) {
// [ts] let state = node._wrapperState;
// [ts] if (!state || !state.controlled || node.type != "number") {
// [ts] return;
// [ts] }
// [ts] {
// If controlled, assign the value attribute to the current value on blur
// [ts] setDefaultValue(node, "number", node.value);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * This plugin creates an `onChange` event that normalizes change events
// [ts] * across form elements. This event fires at a time when it's possible to
// [ts] * change the element's value without seeing a flicker.
// [ts] *
// [ts] * Supported elements are:
// [ts] * - input (see `isTextInputElement`)
// [ts] * - textarea
// [ts] * - select
// [ts] */
// [ts] let ChangeEventPlugin = {
// [ts] eventTypes: eventTypes$1,
// [ts] _isInputEventSupported: isInputEventSupported,
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget, eventSystemFlags) => {
// [ts] let targetNode = if (targetInst) getNodeFromInstance$1(targetInst) else window;
// [ts] let getTargetInstFunc, handleEventFunc;
// [ts] if (shouldUseChangeEvent(targetNode)) {
// [ts] getTargetInstFunc = getTargetInstForChangeEvent;
// [ts] } else if (isTextInputElement(targetNode)) {
// [ts] if (isInputEventSupported) {
// [ts] getTargetInstFunc = getTargetInstForInputOrChangeEvent;
// [ts] } else {
// [ts] getTargetInstFunc = getTargetInstForInputEventPolyfill;
// [ts] handleEventFunc = handleEventsForInputEventPolyfill;
// [ts] }
// [ts] } else if (shouldUseClickEvent(targetNode)) {
// [ts] getTargetInstFunc = getTargetInstForClickEvent;
// [ts] }
// [ts] if (getTargetInstFunc) {
// [ts] let inst = getTargetInstFunc(topLevelType, targetInst);
// [ts] if (inst) {
// [ts] let event = createAndAccumulateChangeEvent(inst, nativeEvent, nativeEventTarget);
// [ts] return event;
// [ts] }
// [ts] }
// [ts] if (handleEventFunc) {
// [ts] handleEventFunc(topLevelType, targetNode, targetInst);
// [ts] } // When blurring, set the value attribute for number inputs
// [ts] if (topLevelType == TOP_BLUR) {
// [ts] handleControlledInputBlur(targetNode);
// [ts] }
// [ts] }
// [ts] };
// [ts] let SyntheticUIEvent = SyntheticEvent.extend({
// [ts] view: null,
// [ts] detail: null
// [ts] });
// [ts] /**
// [ts] * Translation from modifier key to the associated property in the event.
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/#keys-Modifiers
// [ts] */
// [ts] let modifierKeyToProp = {
// [ts] Alt: "altKey",
// [ts] Control: "ctrlKey",
// [ts] Meta: "metaKey",
// [ts] Shift: "shiftKey"
// [ts] }; // Older browsers (Safari <= 10, iOS Safari <= 10.2) do not support
// getModifierState. If getModifierState is not supported, we map it to a set of
// modifier keys exposed by the event. In this case, Lock-keys are not supported.
// [ts] fn modifierStateGetter(keyArg: auto) {
// [ts] let syntheticEvent = this;
// [ts] let nativeEvent = syntheticEvent.nativeEvent;
// [ts] if (nativeEvent.getModifierState) {
// [ts] return nativeEvent.getModifierState(keyArg);
// [ts] }
// [ts] let keyProp = modifierKeyToProp[keyArg];
// [ts] return if (keyProp) !!nativeEvent[keyProp] else false;
// [ts] }
// [ts] fn getEventModifierState(nativeEvent: auto) {
// [ts] return modifierStateGetter;
// [ts] }
// [ts] let previousScreenX = 0;
// [ts] let previousScreenY = 0; // Use flags to signal movementX/Y has already been set
// [ts] let isMovementXSet = false;
// [ts] let isMovementYSet = false;
// [ts] /**
// [ts] * @interface MouseEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let SyntheticMouseEvent = SyntheticUIEvent.extend({
// [ts] screenX: null,
// [ts] screenY: null,
// [ts] clientX: null,
// [ts] clientY: null,
// [ts] pageX: null,
// [ts] pageY: null,
// [ts] ctrlKey: null,
// [ts] shiftKey: null,
// [ts] altKey: null,
// [ts] metaKey: null,
// [ts] getModifierState: getEventModifierState,
// [ts] button: null,
// [ts] buttons: null,
// [ts] relatedTarget: (event) => {
// [ts] return event.relatedTarget || (if (event.fromElement == event.srcElement) event.toElement else event.fromElement);
// [ts] },
// [ts] movementX: (event) => {
// [ts] if ("movementX" == event) {
// [ts] return event.movementX;
// [ts] }
// [ts] let screenX = previousScreenX;
// [ts] previousScreenX = event.screenX;
// [ts] if (!isMovementXSet) {
// [ts] isMovementXSet = true;
// [ts] return 0;
// [ts] }
// [ts] return if (event.type == "mousemove") event.screenX - screenX else 0;
// [ts] },
// [ts] movementY: (event) => {
// [ts] if ("movementY" == event) {
// [ts] return event.movementY;
// [ts] }
// [ts] let screenY = previousScreenY;
// [ts] previousScreenY = event.screenY;
// [ts] if (!isMovementYSet) {
// [ts] isMovementYSet = true;
// [ts] return 0;
// [ts] }
// [ts] return if (event.type == "mousemove") event.screenY - screenY else 0;
// [ts] }
// [ts] });
// [ts] /**
// [ts] * @interface PointerEvent
// [ts] * @see http://www.w3.org/TR/pointerevents/
// [ts] */
// [ts] let SyntheticPointerEvent = SyntheticMouseEvent.extend({
// [ts] pointerId: null,
// [ts] width: null,
// [ts] height: null,
// [ts] pressure: null,
// [ts] tangentialPressure: null,
// [ts] tiltX: null,
// [ts] tiltY: null,
// [ts] twist: null,
// [ts] pointerType: null,
// [ts] isPrimary: null
// [ts] });
// [ts] let eventTypes$2 = {
// [ts] mouseEnter: {
// [ts] registrationName: "onMouseEnter",
// [ts] dependencies: [TOP_MOUSE_OUT, TOP_MOUSE_OVER]
// [ts] },
// [ts] mouseLeave: {
// [ts] registrationName: "onMouseLeave",
// [ts] dependencies: [TOP_MOUSE_OUT, TOP_MOUSE_OVER]
// [ts] },
// [ts] pointerEnter: {
// [ts] registrationName: "onPointerEnter",
// [ts] dependencies: [TOP_POINTER_OUT, TOP_POINTER_OVER]
// [ts] },
// [ts] pointerLeave: {
// [ts] registrationName: "onPointerLeave",
// [ts] dependencies: [TOP_POINTER_OUT, TOP_POINTER_OVER]
// [ts] }
// [ts] };
// [ts] let EnterLeaveEventPlugin = {
// [ts] eventTypes: eventTypes$2,
// [ts] /**
// [ts] * For almost every interaction we care about, there will be both a top-level
// [ts] * `mouseover` and `mouseout` event that occurs. Only use `mouseout` so that
// [ts] * we do not extract duplicate events. However, moving the mouse into the
// [ts] * browser from outside will not fire a `mouseout` event. In this case, we use
// [ts] * the `mouseover` top-level event.
// [ts] */
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget, eventSystemFlags) => {
// [ts] let isOverEvent = topLevelType == TOP_MOUSE_OVER || topLevelType == TOP_POINTER_OVER;
// [ts] let isOutEvent = topLevelType == TOP_MOUSE_OUT || topLevelType == TOP_POINTER_OUT;
// [ts] if (isOverEvent && (eventSystemFlags & IS_REPLAYED) == 0 && (nativeEvent.relatedTarget || nativeEvent.fromElement)) {
// If this is an over event with a target, then we've already dispatched
// the event in the out event of the other target. If this is replayed,
// then it's because we couldn't dispatch against this target previously
// so we have to do it now instead.
// [ts] return null;
// [ts] }
// [ts] if (!isOutEvent && !isOverEvent) {
// Must not be a mouse or pointer in or out - ignoring.
// [ts] return null;
// [ts] }
// [ts] let win;
// [ts] if (nativeEventTarget.window == nativeEventTarget) {
// `nativeEventTarget` is probably a window object.
// [ts] win = nativeEventTarget;
// [ts] } else {
// TODO: Figure out why `ownerDocument` is sometimes undefined in IE8.
// [ts] let doc = nativeEventTarget.ownerDocument;
// [ts] if (doc) {
// [ts] win = doc.defaultView || doc.parentWindow;
// [ts] } else {
// [ts] win = window;
// [ts] }
// [ts] }
// [ts] let from;
// [ts] let to;
// [ts] if (isOutEvent) {
// [ts] from = targetInst;
// [ts] let related = nativeEvent.relatedTarget || nativeEvent.toElement;
// [ts] to = if (related) getClosestInstanceFromNode(related) else null;
// [ts] if (to != null) {
// [ts] let nearestMounted = getNearestMountedFiber(to);
// [ts] if (to != nearestMounted || to.tag != HostComponent && to.tag != HostText) {
// [ts] to = null;
// [ts] }
// [ts] }
// [ts] } else {
// Moving to a node from outside the window.
// [ts] from = null;
// [ts] to = targetInst;
// [ts] }
// [ts] if (from == to) {
// Nothing pertains to our managed components.
// [ts] return null;
// [ts] }
// [ts] let eventInterface, leaveEventType, enterEventType, eventTypePrefix;
// [ts] if (topLevelType == TOP_MOUSE_OUT || topLevelType == TOP_MOUSE_OVER) {
// [ts] eventInterface = SyntheticMouseEvent;
// [ts] leaveEventType = eventTypes$2.mouseLeave;
// [ts] enterEventType = eventTypes$2.mouseEnter;
// [ts] eventTypePrefix = "mouse";
// [ts] } else if (topLevelType == TOP_POINTER_OUT || topLevelType == TOP_POINTER_OVER) {
// [ts] eventInterface = SyntheticPointerEvent;
// [ts] leaveEventType = eventTypes$2.pointerLeave;
// [ts] enterEventType = eventTypes$2.pointerEnter;
// [ts] eventTypePrefix = "pointer";
// [ts] }
// [ts] let fromNode = if (from == null) win else getNodeFromInstance$1(from);
// [ts] let toNode = if (to == null) win else getNodeFromInstance$1(to);
// [ts] let leave = eventInterface.getPooled(leaveEventType, from, nativeEvent, nativeEventTarget);
// [ts] leave.type = eventTypePrefix + "leave";
// [ts] leave.target = fromNode;
// [ts] leave.relatedTarget = toNode;
// [ts] let enter = eventInterface.getPooled(enterEventType, to, nativeEvent, nativeEventTarget);
// [ts] enter.type = eventTypePrefix + "enter";
// [ts] enter.target = toNode;
// [ts] enter.relatedTarget = fromNode;
// [ts] accumulateEnterLeaveDispatches(leave, enter, from, to); // If we are not processing the first ancestor, then we
// should not process the same nativeEvent again, as we
// will have already processed it in the first ancestor.
// [ts] if ((eventSystemFlags & IS_FIRST_ANCESTOR) == 0) {
// [ts] return [leave];
// [ts] }
// [ts] return [leave, enter];
// [ts] }
// [ts] };
// [ts] /**
// [ts] * inlined Object.is polyfill to avoid requiring consumers ship their own
// [ts] * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/is
// [ts] */
// [ts] fn is(x: auto, y: auto) {
// [ts] return x == y && (x != 0 || 1 / x == 1 / y) || x != x && y != y // eslint-disable-line no-self-compare
// [ts] ;
// [ts] }
// [ts] let objectIs = if (@typeName(Object.is) == "function") Object.is else is;
// [ts] let hasOwnProperty$2 = Object.prototype.hasOwnProperty;
// [ts] /**
// [ts] * Performs equality by iterating through keys on an object and returning false
// [ts] * when any key has values which are not strictly equal between the arguments.
// [ts] * Returns true when the values of all keys are strictly equal.
// [ts] */
// [ts] fn shallowEqual(objA: auto, objB: auto) {
// [ts] if (objectIs(objA, objB)) {
// [ts] return true;
// [ts] }
// [ts] if (@typeName(objA) != "object" || objA == null || @typeName(objB) != "object" || objB == null) {
// [ts] return false;
// [ts] }
// [ts] let keysA = Object.keys(objA);
// [ts] let keysB = Object.keys(objB);
// [ts] if (keysA.length != keysB.length) {
// [ts] return false;
// [ts] } // Test for A's keys different from B.
// [ts] let i = 0;
// [ts] while (i < keysA.length) { // [cap-migrate] for-step: i++
// [ts] if (!hasOwnProperty$2.call(objB, keysA[i]) || !objectIs(objA[keysA[i]], objB[keysA[i]])) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] }
// [ts] let skipSelectionChangeEvent = canUseDOM && "documentMode" == document && document.documentMode <= 11;
// [ts] let eventTypes$3 = {
// [ts] select: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onSelect",
// [ts] captured: "onSelectCapture"
// [ts] },
// [ts] dependencies: [TOP_BLUR, TOP_CONTEXT_MENU, TOP_DRAG_END, TOP_FOCUS, TOP_KEY_DOWN, TOP_KEY_UP, TOP_MOUSE_DOWN, TOP_MOUSE_UP, TOP_SELECTION_CHANGE]
// [ts] }
// [ts] };
// [ts] let activeElement$1 = null;
// [ts] let activeElementInst$1 = null;
// [ts] let lastSelection = null;
// [ts] let mouseDown = false;
// [ts] /**
// [ts] * Get an object which is a unique representation of the current selection.
// [ts] *
// [ts] * The return value will not be consistent across nodes or browsers, but
// [ts] * two identical selections on the same node will return identical objects.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @return {object}
// [ts] */
// [ts] (node) => {
// [ts] if ("selectionStart" == node && hasSelectionCapabilities(node)) {
// [ts] return {
// [ts] start: node.selectionStart,
// [ts] end: node.selectionEnd
// [ts] };
// [ts] } else {
// [ts] let win = node.ownerDocument && node.ownerDocument.defaultView || window;
// [ts] let selection = win.getSelection();
// [ts] return {
// [ts] anchorNode: selection.anchorNode,
// [ts] anchorOffset: selection.anchorOffset,
// [ts] focusNode: selection.focusNode,
// [ts] focusOffset: selection.focusOffset
// [ts] };
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Get document associated with the event target.
// [ts] *
// [ts] * @param {object} nativeEventTarget
// [ts] * @return {Document}
// [ts] */
// [ts] fn getEventTargetDocument(eventTarget: auto) {
// [ts] return if (eventTarget.window == eventTarget) eventTarget.document else if (eventTarget.nodeType == DOCUMENT_NODE) eventTarget else eventTarget.ownerDocument;
// [ts] }
// [ts] /**
// [ts] * Poll selection to see whether it's changed.
// [ts] *
// [ts] * @param {object} nativeEvent
// [ts] * @param {object} nativeEventTarget
// [ts] * @return {?SyntheticEvent}
// [ts] */
// [ts] fn constructSelectEvent(nativeEvent: auto, nativeEventTarget: auto) {
// Ensure we have the right element, and that the user is not dragging a
// selection (this matches native `select` event behavior). In HTML5, select
// fires only on input and textarea thus if there's no focused element we
// won't dispatch.
// [ts] let doc = getEventTargetDocument(nativeEventTarget);
// [ts] if (mouseDown || activeElement$1 == null || activeElement$1 != getActiveElement(doc)) {
// [ts] return null;
// [ts] } // Only fire when selection has actually changed.
// [ts] let currentSelection = getSelection$1(activeElement$1);
// [ts] if (!lastSelection || !shallowEqual(lastSelection, currentSelection)) {
// [ts] lastSelection = currentSelection;
// [ts] let syntheticEvent = SyntheticEvent.getPooled(eventTypes$3.select, activeElementInst$1, nativeEvent, nativeEventTarget);
// [ts] syntheticEvent.type = "select";
// [ts] syntheticEvent.target = activeElement$1;
// [ts] accumulateTwoPhaseDispatches(syntheticEvent);
// [ts] return syntheticEvent;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * This plugin creates an `onSelect` event that normalizes select events
// [ts] * across form elements.
// [ts] *
// [ts] * Supported elements are:
// [ts] * - input (see `isTextInputElement`)
// [ts] * - textarea
// [ts] * - contentEditable
// [ts] *
// [ts] * This differs from native browser implementations in the following ways:
// [ts] * - Fires on contentEditable fields as well as inputs.
// [ts] * - Fires for collapsed selection.
// [ts] * - Fires after user input.
// [ts] */
// [ts] let SelectEventPlugin = {
// [ts] eventTypes: eventTypes$3,
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget, eventSystemFlags, container) => {
// [ts] let containerOrDoc = container || getEventTargetDocument(nativeEventTarget); // Track whether all listeners exists for this plugin. If none exist, we do
// not extract events. See #3639.
// [ts] if (!containerOrDoc || !isListeningToAllDependencies("onSelect", containerOrDoc)) {
// [ts] return null;
// [ts] }
// [ts] let targetNode = if (targetInst) getNodeFromInstance$1(targetInst) else window;
// [ts] switch (topLevelType) {
// Track the input node that has focus.
// [ts] TOP_FOCUS => {
// [ts] if (isTextInputElement(targetNode) || targetNode.contentEditable == "true") {
// [ts] activeElement$1 = targetNode;
// [ts] activeElementInst$1 = targetInst;
// [ts] lastSelection = null;
// [ts] }
// break;
// [ts] },
// [ts] TOP_BLUR => {
// [ts] activeElement$1 = null;
// [ts] activeElementInst$1 = null;
// [ts] lastSelection = null;
// break;
// Don't fire the event while the user is dragging. This matches the
// semantics of the native select event.
// [ts] },
// [ts] TOP_MOUSE_DOWN => {
// [ts] mouseDown = true;
// break;
// [ts] },
// [ts] TOP_CONTEXT_MENU, TOP_MOUSE_UP, TOP_DRAG_END => {
// [ts] mouseDown = false;
// [ts] return constructSelectEvent(nativeEvent, nativeEventTarget);
// Chrome and IE fire non-standard event when selection is changed (and
// sometimes when it hasn't). IE's event fires out of order with respect
// to key and input events on deletion, so we discard it.
//
// Firefox doesn't support selectionchange, so check selection status
// after each key entry. The selection changes after keydown and before
// keyup, but we check on keydown as well in the case of holding down a
// key, when multiple keydown events are fired but only one keyup is.
// This is also our approach for IE handling, for the reason above.
// [ts] },
// [ts] TOP_SELECTION_CHANGE => {
// [ts] if (skipSelectionChangeEvent) {
// [ts] break;
// [ts] }
// falls through
// [ts] },
// [ts] TOP_KEY_DOWN, TOP_KEY_UP => {
// [ts] return constructSelectEvent(nativeEvent, nativeEventTarget);
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/css3-animations/#AnimationEvent-interface
// [ts] * @see https://developer.mozilla.org/en-US/docs/Web/API/AnimationEvent
// [ts] */
// [ts] let SyntheticAnimationEvent = SyntheticEvent.extend({
// [ts] animationName: null,
// [ts] elapsedTime: null,
// [ts] pseudoElement: null
// [ts] });
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/clipboard-apis/
// [ts] */
// [ts] let SyntheticClipboardEvent = SyntheticEvent.extend({
// [ts] clipboardData: (event) => {
// [ts] return if ("clipboardData" == event) event.clipboardData else window.clipboardData;
// [ts] }
// [ts] });
// [ts] /**
// [ts] * @interface FocusEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let SyntheticFocusEvent = SyntheticUIEvent.extend({
// [ts] relatedTarget: null
// [ts] });
// [ts] /**
// [ts] * `charCode` represents the actual "character code" and is safe to use with
// [ts] * `String.fromCharCode`. As such, only keys that correspond to printable
// [ts] * characters produce a valid `charCode`, the only exception to this is Enter.
// [ts] * The Tab-key is considered non-printable and does not have a `charCode`,
// [ts] * presumably because it does not produce a tab-character in browsers.
// [ts] *
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {number} Normalized `charCode` property.
// [ts] */
// [ts] fn getEventCharCode(nativeEvent: auto) {
// [ts] let charCode;
// [ts] let keyCode = nativeEvent.keyCode;
// [ts] if ("charCode" == nativeEvent) {
// [ts] charCode = nativeEvent.charCode; // FF does not set `charCode` for the Enter-key, check against `keyCode`.
// [ts] if (charCode == 0 && keyCode == 13) {
// [ts] charCode = 13;
// [ts] }
// [ts] } else {
// IE8 does not implement `charCode`, but `keyCode` has the correct value.
// [ts] charCode = keyCode;
// [ts] } // IE and Edge (on Windows) and Chrome / Safari (on Windows and Linux)
// report Enter as charCode 10 when ctrl is pressed.
// [ts] if (charCode == 10) {
// [ts] charCode = 13;
// [ts] } // Some non-printable keys are reported in `charCode`/`keyCode`, discard them.
// Must not discard the (non-)printable Enter-key.
// [ts] if (charCode >= 32 || charCode == 13) {
// [ts] return charCode;
// [ts] }
// [ts] return 0;
// [ts] }
// [ts] /**
// [ts] * Normalization of deprecated HTML5 `key` values
// [ts] * @see https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent#Key_names
// [ts] */
// [ts] let normalizeKey = {
// [ts] Esc: "Escape",
// [ts] Spacebar: " ",
// [ts] Left: "ArrowLeft",
// [ts] Up: "ArrowUp",
// [ts] Right: "ArrowRight",
// [ts] Down: "ArrowDown",
// [ts] Del: "Delete",
// [ts] Win: "OS",
// [ts] Menu: "ContextMenu",
// [ts] Apps: "ContextMenu",
// [ts] Scroll: "ScrollLock",
// [ts] MozPrintableKey: "Unidentified"
// [ts] };
// [ts] /**
// [ts] * Translation from legacy `keyCode` to HTML5 `key`
// [ts] * Only special keys supported, all others depend on keyboard layout or browser
// [ts] * @see https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent#Key_names
// [ts] */
// [ts] let translateToKey = {
// [ts] "8": "Backspace",
// [ts] "9": "Tab",
// [ts] "12": "Clear",
// [ts] "13": "Enter",
// [ts] "16": "Shift",
// [ts] "17": "Control",
// [ts] "18": "Alt",
// [ts] "19": "Pause",
// [ts] "20": "CapsLock",
// [ts] "27": "Escape",
// [ts] "32": " ",
// [ts] "33": "PageUp",
// [ts] "34": "PageDown",
// [ts] "35": "End",
// [ts] "36": "Home",
// [ts] "37": "ArrowLeft",
// [ts] "38": "ArrowUp",
// [ts] "39": "ArrowRight",
// [ts] "40": "ArrowDown",
// [ts] "45": "Insert",
// [ts] "46": "Delete",
// [ts] "112": "F1",
// [ts] "113": "F2",
// [ts] "114": "F3",
// [ts] "115": "F4",
// [ts] "116": "F5",
// [ts] "117": "F6",
// [ts] "118": "F7",
// [ts] "119": "F8",
// [ts] "120": "F9",
// [ts] "121": "F10",
// [ts] "122": "F11",
// [ts] "123": "F12",
// [ts] "144": "NumLock",
// [ts] "145": "ScrollLock",
// [ts] "224": "Meta"
// [ts] };
// [ts] /**
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {string} Normalized `key` property.
// [ts] */
// [ts] fn getEventKey(nativeEvent: auto) {
// [ts] if (nativeEvent.key) {
// Normalize inconsistent values reported by browsers due to
// implementations of a working draft specification.
// FireFox implements `key` but returns `MozPrintableKey` for all
// printable characters (normalized to `Unidentified`), ignore it.
// [ts] let key = normalizeKey[nativeEvent.key] || nativeEvent.key;
// [ts] if (key != "Unidentified") {
// [ts] return key;
// [ts] }
// [ts] } // Browser does not implement `key`, polyfill as much of it as we can.
// [ts] if (nativeEvent.type == "keypress") {
// [ts] let charCode = getEventCharCode(nativeEvent); // The enter-key is technically both printable and non-printable and can
// thus be captured by `keypress`, no other non-printable key should.
// [ts] return if (charCode == 13) "Enter" else String.fromCharCode(charCode);
// [ts] }
// [ts] if (nativeEvent.type == "keydown" || nativeEvent.type == "keyup") {
// While user keyboard layout determines the actual meaning of each
// `keyCode` value, almost all function keys have a universal value.
// [ts] return translateToKey[nativeEvent.keyCode] || "Unidentified";
// [ts] }
// [ts] return "";
// [ts] }
// [ts] /**
// [ts] * @interface KeyboardEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let SyntheticKeyboardEvent = SyntheticUIEvent.extend({
// [ts] key: getEventKey,
// [ts] location: null,
// [ts] ctrlKey: null,
// [ts] shiftKey: null,
// [ts] altKey: null,
// [ts] metaKey: null,
// [ts] repeat: null,
// [ts] locale: null,
// [ts] getModifierState: getEventModifierState,
// Legacy Interface
// [ts] charCode: (event) => {
// `charCode` is the result of a KeyPress event and represents the value of
// the actual printable character.
// KeyPress is deprecated, but its replacement is not yet final and not
// implemented in any major browser. Only KeyPress has charCode.
// [ts] if (event.type == "keypress") {
// [ts] return getEventCharCode(event);
// [ts] }
// [ts] return 0;
// [ts] },
// [ts] keyCode: (event) => {
// `keyCode` is the result of a KeyDown/Up event and represents the value of
// physical keyboard key.
// The actual meaning of the value depends on the users' keyboard layout
// which cannot be detected. Assuming that it is a US keyboard layout
// provides a surprisingly accurate mapping for US and European users.
// Due to this, it is left to the user to implement at this time.
// [ts] if (event.type == "keydown" || event.type == "keyup") {
// [ts] return event.keyCode;
// [ts] }
// [ts] return 0;
// [ts] },
// [ts] which: (event) => {
// `which` is an alias for either `keyCode` or `charCode` depending on the
// type of the event.
// [ts] if (event.type == "keypress") {
// [ts] return getEventCharCode(event);
// [ts] }
// [ts] if (event.type == "keydown" || event.type == "keyup") {
// [ts] return event.keyCode;
// [ts] }
// [ts] return 0;
// [ts] }
// [ts] });
// [ts] /**
// [ts] * @interface DragEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let SyntheticDragEvent = SyntheticMouseEvent.extend({
// [ts] dataTransfer: null
// [ts] });
// [ts] /**
// [ts] * @interface TouchEvent
// [ts] * @see http://www.w3.org/TR/touch-events/
// [ts] */
// [ts] let SyntheticTouchEvent = SyntheticUIEvent.extend({
// [ts] touches: null,
// [ts] targetTouches: null,
// [ts] changedTouches: null,
// [ts] altKey: null,
// [ts] metaKey: null,
// [ts] ctrlKey: null,
// [ts] shiftKey: null,
// [ts] getModifierState: getEventModifierState
// [ts] });
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/2009/WD-css3-transitions-20090320/#transition-events-
// [ts] * @see https://developer.mozilla.org/en-US/docs/Web/API/TransitionEvent
// [ts] */
// [ts] let SyntheticTransitionEvent = SyntheticEvent.extend({
// [ts] propertyName: null,
// [ts] elapsedTime: null,
// [ts] pseudoElement: null
// [ts] });
// [ts] /**
// [ts] * @interface WheelEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let SyntheticWheelEvent = SyntheticMouseEvent.extend({
// [ts] deltaX: (event) => {
// [ts] return if ("deltaX" == event) event.deltaX else  // Fallback to `wheelDeltaX` for Webkit and normalize (right is positive).
// [ts] if ("wheelDeltaX" == event) -event.wheelDeltaX else 0;
// [ts] },
// [ts] deltaY: (event) => {
// [ts] return if ("deltaY" == event) event.deltaY else  // Fallback to `wheelDeltaY` for Webkit and normalize (down is positive).
// [ts] if ("wheelDeltaY" == event) { -event.wheelDeltaY else; } // Fallback to `wheelDelta` for IE<9 and normalize (down is positive).
// [ts] if ("wheelDelta" == event) -event.wheelDelta else 0;
// [ts] },
// [ts] deltaZ: null,
// Browsers without "deltaMode" is reporting in raw wheel delta where one
// notch on the scroll is always +/- 120, roughly equivalent to pixels.
// A good approximation of DOM_DELTA_LINE (1) is 5% of viewport size or
// ~40 pixels, for DOM_DELTA_SCREEN (2) it is 87.5% of viewport size.
// [ts] deltaMode: null
// [ts] });
// [ts] let knownHTMLTopLevelTypes = [TOP_ABORT, TOP_CANCEL, TOP_CAN_PLAY, TOP_CAN_PLAY_THROUGH, TOP_CLOSE, TOP_DURATION_CHANGE, TOP_EMPTIED, TOP_ENCRYPTED, TOP_ENDED, TOP_ERROR, TOP_INPUT, TOP_INVALID, TOP_LOAD, TOP_LOADED_DATA, TOP_LOADED_METADATA, TOP_LOAD_START, TOP_PAUSE, TOP_PLAY, TOP_PLAYING, TOP_PROGRESS, TOP_RATE_CHANGE, TOP_RESET, TOP_SEEKED, TOP_SEEKING, TOP_STALLED, TOP_SUBMIT, TOP_SUSPEND, TOP_TIME_UPDATE, TOP_TOGGLE, TOP_VOLUME_CHANGE, TOP_WAITING];
// [ts] let SimpleEventPlugin = {
// simpleEventPluginEventTypes gets populated from
// the DOMEventProperties module.
// [ts] eventTypes: simpleEventPluginEventTypes,
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget, eventSystemFlags) => {
// [ts] let dispatchConfig = topLevelEventsToDispatchConfig.get(topLevelType);
// [ts] if (!dispatchConfig) {
// [ts] return null;
// [ts] }
// [ts] let EventConstructor;
// [ts] switch (topLevelType) {
// Firefox creates a keypress event for function keys too. This removes
// the unwanted keypress events. Enter is however both printable and
// non-printable. One would expect Tab to be as well (but it isn't).
// [ts] TOP_KEY_PRESS => {
// [ts] if (getEventCharCode(nativeEvent) == 0) {
// [ts] return null;
// [ts] }
// [ts] /* falls through */
// [ts] },
// [ts] TOP_KEY_DOWN, TOP_KEY_UP => {
// [ts] EventConstructor = SyntheticKeyboardEvent;
// break;
// [ts] },
// [ts] TOP_BLUR, TOP_FOCUS => {
// [ts] EventConstructor = SyntheticFocusEvent;
// break;
// [ts] },
// Firefox creates a click event on right mouse clicks. This removes the
// unwanted click events.
// [ts] TOP_CLICK => {
// [ts] if (nativeEvent.button == 2) {
// [ts] return null;
// [ts] }
// [ts] /* falls through */
// [ts] },
// [ts] TOP_AUX_CLICK, TOP_DOUBLE_CLICK, TOP_MOUSE_DOWN, TOP_MOUSE_MOVE, TOP_MOUSE_UP => { // TODO: Disabled elements should not respond to mouse events; },
// [ts] /* falls through */
// [ts] TOP_MOUSE_OUT, TOP_MOUSE_OVER, TOP_CONTEXT_MENU => {
// [ts] EventConstructor = SyntheticMouseEvent;
// break;
// [ts] },
// [ts] TOP_DRAG, TOP_DRAG_END, TOP_DRAG_ENTER, TOP_DRAG_EXIT, TOP_DRAG_LEAVE, TOP_DRAG_OVER, TOP_DRAG_START, TOP_DROP => {
// [ts] EventConstructor = SyntheticDragEvent;
// break;
// [ts] },
// [ts] TOP_TOUCH_CANCEL, TOP_TOUCH_END, TOP_TOUCH_MOVE, TOP_TOUCH_START => {
// [ts] EventConstructor = SyntheticTouchEvent;
// break;
// [ts] },
// [ts] TOP_ANIMATION_END, TOP_ANIMATION_ITERATION, TOP_ANIMATION_START => {
// [ts] EventConstructor = SyntheticAnimationEvent;
// break;
// [ts] },
// [ts] TOP_TRANSITION_END => {
// [ts] EventConstructor = SyntheticTransitionEvent;
// break;
// [ts] },
// [ts] TOP_SCROLL => {
// [ts] EventConstructor = SyntheticUIEvent;
// break;
// [ts] },
// [ts] TOP_WHEEL => {
// [ts] EventConstructor = SyntheticWheelEvent;
// break;
// [ts] },
// [ts] TOP_COPY, TOP_CUT, TOP_PASTE => {
// [ts] EventConstructor = SyntheticClipboardEvent;
// break;
// [ts] },
// [ts] TOP_GOT_POINTER_CAPTURE, TOP_LOST_POINTER_CAPTURE, TOP_POINTER_CANCEL, TOP_POINTER_DOWN, TOP_POINTER_MOVE, TOP_POINTER_OUT, TOP_POINTER_OVER, TOP_POINTER_UP => {
// [ts] EventConstructor = SyntheticPointerEvent;
// break;
// [ts] },
// [ts] else => {
// [ts] {
// [ts] if (knownHTMLTopLevelTypes.indexOf(topLevelType) == -1) {
// [ts] error("SimpleEventPlugin: Unhandled event type, `%s`. This warning " + "is likely caused by a bug in React. Please file an issue.", topLevelType);
// [ts] }
// [ts] } // HTML Events
// @see http://www.w3.org/TR/html5/index.html#events-0
// [ts] EventConstructor = SyntheticEvent;
// break;
// [ts] }
// [ts] }
// [ts] let event = EventConstructor.getPooled(dispatchConfig, targetInst, nativeEvent, nativeEventTarget);
// [ts] accumulateTwoPhaseDispatches(event);
// [ts] return event;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Specifies a deterministic ordering of `EventPlugin`s. A convenient way to
// [ts] * reason about plugins, without having to package every one of them. This
// [ts] * is better than having plugins be ordered in the same order that they
// [ts] * are injected because that ordering would be influenced by the packaging order.
// [ts] * `ResponderEventPlugin` must occur before `SimpleEventPlugin` so that
// [ts] * preventing default on events is convenient in `SimpleEventPlugin` handlers.
// [ts] */
// [ts] let DOMEventPluginOrder = ["ResponderEventPlugin", "SimpleEventPlugin", "EnterLeaveEventPlugin", "ChangeEventPlugin", "SelectEventPlugin", "BeforeInputEventPlugin"];
// [ts] /**
// [ts] * Inject modules for resolving DOM hierarchy and plugin ordering.
// [ts] */
// [ts] injectEventPluginOrder(DOMEventPluginOrder);
// [ts] setComponentTree(getFiberCurrentPropsFromNode$1, getInstanceFromNode$1, getNodeFromInstance$1);
// [ts] /**
// [ts] * Some important event plugins included by default (without having to require
// [ts] * them).
// [ts] */
// [ts] injectEventPluginsByName({
// [ts] SimpleEventPlugin: SimpleEventPlugin,
// [ts] EnterLeaveEventPlugin: EnterLeaveEventPlugin,
// [ts] ChangeEventPlugin: ChangeEventPlugin,
// [ts] SelectEventPlugin: SelectEventPlugin,
// [ts] BeforeInputEventPlugin: BeforeInputEventPlugin
// [ts] });
// Prefix measurements so that it's possible to filter them.
// Longer prefixes are hard to read in DevTools.
// [ts] let reactEmoji = "\u269B";
// [ts] let warningEmoji = "\u26D4";
// [ts] let supportsUserTiming = @typeName(performance) != "undefined" && @typeName(performance.mark) == "function" && @typeName(performance.clearMarks) == "function" && @typeName(performance.measure) == "function" && @typeName(performance.clearMeasures) == "function"; // Keep track of current fiber so that we know the path to unwind on pause.
// TODO: this looks the same as nextUnitOfWork in scheduler. Can we unify them?
// [ts] let currentFiber = null; // If we're in the middle of user code, which fiber and method is it?
// Reusing `currentFiber` would be confusing for this because user code fiber
// can change during commit phase too, but we don't need to unwind it (since
// lifecycles in the commit phase don't resemble a tree).
// [ts] let currentPhase = null;
// [ts] let currentPhaseFiber = null; // Did lifecycle hook schedule an update? This is often a performance problem,
// so we will keep track of it, and include it in the report.
// Track commits caused by cascading updates.
// [ts] let isCommitting = false;
// [ts] let hasScheduledUpdateInCurrentCommit = false;
// [ts] let hasScheduledUpdateInCurrentPhase = false;
// [ts] let commitCountInCurrentWorkLoop = 0;
// [ts] let effectCountInCurrentCommit = 0;
// to avoid stretch the commit phase with measurement overhead.
// [ts] let labelsInCurrentCommit = Set.init();
// [ts] let formatMarkName = (markName) => {
// [ts] return reactEmoji + " " + markName;
// [ts] };
// [ts] let formatLabel = (label, warning) => {
// [ts] let prefix = if (warning) warningEmoji + " " else reactEmoji + " ";
// [ts] let suffix = if (warning) " Warning: " + warning else "";
// [ts] return "" + prefix + label + suffix;
// [ts] };
// [ts] let beginMark = (markName) => {
// [ts] performance.mark(formatMarkName(markName));
// [ts] };
// [ts] let clearMark = (markName) => {
// [ts] performance.clearMarks(formatMarkName(markName));
// [ts] };
// [ts] let endMark = (label, markName, warning) => {
// [ts] let formattedMarkName = formatMarkName(markName);
// [ts] let formattedLabel = formatLabel(label, warning);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] performance.measure(formattedLabel, formattedMarkName);
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {} // If previous mark was missing for some reason, this will throw.)
// This could only happen if React crashed in an unexpected place earlier.
// Don't pile on with more errors.
// Clear marks immediately to avoid growing buffer.
// [ts] performance.clearMarks(formattedMarkName);
// [ts] performance.clearMeasures(formattedLabel);
// [ts] };
// [ts] let getFiberMarkName = (label, debugID) => {
// [ts] return label + " (#" + debugID + ")";
// [ts] };
// [ts] let getFiberLabel = (componentName, isMounted, phase) => {
// [ts] if (phase == null) {
// These are composite component total time measurements.
// [ts] return componentName + " [" + (if (isMounted) "update" else "mount") + "]";
// [ts] } else {
// Composite component methods.
// [ts] return componentName + "." + phase;
// [ts] }
// [ts] };
// [ts] let beginFiberMark = (fiber, phase) => {
// [ts] let componentName = getComponentName(fiber.type) || "Unknown";
// [ts] let debugID = fiber._debugID;
// [ts] let isMounted = fiber.alternate != null;
// [ts] let label = getFiberLabel(componentName, isMounted, phase);
// [ts] if (isCommitting && labelsInCurrentCommit.has(label)) {
// During the commit phase, we don't show duplicate labels because
// there is a fixed overhead for every measurement, and we don't
// want to stretch the commit phase beyond necessary.
// [ts] return false;
// [ts] }
// [ts] labelsInCurrentCommit.add(label);
// [ts] let markName = getFiberMarkName(label, debugID);
// [ts] beginMark(markName);
// [ts] return true;
// [ts] };
// [ts] let clearFiberMark = (fiber, phase) => {
// [ts] let componentName = getComponentName(fiber.type) || "Unknown";
// [ts] let debugID = fiber._debugID;
// [ts] let isMounted = fiber.alternate != null;
// [ts] let label = getFiberLabel(componentName, isMounted, phase);
// [ts] let markName = getFiberMarkName(label, debugID);
// [ts] clearMark(markName);
// [ts] };
// [ts] let endFiberMark = (fiber, phase, warning) => {
// [ts] let componentName = getComponentName(fiber.type) || "Unknown";
// [ts] let debugID = fiber._debugID;
// [ts] let isMounted = fiber.alternate != null;
// [ts] let label = getFiberLabel(componentName, isMounted, phase);
// [ts] let markName = getFiberMarkName(label, debugID);
// [ts] endMark(label, markName, warning);
// [ts] };
// [ts] let shouldIgnoreFiber = (fiber) => {
// Host components should be skipped in the timeline.
// We could check typeof fiber.type, but does this work with RN?
// [ts] switch (fiber.tag) {
// [ts] HostRoot, HostComponent, HostText, HostPortal, Fragment, ContextProvider, ContextConsumer, Mode => {
// [ts] return true;
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] };
// [ts] let clearPendingPhaseMeasurement = () => {
// [ts] if (currentPhase != null && currentPhaseFiber != null) {
// [ts] clearFiberMark(currentPhaseFiber, currentPhase);
// [ts] }
// [ts] currentPhaseFiber = null;
// [ts] currentPhase = null;
// [ts] hasScheduledUpdateInCurrentPhase = false;
// [ts] };
// [ts] let pauseTimers = () => {
// Stops all currently active measurements so that they can be resumed
// if we continue in a later deferred loop from the same unit of work.
// [ts] let fiber = currentFiber;
// [ts] while (fiber) {
// [ts] if (fiber._debugIsCurrentlyTiming) {
// [ts] endFiberMark(fiber, null, null);
// [ts] }
// [ts] fiber = fiber.return;
// [ts] }
// [ts] };
// [ts] let resumeTimersRecursively = (fiber) => {
// [ts] if (fiber.return != null) {
// [ts] resumeTimersRecursively(fiber.return);
// [ts] }
// [ts] if (fiber._debugIsCurrentlyTiming) {
// [ts] beginFiberMark(fiber, null);
// [ts] }
// [ts] };
// [ts] let resumeTimers = () => {
// Resumes all measurements that were active during the last deferred loop.
// [ts] if (currentFiber != null) {
// [ts] resumeTimersRecursively(currentFiber);
// [ts] }
// [ts] };
// [ts] fn recordEffect() {
// [ts] {
// [ts] effectCountInCurrentCommit++;
// [ts] }
// [ts] }
// [ts] fn recordScheduleUpdate() {
// [ts] {
// [ts] if (isCommitting) {
// [ts] hasScheduledUpdateInCurrentCommit = true;
// [ts] }
// [ts] if (currentPhase != null && currentPhase != "componentWillMount" && currentPhase != "componentWillReceiveProps") {
// [ts] hasScheduledUpdateInCurrentPhase = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn startWorkTimer(fiber: auto) {
// [ts] {
// [ts] if (!supportsUserTiming || shouldIgnoreFiber(fiber)) {
// [ts] return;
// [ts] } // If we pause, this is the fiber to unwind from.
// [ts] currentFiber = fiber;
// [ts] if (!beginFiberMark(fiber, null)) {
// [ts] return;
// [ts] }
// [ts] fiber._debugIsCurrentlyTiming = true;
// [ts] }
// [ts] }
// [ts] fn cancelWorkTimer(fiber: auto) {
// [ts] {
// [ts] if (!supportsUserTiming || shouldIgnoreFiber(fiber)) {
// [ts] return;
// [ts] } // Remember we shouldn't complete measurement for this fiber.
// Otherwise flamechart will be deep even for small updates.
// [ts] fiber._debugIsCurrentlyTiming = false;
// [ts] clearFiberMark(fiber, null);
// [ts] }
// [ts] }
// [ts] fn stopWorkTimer(fiber: auto) {
// [ts] {
// [ts] if (!supportsUserTiming || shouldIgnoreFiber(fiber)) {
// [ts] return;
// [ts] } // If we pause, its parent is the fiber to unwind from.
// [ts] currentFiber = fiber.return;
// [ts] if (!fiber._debugIsCurrentlyTiming) {
// [ts] return;
// [ts] }
// [ts] fiber._debugIsCurrentlyTiming = false;
// [ts] endFiberMark(fiber, null, null);
// [ts] }
// [ts] }
// [ts] fn stopFailedWorkTimer(fiber: auto) {
// [ts] {
// [ts] if (!supportsUserTiming || shouldIgnoreFiber(fiber)) {
// [ts] return;
// [ts] } // If we pause, its parent is the fiber to unwind from.
// [ts] currentFiber = fiber.return;
// [ts] if (!fiber._debugIsCurrentlyTiming) {
// [ts] return;
// [ts] }
// [ts] fiber._debugIsCurrentlyTiming = false;
// [ts] let warning = if (fiber.tag == SuspenseComponent) "Rendering was suspended" else "An error was thrown inside this error boundary";
// [ts] endFiberMark(fiber, null, warning);
// [ts] }
// [ts] }
// [ts] fn startPhaseTimer(fiber: auto, phase: auto) {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] clearPendingPhaseMeasurement();
// [ts] if (!beginFiberMark(fiber, phase)) {
// [ts] return;
// [ts] }
// [ts] currentPhaseFiber = fiber;
// [ts] currentPhase = phase;
// [ts] }
// [ts] }
// [ts] fn stopPhaseTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] if (currentPhase != null && currentPhaseFiber != null) {
// [ts] let warning = if (hasScheduledUpdateInCurrentPhase) "Scheduled a cascading update" else null;
// [ts] endFiberMark(currentPhaseFiber, currentPhase, warning);
// [ts] }
// [ts] currentPhase = null;
// [ts] currentPhaseFiber = null;
// [ts] }
// [ts] }
// [ts] fn startWorkLoopTimer(nextUnitOfWork: auto) {
// [ts] {
// [ts] currentFiber = nextUnitOfWork;
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] commitCountInCurrentWorkLoop = 0; // This is top level call.
// Any other measurements are performed within.
// [ts] beginMark("(React Tree Reconciliation)"); // Resume any measurements that were in progress during the last loop.
// [ts] resumeTimers();
// [ts] }
// [ts] }
// [ts] fn stopWorkLoopTimer(interruptedBy: auto, didCompleteRoot: auto) {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] let warning = null;
// [ts] if (interruptedBy != null) {
// [ts] if (interruptedBy.tag == HostRoot) {
// [ts] warning = "A top-level update interrupted the previous render";
// [ts] } else {
// [ts] let componentName = getComponentName(interruptedBy.type) || "Unknown";
// [ts] warning = "An update to " + componentName + " interrupted the previous render";
// [ts] }
// [ts] } else if (commitCountInCurrentWorkLoop > 1) {
// [ts] warning = "There were cascading updates";
// [ts] }
// [ts] commitCountInCurrentWorkLoop = 0;
// [ts] let label = if (didCompleteRoot) "(React Tree Reconciliation: Completed Root)" else "(React Tree Reconciliation: Yielded)"; // Pause any measurements until the next loop.
// [ts] pauseTimers();
// [ts] endMark(label, "(React Tree Reconciliation)", warning);
// [ts] }
// [ts] }
// [ts] fn startCommitTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] isCommitting = true;
// [ts] hasScheduledUpdateInCurrentCommit = false;
// [ts] labelsInCurrentCommit.clear();
// [ts] beginMark("(Committing Changes)");
// [ts] }
// [ts] }
// [ts] fn stopCommitTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] let warning = null;
// [ts] if (hasScheduledUpdateInCurrentCommit) {
// [ts] warning = "Lifecycle hook scheduled a cascading update";
// [ts] } else if (commitCountInCurrentWorkLoop > 0) {
// [ts] warning = "Caused by a cascading update in earlier commit";
// [ts] }
// [ts] hasScheduledUpdateInCurrentCommit = false;
// [ts] commitCountInCurrentWorkLoop++;
// [ts] isCommitting = false;
// [ts] labelsInCurrentCommit.clear();
// [ts] endMark("(Committing Changes)", "(Committing Changes)", warning);
// [ts] }
// [ts] }
// [ts] fn startCommitSnapshotEffectsTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] effectCountInCurrentCommit = 0;
// [ts] beginMark("(Committing Snapshot Effects)");
// [ts] }
// [ts] }
// [ts] fn stopCommitSnapshotEffectsTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] let count = effectCountInCurrentCommit;
// [ts] effectCountInCurrentCommit = 0;
// [ts] endMark("(Committing Snapshot Effects: " + count + " Total)", "(Committing Snapshot Effects)", null);
// [ts] }
// [ts] }
// [ts] fn startCommitHostEffectsTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] effectCountInCurrentCommit = 0;
// [ts] beginMark("(Committing Host Effects)");
// [ts] }
// [ts] }
// [ts] fn stopCommitHostEffectsTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] let count = effectCountInCurrentCommit;
// [ts] effectCountInCurrentCommit = 0;
// [ts] endMark("(Committing Host Effects: " + count + " Total)", "(Committing Host Effects)", null);
// [ts] }
// [ts] }
// [ts] fn startCommitLifeCyclesTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] effectCountInCurrentCommit = 0;
// [ts] beginMark("(Calling Lifecycle Methods)");
// [ts] }
// [ts] }
// [ts] fn stopCommitLifeCyclesTimer() {
// [ts] {
// [ts] if (!supportsUserTiming) {
// [ts] return;
// [ts] }
// [ts] let count = effectCountInCurrentCommit;
// [ts] effectCountInCurrentCommit = 0;
// [ts] endMark("(Calling Lifecycle Methods: " + count + " Total)", "(Calling Lifecycle Methods)", null);
// [ts] }
// [ts] }
// [ts] let valueStack = [];
// [ts] let fiberStack;
// [ts] {
// [ts] fiberStack = [];
// [ts] }
// [ts] let index = -1;
// [ts] fn createCursor(defaultValue: auto) {
// [ts] return {
// [ts] current: defaultValue
// [ts] };
// [ts] }
// [ts] fn pop(cursor: auto, fiber: auto) {
// [ts] if (index < 0) {
// [ts] {
// [ts] error("Unexpected pop.");
// [ts] }
// [ts] return;
// [ts] }
// [ts] {
// [ts] if (fiber != fiberStack[index]) {
// [ts] error("Unexpected Fiber popped.");
// [ts] }
// [ts] }
// [ts] cursor.current = valueStack[index];
// [ts] valueStack[index] = null;
// [ts] {
// [ts] fiberStack[index] = null;
// [ts] }
// [ts] index--;
// [ts] }
// [ts] fn push(cursor: auto, value: auto, fiber: auto) {
// [ts] index++;
// [ts] valueStack[index] = cursor.current;
// [ts] {
// [ts] fiberStack[index] = fiber;
// [ts] }
// [ts] cursor.current = value;
// [ts] }
// [ts] let warnedAboutMissingGetChildContext;
// [ts] {
// [ts] warnedAboutMissingGetChildContext = {};
// [ts] }
// [ts] let emptyContextObject = {};
// [ts] {
// [ts] Object.freeze(emptyContextObject);
// [ts] } // A cursor to the current merged context object on the stack.
// [ts] let contextStackCursor = createCursor(emptyContextObject); // A cursor to a boolean indicating whether the context has changed.
// [ts] let didPerformWorkStackCursor = createCursor(false); // Keep track of the previous context object that was on the stack.
// We use this to get access to the parent context after we have already
// pushed the next context provider, and now need to merge their contexts.
// [ts] let previousContext = emptyContextObject;
// [ts] fn getUnmaskedContext(workInProgress: auto, Component: auto, didPushOwnContextIfProvider: auto) {
// [ts] {
// [ts] if (didPushOwnContextIfProvider && isContextProvider(Component)) {
// If the fiber is a context provider itself, when we read its context
// we may have already pushed its own child context on the stack. A context
// provider should not "see" its own child context. Therefore we read the
// previous (parent) context instead for a context provider.
// [ts] return previousContext;
// [ts] }
// [ts] return contextStackCursor.current;
// [ts] }
// [ts] }
// [ts] fn cacheContext(workInProgress: auto, unmaskedContext: auto, maskedContext: auto) {
// [ts] {
// [ts] let instance = workInProgress.stateNode;
// [ts] instance.__reactInternalMemoizedUnmaskedChildContext = unmaskedContext;
// [ts] instance.__reactInternalMemoizedMaskedChildContext = maskedContext;
// [ts] }
// [ts] }
// [ts] fn getMaskedContext(workInProgress: auto, unmaskedContext: auto) {
// [ts] {
// [ts] let type = workInProgress.type;
// [ts] let contextTypes = type.contextTypes;
// [ts] if (!contextTypes) {
// [ts] return emptyContextObject;
// [ts] } // Avoid recreating masked context unless unmasked context has changed.
// Failing to do this will result in unnecessary calls to componentWillReceiveProps.
// This may trigger infinite loops if componentWillReceiveProps calls setState.
// [ts] let instance = workInProgress.stateNode;
// [ts] if (instance && instance.__reactInternalMemoizedUnmaskedChildContext == unmaskedContext) {
// [ts] return instance.__reactInternalMemoizedMaskedChildContext;
// [ts] }
// [ts] let context = {};
// [ts] for (let key in contextTypes) {
// [ts] context[key] = unmaskedContext[key];
// [ts] }
// [ts] {
// [ts] let name = getComponentName(type) || "Unknown";
// [ts] checkPropTypes_1(contextTypes, context, "context", name, getCurrentFiberStackInDev);
// [ts] } // Cache unmasked context so we can avoid recreating masked context unless necessary.
// Context is created before the class component is instantiated so check for instance.
// [ts] if (instance) {
// [ts] cacheContext(workInProgress, unmaskedContext, context);
// [ts] }
// [ts] return context;
// [ts] }
// [ts] }
// [ts] fn hasContextChanged() {
// [ts] {
// [ts] return didPerformWorkStackCursor.current;
// [ts] }
// [ts] }
// [ts] fn isContextProvider(type: auto) {
// [ts] {
// [ts] let childContextTypes = type.childContextTypes;
// [ts] return childContextTypes != null && childContextTypes != null;
// [ts] }
// [ts] }
// [ts] fn popContext(fiber: auto) {
// [ts] {
// [ts] pop(didPerformWorkStackCursor, fiber);
// [ts] pop(contextStackCursor, fiber);
// [ts] }
// [ts] }
// [ts] fn popTopLevelContextObject(fiber: auto) {
// [ts] {
// [ts] pop(didPerformWorkStackCursor, fiber);
// [ts] pop(contextStackCursor, fiber);
// [ts] }
// [ts] }
// [ts] fn pushTopLevelContextObject(fiber: auto, context: auto, didChange: auto) {
// [ts] {
// [ts] if (!(contextStackCursor.current == emptyContextObject)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unexpected context found on stack. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] push(contextStackCursor, context, fiber);
// [ts] push(didPerformWorkStackCursor, didChange, fiber);
// [ts] }
// [ts] }
// [ts] fn processChildContext(fiber: auto, type: auto, parentContext: auto) {
// [ts] {
// [ts] let instance = fiber.stateNode;
// [ts] let childContextTypes = type.childContextTypes; // TODO (bvaughn) Replace this behavior with an invariant() in the future.
// It has only been added in Fiber to match the (unintentional) behavior in Stack.
// [ts] if (@typeName(instance.getChildContext) != "function") {
// [ts] {
// [ts] let componentName = getComponentName(type) || "Unknown";
// [ts] if (!warnedAboutMissingGetChildContext[componentName]) {
// [ts] warnedAboutMissingGetChildContext[componentName] = true;
// [ts] error("%s.childContextTypes is specified but there is no getChildContext() method " + "on the instance. You can either define getChildContext() on %s or remove " + "childContextTypes from it.", componentName, componentName);
// [ts] }
// [ts] }
// [ts] return parentContext;
// [ts] }
// [ts] let childContext;
// [ts] startPhaseTimer(fiber, "getChildContext");
// [ts] childContext = instance.getChildContext();
// [ts] stopPhaseTimer();
// [ts] for (let contextKey in childContext) {
// [ts] if (!(contextKey == childContextTypes)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( (getComponentName(type) || 'Unknown') + ".getChildContext(): key \"" + contextKey + "\" is not defined in childContextTypes." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] let name = getComponentName(type) || "Unknown";
// [ts] checkPropTypes_1(childContextTypes, childContext, "child context", name, // In practice, there is one case in which we won't get a stack. It's when
// somebody calls unstable_renderSubtreeIntoContainer() and we process
// context from the parent component instance. The stack will be missing
// because it's outside of the reconciliation, and so the pointer has not
// been set. This is rare and doesn't matter. We'll also remove that API.
// [ts] getCurrentFiberStackInDev);
// [ts] }
// [ts] return _assign({}, parentContext, {}, childContext);
// [ts] }
// [ts] }
// [ts] fn pushContextProvider(workInProgress: auto) {
// [ts] {
// [ts] let instance = workInProgress.stateNode; // We push the context as early as possible to ensure stack integrity.
// If the instance does not exist yet, we will push null at first,
// and replace it on the stack later when invalidating the context.
// [ts] let memoizedMergedChildContext = instance && instance.__reactInternalMemoizedMergedChildContext || emptyContextObject; // Remember the parent context so we can merge with it later.
// Inherit the parent's did-perform-work value to avoid inadvertently blocking updates.
// [ts] previousContext = contextStackCursor.current;
// [ts] push(contextStackCursor, memoizedMergedChildContext, workInProgress);
// [ts] push(didPerformWorkStackCursor, didPerformWorkStackCursor.current, workInProgress);
// [ts] return true;
// [ts] }
// [ts] }
// [ts] fn invalidateContextProvider(workInProgress: auto, type: auto, didChange: auto) {
// [ts] {
// [ts] let instance = workInProgress.stateNode;
// [ts] if (!instance) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected to have an instance by this point. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] if (didChange) {
// Merge parent and own context.
// Skip this if we're not updating due to sCU.
// This avoids unnecessarily recomputing memoized values.
// [ts] let mergedContext = processChildContext(workInProgress, type, previousContext);
// [ts] instance.__reactInternalMemoizedMergedChildContext = mergedContext; // Replace the old (or empty) context with the new one.
// It is important to unwind the context in the reverse order.
// [ts] pop(didPerformWorkStackCursor, workInProgress);
// [ts] pop(contextStackCursor, workInProgress); // Now push the new context and mark that it has changed.
// [ts] push(contextStackCursor, mergedContext, workInProgress);
// [ts] push(didPerformWorkStackCursor, didChange, workInProgress);
// [ts] } else {
// [ts] pop(didPerformWorkStackCursor, workInProgress);
// [ts] push(didPerformWorkStackCursor, didChange, workInProgress);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn findCurrentUnmaskedContext(fiber: auto) {
// [ts] {
// Currently this is only used with renderSubtreeIntoContainer; not sure if it
// makes sense elsewhere
// [ts] if (!(isFiberMounted(fiber) && fiber.tag == ClassComponent)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected subtree parent to be a mounted class component. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] let node = fiber;
// [ts] do {
// [ts] switch (node.tag) {
// [ts] HostRoot => {
// [ts] return node.stateNode.context;
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] let Component = node.type;
// [ts] if (isContextProvider(Component)) {
// [ts] return node.stateNode.__reactInternalMemoizedMergedChildContext;
// [ts] }
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] node = node.return;
// [ts] } while (node != null);
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Found unexpected detached subtree parent. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let LegacyRoot = 0;
// [ts] let BlockingRoot = 1;
// [ts] let ConcurrentRoot = 2;
// [ts] let ReactInternals$2 = React.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED;
// [ts] let _ReactInternals$Sched$1 = ReactInternals$2.SchedulerTracing,
// [ts] __interactionsRef = _ReactInternals$Sched$1.__interactionsRef,
// [ts] __subscriberRef = _ReactInternals$Sched$1.__subscriberRef,
// [ts] unstable_clear = _ReactInternals$Sched$1.unstable_clear,
// [ts] unstable_getCurrent = _ReactInternals$Sched$1.unstable_getCurrent,
// [ts] unstable_getThreadID = _ReactInternals$Sched$1.unstable_getThreadID,
// [ts] unstable_subscribe = _ReactInternals$Sched$1.unstable_subscribe,
// [ts] unstable_trace = _ReactInternals$Sched$1.unstable_trace,
// [ts] unstable_unsubscribe = _ReactInternals$Sched$1.unstable_unsubscribe,
// [ts] unstable_wrap = _ReactInternals$Sched$1.unstable_wrap;
// [ts] let Scheduler_runWithPriority = unstable_runWithPriority,
// [ts] Scheduler_scheduleCallback = unstable_scheduleCallback,
// [ts] Scheduler_cancelCallback = unstable_cancelCallback,
// [ts] Scheduler_shouldYield = unstable_shouldYield,
// [ts] Scheduler_requestPaint = unstable_requestPaint,
// [ts] Scheduler_now = unstable_now,
// [ts] Scheduler_getCurrentPriorityLevel = unstable_getCurrentPriorityLevel,
// [ts] Scheduler_ImmediatePriority = unstable_ImmediatePriority,
// [ts] Scheduler_UserBlockingPriority = unstable_UserBlockingPriority,
// [ts] Scheduler_NormalPriority = unstable_NormalPriority,
// [ts] Scheduler_LowPriority = unstable_LowPriority,
// [ts] Scheduler_IdlePriority = unstable_IdlePriority;
// [ts] {
// Provide explicit error message when production+profiling bundle of e.g.
// react-dom is used with production (non-profiling) bundle of
// scheduler/tracing
// [ts] if (!(__interactionsRef != null && __interactionsRef.current != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "It is not supported to run the profiling version of a renderer (for example, `react-dom/profiling`) without also replacing the `scheduler/tracing` module with `scheduler/tracing-profiling`. Your bundler might have a setting for aliasing both modules. Learn more at http://fb.me/react-profiling" );)
// [ts] }
// [ts] }
// [ts] }
// [ts] let fakeCallbackNode = {}; // Except for NoPriority, these correspond to Scheduler priorities. We use
// ascending numbers so we can compare them like numbers. They start at 90 to
// avoid clashing with Scheduler's priorities.
// [ts] let ImmediatePriority = 99;
// [ts] let UserBlockingPriority$1 = 98;
// [ts] let NormalPriority = 97;
// [ts] let LowPriority = 96;
// [ts] let IdlePriority = 95; // NoPriority is the absence of priority. Also React-only.
// [ts] let NoPriority = 90;
// [ts] let shouldYield = Scheduler_shouldYield;
// [ts] let requestPaint = // Fall back gracefully if we're running an older version of Scheduler.
// [ts] if (Scheduler_requestPaint != null) Scheduler_requestPaint else () => {};
// [ts] let syncQueue = null;
// [ts] let immediateQueueCallbackNode = null;
// [ts] let isFlushingSyncQueue = false;
// [ts] let initialTimeMs = Scheduler_now(); // If the initial timestamp is reasonably small, use Scheduler's `now` directly.
// This will be the case for modern browsers that support `performance.now`. In
// older browsers, Scheduler falls back to `Date.now`, which returns a Unix
// timestamp. In that case, subtract the module initialization time to simulate
// the behavior of performance.now and keep our times small enough to fit
// within 32 bits.
// TODO: Consider lifting this into Scheduler.
// [ts] let now = if (initialTimeMs < 10000) Scheduler_now else () => {
// [ts] return Scheduler_now() - initialTimeMs;
// [ts] };
// [ts] fn getCurrentPriorityLevel() {
// [ts] switch (Scheduler_getCurrentPriorityLevel()) {
// [ts] Scheduler_ImmediatePriority => {
// [ts] return ImmediatePriority;
// [ts] },
// [ts] Scheduler_UserBlockingPriority => {
// [ts] return UserBlockingPriority$1;
// [ts] },
// [ts] Scheduler_NormalPriority => {
// [ts] return NormalPriority;
// [ts] },
// [ts] Scheduler_LowPriority => {
// [ts] return LowPriority;
// [ts] },
// [ts] Scheduler_IdlePriority => {
// [ts] return IdlePriority;
// [ts] },
// [ts] else => {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unknown priority level." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn reactPriorityToSchedulerPriority(reactPriorityLevel: auto) {
// [ts] switch (reactPriorityLevel) {
// [ts] ImmediatePriority => {
// [ts] return Scheduler_ImmediatePriority;
// [ts] },
// [ts] UserBlockingPriority$1 => {
// [ts] return Scheduler_UserBlockingPriority;
// [ts] },
// [ts] NormalPriority => {
// [ts] return Scheduler_NormalPriority;
// [ts] },
// [ts] LowPriority => {
// [ts] return Scheduler_LowPriority;
// [ts] },
// [ts] IdlePriority => {
// [ts] return Scheduler_IdlePriority;
// [ts] },
// [ts] else => {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unknown priority level." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] (reactPriorityLevel, fn) => {
// [ts] let priorityLevel = reactPriorityToSchedulerPriority(reactPriorityLevel);
// [ts] return Scheduler_runWithPriority(priorityLevel, fn);
// [ts] }
// [ts] fn scheduleCallback(reactPriorityLevel: auto, callback: auto, options: auto) {
// [ts] let priorityLevel = reactPriorityToSchedulerPriority(reactPriorityLevel);
// [ts] return Scheduler_scheduleCallback(priorityLevel, callback, options);
// [ts] }
// [ts] fn scheduleSyncCallback(callback: auto) {
// Push this callback into an internal queue. We'll flush these either in
// the next tick, or earlier if something calls `flushSyncCallbackQueue`.
// [ts] if (syncQueue == null) {
// [ts] syncQueue = [callback]; // Flush the queue in the next tick, at the earliest.
// [ts] immediateQueueCallbackNode = Scheduler_scheduleCallback(Scheduler_ImmediatePriority, flushSyncCallbackQueueImpl);
// [ts] } else {
// Push onto existing queue. Don't need to schedule a callback because
// we already scheduled one when we created the queue.
// [ts] syncQueue.push(callback);
// [ts] }
// [ts] return fakeCallbackNode;
// [ts] }
// [ts] fn cancelCallback(callbackNode: auto) {
// [ts] if (callbackNode != fakeCallbackNode) {
// [ts] Scheduler_cancelCallback(callbackNode);
// [ts] }
// [ts] }
// [ts] fn flushSyncCallbackQueue() {
// [ts] if (immediateQueueCallbackNode != null) {
// [ts] let node = immediateQueueCallbackNode;
// [ts] immediateQueueCallbackNode = null;
// [ts] Scheduler_cancelCallback(node);
// [ts] }
// [ts] flushSyncCallbackQueueImpl();
// [ts] }
// [ts] fn flushSyncCallbackQueueImpl() {
// [ts] if (!isFlushingSyncQueue && syncQueue != null) {
// Prevent re-entrancy.
// [ts] isFlushingSyncQueue = true;
// [ts] let i = 0;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let _isSync = true;
// [ts] let queue = syncQueue;
// [ts] runWithPriority$1(ImmediatePriority, () => {
// [ts] while (i < queue.length) { // [cap-migrate] for-step: i++
// [ts] let callback = queue[i];
// [ts] do {
// [ts] callback = callback(_isSync);
// [ts] } while (callback != null);
// [ts] }
// [ts] });
// [ts] syncQueue = null;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// If something throws, leave the remaining callbacks on the queue.
// [ts] if (syncQueue != null) {
// [ts] syncQueue = syncQueue.slice(i + 1);
// [ts] } // Resume flushing in the next tick
// [ts] Scheduler_scheduleCallback(Scheduler_ImmediatePriority, flushSyncCallbackQueue);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] isFlushingSyncQueue = false;
// [ts] }
// [ts] }
// [ts] }
// [ts] let NoMode = 0;
// [ts] let StrictMode = 1; // TODO: Remove BlockingMode and ConcurrentMode by reading from the root
// tag instead
// [ts] let BlockingMode = 2;
// [ts] let ConcurrentMode = 4;
// [ts] let ProfileMode = 8;
// Max 31 bit integer. The max integer size in V8 for 32-bit systems.
// Math.pow(2, 30) - 1
// 0b111111111111111111111111111111
// [ts] let MAX_SIGNED_31_BIT_INT = 1073741823;
// [ts] let NoWork = 0; // TODO: Think of a better name for Never. The key difference with Idle is that
// Never work can be committed in an inconsistent state without tearing the UI.
// The main example is offscreen content, like a hidden subtree. So one possible
// name is Offscreen. However, it also includes dehydrated Suspense boundaries,
// which are inconsistent in the sense that they haven't finished yet, but
// aren't visibly inconsistent because the server rendered HTML matches what the
// hydrated tree would look like.
// [ts] let Never = 1; // Idle is slightly higher priority than Never. It must completely finish in
// order to be consistent.
// [ts] let Idle = 2; // Continuous Hydration is slightly higher than Idle and is used to increase
// priority of hover targets.
// [ts] let ContinuousHydration = 3;
// [ts] let Sync = MAX_SIGNED_31_BIT_INT;
// [ts] let Batched = Sync - 1;
// [ts] let UNIT_SIZE = 10;
// [ts] let MAGIC_NUMBER_OFFSET = Batched - 1; // 1 unit of expiration time represents 10ms.
// [ts] fn msToExpirationTime(ms: auto) {
// Always subtract from the offset so that we don't clash with the magic number for NoWork.
// [ts] return MAGIC_NUMBER_OFFSET - (ms / UNIT_SIZE | 0);
// [ts] }
// [ts] fn expirationTimeToMs(expirationTime: auto) {
// [ts] return (MAGIC_NUMBER_OFFSET - expirationTime) * UNIT_SIZE;
// [ts] }
// [ts] fn ceiling(num: auto, precision: auto) {
// [ts] return ((num / precision | 0) + 1) * precision;
// [ts] }
// [ts] fn computeExpirationBucket(currentTime: auto, expirationInMs: auto, bucketSizeMs: auto) {
// [ts] return MAGIC_NUMBER_OFFSET - ceiling(MAGIC_NUMBER_OFFSET - currentTime + expirationInMs / UNIT_SIZE, bucketSizeMs / UNIT_SIZE);
// [ts] } // TODO: This corresponds to Scheduler's NormalPriority, not LowPriority. Update
// the names to reflect.
// [ts] let LOW_PRIORITY_EXPIRATION = 5000;
// [ts] let LOW_PRIORITY_BATCH_SIZE = 250;
// [ts] fn computeAsyncExpiration(currentTime: auto) {
// [ts] return computeExpirationBucket(currentTime, LOW_PRIORITY_EXPIRATION, LOW_PRIORITY_BATCH_SIZE);
// [ts] }
// [ts] fn computeSuspenseExpiration(currentTime: auto, timeoutMs: auto) {
// TODO: Should we warn if timeoutMs is lower than the normal pri expiration time?
// [ts] return computeExpirationBucket(currentTime, timeoutMs, LOW_PRIORITY_BATCH_SIZE);
// [ts] } // We intentionally set a higher expiration time for interactive updates in
// dev than in production.
//
// If the main thread is being blocked so long that you hit the expiration,
// it's a problem that could be solved with better scheduling.
//
// People will be more likely to notice this and fix it with the long
// expiration time in development.
//
// In production we opt for better UX at the risk of masking scheduling
// problems, by expiring fast.
// [ts] let HIGH_PRIORITY_EXPIRATION =  500 ;
// [ts] let HIGH_PRIORITY_BATCH_SIZE = 100;
// [ts] fn computeInteractiveExpiration(currentTime: auto) {
// [ts] return computeExpirationBucket(currentTime, HIGH_PRIORITY_EXPIRATION, HIGH_PRIORITY_BATCH_SIZE);
// [ts] }
// [ts] fn inferPriorityFromExpirationTime(currentTime: auto, expirationTime: auto) {
// [ts] if (expirationTime == Sync) {
// [ts] return ImmediatePriority;
// [ts] }
// [ts] if (expirationTime == Never || expirationTime == Idle) {
// [ts] return IdlePriority;
// [ts] }
// [ts] let msUntil = expirationTimeToMs(expirationTime) - expirationTimeToMs(currentTime);
// [ts] if (msUntil <= 0) {
// [ts] return ImmediatePriority;
// [ts] }
// [ts] if (msUntil <= HIGH_PRIORITY_EXPIRATION + HIGH_PRIORITY_BATCH_SIZE) {
// [ts] return UserBlockingPriority$1;
// [ts] }
// [ts] if (msUntil <= LOW_PRIORITY_EXPIRATION + LOW_PRIORITY_BATCH_SIZE) {
// [ts] return NormalPriority;
// [ts] } // TODO: Handle LowPriority
// Assume anything lower has idle priority
// [ts] return IdlePriority;
// [ts] }
// [ts] let ReactStrictModeWarnings = {
// [ts] recordUnsafeLifecycleWarnings: (fiber, instance) => {},
// [ts] flushPendingUnsafeLifecycleWarnings: () => {},
// [ts] recordLegacyContextWarning: (fiber, instance) => {},
// [ts] flushLegacyContextWarning: () => {},
// [ts] discardPendingWarnings: () => {}
// [ts] };
// [ts] {
// [ts] let findStrictRoot = (fiber) => {
// [ts] let maybeStrictRoot = null;
// [ts] let node = fiber;
// [ts] while (node != null) {
// [ts] if (node.mode & StrictMode) {
// [ts] maybeStrictRoot = node;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] return maybeStrictRoot;
// [ts] };
// [ts] let setToSortedString = (set) => {
// [ts] let array = [];
// [ts] set.forEach((value) => {
// [ts] array.push(value);
// [ts] });
// [ts] return array.sort().join(", ");
// [ts] };
// [ts] let pendingComponentWillMountWarnings = [];
// [ts] let pendingUNSAFE_ComponentWillMountWarnings = [];
// [ts] let pendingComponentWillReceivePropsWarnings = [];
// [ts] let pendingUNSAFE_ComponentWillReceivePropsWarnings = [];
// [ts] let pendingComponentWillUpdateWarnings = [];
// [ts] let pendingUNSAFE_ComponentWillUpdateWarnings = []; // Tracks components we have already warned about.
// [ts] let didWarnAboutUnsafeLifecycles = Set.init();
// [ts] ReactStrictModeWarnings.recordUnsafeLifecycleWarnings = (fiber, instance) => {
// Dedup strategy: Warn once per component.
// [ts] if (didWarnAboutUnsafeLifecycles.has(fiber.type)) {
// [ts] return;
// [ts] }
// [ts] if (@typeName(instance.componentWillMount) == "function" && // Don't warn about react-lifecycles-compat polyfilled components.
// [ts] instance.componentWillMount.__suppressDeprecationWarning != true) {
// [ts] pendingComponentWillMountWarnings.push(fiber);
// [ts] }
// [ts] if (fiber.mode & StrictMode && @typeName(instance.UNSAFE_componentWillMount) == "function") {
// [ts] pendingUNSAFE_ComponentWillMountWarnings.push(fiber);
// [ts] }
// [ts] if (@typeName(instance.componentWillReceiveProps) == "function" && instance.componentWillReceiveProps.__suppressDeprecationWarning != true) {
// [ts] pendingComponentWillReceivePropsWarnings.push(fiber);
// [ts] }
// [ts] if (fiber.mode & StrictMode && @typeName(instance.UNSAFE_componentWillReceiveProps) == "function") {
// [ts] pendingUNSAFE_ComponentWillReceivePropsWarnings.push(fiber);
// [ts] }
// [ts] if (@typeName(instance.componentWillUpdate) == "function" && instance.componentWillUpdate.__suppressDeprecationWarning != true) {
// [ts] pendingComponentWillUpdateWarnings.push(fiber);
// [ts] }
// [ts] if (fiber.mode & StrictMode && @typeName(instance.UNSAFE_componentWillUpdate) == "function") {
// [ts] pendingUNSAFE_ComponentWillUpdateWarnings.push(fiber);
// [ts] }
// [ts] };
// [ts] ReactStrictModeWarnings.flushPendingUnsafeLifecycleWarnings = () => {
// We do an initial pass to gather component names
// [ts] let componentWillMountUniqueNames = Set.init();
// [ts] if (pendingComponentWillMountWarnings.length > 0) {
// [ts] pendingComponentWillMountWarnings.forEach((fiber) => {
// [ts] componentWillMountUniqueNames.add(getComponentName(fiber.type) || "Component");
// [ts] didWarnAboutUnsafeLifecycles.add(fiber.type);
// [ts] });
// [ts] pendingComponentWillMountWarnings = [];
// [ts] }
// [ts] let UNSAFE_componentWillMountUniqueNames = Set.init();
// [ts] if (pendingUNSAFE_ComponentWillMountWarnings.length > 0) {
// [ts] pendingUNSAFE_ComponentWillMountWarnings.forEach((fiber) => {
// [ts] UNSAFE_componentWillMountUniqueNames.add(getComponentName(fiber.type) || "Component");
// [ts] didWarnAboutUnsafeLifecycles.add(fiber.type);
// [ts] });
// [ts] pendingUNSAFE_ComponentWillMountWarnings = [];
// [ts] }
// [ts] let componentWillReceivePropsUniqueNames = Set.init();
// [ts] if (pendingComponentWillReceivePropsWarnings.length > 0) {
// [ts] pendingComponentWillReceivePropsWarnings.forEach((fiber) => {
// [ts] componentWillReceivePropsUniqueNames.add(getComponentName(fiber.type) || "Component");
// [ts] didWarnAboutUnsafeLifecycles.add(fiber.type);
// [ts] });
// [ts] pendingComponentWillReceivePropsWarnings = [];
// [ts] }
// [ts] let UNSAFE_componentWillReceivePropsUniqueNames = Set.init();
// [ts] if (pendingUNSAFE_ComponentWillReceivePropsWarnings.length > 0) {
// [ts] pendingUNSAFE_ComponentWillReceivePropsWarnings.forEach((fiber) => {
// [ts] UNSAFE_componentWillReceivePropsUniqueNames.add(getComponentName(fiber.type) || "Component");
// [ts] didWarnAboutUnsafeLifecycles.add(fiber.type);
// [ts] });
// [ts] pendingUNSAFE_ComponentWillReceivePropsWarnings = [];
// [ts] }
// [ts] let componentWillUpdateUniqueNames = Set.init();
// [ts] if (pendingComponentWillUpdateWarnings.length > 0) {
// [ts] pendingComponentWillUpdateWarnings.forEach((fiber) => {
// [ts] componentWillUpdateUniqueNames.add(getComponentName(fiber.type) || "Component");
// [ts] didWarnAboutUnsafeLifecycles.add(fiber.type);
// [ts] });
// [ts] pendingComponentWillUpdateWarnings = [];
// [ts] }
// [ts] let UNSAFE_componentWillUpdateUniqueNames = Set.init();
// [ts] if (pendingUNSAFE_ComponentWillUpdateWarnings.length > 0) {
// [ts] pendingUNSAFE_ComponentWillUpdateWarnings.forEach((fiber) => {
// [ts] UNSAFE_componentWillUpdateUniqueNames.add(getComponentName(fiber.type) || "Component");
// [ts] didWarnAboutUnsafeLifecycles.add(fiber.type);
// [ts] });
// [ts] pendingUNSAFE_ComponentWillUpdateWarnings = [];
// [ts] } // Finally, we flush all the warnings
// UNSAFE_ ones before the deprecated ones, since they'll be 'louder'
// [ts] if (UNSAFE_componentWillMountUniqueNames.size > 0) {
// [ts] let sortedNames = setToSortedString(UNSAFE_componentWillMountUniqueNames);
// [ts] error("Using UNSAFE_componentWillMount in strict mode is not recommended and may indicate bugs in your code. " + "See https://fb.me/react-unsafe-component-lifecycles for details.\n\n" + "* Move code with side effects to componentDidMount, and set initial state in the constructor.\n" + "\nPlease update the following components: %s", sortedNames);
// [ts] }
// [ts] if (UNSAFE_componentWillReceivePropsUniqueNames.size > 0) {
// [ts] let _sortedNames = setToSortedString(UNSAFE_componentWillReceivePropsUniqueNames);
// [ts] error("Using UNSAFE_componentWillReceiveProps in strict mode is not recommended " + "and may indicate bugs in your code. " + "See https://fb.me/react-unsafe-component-lifecycles for details.\n\n" + "* Move data fetching code or side effects to componentDidUpdate.\n" + "* If you're updating state whenever props change, " + "refactor your code to use memoization techniques or move it to " + "static getDerivedStateFromProps. Learn more at: https://fb.me/react-derived-state\n" + "\nPlease update the following components: %s", _sortedNames);
// [ts] }
// [ts] if (UNSAFE_componentWillUpdateUniqueNames.size > 0) {
// [ts] let _sortedNames2 = setToSortedString(UNSAFE_componentWillUpdateUniqueNames);
// [ts] error("Using UNSAFE_componentWillUpdate in strict mode is not recommended " + "and may indicate bugs in your code. " + "See https://fb.me/react-unsafe-component-lifecycles for details.\n\n" + "* Move data fetching code or side effects to componentDidUpdate.\n" + "\nPlease update the following components: %s", _sortedNames2);
// [ts] }
// [ts] if (componentWillMountUniqueNames.size > 0) {
// [ts] let _sortedNames3 = setToSortedString(componentWillMountUniqueNames);
// [ts] warn("componentWillMount has been renamed, and is not recommended for use. " + "See https://fb.me/react-unsafe-component-lifecycles for details.\n\n" + "* Move code with side effects to componentDidMount, and set initial state in the constructor.\n" + "* Rename componentWillMount to UNSAFE_componentWillMount to suppress " + "this warning in non-strict mode. In React 17.x, only the UNSAFE_ name will work. " + "To rename all deprecated lifecycles to their new names, you can run " + "`npx react-codemod rename-unsafe-lifecycles` in your project source folder.\n" + "\nPlease update the following components: %s", _sortedNames3);
// [ts] }
// [ts] if (componentWillReceivePropsUniqueNames.size > 0) {
// [ts] let _sortedNames4 = setToSortedString(componentWillReceivePropsUniqueNames);
// [ts] warn("componentWillReceiveProps has been renamed, and is not recommended for use. " + "See https://fb.me/react-unsafe-component-lifecycles for details.\n\n" + "* Move data fetching code or side effects to componentDidUpdate.\n" + "* If you're updating state whenever props change, refactor your " + "code to use memoization techniques or move it to " + "static getDerivedStateFromProps. Learn more at: https://fb.me/react-derived-state\n" + "* Rename componentWillReceiveProps to UNSAFE_componentWillReceiveProps to suppress " + "this warning in non-strict mode. In React 17.x, only the UNSAFE_ name will work. " + "To rename all deprecated lifecycles to their new names, you can run " + "`npx react-codemod rename-unsafe-lifecycles` in your project source folder.\n" + "\nPlease update the following components: %s", _sortedNames4);
// [ts] }
// [ts] if (componentWillUpdateUniqueNames.size > 0) {
// [ts] let _sortedNames5 = setToSortedString(componentWillUpdateUniqueNames);
// [ts] warn("componentWillUpdate has been renamed, and is not recommended for use. " + "See https://fb.me/react-unsafe-component-lifecycles for details.\n\n" + "* Move data fetching code or side effects to componentDidUpdate.\n" + "* Rename componentWillUpdate to UNSAFE_componentWillUpdate to suppress " + "this warning in non-strict mode. In React 17.x, only the UNSAFE_ name will work. " + "To rename all deprecated lifecycles to their new names, you can run " + "`npx react-codemod rename-unsafe-lifecycles` in your project source folder.\n" + "\nPlease update the following components: %s", _sortedNames5);
// [ts] }
// [ts] };
// [ts] let pendingLegacyContextWarning = Map.init(); // Tracks components we have already warned about.
// [ts] let didWarnAboutLegacyContext = Set.init();
// [ts] ReactStrictModeWarnings.recordLegacyContextWarning = (fiber, instance) => {
// [ts] let strictRoot = findStrictRoot(fiber);
// [ts] if (strictRoot == null) {
// [ts] error("Expected to find a StrictMode component in a strict mode tree. " + "This error is likely caused by a bug in React. Please file an issue.");
// [ts] return;
// [ts] } // Dedup strategy: Warn once per component.
// [ts] if (didWarnAboutLegacyContext.has(fiber.type)) {
// [ts] return;
// [ts] }
// [ts] let warningsForRoot = pendingLegacyContextWarning.get(strictRoot);
// [ts] if (fiber.type.contextTypes != null || fiber.type.childContextTypes != null || instance != null && @typeName(instance.getChildContext) == "function") {
// [ts] if (warningsForRoot == null) {
// [ts] warningsForRoot = [];
// [ts] pendingLegacyContextWarning.set(strictRoot, warningsForRoot);
// [ts] }
// [ts] warningsForRoot.push(fiber);
// [ts] }
// [ts] };
// [ts] ReactStrictModeWarnings.flushLegacyContextWarning = () => {
// [ts] pendingLegacyContextWarning.forEach((fiberArray, strictRoot) => {
// [ts] if (fiberArray.length == 0) {
// [ts] return;
// [ts] }
// [ts] let firstFiber = fiberArray[0];
// [ts] let uniqueNames = Set.init();
// [ts] fiberArray.forEach((fiber) => {
// [ts] uniqueNames.add(getComponentName(fiber.type) || "Component");
// [ts] didWarnAboutLegacyContext.add(fiber.type);
// [ts] });
// [ts] let sortedNames = setToSortedString(uniqueNames);
// [ts] let firstComponentStack = getStackByFiberInDevAndProd(firstFiber);
// [ts] error("Legacy context API has been detected within a strict-mode tree." + "\n\nThe old API will be supported in all 16.x releases, but applications " + "using it should migrate to the new version." + "\n\nPlease update the following components: %s" + "\n\nLearn more about this warning here: https://fb.me/react-legacy-context" + "%s", sortedNames, firstComponentStack);
// [ts] });
// [ts] };
// [ts] ReactStrictModeWarnings.discardPendingWarnings = () => {
// [ts] pendingComponentWillMountWarnings = [];
// [ts] pendingUNSAFE_ComponentWillMountWarnings = [];
// [ts] pendingComponentWillReceivePropsWarnings = [];
// [ts] pendingUNSAFE_ComponentWillReceivePropsWarnings = [];
// [ts] pendingComponentWillUpdateWarnings = [];
// [ts] pendingUNSAFE_ComponentWillUpdateWarnings = [];
// [ts] pendingLegacyContextWarning = Map.init();
// [ts] };
// [ts] }
// [ts] let resolveFamily = null; // $FlowFixMe Flow gets confused by a WeakSet feature check below.
// [ts] let failedBoundaries = null;
// [ts] let setRefreshHandler = (handler) => {
// [ts] {
// [ts] resolveFamily = handler;
// [ts] }
// [ts] };
// [ts] fn resolveFunctionForHotReloading(type: auto) {
// [ts] {
// [ts] if (resolveFamily == null) {
// Hot reloading is disabled.
// [ts] return type;
// [ts] }
// [ts] let family = resolveFamily(type);
// [ts] if (family == null) {
// [ts] return type;
// [ts] } // Use the latest known implementation.
// [ts] return family.current;
// [ts] }
// [ts] }
// [ts] fn resolveClassForHotReloading(type: auto) {
// No implementation differences.
// [ts] return resolveFunctionForHotReloading(type);
// [ts] }
// [ts] fn resolveForwardRefForHotReloading(type: auto) {
// [ts] {
// [ts] if (resolveFamily == null) {
// Hot reloading is disabled.
// [ts] return type;
// [ts] }
// [ts] let family = resolveFamily(type);
// [ts] if (family == null) {
// Check if we're dealing with a real forwardRef. Don't want to crash early.
// [ts] if (type != null && type != null && @typeName(type.render) == "function") {
// ForwardRef is special because its resolved .type is an object,
// but it's possible that we only have its inner render function in the map.
// If that inner render function is different, we'll build a new forwardRef type.
// [ts] let currentRender = resolveFunctionForHotReloading(type.render);
// [ts] if (type.render != currentRender) {
// [ts] let syntheticType = {
// [ts] $$typeof: REACT_FORWARD_REF_TYPE,
// [ts] render: currentRender
// [ts] };
// [ts] if (type.displayName != null) {
// [ts] syntheticType.displayName = type.displayName;
// [ts] }
// [ts] return syntheticType;
// [ts] }
// [ts] }
// [ts] return type;
// [ts] } // Use the latest known implementation.
// [ts] return family.current;
// [ts] }
// [ts] }
// [ts] fn isCompatibleFamilyForHotReloading(fiber: auto, element: auto) {
// [ts] {
// [ts] if (resolveFamily == null) {
// Hot reloading is disabled.
// [ts] return false;
// [ts] }
// [ts] let prevType = fiber.elementType;
// [ts] let nextType = element.type; // If we got here, we know types aren't === equal.
// [ts] let needsCompareFamilies = false;
// [ts] let $$typeofNextType = if (@typeName(nextType) == "object" && nextType != null) nextType.$$typeof else null;
// [ts] switch (fiber.tag) {
// [ts] ClassComponent => {
// [ts] {
// [ts] if (@typeName(nextType) == "function") {
// [ts] needsCompareFamilies = true;
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] FunctionComponent => {
// [ts] {
// [ts] if (@typeName(nextType) == "function") {
// [ts] needsCompareFamilies = true;
// [ts] } else if ($$typeofNextType == REACT_LAZY_TYPE) {
// We don't know the inner type yet.
// We're going to assume that the lazy inner type is stable,
// and so it is sufficient to avoid reconciling it away.
// We're not going to unwrap or actually use the new lazy type.
// [ts] needsCompareFamilies = true;
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] ForwardRef => {
// [ts] {
// [ts] if ($$typeofNextType == REACT_FORWARD_REF_TYPE) {
// [ts] needsCompareFamilies = true;
// [ts] } else if ($$typeofNextType == REACT_LAZY_TYPE) {
// [ts] needsCompareFamilies = true;
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] MemoComponent, SimpleMemoComponent => {
// [ts] {
// [ts] if ($$typeofNextType == REACT_MEMO_TYPE) {
// TODO: if it was but can no longer be simple,
// we shouldn't set this.
// [ts] needsCompareFamilies = true;
// [ts] } else if ($$typeofNextType == REACT_LAZY_TYPE) {
// [ts] needsCompareFamilies = true;
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] } // Check if both types have a family and it's the same one.
// [ts] if (needsCompareFamilies) {
// Note: memo() and forwardRef() we'll compare outer rather than inner type.
// This means both of them need to be registered to preserve state.
// If we unwrapped and compared the inner types for wrappers instead,
// then we would risk falsely saying two separate memo(Foo)
// calls are equivalent because they wrap the same Foo function.
// [ts] let prevFamily = resolveFamily(prevType);
// [ts] if (prevFamily != null && prevFamily == resolveFamily(nextType)) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] }
// [ts] fn markFailedErrorBoundaryForHotReloading(fiber: auto) {
// [ts] {
// [ts] if (resolveFamily == null) {
// Hot reloading is disabled.
// [ts] return;
// [ts] }
// [ts] if (@typeName(WeakSet) != "function") {
// [ts] return;
// [ts] }
// [ts] if (failedBoundaries == null) {
// [ts] failedBoundaries = WeakSet.init();
// [ts] }
// [ts] failedBoundaries.add(fiber);
// [ts] }
// [ts] }
// [ts] let scheduleRefresh = (root, update) => {
// [ts] {
// [ts] if (resolveFamily == null) {
// Hot reloading is disabled.
// [ts] return;
// [ts] }
// [ts] let staleFamilies = update.staleFamilies,
// [ts] updatedFamilies = update.updatedFamilies;
// [ts] flushPassiveEffects();
// [ts] flushSync(() => {
// [ts] scheduleFibersWithFamiliesRecursively(root.current, updatedFamilies, staleFamilies);
// [ts] });
// [ts] }
// [ts] };
// [ts] let scheduleRoot = (root, element) => {
// [ts] {
// [ts] if (root.context != emptyContextObject) {
// Super edge case: root has a legacy _renderSubtree context
// but we don't know the parentComponent so we can't pass it.
// Just ignore. We'll delete this with _renderSubtree code path later.
// [ts] return;
// [ts] }
// [ts] flushPassiveEffects();
// [ts] syncUpdates(() => {
// [ts] updateContainer(element, root, null, null);
// [ts] });
// [ts] }
// [ts] };
// [ts] fn scheduleFibersWithFamiliesRecursively(fiber: auto, updatedFamilies: auto, staleFamilies: auto) {
// [ts] {
// [ts] let alternate = fiber.alternate,
// [ts] child = fiber.child,
// [ts] sibling = fiber.sibling,
// [ts] tag = fiber.tag,
// [ts] type = fiber.type;
// [ts] let candidateType = null;
// [ts] switch (tag) {
// [ts] FunctionComponent, SimpleMemoComponent, ClassComponent => {
// [ts] candidateType = type;
// break;
// [ts] },
// [ts] ForwardRef => {
// [ts] candidateType = type.render;
// break;
// [ts] }
// [ts] }
// [ts] if (resolveFamily == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Expected resolveFamily to be set during hot reload.');)
// [ts] }
// [ts] let needsRender = false;
// [ts] let needsRemount = false;
// [ts] if (candidateType != null) {
// [ts] let family = resolveFamily(candidateType);
// [ts] if (family != null) {
// [ts] if (staleFamilies.has(family)) {
// [ts] needsRemount = true;
// [ts] } else if (updatedFamilies.has(family)) {
// [ts] if (tag == ClassComponent) {
// [ts] needsRemount = true;
// [ts] } else {
// [ts] needsRender = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (failedBoundaries != null) {
// [ts] if (failedBoundaries.has(fiber) || alternate != null && failedBoundaries.has(alternate)) {
// [ts] needsRemount = true;
// [ts] }
// [ts] }
// [ts] if (needsRemount) {
// [ts] fiber._debugNeedsRemount = true;
// [ts] }
// [ts] if (needsRemount || needsRender) {
// [ts] scheduleWork(fiber, Sync);
// [ts] }
// [ts] if (child != null && !needsRemount) {
// [ts] scheduleFibersWithFamiliesRecursively(child, updatedFamilies, staleFamilies);
// [ts] }
// [ts] if (sibling != null) {
// [ts] scheduleFibersWithFamiliesRecursively(sibling, updatedFamilies, staleFamilies);
// [ts] }
// [ts] }
// [ts] }
// [ts] let findHostInstancesForRefresh = (root, families) => {
// [ts] {
// [ts] let hostInstances = Set.init();
// [ts] let types = Set.init(families.map((family) => {
// [ts] return family.current;
// [ts] }));
// [ts] findHostInstancesForMatchingFibersRecursively(root.current, types, hostInstances);
// [ts] return hostInstances;
// [ts] }
// [ts] };
// [ts] fn findHostInstancesForMatchingFibersRecursively(fiber: auto, types: auto, hostInstances: auto) {
// [ts] {
// [ts] let child = fiber.child,
// [ts] sibling = fiber.sibling,
// [ts] tag = fiber.tag,
// [ts] type = fiber.type;
// [ts] let candidateType = null;
// [ts] switch (tag) {
// [ts] FunctionComponent, SimpleMemoComponent, ClassComponent => {
// [ts] candidateType = type;
// break;
// [ts] },
// [ts] ForwardRef => {
// [ts] candidateType = type.render;
// break;
// [ts] }
// [ts] }
// [ts] let didMatch = false;
// [ts] if (candidateType != null) {
// [ts] if (types.has(candidateType)) {
// [ts] didMatch = true;
// [ts] }
// [ts] }
// [ts] if (didMatch) {
// We have a match. This only drills down to the closest host components.
// There's no need to search deeper because for the purpose of giving
// visual feedback, "flashing" outermost parent rectangles is sufficient.
// [ts] findHostInstancesForFiberShallowly(fiber, hostInstances);
// [ts] } else {
// If there's no match, maybe there will be one further down in the child tree.
// [ts] if (child != null) {
// [ts] findHostInstancesForMatchingFibersRecursively(child, types, hostInstances);
// [ts] }
// [ts] }
// [ts] if (sibling != null) {
// [ts] findHostInstancesForMatchingFibersRecursively(sibling, types, hostInstances);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn findHostInstancesForFiberShallowly(fiber: auto, hostInstances: auto) {
// [ts] {
// [ts] let foundHostInstances = findChildHostInstancesForFiberShallowly(fiber, hostInstances);
// [ts] if (foundHostInstances) {
// [ts] return;
// [ts] } // If we didn't find any host children, fallback to closest host parent.
// [ts] let node = fiber;
// [ts] while (true) {
// [ts] switch (node.tag) {
// [ts] HostComponent => {
// [ts] hostInstances.add(node.stateNode);
// [ts] return;
// [ts] },
// [ts] HostPortal => {
// [ts] hostInstances.add(node.stateNode.containerInfo);
// [ts] return;
// [ts] },
// [ts] HostRoot => {
// [ts] hostInstances.add(node.stateNode.containerInfo);
// [ts] return;
// [ts] }
// [ts] }
// [ts] if (node.return == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Expected to reach root first.');)
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn findChildHostInstancesForFiberShallowly(fiber: auto, hostInstances: auto) {
// [ts] {
// [ts] let node = fiber;
// [ts] let foundHostInstances = false;
// [ts] while (true) {
// [ts] if (node.tag == HostComponent) {
// We got a match.
// [ts] foundHostInstances = true;
// [ts] hostInstances.add(node.stateNode); // There may still be more, so keep searching.
// [ts] } else if (node.child != null) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] if (node == fiber) {
// [ts] return foundHostInstances;
// [ts] }
// [ts] while (node.sibling == null) {
// [ts] if (node.return == null || node.return == fiber) {
// [ts] return foundHostInstances;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn resolveDefaultProps(Component: auto, baseProps: auto) {
// [ts] if (Component && Component.defaultProps) {
// Resolve default props. Taken from ReactElement
// [ts] let props = _assign({}, baseProps);
// [ts] let defaultProps = Component.defaultProps;
// [ts] for (let propName in defaultProps) {
// [ts] if (props[propName] == null) {
// [ts] props[propName] = defaultProps[propName];
// [ts] }
// [ts] }
// [ts] return props;
// [ts] }
// [ts] return baseProps;
// [ts] }
// [ts] fn readLazyComponentType(lazyComponent: auto) {
// [ts] initializeLazyComponentType(lazyComponent);
// [ts] if (lazyComponent._status != Resolved) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw lazyComponent._result;)
// [ts] }
// [ts] return lazyComponent._result;
// [ts] }
// [ts] let valueCursor = createCursor(null);
// [ts] let rendererSigil;
// [ts] {
// Use this to detect multiple renderers using the same context
// [ts] rendererSigil = {};
// [ts] }
// [ts] let currentlyRenderingFiber = null;
// [ts] let lastContextDependency = null;
// [ts] let lastContextWithAllBitsObserved = null;
// [ts] let isDisallowedContextReadInDEV = false;
// [ts] fn resetContextDependencies() {
// This is called right before React yields execution, to ensure `readContext`
// cannot be called outside the render phase.
// [ts] currentlyRenderingFiber = null;
// [ts] lastContextDependency = null;
// [ts] lastContextWithAllBitsObserved = null;
// [ts] {
// [ts] isDisallowedContextReadInDEV = false;
// [ts] }
// [ts] }
// [ts] fn enterDisallowedContextReadInDEV() {
// [ts] {
// [ts] isDisallowedContextReadInDEV = true;
// [ts] }
// [ts] }
// [ts] fn exitDisallowedContextReadInDEV() {
// [ts] {
// [ts] isDisallowedContextReadInDEV = false;
// [ts] }
// [ts] }
// [ts] fn pushProvider(providerFiber: auto, nextValue: auto) {
// [ts] let context = providerFiber.type._context;
// [ts] {
// [ts] push(valueCursor, context._currentValue, providerFiber);
// [ts] context._currentValue = nextValue;
// [ts] {
// [ts] if (context._currentRenderer != null && context._currentRenderer != null && context._currentRenderer != rendererSigil) {
// [ts] error("Detected multiple renderers concurrently rendering the " + "same context provider. This is currently unsupported.");
// [ts] }
// [ts] context._currentRenderer = rendererSigil;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn popProvider(providerFiber: auto) {
// [ts] let currentValue = valueCursor.current;
// [ts] pop(valueCursor, providerFiber);
// [ts] let context = providerFiber.type._context;
// [ts] {
// [ts] context._currentValue = currentValue;
// [ts] }
// [ts] }
// [ts] fn calculateChangedBits(context: auto, newValue: auto, oldValue: auto) {
// [ts] if (objectIs(oldValue, newValue)) {
// No change
// [ts] return 0;
// [ts] } else {
// [ts] let changedBits = if (@typeName(context._calculateChangedBits) == "function") context._calculateChangedBits(oldValue, newValue) else MAX_SIGNED_31_BIT_INT;
// [ts] {
// [ts] if ((changedBits & MAX_SIGNED_31_BIT_INT) != changedBits) {
// [ts] error("calculateChangedBits: Expected the return value to be a " + "31-bit integer. Instead received: %s", changedBits);
// [ts] }
// [ts] }
// [ts] return changedBits | 0;
// [ts] }
// [ts] }
// [ts] fn scheduleWorkOnParentPath(parent: auto, renderExpirationTime: auto) {
// Update the child expiration time of all the ancestors, including
// the alternates.
// [ts] let node = parent;
// [ts] while (node != null) {
// [ts] let alternate = node.alternate;
// [ts] if (node.childExpirationTime < renderExpirationTime) {
// [ts] node.childExpirationTime = renderExpirationTime;
// [ts] if (alternate != null && alternate.childExpirationTime < renderExpirationTime) {
// [ts] alternate.childExpirationTime = renderExpirationTime;
// [ts] }
// [ts] } else if (alternate != null && alternate.childExpirationTime < renderExpirationTime) {
// [ts] alternate.childExpirationTime = renderExpirationTime;
// [ts] } else {
// Neither alternate was updated, which means the rest of the
// ancestor path already has sufficient priority.
// [ts] break;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] }
// [ts] fn propagateContextChange(workInProgress: auto, context: auto, changedBits: auto, renderExpirationTime: auto) {
// [ts] let fiber = workInProgress.child;
// [ts] if (fiber != null) {
// Set the return pointer of the child to the work-in-progress fiber.
// [ts] fiber.return = workInProgress;
// [ts] }
// [ts] while (fiber != null) {
// [ts] let nextFiber = void 0; // Visit this fiber.
// [ts] let list = fiber.dependencies;
// [ts] if (list != null) {
// [ts] nextFiber = fiber.child;
// [ts] let dependency = list.firstContext;
// [ts] while (dependency != null) {
// Check if the context matches.
// [ts] if (dependency.context == context && (dependency.observedBits & changedBits) != 0) {
// Match! Schedule an update on this fiber.
// [ts] if (fiber.tag == ClassComponent) {
// Schedule a force update on the work-in-progress.
// [ts] let update = createUpdate(renderExpirationTime, null);
// [ts] update.tag = ForceUpdate; // TODO: Because we don't have a work-in-progress, this will add the
// update to the current fiber, too, which means it will persist even if
// this render is thrown away. Since it's a race condition, not sure it's
// worth fixing.
// [ts] enqueueUpdate(fiber, update);
// [ts] }
// [ts] if (fiber.expirationTime < renderExpirationTime) {
// [ts] fiber.expirationTime = renderExpirationTime;
// [ts] }
// [ts] let alternate = fiber.alternate;
// [ts] if (alternate != null && alternate.expirationTime < renderExpirationTime) {
// [ts] alternate.expirationTime = renderExpirationTime;
// [ts] }
// [ts] scheduleWorkOnParentPath(fiber.return, renderExpirationTime); // Mark the expiration time on the list, too.
// [ts] if (list.expirationTime < renderExpirationTime) {
// [ts] list.expirationTime = renderExpirationTime;
// [ts] } // Since we already found a match, we can stop traversing the
// dependency list.
// [ts] break;
// [ts] }
// [ts] dependency = dependency.next;
// [ts] }
// [ts] } else if (fiber.tag == ContextProvider) {
// Don't scan deeper if this is a matching provider
// [ts] nextFiber = if (fiber.type == workInProgress.type) null else fiber.child;
// [ts] } else {
// Traverse down.
// [ts] nextFiber = fiber.child;
// [ts] }
// [ts] if (nextFiber != null) {
// Set the return pointer of the child to the work-in-progress fiber.
// [ts] nextFiber.return = fiber;
// [ts] } else {
// No child. Traverse to next sibling.
// [ts] nextFiber = fiber;
// [ts] while (nextFiber != null) {
// [ts] if (nextFiber == workInProgress) {
// We're back to the root of this subtree. Exit.
// [ts] nextFiber = null;
// [ts] break;
// [ts] }
// [ts] let sibling = nextFiber.sibling;
// [ts] if (sibling != null) {
// Set the return pointer of the sibling to the work-in-progress fiber.
// [ts] sibling.return = nextFiber.return;
// [ts] nextFiber = sibling;
// [ts] break;
// [ts] } // No more siblings. Traverse up.
// [ts] nextFiber = nextFiber.return;
// [ts] }
// [ts] }
// [ts] fiber = nextFiber;
// [ts] }
// [ts] }
// [ts] fn prepareToReadContext(workInProgress: auto, renderExpirationTime: auto) {
// [ts] currentlyRenderingFiber = workInProgress;
// [ts] lastContextDependency = null;
// [ts] lastContextWithAllBitsObserved = null;
// [ts] let dependencies = workInProgress.dependencies;
// [ts] if (dependencies != null) {
// [ts] let firstContext = dependencies.firstContext;
// [ts] if (firstContext != null) {
// [ts] if (dependencies.expirationTime >= renderExpirationTime) {
// Context list has a pending update. Mark that this fiber performed work.
// [ts] markWorkInProgressReceivedUpdate();
// [ts] } // Reset the work-in-progress list
// [ts] dependencies.firstContext = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn readContext(context: auto, observedBits: auto) {
// [ts] {
// This warning would fire if you read context inside a Hook like useMemo.
// Unlike the class check below, it's not enforced in production for perf.
// [ts] if (isDisallowedContextReadInDEV) {
// [ts] error("Context can only be read while React is rendering. " + "In classes, you can read it in the render method or getDerivedStateFromProps. " + "In function components, you can read it directly in the function body, but not " + "inside Hooks like useReducer() or useMemo().");
// [ts] }
// [ts] }
// [ts] if (lastContextWithAllBitsObserved == context) ; else if (observedBits == false || observedBits == 0) ; else {
// [ts] let resolvedObservedBits; // Avoid deopting on observable arguments or heterogeneous types.
// [ts] if (@typeName(observedBits) != "number" || observedBits == MAX_SIGNED_31_BIT_INT) {
// Observe all updates.
// [ts] lastContextWithAllBitsObserved = context;
// [ts] resolvedObservedBits = MAX_SIGNED_31_BIT_INT;
// [ts] } else {
// [ts] resolvedObservedBits = observedBits;
// [ts] }
// [ts] let contextItem = {
// [ts] context: context,
// [ts] observedBits: resolvedObservedBits,
// [ts] next: null
// [ts] };
// [ts] if (lastContextDependency == null) {
// [ts] if (!(currentlyRenderingFiber != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Context can only be read while React is rendering. In classes, you can read it in the render method or getDerivedStateFromProps. In function components, you can read it directly in the function body, but not inside Hooks like useReducer() or useMemo()." );)
// [ts] }
// [ts] } // This is the first dependency for this component. Create a new list.
// [ts] lastContextDependency = contextItem;
// [ts] currentlyRenderingFiber.dependencies = {
// [ts] expirationTime: NoWork,
// [ts] firstContext: contextItem,
// [ts] responders: null
// [ts] };
// [ts] } else {
// Append a new context item.
// [ts] lastContextDependency = lastContextDependency.next = contextItem;
// [ts] }
// [ts] }
// [ts] return  context._currentValue ;
// [ts] }
// [ts] let UpdateState = 0;
// [ts] let ReplaceState = 1;
// [ts] let ForceUpdate = 2;
// [ts] let CaptureUpdate = 3; // Global state that is reset at the beginning of calling `processUpdateQueue`.
// It should only be read right after calling `processUpdateQueue`, via
// `checkHasForceUpdateAfterProcessing`.
// [ts] let hasForceUpdate = false;
// [ts] let didWarnUpdateInsideUpdate;
// [ts] let currentlyProcessingQueue;
// [ts] {
// [ts] didWarnUpdateInsideUpdate = false;
// [ts] currentlyProcessingQueue = null;
// [ts] }
// [ts] fn initializeUpdateQueue(fiber: auto) {
// [ts] let queue = {
// [ts] baseState: fiber.memoizedState,
// [ts] baseQueue: null,
// [ts] shared: {
// [ts] pending: null
// [ts] },
// [ts] effects: null
// [ts] };
// [ts] fiber.updateQueue = queue;
// [ts] }
// [ts] fn cloneUpdateQueue(current: auto, workInProgress: auto) {
// Clone the update queue from current. Unless it's already a clone.
// [ts] let queue = workInProgress.updateQueue;
// [ts] let currentQueue = current.updateQueue;
// [ts] if (queue == currentQueue) {
// [ts] let clone = {
// [ts] baseState: currentQueue.baseState,
// [ts] baseQueue: currentQueue.baseQueue,
// [ts] shared: currentQueue.shared,
// [ts] effects: currentQueue.effects
// [ts] };
// [ts] workInProgress.updateQueue = clone;
// [ts] }
// [ts] }
// [ts] fn createUpdate(expirationTime: auto, suspenseConfig: auto) {
// [ts] let update = {
// [ts] expirationTime: expirationTime,
// [ts] suspenseConfig: suspenseConfig,
// [ts] tag: UpdateState,
// [ts] payload: null,
// [ts] callback: null,
// [ts] next: null
// [ts] };
// [ts] update.next = update;
// [ts] {
// [ts] update.priority = getCurrentPriorityLevel();
// [ts] }
// [ts] return update;
// [ts] }
// [ts] fn enqueueUpdate(fiber: auto, update: auto) {
// [ts] let updateQueue = fiber.updateQueue;
// [ts] if (updateQueue == null) {
// Only occurs if the fiber has been unmounted.
// [ts] return;
// [ts] }
// [ts] let sharedQueue = updateQueue.shared;
// [ts] let pending = sharedQueue.pending;
// [ts] if (pending == null) {
// This is the first update. Create a circular list.
// [ts] update.next = update;
// [ts] } else {
// [ts] update.next = pending.next;
// [ts] pending.next = update;
// [ts] }
// [ts] sharedQueue.pending = update;
// [ts] {
// [ts] if (currentlyProcessingQueue == sharedQueue && !didWarnUpdateInsideUpdate) {
// [ts] error("An update (setState, replaceState, or forceUpdate) was scheduled " + "from inside an update function. Update functions should be pure, " + "with zero side-effects. Consider using componentDidUpdate or a " + "callback.");
// [ts] didWarnUpdateInsideUpdate = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn enqueueCapturedUpdate(workInProgress: auto, update: auto) {
// [ts] let current = workInProgress.alternate;
// [ts] if (current != null) {
// Ensure the work-in-progress queue is a clone
// [ts] cloneUpdateQueue(current, workInProgress);
// [ts] } // Captured updates go only on the work-in-progress queue.
// [ts] let queue = workInProgress.updateQueue; // Append the update to the end of the list.
// [ts] let last = queue.baseQueue;
// [ts] if (last == null) {
// [ts] queue.baseQueue = update.next = update;
// [ts] update.next = update;
// [ts] } else {
// [ts] update.next = last.next;
// [ts] last.next = update;
// [ts] }
// [ts] }
// [ts] fn getStateFromUpdate(workInProgress: auto, queue: auto, update: auto, prevState: auto, nextProps: auto, instance: auto) {
// [ts] switch (update.tag) {
// [ts] ReplaceState => {
// [ts] {
// [ts] let payload = update.payload;
// [ts] if (@typeName(payload) == "function") {
// Updater function
// [ts] {
// [ts] enterDisallowedContextReadInDEV();
// [ts] if ( workInProgress.mode & StrictMode) {
// [ts] payload.call(instance, prevState, nextProps);
// [ts] }
// [ts] }
// [ts] let nextState = payload.call(instance, prevState, nextProps);
// [ts] {
// [ts] exitDisallowedContextReadInDEV();
// [ts] }
// [ts] return nextState;
// [ts] } // State object
// [ts] return payload;
// [ts] }
// [ts] },
// [ts] CaptureUpdate => {
// [ts] {
// [ts] workInProgress.effectTag = workInProgress.effectTag & ~ShouldCapture | DidCapture;
// [ts] }
// Intentional fallthrough
// [ts] },
// [ts] UpdateState => {
// [ts] {
// [ts] let _payload = update.payload;
// [ts] let partialState;
// [ts] if (@typeName(_payload) == "function") {
// Updater function
// [ts] {
// [ts] enterDisallowedContextReadInDEV();
// [ts] if ( workInProgress.mode & StrictMode) {
// [ts] _payload.call(instance, prevState, nextProps);
// [ts] }
// [ts] }
// [ts] partialState = _payload.call(instance, prevState, nextProps);
// [ts] {
// [ts] exitDisallowedContextReadInDEV();
// [ts] }
// [ts] } else {
// Partial state object
// [ts] partialState = _payload;
// [ts] }
// [ts] if (partialState == null || partialState == null) {
// Null and undefined are treated as no-ops.
// [ts] return prevState;
// [ts] } // Merge the partial state and the previous state.
// [ts] return _assign({}, prevState, partialState);
// [ts] }
// [ts] },
// [ts] ForceUpdate => {
// [ts] {
// [ts] hasForceUpdate = true;
// [ts] return prevState;
// [ts] }
// [ts] }
// [ts] }
// [ts] return prevState;
// [ts] }
// [ts] fn processUpdateQueue(workInProgress: auto, props: auto, instance: auto, renderExpirationTime: auto) {
// This is always non-null on a ClassComponent or HostRoot
// [ts] let queue = workInProgress.updateQueue;
// [ts] hasForceUpdate = false;
// [ts] {
// [ts] currentlyProcessingQueue = queue.shared;
// [ts] } // The last rebase update that is NOT part of the base state.
// [ts] let baseQueue = queue.baseQueue; // The last pending update that hasn't been processed yet.
// [ts] let pendingQueue = queue.shared.pending;
// [ts] if (pendingQueue != null) {
// We have new updates that haven't been processed yet.
// We'll add them to the base queue.
// [ts] if (baseQueue != null) {
// Merge the pending queue and the base queue.
// [ts] let baseFirst = baseQueue.next;
// [ts] let pendingFirst = pendingQueue.next;
// [ts] baseQueue.next = pendingFirst;
// [ts] pendingQueue.next = baseFirst;
// [ts] }
// [ts] baseQueue = pendingQueue;
// [ts] queue.shared.pending = null; // TODO: Pass `current` as argument
// [ts] let current = workInProgress.alternate;
// [ts] if (current != null) {
// [ts] let currentQueue = current.updateQueue;
// [ts] if (currentQueue != null) {
// [ts] currentQueue.baseQueue = pendingQueue;
// [ts] }
// [ts] }
// [ts] } // These values may change as we process the queue.
// [ts] if (baseQueue != null) {
// [ts] let first = baseQueue.next; // Iterate through the list of updates to compute the result.
// [ts] let newState = queue.baseState;
// [ts] let newExpirationTime = NoWork;
// [ts] let newBaseState = null;
// [ts] let newBaseQueueFirst = null;
// [ts] let newBaseQueueLast = null;
// [ts] if (first != null) {
// [ts] let update = first;
// [ts] do {
// [ts] let updateExpirationTime = update.expirationTime;
// [ts] if (updateExpirationTime < renderExpirationTime) {
// Priority is insufficient. Skip this update. If this is the first
// skipped update, the previous update/state is the new base
// update/state.
// [ts] let clone = {
// [ts] expirationTime: update.expirationTime,
// [ts] suspenseConfig: update.suspenseConfig,
// [ts] tag: update.tag,
// [ts] payload: update.payload,
// [ts] callback: update.callback,
// [ts] next: null
// [ts] };
// [ts] if (newBaseQueueLast == null) {
// [ts] newBaseQueueFirst = newBaseQueueLast = clone;
// [ts] newBaseState = newState;
// [ts] } else {
// [ts] newBaseQueueLast = newBaseQueueLast.next = clone;
// [ts] } // Update the remaining priority in the queue.
// [ts] if (updateExpirationTime > newExpirationTime) {
// [ts] newExpirationTime = updateExpirationTime;
// [ts] }
// [ts] } else {
// This update does have sufficient priority.
// [ts] if (newBaseQueueLast != null) {
// [ts] let _clone = {
// [ts] expirationTime: Sync,
// This update is going to be committed so we never want uncommit it.
// [ts] suspenseConfig: update.suspenseConfig,
// [ts] tag: update.tag,
// [ts] payload: update.payload,
// [ts] callback: update.callback,
// [ts] next: null
// [ts] };
// [ts] newBaseQueueLast = newBaseQueueLast.next = _clone;
// [ts] } // Mark the event time of this update as relevant to this render pass.
// TODO: This should ideally use the true event time of this update rather than
// its priority which is a derived and not reverseable value.
// TODO: We should skip this update if it was already committed but currently
// we have no way of detecting the difference between a committed and suspended
// update here.
// [ts] markRenderEventTimeAndConfig(updateExpirationTime, update.suspenseConfig); // Process this update.
// [ts] newState = getStateFromUpdate(workInProgress, queue, update, newState, props, instance);
// [ts] let callback = update.callback;
// [ts] if (callback != null) {
// [ts] workInProgress.effectTag |= Callback;
// [ts] let effects = queue.effects;
// [ts] if (effects == null) {
// [ts] queue.effects = [update];
// [ts] } else {
// [ts] effects.push(update);
// [ts] }
// [ts] }
// [ts] }
// [ts] update = update.next;
// [ts] if (update == null || update == first) {
// [ts] pendingQueue = queue.shared.pending;
// [ts] if (pendingQueue == null) {
// [ts] break;
// [ts] } else {
// An update was scheduled from inside a reducer. Add the new
// pending updates to the end of the list and keep processing.
// [ts] update = baseQueue.next = pendingQueue.next;
// [ts] pendingQueue.next = first;
// [ts] queue.baseQueue = baseQueue = pendingQueue;
// [ts] queue.shared.pending = null;
// [ts] }
// [ts] }
// [ts] } while (true);
// [ts] }
// [ts] if (newBaseQueueLast == null) {
// [ts] newBaseState = newState;
// [ts] } else {
// [ts] newBaseQueueLast.next = newBaseQueueFirst;
// [ts] }
// [ts] queue.baseState = newBaseState;
// [ts] queue.baseQueue = newBaseQueueLast; // Set the remaining expiration time to be whatever is remaining in the queue.
// This should be fine because the only two other things that contribute to
// expiration time are props and context. We're already in the middle of the
// begin phase by the time we start processing the queue, so we've already
// dealt with the props. Context in components that specify
// shouldComponentUpdate is tricky; but we'll have to account for
// that regardless.
// [ts] markUnprocessedUpdateTime(newExpirationTime);
// [ts] workInProgress.expirationTime = newExpirationTime;
// [ts] workInProgress.memoizedState = newState;
// [ts] }
// [ts] {
// [ts] currentlyProcessingQueue = null;
// [ts] }
// [ts] }
// [ts] fn callCallback(callback: auto, context: auto) {
// [ts] if (!(@typeName(callback) == "function")) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Invalid argument passed as callback. Expected a function. Instead received: " + callback );)
// [ts] }
// [ts] }
// [ts] callback.call(context);
// [ts] }
// [ts] fn resetHasForceUpdateBeforeProcessing() {
// [ts] hasForceUpdate = false;
// [ts] }
// [ts] fn checkHasForceUpdateAfterProcessing() {
// [ts] return hasForceUpdate;
// [ts] }
// [ts] fn commitUpdateQueue(finishedWork: auto, finishedQueue: auto, instance: auto) {
// Commit the effects
// [ts] let effects = finishedQueue.effects;
// [ts] finishedQueue.effects = null;
// [ts] if (effects != null) {
// [ts] let i = 0;
// [ts] while (i < effects.length) { // [cap-migrate] for-step: i++
// [ts] let effect = effects[i];
// [ts] let callback = effect.callback;
// [ts] if (callback != null) {
// [ts] effect.callback = null;
// [ts] callCallback(callback, instance);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let ReactCurrentBatchConfig = ReactSharedInternals.ReactCurrentBatchConfig;
// [ts] fn requestCurrentSuspenseConfig() {
// [ts] return ReactCurrentBatchConfig.suspense;
// [ts] }
// [ts] let fakeInternalInstance = {};
// [ts] let isArray = Array.isArray; // React.Component uses a shared frozen object by default.
// We'll use it to determine whether we need to initialize legacy refs.
// [ts] let emptyRefsObject = React.Component.init().refs;
// [ts] let didWarnAboutStateAssignmentForComponent;
// [ts] let didWarnAboutUninitializedState;
// [ts] let didWarnAboutGetSnapshotBeforeUpdateWithoutDidUpdate;
// [ts] let didWarnAboutLegacyLifecyclesAndDerivedState;
// [ts] let didWarnAboutUndefinedDerivedState;
// [ts] let warnOnUndefinedDerivedState;
// [ts] let warnOnInvalidCallback;
// [ts] let didWarnAboutDirectlyAssigningPropsToState;
// [ts] let didWarnAboutContextTypeAndContextTypes;
// [ts] let didWarnAboutInvalidateContextType;
// [ts] {
// [ts] didWarnAboutStateAssignmentForComponent = Set.init();
// [ts] didWarnAboutUninitializedState = Set.init();
// [ts] didWarnAboutGetSnapshotBeforeUpdateWithoutDidUpdate = Set.init();
// [ts] didWarnAboutLegacyLifecyclesAndDerivedState = Set.init();
// [ts] didWarnAboutDirectlyAssigningPropsToState = Set.init();
// [ts] didWarnAboutUndefinedDerivedState = Set.init();
// [ts] didWarnAboutContextTypeAndContextTypes = Set.init();
// [ts] didWarnAboutInvalidateContextType = Set.init();
// [ts] let didWarnOnInvalidCallback = Set.init();
// [ts] warnOnInvalidCallback = (callback, callerName) => {
// [ts] if (callback == null || @typeName(callback) == "function") {
// [ts] return;
// [ts] }
// [ts] let key = callerName + "_" + callback;
// [ts] if (!didWarnOnInvalidCallback.has(key)) {
// [ts] didWarnOnInvalidCallback.add(key);
// [ts] error("%s(...): Expected the last optional `callback` argument to be a " + "function. Instead received: %s.", callerName, callback);
// [ts] }
// [ts] };
// [ts] warnOnUndefinedDerivedState = (type, partialState) => {
// [ts] if (partialState == null) {
// [ts] let componentName = getComponentName(type) || "Component";
// [ts] if (!didWarnAboutUndefinedDerivedState.has(componentName)) {
// [ts] didWarnAboutUndefinedDerivedState.add(componentName);
// [ts] error("%s.getDerivedStateFromProps(): A valid state object (or null) must be returned. " + "You have returned undefined.", componentName);
// [ts] }
// [ts] }
// [ts] }; // This is so gross but it's at least non-critical and can be removed if
// it causes problems. This is meant to give a nicer error message for
// ReactDOM15.unstable_renderSubtreeIntoContainer(reactDOM16Component,
// ...)) which otherwise throws a "_processChildContext is not a function"
// exception.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(fakeInternalInstance, '_processChildContext', {
// [ts-skip] enumerable: false,
// [ts-skip] value: function () {
// [ts-skip] {
// [ts-skip] {
// [ts-skip] throw Error( "_processChildContext is not available in React 16+. This likely means you have multiple copies of React and are attempting to nest a React 15 tree inside a React 16 tree using unstable_renderSubtreeIntoContainer, which isn't supported. Try to make sure you have only one copy of React (and ideally, switch to ReactDOM.createPortal)." );
// [ts-skip] }
// [ts-skip] }
// [ts-skip] }
// [ts-skip] });
// [ts] Object.freeze(fakeInternalInstance);
// [ts] }
// [ts] fn applyDerivedStateFromProps(workInProgress: auto, ctor: auto, getDerivedStateFromProps: auto, nextProps: auto) {
// [ts] let prevState = workInProgress.memoizedState;
// [ts] {
// [ts] if ( workInProgress.mode & StrictMode) {
// Invoke the function an extra time to help detect side-effects.
// [ts] getDerivedStateFromProps(nextProps, prevState);
// [ts] }
// [ts] }
// [ts] let partialState = getDerivedStateFromProps(nextProps, prevState);
// [ts] {
// [ts] warnOnUndefinedDerivedState(ctor, partialState);
// [ts] } // Merge the partial state and the previous state.
// [ts] let memoizedState = if (partialState == null || partialState == null) prevState else _assign({}, prevState, partialState);
// [ts] workInProgress.memoizedState = memoizedState; // Once the update queue is empty, persist the derived state onto the
// base state.
// [ts] if (workInProgress.expirationTime == NoWork) {
// Queue is always non-null for classes
// [ts] let updateQueue = workInProgress.updateQueue;
// [ts] updateQueue.baseState = memoizedState;
// [ts] }
// [ts] }
// [ts] let classComponentUpdater = {
// [ts] isMounted: isMounted,
// [ts] enqueueSetState: (inst, payload, callback) => {
// [ts] let fiber = get(inst);
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] let suspenseConfig = requestCurrentSuspenseConfig();
// [ts] let expirationTime = computeExpirationForFiber(currentTime, fiber, suspenseConfig);
// [ts] let update = createUpdate(expirationTime, suspenseConfig);
// [ts] update.payload = payload;
// [ts] if (callback != null && callback != null) {
// [ts] {
// [ts] warnOnInvalidCallback(callback, "setState");
// [ts] }
// [ts] update.callback = callback;
// [ts] }
// [ts] enqueueUpdate(fiber, update);
// [ts] scheduleWork(fiber, expirationTime);
// [ts] },
// [ts] enqueueReplaceState: (inst, payload, callback) => {
// [ts] let fiber = get(inst);
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] let suspenseConfig = requestCurrentSuspenseConfig();
// [ts] let expirationTime = computeExpirationForFiber(currentTime, fiber, suspenseConfig);
// [ts] let update = createUpdate(expirationTime, suspenseConfig);
// [ts] update.tag = ReplaceState;
// [ts] update.payload = payload;
// [ts] if (callback != null && callback != null) {
// [ts] {
// [ts] warnOnInvalidCallback(callback, "replaceState");
// [ts] }
// [ts] update.callback = callback;
// [ts] }
// [ts] enqueueUpdate(fiber, update);
// [ts] scheduleWork(fiber, expirationTime);
// [ts] },
// [ts] enqueueForceUpdate: (inst, callback) => {
// [ts] let fiber = get(inst);
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] let suspenseConfig = requestCurrentSuspenseConfig();
// [ts] let expirationTime = computeExpirationForFiber(currentTime, fiber, suspenseConfig);
// [ts] let update = createUpdate(expirationTime, suspenseConfig);
// [ts] update.tag = ForceUpdate;
// [ts] if (callback != null && callback != null) {
// [ts] {
// [ts] warnOnInvalidCallback(callback, "forceUpdate");
// [ts] }
// [ts] update.callback = callback;
// [ts] }
// [ts] enqueueUpdate(fiber, update);
// [ts] scheduleWork(fiber, expirationTime);
// [ts] }
// [ts] };
// [ts] fn checkShouldComponentUpdate(workInProgress: auto, ctor: auto, oldProps: auto, newProps: auto, oldState: auto, newState: auto, nextContext: auto) {
// [ts] let instance = workInProgress.stateNode;
// [ts] if (@typeName(instance.shouldComponentUpdate) == "function") {
// [ts] {
// [ts] if ( workInProgress.mode & StrictMode) {
// Invoke the function an extra time to help detect side-effects.
// [ts] instance.shouldComponentUpdate(newProps, newState, nextContext);
// [ts] }
// [ts] }
// [ts] startPhaseTimer(workInProgress, "shouldComponentUpdate");
// [ts] let shouldUpdate = instance.shouldComponentUpdate(newProps, newState, nextContext);
// [ts] stopPhaseTimer();
// [ts] {
// [ts] if (shouldUpdate == null) {
// [ts] error("%s.shouldComponentUpdate(): Returned undefined instead of a " + "boolean value. Make sure to return true or false.", getComponentName(ctor) || "Component");
// [ts] }
// [ts] }
// [ts] return shouldUpdate;
// [ts] }
// [ts] if (ctor.prototype && ctor.prototype.isPureReactComponent) {
// [ts] return !shallowEqual(oldProps, newProps) || !shallowEqual(oldState, newState);
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn checkClassInstance(workInProgress: auto, ctor: auto, newProps: auto) {
// [ts] let instance = workInProgress.stateNode;
// [ts] {
// [ts] let name = getComponentName(ctor) || "Component";
// [ts] let renderPresent = instance.render;
// [ts] if (!renderPresent) {
// [ts] if (ctor.prototype && @typeName(ctor.prototype.render) == "function") {
// [ts] error("%s(...): No `render` method found on the returned component " + "instance: did you accidentally return an object from the constructor?", name);
// [ts] } else {
// [ts] error("%s(...): No `render` method found on the returned component " + "instance: you may have forgotten to define `render`.", name);
// [ts] }
// [ts] }
// [ts] if (instance.getInitialState && !instance.getInitialState.isReactClassApproved && !instance.state) {
// [ts] error("getInitialState was defined on %s, a plain JavaScript class. " + "This is only supported for classes created using React.createClass. " + "Did you mean to define a state property instead?", name);
// [ts] }
// [ts] if (instance.getDefaultProps && !instance.getDefaultProps.isReactClassApproved) {
// [ts] error("getDefaultProps was defined on %s, a plain JavaScript class. " + "This is only supported for classes created using React.createClass. " + "Use a static property to define defaultProps instead.", name);
// [ts] }
// [ts] if (instance.propTypes) {
// [ts] error("propTypes was defined as an instance property on %s. Use a static " + "property to define propTypes instead.", name);
// [ts] }
// [ts] if (instance.contextType) {
// [ts] error("contextType was defined as an instance property on %s. Use a static " + "property to define contextType instead.", name);
// [ts] }
// [ts] {
// [ts] if (instance.contextTypes) {
// [ts] error("contextTypes was defined as an instance property on %s. Use a static " + "property to define contextTypes instead.", name);
// [ts] }
// [ts] if (ctor.contextType && ctor.contextTypes && !didWarnAboutContextTypeAndContextTypes.has(ctor)) {
// [ts] didWarnAboutContextTypeAndContextTypes.add(ctor);
// [ts] error("%s declares both contextTypes and contextType static properties. " + "The legacy contextTypes property will be ignored.", name);
// [ts] }
// [ts] }
// [ts] if (@typeName(instance.componentShouldUpdate) == "function") {
// [ts] error("%s has a method called " + "componentShouldUpdate(). Did you mean shouldComponentUpdate()? " + "The name is phrased as a question because the function is " + "expected to return a value.", name);
// [ts] }
// [ts] if (ctor.prototype && ctor.prototype.isPureReactComponent && @typeName(instance.shouldComponentUpdate) != "undefined") {
// [ts] error("%s has a method called shouldComponentUpdate(). " + "shouldComponentUpdate should not be used when extending React.PureComponent. " + "Please extend React.Component if shouldComponentUpdate is used.", getComponentName(ctor) || "A pure component");
// [ts] }
// [ts] if (@typeName(instance.componentDidUnmount) == "function") {
// [ts] error("%s has a method called " + "componentDidUnmount(). But there is no such lifecycle method. " + "Did you mean componentWillUnmount()?", name);
// [ts] }
// [ts] if (@typeName(instance.componentDidReceiveProps) == "function") {
// [ts] error("%s has a method called " + "componentDidReceiveProps(). But there is no such lifecycle method. " + "If you meant to update the state in response to changing props, " + "use componentWillReceiveProps(). If you meant to fetch data or " + "run side-effects or mutations after React has updated the UI, use componentDidUpdate().", name);
// [ts] }
// [ts] if (@typeName(instance.componentWillRecieveProps) == "function") {
// [ts] error("%s has a method called " + "componentWillRecieveProps(). Did you mean componentWillReceiveProps()?", name);
// [ts] }
// [ts] if (@typeName(instance.UNSAFE_componentWillRecieveProps) == "function") {
// [ts] error("%s has a method called " + "UNSAFE_componentWillRecieveProps(). Did you mean UNSAFE_componentWillReceiveProps()?", name);
// [ts] }
// [ts] let hasMutatedProps = instance.props != newProps;
// [ts] if (instance.props != null && hasMutatedProps) {
// [ts] error("%s(...): When calling super() in `%s`, make sure to pass " + "up the same props that your component's constructor was passed.", name, name);
// [ts] }
// [ts] if (instance.defaultProps) {
// [ts] error("Setting defaultProps as an instance property on %s is not supported and will be ignored." + " Instead, define defaultProps as a static property on %s.", name, name);
// [ts] }
// [ts] if (@typeName(instance.getSnapshotBeforeUpdate) == "function" && @typeName(instance.componentDidUpdate) != "function" && !didWarnAboutGetSnapshotBeforeUpdateWithoutDidUpdate.has(ctor)) {
// [ts] didWarnAboutGetSnapshotBeforeUpdateWithoutDidUpdate.add(ctor);
// [ts] error("%s: getSnapshotBeforeUpdate() should be used with componentDidUpdate(). " + "This component defines getSnapshotBeforeUpdate() only.", getComponentName(ctor));
// [ts] }
// [ts] if (@typeName(instance.getDerivedStateFromProps) == "function") {
// [ts] error("%s: getDerivedStateFromProps() is defined as an instance method " + "and will be ignored. Instead, declare it as a static method.", name);
// [ts] }
// [ts] if (@typeName(instance.getDerivedStateFromError) == "function") {
// [ts] error("%s: getDerivedStateFromError() is defined as an instance method " + "and will be ignored. Instead, declare it as a static method.", name);
// [ts] }
// [ts] if (@typeName(ctor.getSnapshotBeforeUpdate) == "function") {
// [ts] error("%s: getSnapshotBeforeUpdate() is defined as a static method " + "and will be ignored. Instead, declare it as an instance method.", name);
// [ts] }
// [ts] let _state = instance.state;
// [ts] if (_state && (@typeName(_state) != "object" || isArray(_state))) {
// [ts] error("%s.state: must be set to an object or null", name);
// [ts] }
// [ts] if (@typeName(instance.getChildContext) == "function" && @typeName(ctor.childContextTypes) != "object") {
// [ts] error("%s.getChildContext(): childContextTypes must be defined in order to " + "use getChildContext().", name);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn adoptClassInstance(workInProgress: auto, instance: auto) {
// [ts] instance.updater = classComponentUpdater;
// [ts] workInProgress.stateNode = instance; // The instance needs access to the fiber so that it can schedule updates
// [ts] set(instance, workInProgress);
// [ts] {
// [ts] instance._reactInternalInstance = fakeInternalInstance;
// [ts] }
// [ts] }
// [ts] fn constructClassInstance(workInProgress: auto, ctor: auto, props: auto) {
// [ts] let isLegacyContextConsumer = false;
// [ts] let unmaskedContext = emptyContextObject;
// [ts] let context = emptyContextObject;
// [ts] let contextType = ctor.contextType;
// [ts] {
// [ts] if ("contextType" == ctor) {
// [ts] let isValid = // Allow null for conditional declaration
// [ts] contextType == null || contextType != null && contextType.$$typeof == REACT_CONTEXT_TYPE && contextType._context == null; // Not a <Context.Consumer>
// [ts] if (!isValid && !didWarnAboutInvalidateContextType.has(ctor)) {
// [ts] didWarnAboutInvalidateContextType.add(ctor);
// [ts] let addendum = "";
// [ts] if (contextType == null) {
// [ts] addendum = " However, it is set to undefined. " + "This can be caused by a typo or by mixing up named and default imports. " + "This can also happen due to a circular dependency, so " + "try moving the createContext() call to a separate file.";
// [ts] } else if (@typeName(contextType) != "object") {
// [ts] addendum = " However, it is set to a " + @typeName(contextType) + ".";
// [ts] } else if (contextType.$$typeof == REACT_PROVIDER_TYPE) {
// [ts] addendum = " Did you accidentally pass the Context.Provider instead?";
// [ts] } else if (contextType._context != null) {
// <Context.Consumer>
// [ts] addendum = " Did you accidentally pass the Context.Consumer instead?";
// [ts] } else {
// [ts] addendum = " However, it is set to an object with keys {" + Object.keys(contextType).join(", ") + "}.";
// [ts] }
// [ts] error("%s defines an invalid contextType. " + "contextType should point to the Context object returned by React.createContext().%s", getComponentName(ctor) || "Component", addendum);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (@typeName(contextType) == "object" && contextType != null) {
// [ts] context = readContext(contextType);
// [ts] } else {
// [ts] unmaskedContext = getUnmaskedContext(workInProgress, ctor, true);
// [ts] let contextTypes = ctor.contextTypes;
// [ts] isLegacyContextConsumer = contextTypes != null && contextTypes != null;
// [ts] context = if (isLegacyContextConsumer) getMaskedContext(workInProgress, unmaskedContext) else emptyContextObject;
// [ts] } // Instantiate twice to help detect side-effects.
// [ts] {
// [ts] if ( workInProgress.mode & StrictMode) {
// [ts] ctor.init(props, context); // eslint-disable-line no-new
// [ts] }
// [ts] }
// [ts] let instance = ctor.init(props, context);
// [ts] let state = workInProgress.memoizedState = if (instance.state != null && instance.state != null) instance.state else null;
// [ts] adoptClassInstance(workInProgress, instance);
// [ts] {
// [ts] if (@typeName(ctor.getDerivedStateFromProps) == "function" && state == null) {
// [ts] let componentName = getComponentName(ctor) || "Component";
// [ts] if (!didWarnAboutUninitializedState.has(componentName)) {
// [ts] didWarnAboutUninitializedState.add(componentName);
// [ts] error(if ("`%s` uses `getDerivedStateFromProps` but its initial state is " + "%s. This is not recommended. Instead, define the initial state by " + "assigning an object to `this.state` in the constructor of `%s`. " + "This ensures that `getDerivedStateFromProps` arguments have a consistent shape.", componentName, instance.state == null) "null" else "undefined", componentName);
// [ts] }
// [ts] } // If new component APIs are defined, "unsafe" lifecycles won't be called.
// Warn about these lifecycles if they are present.
// Don't warn about react-lifecycles-compat polyfilled methods though.
// [ts] if (@typeName(ctor.getDerivedStateFromProps) == "function" || @typeName(instance.getSnapshotBeforeUpdate) == "function") {
// [ts] let foundWillMountName = null;
// [ts] let foundWillReceivePropsName = null;
// [ts] let foundWillUpdateName = null;
// [ts] if (@typeName(instance.componentWillMount) == "function" && instance.componentWillMount.__suppressDeprecationWarning != true) {
// [ts] foundWillMountName = "componentWillMount";
// [ts] } else if (@typeName(instance.UNSAFE_componentWillMount) == "function") {
// [ts] foundWillMountName = "UNSAFE_componentWillMount";
// [ts] }
// [ts] if (@typeName(instance.componentWillReceiveProps) == "function" && instance.componentWillReceiveProps.__suppressDeprecationWarning != true) {
// [ts] foundWillReceivePropsName = "componentWillReceiveProps";
// [ts] } else if (@typeName(instance.UNSAFE_componentWillReceiveProps) == "function") {
// [ts] foundWillReceivePropsName = "UNSAFE_componentWillReceiveProps";
// [ts] }
// [ts] if (@typeName(instance.componentWillUpdate) == "function" && instance.componentWillUpdate.__suppressDeprecationWarning != true) {
// [ts] foundWillUpdateName = "componentWillUpdate";
// [ts] } else if (@typeName(instance.UNSAFE_componentWillUpdate) == "function") {
// [ts] foundWillUpdateName = "UNSAFE_componentWillUpdate";
// [ts] }
// [ts] if (foundWillMountName != null || foundWillReceivePropsName != null || foundWillUpdateName != null) {
// [ts] let _componentName = getComponentName(ctor) || "Component";
// [ts] let newApiName = if (@typeName(ctor.getDerivedStateFromProps) == "function") "getDerivedStateFromProps()" else "getSnapshotBeforeUpdate()";
// [ts] if (!didWarnAboutLegacyLifecyclesAndDerivedState.has(_componentName)) {
// [ts] didWarnAboutLegacyLifecyclesAndDerivedState.add(_componentName);
// [ts] error(if ("Unsafe legacy lifecycles will not be called for components using new component APIs.\n\n" + "%s uses %s but also contains the following legacy lifecycles:%s%s%s\n\n" + "The above lifecycles should be removed. Learn more about this warning here:\n" + "https://fb.me/react-unsafe-component-lifecycles", _componentName, newApiName, foundWillMountName != null) "\n  " + foundWillMountName else if ("", foundWillReceivePropsName != null) "\n  " + foundWillReceivePropsName else if ("", foundWillUpdateName != null) "\n  " + foundWillUpdateName else "");
// [ts] }
// [ts] }
// [ts] }
// [ts] } // Cache unmasked context so we can avoid recreating masked context unless necessary.
// ReactFiberContext usually updates this cache but can't for newly-created instances.
// [ts] if (isLegacyContextConsumer) {
// [ts] cacheContext(workInProgress, unmaskedContext, context);
// [ts] }
// [ts] return instance;
// [ts] }
// [ts] fn callComponentWillMount(workInProgress: auto, instance: auto) {
// [ts] startPhaseTimer(workInProgress, "componentWillMount");
// [ts] let oldState = instance.state;
// [ts] if (@typeName(instance.componentWillMount) == "function") {
// [ts] instance.componentWillMount();
// [ts] }
// [ts] if (@typeName(instance.UNSAFE_componentWillMount) == "function") {
// [ts] instance.UNSAFE_componentWillMount();
// [ts] }
// [ts] stopPhaseTimer();
// [ts] if (oldState != instance.state) {
// [ts] {
// [ts] error("%s.componentWillMount(): Assigning directly to this.state is " + "deprecated (except inside a component's " + "constructor). Use setState instead.", getComponentName(workInProgress.type) || "Component");
// [ts] }
// [ts] classComponentUpdater.enqueueReplaceState(instance, instance.state, null);
// [ts] }
// [ts] }
// [ts] fn callComponentWillReceiveProps(workInProgress: auto, instance: auto, newProps: auto, nextContext: auto) {
// [ts] let oldState = instance.state;
// [ts] startPhaseTimer(workInProgress, "componentWillReceiveProps");
// [ts] if (@typeName(instance.componentWillReceiveProps) == "function") {
// [ts] instance.componentWillReceiveProps(newProps, nextContext);
// [ts] }
// [ts] if (@typeName(instance.UNSAFE_componentWillReceiveProps) == "function") {
// [ts] instance.UNSAFE_componentWillReceiveProps(newProps, nextContext);
// [ts] }
// [ts] stopPhaseTimer();
// [ts] if (instance.state != oldState) {
// [ts] {
// [ts] let componentName = getComponentName(workInProgress.type) || "Component";
// [ts] if (!didWarnAboutStateAssignmentForComponent.has(componentName)) {
// [ts] didWarnAboutStateAssignmentForComponent.add(componentName);
// [ts] error("%s.componentWillReceiveProps(): Assigning directly to " + "this.state is deprecated (except inside a component's " + "constructor). Use setState instead.", componentName);
// [ts] }
// [ts] }
// [ts] classComponentUpdater.enqueueReplaceState(instance, instance.state, null);
// [ts] }
// [ts] } // Invokes the mount life-cycles on a previously never rendered instance.
// [ts] fn mountClassInstance(workInProgress: auto, ctor: auto, newProps: auto, renderExpirationTime: auto) {
// [ts] {
// [ts] checkClassInstance(workInProgress, ctor, newProps);
// [ts] }
// [ts] let instance = workInProgress.stateNode;
// [ts] instance.props = newProps;
// [ts] instance.state = workInProgress.memoizedState;
// [ts] instance.refs = emptyRefsObject;
// [ts] initializeUpdateQueue(workInProgress);
// [ts] let contextType = ctor.contextType;
// [ts] if (@typeName(contextType) == "object" && contextType != null) {
// [ts] instance.context = readContext(contextType);
// [ts] } else {
// [ts] let unmaskedContext = getUnmaskedContext(workInProgress, ctor, true);
// [ts] instance.context = getMaskedContext(workInProgress, unmaskedContext);
// [ts] }
// [ts] {
// [ts] if (instance.state == newProps) {
// [ts] let componentName = getComponentName(ctor) || "Component";
// [ts] if (!didWarnAboutDirectlyAssigningPropsToState.has(componentName)) {
// [ts] didWarnAboutDirectlyAssigningPropsToState.add(componentName);
// [ts] error("%s: It is not recommended to assign props directly to state " + "because updates to props won't be reflected in state. " + "In most cases, it is better to use props directly.", componentName);
// [ts] }
// [ts] }
// [ts] if (workInProgress.mode & StrictMode) {
// [ts] ReactStrictModeWarnings.recordLegacyContextWarning(workInProgress, instance);
// [ts] }
// [ts] {
// [ts] ReactStrictModeWarnings.recordUnsafeLifecycleWarnings(workInProgress, instance);
// [ts] }
// [ts] }
// [ts] processUpdateQueue(workInProgress, newProps, instance, renderExpirationTime);
// [ts] instance.state = workInProgress.memoizedState;
// [ts] let getDerivedStateFromProps = ctor.getDerivedStateFromProps;
// [ts] if (@typeName(getDerivedStateFromProps) == "function") {
// [ts] applyDerivedStateFromProps(workInProgress, ctor, getDerivedStateFromProps, newProps);
// [ts] instance.state = workInProgress.memoizedState;
// [ts] } // In order to support react-lifecycles-compat polyfilled components,
// Unsafe lifecycles should not be invoked for components using the new APIs.
// [ts] if (@typeName(ctor.getDerivedStateFromProps) != "function" && @typeName(instance.getSnapshotBeforeUpdate) != "function" && (@typeName(instance.UNSAFE_componentWillMount) == "function" || @typeName(instance.componentWillMount) == "function")) {
// [ts] callComponentWillMount(workInProgress, instance); // If we had additional state updates during this life-cycle, let's
// process them now.
// [ts] processUpdateQueue(workInProgress, newProps, instance, renderExpirationTime);
// [ts] instance.state = workInProgress.memoizedState;
// [ts] }
// [ts] if (@typeName(instance.componentDidMount) == "function") {
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] }
// [ts] fn resumeMountClassInstance(workInProgress: auto, ctor: auto, newProps: auto, renderExpirationTime: auto) {
// [ts] let instance = workInProgress.stateNode;
// [ts] let oldProps = workInProgress.memoizedProps;
// [ts] instance.props = oldProps;
// [ts] let oldContext = instance.context;
// [ts] let contextType = ctor.contextType;
// [ts] let nextContext = emptyContextObject;
// [ts] if (@typeName(contextType) == "object" && contextType != null) {
// [ts] nextContext = readContext(contextType);
// [ts] } else {
// [ts] let nextLegacyUnmaskedContext = getUnmaskedContext(workInProgress, ctor, true);
// [ts] nextContext = getMaskedContext(workInProgress, nextLegacyUnmaskedContext);
// [ts] }
// [ts] let getDerivedStateFromProps = ctor.getDerivedStateFromProps;
// [ts] let hasNewLifecycles = @typeName(getDerivedStateFromProps) == "function" || @typeName(instance.getSnapshotBeforeUpdate) == "function"; // Note: During these life-cycles, instance.props/instance.state are what
// ever the previously attempted to render - not the "current". However,
// during componentDidUpdate we pass the "current" props.
// In order to support react-lifecycles-compat polyfilled components,
// Unsafe lifecycles should not be invoked for components using the new APIs.
// [ts] if (!hasNewLifecycles && (@typeName(instance.UNSAFE_componentWillReceiveProps) == "function" || @typeName(instance.componentWillReceiveProps) == "function")) {
// [ts] if (oldProps != newProps || oldContext != nextContext) {
// [ts] callComponentWillReceiveProps(workInProgress, instance, newProps, nextContext);
// [ts] }
// [ts] }
// [ts] resetHasForceUpdateBeforeProcessing();
// [ts] let oldState = workInProgress.memoizedState;
// [ts] let newState = instance.state = oldState;
// [ts] processUpdateQueue(workInProgress, newProps, instance, renderExpirationTime);
// [ts] newState = workInProgress.memoizedState;
// [ts] if (oldProps == newProps && oldState == newState && !hasContextChanged() && !checkHasForceUpdateAfterProcessing()) {
// If an update was already in progress, we should schedule an Update
// effect even though we're bailing out, so that cWU/cDU are called.
// [ts] if (@typeName(instance.componentDidMount) == "function") {
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] if (@typeName(getDerivedStateFromProps) == "function") {
// [ts] applyDerivedStateFromProps(workInProgress, ctor, getDerivedStateFromProps, newProps);
// [ts] newState = workInProgress.memoizedState;
// [ts] }
// [ts] let shouldUpdate = checkHasForceUpdateAfterProcessing() || checkShouldComponentUpdate(workInProgress, ctor, oldProps, newProps, oldState, newState, nextContext);
// [ts] if (shouldUpdate) {
// In order to support react-lifecycles-compat polyfilled components,
// Unsafe lifecycles should not be invoked for components using the new APIs.
// [ts] if (!hasNewLifecycles && (@typeName(instance.UNSAFE_componentWillMount) == "function" || @typeName(instance.componentWillMount) == "function")) {
// [ts] startPhaseTimer(workInProgress, "componentWillMount");
// [ts] if (@typeName(instance.componentWillMount) == "function") {
// [ts] instance.componentWillMount();
// [ts] }
// [ts] if (@typeName(instance.UNSAFE_componentWillMount) == "function") {
// [ts] instance.UNSAFE_componentWillMount();
// [ts] }
// [ts] stopPhaseTimer();
// [ts] }
// [ts] if (@typeName(instance.componentDidMount) == "function") {
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] } else {
// If an update was already in progress, we should schedule an Update
// effect even though we're bailing out, so that cWU/cDU are called.
// [ts] if (@typeName(instance.componentDidMount) == "function") {
// [ts] workInProgress.effectTag |= Update;
// [ts] } // If shouldComponentUpdate returned false, we should still update the
// memoized state to indicate that this work can be reused.
// [ts] workInProgress.memoizedProps = newProps;
// [ts] workInProgress.memoizedState = newState;
// [ts] } // Update the existing instance's state, props, and context pointers even
// if shouldComponentUpdate returns false.
// [ts] instance.props = newProps;
// [ts] instance.state = newState;
// [ts] instance.context = nextContext;
// [ts] return shouldUpdate;
// [ts] } // Invokes the update life-cycles and returns false if it shouldn't rerender.
// [ts] fn updateClassInstance(current: auto, workInProgress: auto, ctor: auto, newProps: auto, renderExpirationTime: auto) {
// [ts] let instance = workInProgress.stateNode;
// [ts] cloneUpdateQueue(current, workInProgress);
// [ts] let oldProps = workInProgress.memoizedProps;
// [ts] instance.props = if (workInProgress.type == workInProgress.elementType) oldProps else resolveDefaultProps(workInProgress.type, oldProps);
// [ts] let oldContext = instance.context;
// [ts] let contextType = ctor.contextType;
// [ts] let nextContext = emptyContextObject;
// [ts] if (@typeName(contextType) == "object" && contextType != null) {
// [ts] nextContext = readContext(contextType);
// [ts] } else {
// [ts] let nextUnmaskedContext = getUnmaskedContext(workInProgress, ctor, true);
// [ts] nextContext = getMaskedContext(workInProgress, nextUnmaskedContext);
// [ts] }
// [ts] let getDerivedStateFromProps = ctor.getDerivedStateFromProps;
// [ts] let hasNewLifecycles = @typeName(getDerivedStateFromProps) == "function" || @typeName(instance.getSnapshotBeforeUpdate) == "function"; // Note: During these life-cycles, instance.props/instance.state are what
// ever the previously attempted to render - not the "current". However,
// during componentDidUpdate we pass the "current" props.
// In order to support react-lifecycles-compat polyfilled components,
// Unsafe lifecycles should not be invoked for components using the new APIs.
// [ts] if (!hasNewLifecycles && (@typeName(instance.UNSAFE_componentWillReceiveProps) == "function" || @typeName(instance.componentWillReceiveProps) == "function")) {
// [ts] if (oldProps != newProps || oldContext != nextContext) {
// [ts] callComponentWillReceiveProps(workInProgress, instance, newProps, nextContext);
// [ts] }
// [ts] }
// [ts] resetHasForceUpdateBeforeProcessing();
// [ts] let oldState = workInProgress.memoizedState;
// [ts] let newState = instance.state = oldState;
// [ts] processUpdateQueue(workInProgress, newProps, instance, renderExpirationTime);
// [ts] newState = workInProgress.memoizedState;
// [ts] if (oldProps == newProps && oldState == newState && !hasContextChanged() && !checkHasForceUpdateAfterProcessing()) {
// If an update was already in progress, we should schedule an Update
// effect even though we're bailing out, so that cWU/cDU are called.
// [ts] if (@typeName(instance.componentDidUpdate) == "function") {
// [ts] if (oldProps != current.memoizedProps || oldState != current.memoizedState) {
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] }
// [ts] if (@typeName(instance.getSnapshotBeforeUpdate) == "function") {
// [ts] if (oldProps != current.memoizedProps || oldState != current.memoizedState) {
// [ts] workInProgress.effectTag |= Snapshot;
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] if (@typeName(getDerivedStateFromProps) == "function") {
// [ts] applyDerivedStateFromProps(workInProgress, ctor, getDerivedStateFromProps, newProps);
// [ts] newState = workInProgress.memoizedState;
// [ts] }
// [ts] let shouldUpdate = checkHasForceUpdateAfterProcessing() || checkShouldComponentUpdate(workInProgress, ctor, oldProps, newProps, oldState, newState, nextContext);
// [ts] if (shouldUpdate) {
// In order to support react-lifecycles-compat polyfilled components,
// Unsafe lifecycles should not be invoked for components using the new APIs.
// [ts] if (!hasNewLifecycles && (@typeName(instance.UNSAFE_componentWillUpdate) == "function" || @typeName(instance.componentWillUpdate) == "function")) {
// [ts] startPhaseTimer(workInProgress, "componentWillUpdate");
// [ts] if (@typeName(instance.componentWillUpdate) == "function") {
// [ts] instance.componentWillUpdate(newProps, newState, nextContext);
// [ts] }
// [ts] if (@typeName(instance.UNSAFE_componentWillUpdate) == "function") {
// [ts] instance.UNSAFE_componentWillUpdate(newProps, newState, nextContext);
// [ts] }
// [ts] stopPhaseTimer();
// [ts] }
// [ts] if (@typeName(instance.componentDidUpdate) == "function") {
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] if (@typeName(instance.getSnapshotBeforeUpdate) == "function") {
// [ts] workInProgress.effectTag |= Snapshot;
// [ts] }
// [ts] } else {
// If an update was already in progress, we should schedule an Update
// effect even though we're bailing out, so that cWU/cDU are called.
// [ts] if (@typeName(instance.componentDidUpdate) == "function") {
// [ts] if (oldProps != current.memoizedProps || oldState != current.memoizedState) {
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] }
// [ts] if (@typeName(instance.getSnapshotBeforeUpdate) == "function") {
// [ts] if (oldProps != current.memoizedProps || oldState != current.memoizedState) {
// [ts] workInProgress.effectTag |= Snapshot;
// [ts] }
// [ts] } // If shouldComponentUpdate returned false, we should still update the
// memoized props/state to indicate that this work can be reused.
// [ts] workInProgress.memoizedProps = newProps;
// [ts] workInProgress.memoizedState = newState;
// [ts] } // Update the existing instance's state, props, and context pointers even
// if shouldComponentUpdate returns false.
// [ts] instance.props = newProps;
// [ts] instance.state = newState;
// [ts] instance.context = nextContext;
// [ts] return shouldUpdate;
// [ts] }
// [ts] let didWarnAboutMaps;
// [ts] let didWarnAboutGenerators;
// [ts] let didWarnAboutStringRefs;
// [ts] let ownerHasKeyUseWarning;
// [ts] let ownerHasFunctionTypeWarning;
// [ts] let warnForMissingKey = (child) => {};
// [ts] {
// [ts] didWarnAboutMaps = false;
// [ts] didWarnAboutGenerators = false;
// [ts] didWarnAboutStringRefs = {};
// [ts] /**
// [ts] * Warn if there's no key explicitly set on dynamic arrays of children or
// [ts] * object keys are not valid. This allows us to keep track of children between
// [ts] * updates.
// [ts] */
// [ts] ownerHasKeyUseWarning = {};
// [ts] ownerHasFunctionTypeWarning = {};
// [ts] warnForMissingKey = (child) => {
// [ts] if (child == null || @typeName(child) != "object") {
// [ts] return;
// [ts] }
// [ts] if (!child._store || child._store.validated || child.key != null) {
// [ts] return;
// [ts] }
// [ts] if (!(@typeName(child._store) == "object")) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "React Component in warnForMissingKey should have a _store. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] child._store.validated = true;
// [ts] let currentComponentErrorInfo = "Each child in a list should have a unique " + "\"key\" prop. See https://fb.me/react-warning-keys for " + "more information." + getCurrentFiberStackInDev();
// [ts] if (ownerHasKeyUseWarning[currentComponentErrorInfo]) {
// [ts] return;
// [ts] }
// [ts] ownerHasKeyUseWarning[currentComponentErrorInfo] = true;
// [ts] error("Each child in a list should have a unique " + "\"key\" prop. See https://fb.me/react-warning-keys for " + "more information.");
// [ts] };
// [ts] }
// [ts] let isArray$1 = Array.isArray;
// [ts] fn coerceRef(returnFiber: auto, current: auto, element: auto) {
// [ts] let mixedRef = element.ref;
// [ts] if (mixedRef != null && @typeName(mixedRef) != "function" && @typeName(mixedRef) != "object") {
// [ts] {
// TODO: Clean this up once we turn on the string ref warning for
// everyone, because the strict mode case will no longer be relevant
// [ts] if ((returnFiber.mode & StrictMode || warnAboutStringRefs) && // We warn in ReactElement.js if owner and self are equal for string refs
// because these cannot be automatically converted to an arrow function
// using a codemod. Therefore, we don't have to warn about string refs again.
// [ts] !(element._owner && element._self && element._owner.stateNode != element._self)) {
// [ts] let componentName = getComponentName(returnFiber.type) || "Component";
// [ts] if (!didWarnAboutStringRefs[componentName]) {
// [ts] {
// [ts] error("A string ref, \"%s\", has been found within a strict mode tree. " + "String refs are a source of potential bugs and should be avoided. " + "We recommend using useRef() or createRef() instead. " + "Learn more about using refs safely here: " + "https://fb.me/react-strict-mode-string-ref%s", mixedRef, getStackByFiberInDevAndProd(returnFiber));
// [ts] }
// [ts] didWarnAboutStringRefs[componentName] = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] if (element._owner) {
// [ts] let owner = element._owner;
// [ts] let inst;
// [ts] if (owner) {
// [ts] let ownerFiber = owner;
// [ts] if (!(ownerFiber.tag == ClassComponent)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Function components cannot have string refs. We recommend using useRef() instead. Learn more about using refs safely here: https://fb.me/react-strict-mode-string-ref" );)
// [ts] }
// [ts] }
// [ts] inst = ownerFiber.stateNode;
// [ts] }
// [ts] if (!inst) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Missing owner for string ref " + mixedRef + ". This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] let stringRef = "" + mixedRef; // Check if previous string ref matches new string ref
// [ts] if (current != null && current.ref != null && @typeName(current.ref) == "function" && current.ref._stringRef == stringRef) {
// [ts] return current.ref;
// [ts] }
// [ts] let ref = (value) => {
// [ts] let refs = inst.refs;
// [ts] if (refs == emptyRefsObject) {
// This is a lazy pooled frozen object, so we need to initialize.
// [ts] refs = inst.refs = {};
// [ts] }
// [ts] if (value == null) {
// [ts] delete refs[stringRef];
// [ts] } else {
// [ts] refs[stringRef] = value;
// [ts] }
// [ts] };
// [ts] ref._stringRef = stringRef;
// [ts] return ref;
// [ts] } else {
// [ts] if (!(@typeName(mixedRef) == "string")) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected ref to be a function, a string, an object returned by React.createRef(), or null." );)
// [ts] }
// [ts] }
// [ts] if (!element._owner) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Element ref was specified as a string (" + mixedRef + ") but no owner was set. This could happen for one of the following reasons:\n1. You may be adding a ref to a function component\n2. You may be adding a ref to a component that was not created inside a component's render method\n3. You have multiple copies of React loaded\nSee https://fb.me/react-refs-must-have-owner for more information." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return mixedRef;
// [ts] }
// [ts] fn throwOnInvalidObjectType(returnFiber: auto, newChild: auto) {
// [ts] if (returnFiber.type != "textarea") {
// [ts] let addendum = "";
// [ts] {
// [ts] addendum = " If you meant to render a collection of children, use an array " + "instead." + getCurrentFiberStackInDev();
// [ts] }
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Objects are not valid as a React child (found: " + (Object.prototype.toString.call(newChild) === '[object Object]' ? 'object with keys {' + Object.keys(newChild).join(', ') + '}' : newChild) + ")." + addendum );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn warnOnFunctionType() {
// [ts] {
// [ts] let currentComponentErrorInfo = "Functions are not valid as a React child. This may happen if " + "you return a Component instead of <Component /> from render. " + "Or maybe you meant to call this function rather than return it." + getCurrentFiberStackInDev();
// [ts] if (ownerHasFunctionTypeWarning[currentComponentErrorInfo]) {
// [ts] return;
// [ts] }
// [ts] ownerHasFunctionTypeWarning[currentComponentErrorInfo] = true;
// [ts] error("Functions are not valid as a React child. This may happen if " + "you return a Component instead of <Component /> from render. " + "Or maybe you meant to call this function rather than return it.");
// [ts] }
// [ts] } // This wrapper function exists because I expect to clone the code in each path
// to be able to optimize each path individually by branching early. This needs
// a compiler or we can do it manually. Helpers that don't need this branching
// live outside of this function.
// [ts] fn ChildReconciler(shouldTrackSideEffects: auto) {
// [ts] fn deleteChild(returnFiber: auto, childToDelete: auto) {
// [ts] if (!shouldTrackSideEffects) {
// Noop.
// [ts] return;
// [ts] } // Deletions are added in reversed order so we add it to the front.
// At this point, the return fiber's effect list is empty except for
// deletions, so we can just append the deletion to the list. The remaining
// effects aren't added until the complete phase. Once we implement
// resuming, this may not be true.
// [ts] let last = returnFiber.lastEffect;
// [ts] if (last != null) {
// [ts] last.nextEffect = childToDelete;
// [ts] returnFiber.lastEffect = childToDelete;
// [ts] } else {
// [ts] returnFiber.firstEffect = returnFiber.lastEffect = childToDelete;
// [ts] }
// [ts] childToDelete.nextEffect = null;
// [ts] childToDelete.effectTag = Deletion;
// [ts] }
// [ts] fn deleteRemainingChildren(returnFiber: auto, currentFirstChild: auto) {
// [ts] if (!shouldTrackSideEffects) {
// Noop.
// [ts] return null;
// [ts] } // TODO: For the shouldClone case, this could be micro-optimized a bit by
// assuming that after the first child we've already added everything.
// [ts] let childToDelete = currentFirstChild;
// [ts] while (childToDelete != null) {
// [ts] deleteChild(returnFiber, childToDelete);
// [ts] childToDelete = childToDelete.sibling;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn mapRemainingChildren(returnFiber: auto, currentFirstChild: auto) {
// Add the remaining children to a temporary map so that we can find them by
// keys quickly. Implicit (null) keys get added to this set with their index
// instead.
// [ts] let existingChildren = Map.init();
// [ts] let existingChild = currentFirstChild;
// [ts] while (existingChild != null) {
// [ts] if (existingChild.key != null) {
// [ts] existingChildren.set(existingChild.key, existingChild);
// [ts] } else {
// [ts] existingChildren.set(existingChild.index, existingChild);
// [ts] }
// [ts] existingChild = existingChild.sibling;
// [ts] }
// [ts] return existingChildren;
// [ts] }
// [ts] fn useFiber(fiber: auto, pendingProps: auto) {
// We currently set sibling to null and index to 0 here because it is easy
// to forget to do before returning it. E.g. for the single child case.
// [ts] let clone = createWorkInProgress(fiber, pendingProps);
// [ts] clone.index = 0;
// [ts] clone.sibling = null;
// [ts] return clone;
// [ts] }
// [ts] fn placeChild(newFiber: auto, lastPlacedIndex: auto, newIndex: auto) {
// [ts] newFiber.index = newIndex;
// [ts] if (!shouldTrackSideEffects) {
// Noop.
// [ts] return lastPlacedIndex;
// [ts] }
// [ts] let current = newFiber.alternate;
// [ts] if (current != null) {
// [ts] let oldIndex = current.index;
// [ts] if (oldIndex < lastPlacedIndex) {
// This is a move.
// [ts] newFiber.effectTag = Placement;
// [ts] return lastPlacedIndex;
// [ts] } else {
// This item can stay in place.
// [ts] return oldIndex;
// [ts] }
// [ts] } else {
// This is an insertion.
// [ts] newFiber.effectTag = Placement;
// [ts] return lastPlacedIndex;
// [ts] }
// [ts] }
// [ts] fn placeSingleChild(newFiber: auto) {
// This is simpler for the single child case. We only need to do a
// placement for inserting new children.
// [ts] if (shouldTrackSideEffects && newFiber.alternate == null) {
// [ts] newFiber.effectTag = Placement;
// [ts] }
// [ts] return newFiber;
// [ts] }
// [ts] fn updateTextNode(returnFiber: auto, current: auto, textContent: auto, expirationTime: auto) {
// [ts] if (current == null || current.tag != HostText) {
// Insert
// [ts] let created = createFiberFromText(textContent, returnFiber.mode, expirationTime);
// [ts] created.return = returnFiber;
// [ts] return created;
// [ts] } else {
// Update
// [ts] let existing = useFiber(current, textContent);
// [ts] existing.return = returnFiber;
// [ts] return existing;
// [ts] }
// [ts] }
// [ts] fn updateElement(returnFiber: auto, current: auto, element: auto, expirationTime: auto) {
// [ts] if (current != null) {
// [ts] if (current.elementType == element.type || ( // Keep this check inline so it only runs on the false path:
// [ts] isCompatibleFamilyForHotReloading: (current, element) => {
// Move based on index
// [ts] let existing = useFiber(current, element.props);
// [ts] existing.ref = coerceRef(returnFiber, current, element);
// [ts] existing.return = returnFiber;
// [ts] {
// [ts] existing._debugSource = element._source;
// [ts] existing._debugOwner = element._owner;
// [ts] }
// [ts] return existing;
// [ts] }
// [ts] } // Insert
// [ts] let created = createFiberFromElement(element, returnFiber.mode, expirationTime);
// [ts] created.ref = coerceRef(returnFiber, current, element);
// [ts] created.return = returnFiber;
// [ts] return created;
// [ts] }
// [ts] fn updatePortal(returnFiber: auto, current: auto, portal: auto, expirationTime: auto) {
// [ts] if (current == null || current.tag != HostPortal || current.stateNode.containerInfo != portal.containerInfo || current.stateNode.implementation != portal.implementation) {
// Insert
// [ts] let created = createFiberFromPortal(portal, returnFiber.mode, expirationTime);
// [ts] created.return = returnFiber;
// [ts] return created;
// [ts] } else {
// Update
// [ts] let existing = useFiber(current, portal.children || []);
// [ts] existing.return = returnFiber;
// [ts] return existing;
// [ts] }
// [ts] }
// [ts] fn updateFragment(returnFiber: auto, current: auto, fragment: auto, expirationTime: auto, key: auto) {
// [ts] if (current == null || current.tag != Fragment) {
// Insert
// [ts] let created = createFiberFromFragment(fragment, returnFiber.mode, expirationTime, key);
// [ts] created.return = returnFiber;
// [ts] return created;
// [ts] } else {
// Update
// [ts] let existing = useFiber(current, fragment);
// [ts] existing.return = returnFiber;
// [ts] return existing;
// [ts] }
// [ts] }
// [ts] fn createChild(returnFiber: auto, newChild: auto, expirationTime: auto) {
// [ts] if (@typeName(newChild) == "string" || @typeName(newChild) == "number") {
// Text nodes don't have keys. If the previous node is implicitly keyed
// we can continue to replace it without aborting even if it is not a text
// node.
// [ts] let created = createFiberFromText("" + newChild, returnFiber.mode, expirationTime);
// [ts] created.return = returnFiber;
// [ts] return created;
// [ts] }
// [ts] if (@typeName(newChild) == "object" && newChild != null) {
// [ts] switch (newChild.$$typeof) {
// [ts] REACT_ELEMENT_TYPE => {
// [ts] {
// [ts] let _created = createFiberFromElement(newChild, returnFiber.mode, expirationTime);
// [ts] _created.ref = coerceRef(returnFiber, null, newChild);
// [ts] _created.return = returnFiber;
// [ts] return _created;
// [ts] }
// [ts] },
// [ts] REACT_PORTAL_TYPE => {
// [ts] {
// [ts] let _created2 = createFiberFromPortal(newChild, returnFiber.mode, expirationTime);
// [ts] _created2.return = returnFiber;
// [ts] return _created2;
// [ts] }
// [ts] }
// [ts] }
// [ts] if (isArray$1(newChild) || getIteratorFn(newChild)) {
// [ts] let _created3 = createFiberFromFragment(newChild, returnFiber.mode, expirationTime, null);
// [ts] _created3.return = returnFiber;
// [ts] return _created3;
// [ts] }
// [ts] throwOnInvalidObjectType(returnFiber, newChild);
// [ts] }
// [ts] {
// [ts] if (@typeName(newChild) == "function") {
// [ts] warnOnFunctionType();
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn updateSlot(returnFiber: auto, oldFiber: auto, newChild: auto, expirationTime: auto) {
// Update the fiber if the keys match, otherwise return null.
// [ts] let key = if (oldFiber != null) oldFiber.key else null;
// [ts] if (@typeName(newChild) == "string" || @typeName(newChild) == "number") {
// Text nodes don't have keys. If the previous node is implicitly keyed
// we can continue to replace it without aborting even if it is not a text
// node.
// [ts] if (key != null) {
// [ts] return null;
// [ts] }
// [ts] return updateTextNode(returnFiber, oldFiber, "" + newChild, expirationTime);
// [ts] }
// [ts] if (@typeName(newChild) == "object" && newChild != null) {
// [ts] switch (newChild.$$typeof) {
// [ts] REACT_ELEMENT_TYPE => {
// [ts] {
// [ts] if (newChild.key == key) {
// [ts] if (newChild.type == REACT_FRAGMENT_TYPE) {
// [ts] return updateFragment(returnFiber, oldFiber, newChild.props.children, expirationTime, key);
// [ts] }
// [ts] return updateElement(returnFiber, oldFiber, newChild, expirationTime);
// [ts] } else {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] },
// [ts] REACT_PORTAL_TYPE => {
// [ts] {
// [ts] if (newChild.key == key) {
// [ts] return updatePortal(returnFiber, oldFiber, newChild, expirationTime);
// [ts] } else {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (isArray$1(newChild) || getIteratorFn(newChild)) {
// [ts] if (key != null) {
// [ts] return null;
// [ts] }
// [ts] return updateFragment(returnFiber, oldFiber, newChild, expirationTime, null);
// [ts] }
// [ts] throwOnInvalidObjectType(returnFiber, newChild);
// [ts] }
// [ts] {
// [ts] if (@typeName(newChild) == "function") {
// [ts] warnOnFunctionType();
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn updateFromMap(existingChildren: auto, returnFiber: auto, newIdx: auto, newChild: auto, expirationTime: auto) {
// [ts] if (@typeName(newChild) == "string" || @typeName(newChild) == "number") {
// Text nodes don't have keys, so we neither have to check the old nor
// new node for the key. If both are text nodes, they match.
// [ts] let matchedFiber = existingChildren.get(newIdx) || null;
// [ts] return updateTextNode(returnFiber, matchedFiber, "" + newChild, expirationTime);
// [ts] }
// [ts] if (@typeName(newChild) == "object" && newChild != null) {
// [ts] switch (newChild.$$typeof) {
// [ts] REACT_ELEMENT_TYPE => {
// [ts] {
// [ts] let _matchedFiber = existingChildren.get(if (newChild.key == null) newIdx else newChild.key) || null;
// [ts] if (newChild.type == REACT_FRAGMENT_TYPE) {
// [ts] return updateFragment(returnFiber, _matchedFiber, newChild.props.children, expirationTime, newChild.key);
// [ts] }
// [ts] return updateElement(returnFiber, _matchedFiber, newChild, expirationTime);
// [ts] }
// [ts] },
// [ts] REACT_PORTAL_TYPE => {
// [ts] {
// [ts] let _matchedFiber2 = existingChildren.get(if (newChild.key == null) newIdx else newChild.key) || null;
// [ts] return updatePortal(returnFiber, _matchedFiber2, newChild, expirationTime);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (isArray$1(newChild) || getIteratorFn(newChild)) {
// [ts] let _matchedFiber3 = existingChildren.get(newIdx) || null;
// [ts] return updateFragment(returnFiber, _matchedFiber3, newChild, expirationTime, null);
// [ts] }
// [ts] throwOnInvalidObjectType(returnFiber, newChild);
// [ts] }
// [ts] {
// [ts] if (@typeName(newChild) == "function") {
// [ts] warnOnFunctionType();
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * Warns if there is a duplicate or missing key
// [ts] */
// [ts] fn warnOnInvalidKey(child: auto, knownKeys: auto) {
// [ts] {
// [ts] if (@typeName(child) != "object" || child == null) {
// [ts] return knownKeys;
// [ts] }
// [ts] switch (child.$$typeof) {
// [ts] REACT_ELEMENT_TYPE, REACT_PORTAL_TYPE => {
// [ts] warnForMissingKey(child);
// [ts] let key = child.key;
// [ts] if (@typeName(key) != "string") {
// [ts] break;
// [ts] }
// [ts] if (knownKeys == null) {
// [ts] knownKeys = Set.init();
// [ts] knownKeys.add(key);
// [ts] break;
// [ts] }
// [ts] if (!knownKeys.has(key)) {
// [ts] knownKeys.add(key);
// [ts] break;
// [ts] }
// [ts] error("Encountered two children with the same key, `%s`. " + "Keys should be unique so that components maintain their identity " + "across updates. Non-unique keys may cause children to be " + "duplicated and/or omitted ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ the behavior is unsupported and " + "could change in a future version.", key);
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] return knownKeys;
// [ts] }
// [ts] fn reconcileChildrenArray(returnFiber: auto, currentFirstChild: auto, newChildren: auto, expirationTime: auto) {
// This algorithm can't optimize by searching from both ends since we
// don't have backpointers on fibers. I'm trying to see how far we can get
// with that model. If it ends up not being worth the tradeoffs, we can
// add it later.
// Even with a two ended optimization, we'd want to optimize for the case
// where there are few changes and brute force the comparison instead of
// going for the Map. It'd like to explore hitting that path first in
// forward-only mode and only go for the Map once we notice that we need
// lots of look ahead. This doesn't handle reversal as well as two ended
// search but that's unusual. Besides, for the two ended optimization to
// work on Iterables, we'd need to copy the whole set.
// In this first iteration, we'll just live with hitting the bad case
// (adding everything to a Map) in for every insert/move.
// If you change this code, also update reconcileChildrenIterator() which
// uses the same algorithm.
// [ts] {
// First, validate keys.
// [ts] let knownKeys = null;
// [ts] let i = 0;
// [ts] while (i < newChildren.length) { // [cap-migrate] for-step: i++
// [ts] let child = newChildren[i];
// [ts] knownKeys = warnOnInvalidKey(child, knownKeys);
// [ts] }
// [ts] }
// [ts] let resultingFirstChild = null;
// [ts] let previousNewFiber = null;
// [ts] let oldFiber = currentFirstChild;
// [ts] let lastPlacedIndex = 0;
// [ts] let newIdx = 0;
// [ts] let nextOldFiber = null;
// [ts] while (oldFiber != null && newIdx < newChildren.length) { // [cap-migrate] for-step: newIdx++
// [ts] if (oldFiber.index > newIdx) {
// [ts] nextOldFiber = oldFiber;
// [ts] oldFiber = null;
// [ts] } else {
// [ts] nextOldFiber = oldFiber.sibling;
// [ts] }
// [ts] let newFiber = updateSlot(returnFiber, oldFiber, newChildren[newIdx], expirationTime);
// [ts] if (newFiber == null) {
// TODO: This breaks on empty slots like null children. That's
// unfortunate because it triggers the slow path all the time. We need
// a better way to communicate whether this was a miss or null,
// boolean, undefined, etc.
// [ts] if (oldFiber == null) {
// [ts] oldFiber = nextOldFiber;
// [ts] }
// [ts] break;
// [ts] }
// [ts] if (shouldTrackSideEffects) {
// [ts] if (oldFiber && newFiber.alternate == null) {
// We matched the slot, but we didn't reuse the existing fiber, so we
// need to delete the existing child.
// [ts] deleteChild(returnFiber, oldFiber);
// [ts] }
// [ts] }
// [ts] lastPlacedIndex = placeChild(newFiber, lastPlacedIndex, newIdx);
// [ts] if (previousNewFiber == null) {
// TODO: Move out of the loop. This only happens for the first run.
// [ts] resultingFirstChild = newFiber;
// [ts] } else {
// TODO: Defer siblings if we're not at the right index for this slot.
// I.e. if we had null values before, then we want to defer this
// for each null value. However, we also don't want to call updateSlot
// with the previous one.
// [ts] previousNewFiber.sibling = newFiber;
// [ts] }
// [ts] previousNewFiber = newFiber;
// [ts] oldFiber = nextOldFiber;
// [ts] }
// [ts] if (newIdx == newChildren.length) {
// We've reached the end of the new children. We can delete the rest.
// [ts] deleteRemainingChildren(returnFiber, oldFiber);
// [ts] return resultingFirstChild;
// [ts] }
// [ts] if (oldFiber == null) {
// If we don't have any more existing children we can choose a fast path
// since the rest will all be insertions.
// [ts] while (newIdx < newChildren.length) { // [cap-migrate] for-step: newIdx++
// [ts] let _newFiber = createChild(returnFiber, newChildren[newIdx], expirationTime);
// [ts] if (_newFiber == null) {
// [ts] continue;
// [ts] }
// [ts] lastPlacedIndex = placeChild(_newFiber, lastPlacedIndex, newIdx);
// [ts] if (previousNewFiber == null) {
// TODO: Move out of the loop. This only happens for the first run.
// [ts] resultingFirstChild = _newFiber;
// [ts] } else {
// [ts] previousNewFiber.sibling = _newFiber;
// [ts] }
// [ts] previousNewFiber = _newFiber;
// [ts] }
// [ts] return resultingFirstChild;
// [ts] } // Add all children to a key map for quick lookups.
// [ts] let existingChildren = mapRemainingChildren(returnFiber, oldFiber); // Keep scanning and use the map to restore deleted items as moves.
// [ts] while (newIdx < newChildren.length) { // [cap-migrate] for-step: newIdx++
// [ts] let _newFiber2 = updateFromMap(existingChildren, returnFiber, newIdx, newChildren[newIdx], expirationTime);
// [ts] if (_newFiber2 != null) {
// [ts] if (shouldTrackSideEffects) {
// [ts] if (_newFiber2.alternate != null) {
// The new fiber is a work in progress, but if there exists a
// current, that means that we reused the fiber. We need to delete
// it from the child list so that we don't add it to the deletion
// list.
// [ts] existingChildren.delete(if (_newFiber2.key == null) newIdx else _newFiber2.key);
// [ts] }
// [ts] }
// [ts] lastPlacedIndex = placeChild(_newFiber2, lastPlacedIndex, newIdx);
// [ts] if (previousNewFiber == null) {
// [ts] resultingFirstChild = _newFiber2;
// [ts] } else {
// [ts] previousNewFiber.sibling = _newFiber2;
// [ts] }
// [ts] previousNewFiber = _newFiber2;
// [ts] }
// [ts] }
// [ts] if (shouldTrackSideEffects) {
// Any existing children that weren't consumed above were deleted. We need
// to add them to the deletion list.
// [ts] existingChildren.forEach((child) => {
// [ts] return deleteChild(returnFiber, child);
// [ts] });
// [ts] }
// [ts] return resultingFirstChild;
// [ts] }
// [ts] fn reconcileChildrenIterator(returnFiber: auto, currentFirstChild: auto, newChildrenIterable: auto, expirationTime: auto) {
// This is the same implementation as reconcileChildrenArray(),
// but using the iterator instead.
// [ts] let iteratorFn = getIteratorFn(newChildrenIterable);
// [ts] if (!(@typeName(iteratorFn) == "function")) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "An object is not an iterable. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] {
// We don't support rendering Generators because it's a mutation.
// See https://github.com/facebook/react/issues/12995
// [ts] if (@typeName(Symbol) == "function" && // $FlowFixMe Flow doesn't know about toStringTag
// [ts] newChildrenIterable["__symbol_toStringTag"] == "Generator") {
// [ts] if (!didWarnAboutGenerators) {
// [ts] error("Using Generators as children is unsupported and will likely yield " + "unexpected results because enumerating a generator mutates it. " + "You may convert it to an array with `Array.from()` or the " + "`[...spread]` operator before rendering. Keep in mind " + "you might need to polyfill these features for older browsers.");
// [ts] }
// [ts] didWarnAboutGenerators = true;
// [ts] } // Warn about using Maps as children
// [ts] if (newChildrenIterable.entries == iteratorFn) {
// [ts] if (!didWarnAboutMaps) {
// [ts] error("Using Maps as children is unsupported and will likely yield " + "unexpected results. Convert it to a sequence/iterable of keyed " + "ReactElements instead.");
// [ts] }
// [ts] didWarnAboutMaps = true;
// [ts] } // First, validate keys.
// We'll get a different iterator later for the main pass.
// [ts] let _newChildren = iteratorFn.call(newChildrenIterable);
// [ts] if (_newChildren) {
// [ts] let knownKeys = null;
// [ts] let _step = _newChildren.next();
// [ts] while (!_step.done) { // [cap-migrate] for-step: _step = _newChildren.next()
// [ts] let child = _step.value;
// [ts] knownKeys = warnOnInvalidKey(child, knownKeys);
// [ts] }
// [ts] }
// [ts] }
// [ts] let newChildren = iteratorFn.call(newChildrenIterable);
// [ts] if (!(newChildren != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "An iterable object provided no iterator." );)
// [ts] }
// [ts] }
// [ts] let resultingFirstChild = null;
// [ts] let previousNewFiber = null;
// [ts] let oldFiber = currentFirstChild;
// [ts] let lastPlacedIndex = 0;
// [ts] let newIdx = 0;
// [ts] let nextOldFiber = null;
// [ts] let step = newChildren.next();
// [ts] while (oldFiber != null && !step.done) { // [cap-migrate] for-step: newIdx++, step = newChildren.next()
// [ts] if (oldFiber.index > newIdx) {
// [ts] nextOldFiber = oldFiber;
// [ts] oldFiber = null;
// [ts] } else {
// [ts] nextOldFiber = oldFiber.sibling;
// [ts] }
// [ts] let newFiber = updateSlot(returnFiber, oldFiber, step.value, expirationTime);
// [ts] if (newFiber == null) {
// TODO: This breaks on empty slots like null children. That's
// unfortunate because it triggers the slow path all the time. We need
// a better way to communicate whether this was a miss or null,
// boolean, undefined, etc.
// [ts] if (oldFiber == null) {
// [ts] oldFiber = nextOldFiber;
// [ts] }
// [ts] break;
// [ts] }
// [ts] if (shouldTrackSideEffects) {
// [ts] if (oldFiber && newFiber.alternate == null) {
// We matched the slot, but we didn't reuse the existing fiber, so we
// need to delete the existing child.
// [ts] deleteChild(returnFiber, oldFiber);
// [ts] }
// [ts] }
// [ts] lastPlacedIndex = placeChild(newFiber, lastPlacedIndex, newIdx);
// [ts] if (previousNewFiber == null) {
// TODO: Move out of the loop. This only happens for the first run.
// [ts] resultingFirstChild = newFiber;
// [ts] } else {
// TODO: Defer siblings if we're not at the right index for this slot.
// I.e. if we had null values before, then we want to defer this
// for each null value. However, we also don't want to call updateSlot
// with the previous one.
// [ts] previousNewFiber.sibling = newFiber;
// [ts] }
// [ts] previousNewFiber = newFiber;
// [ts] oldFiber = nextOldFiber;
// [ts] }
// [ts] if (step.done) {
// We've reached the end of the new children. We can delete the rest.
// [ts] deleteRemainingChildren(returnFiber, oldFiber);
// [ts] return resultingFirstChild;
// [ts] }
// [ts] if (oldFiber == null) {
// If we don't have any more existing children we can choose a fast path
// since the rest will all be insertions.
// [ts] while (!step.done) { // [cap-migrate] for-step: newIdx++, step = newChildren.next()
// [ts] let _newFiber3 = createChild(returnFiber, step.value, expirationTime);
// [ts] if (_newFiber3 == null) {
// [ts] continue;
// [ts] }
// [ts] lastPlacedIndex = placeChild(_newFiber3, lastPlacedIndex, newIdx);
// [ts] if (previousNewFiber == null) {
// TODO: Move out of the loop. This only happens for the first run.
// [ts] resultingFirstChild = _newFiber3;
// [ts] } else {
// [ts] previousNewFiber.sibling = _newFiber3;
// [ts] }
// [ts] previousNewFiber = _newFiber3;
// [ts] }
// [ts] return resultingFirstChild;
// [ts] } // Add all children to a key map for quick lookups.
// [ts] let existingChildren = mapRemainingChildren(returnFiber, oldFiber); // Keep scanning and use the map to restore deleted items as moves.
// [ts] while (!step.done) { // [cap-migrate] for-step: newIdx++, step = newChildren.next()
// [ts] let _newFiber4 = updateFromMap(existingChildren, returnFiber, newIdx, step.value, expirationTime);
// [ts] if (_newFiber4 != null) {
// [ts] if (shouldTrackSideEffects) {
// [ts] if (_newFiber4.alternate != null) {
// The new fiber is a work in progress, but if there exists a
// current, that means that we reused the fiber. We need to delete
// it from the child list so that we don't add it to the deletion
// list.
// [ts] existingChildren.delete(if (_newFiber4.key == null) newIdx else _newFiber4.key);
// [ts] }
// [ts] }
// [ts] lastPlacedIndex = placeChild(_newFiber4, lastPlacedIndex, newIdx);
// [ts] if (previousNewFiber == null) {
// [ts] resultingFirstChild = _newFiber4;
// [ts] } else {
// [ts] previousNewFiber.sibling = _newFiber4;
// [ts] }
// [ts] previousNewFiber = _newFiber4;
// [ts] }
// [ts] }
// [ts] if (shouldTrackSideEffects) {
// Any existing children that weren't consumed above were deleted. We need
// to add them to the deletion list.
// [ts] existingChildren.forEach((child) => {
// [ts] return deleteChild(returnFiber, child);
// [ts] });
// [ts] }
// [ts] return resultingFirstChild;
// [ts] }
// [ts] fn reconcileSingleTextNode(returnFiber: auto, currentFirstChild: auto, textContent: auto, expirationTime: auto) {
// There's no need to check for keys on text nodes since we don't have a
// way to define them.
// [ts] if (currentFirstChild != null && currentFirstChild.tag == HostText) {
// We already have an existing node so let's just update it and delete
// the rest.
// [ts] deleteRemainingChildren(returnFiber, currentFirstChild.sibling);
// [ts] let existing = useFiber(currentFirstChild, textContent);
// [ts] existing.return = returnFiber;
// [ts] return existing;
// [ts] } // The existing first child is not a text node so we need to create one
// and delete the existing ones.
// [ts] deleteRemainingChildren(returnFiber, currentFirstChild);
// [ts] let created = createFiberFromText(textContent, returnFiber.mode, expirationTime);
// [ts] created.return = returnFiber;
// [ts] return created;
// [ts] }
// [ts] fn reconcileSingleElement(returnFiber: auto, currentFirstChild: auto, element: auto, expirationTime: auto) {
// [ts] let key = element.key;
// [ts] let child = currentFirstChild;
// [ts] while (child != null) {
// TODO: If key === null and child.key === null, then this only applies to
// the first item in the list.
// [ts] if (child.key == key) {
// [ts] switch (child.tag) {
// [ts] Fragment => {
// [ts] {
// [ts] if (element.type == REACT_FRAGMENT_TYPE) {
// [ts] deleteRemainingChildren(returnFiber, child.sibling);
// [ts] let existing = useFiber(child, element.props.children);
// [ts] existing.return = returnFiber;
// [ts] {
// [ts] existing._debugSource = element._source;
// [ts] existing._debugOwner = element._owner;
// [ts] }
// [ts] return existing;
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// We intentionally fallthrough here if enableBlocksAPI is not on.
// eslint-disable-next-lined no-fallthrough
// [ts] else => {
// [ts] {
// [ts] if (child.elementType == element.type || ( // Keep this check inline so it only runs on the false path:
// [ts] isCompatibleFamilyForHotReloading: (child, element) => {
// [ts] deleteRemainingChildren(returnFiber, child.sibling);
// [ts] let _existing3 = useFiber(child, element.props);
// [ts] _existing3.ref = coerceRef(returnFiber, child, element);
// [ts] _existing3.return = returnFiber;
// [ts] {
// [ts] _existing3._debugSource = element._source;
// [ts] _existing3._debugOwner = element._owner;
// [ts] }
// [ts] return _existing3;
// [ts] }
// [ts] break;
// [ts] }
// [ts] } // Didn't match.
// [ts] deleteRemainingChildren(returnFiber, child);
// [ts] break;
// [ts] } else {
// [ts] deleteChild(returnFiber, child);
// [ts] }
// [ts] child = child.sibling;
// [ts] }
// [ts] if (element.type == REACT_FRAGMENT_TYPE) {
// [ts] let created = createFiberFromFragment(element.props.children, returnFiber.mode, expirationTime, element.key);
// [ts] created.return = returnFiber;
// [ts] return created;
// [ts] } else {
// [ts] let _created4 = createFiberFromElement(element, returnFiber.mode, expirationTime);
// [ts] _created4.ref = coerceRef(returnFiber, currentFirstChild, element);
// [ts] _created4.return = returnFiber;
// [ts] return _created4;
// [ts] }
// [ts] }
// [ts] fn reconcileSinglePortal(returnFiber: auto, currentFirstChild: auto, portal: auto, expirationTime: auto) {
// [ts] let key = portal.key;
// [ts] let child = currentFirstChild;
// [ts] while (child != null) {
// TODO: If key === null and child.key === null, then this only applies to
// the first item in the list.
// [ts] if (child.key == key) {
// [ts] if (child.tag == HostPortal && child.stateNode.containerInfo == portal.containerInfo && child.stateNode.implementation == portal.implementation) {
// [ts] deleteRemainingChildren(returnFiber, child.sibling);
// [ts] let existing = useFiber(child, portal.children || []);
// [ts] existing.return = returnFiber;
// [ts] return existing;
// [ts] } else {
// [ts] deleteRemainingChildren(returnFiber, child);
// break;
// [ts] }
// [ts] }
// [ts] } else {
// [ts] deleteChild(returnFiber, child);
// [ts] }
// [ts] child = child.sibling;
// [ts] }
// [ts] let created = createFiberFromPortal(portal, returnFiber.mode, expirationTime);
// [ts] created.return = returnFiber;
// [ts] return created;
// [ts] } // This API will tag the children with the side-effect of the reconciliation
// itself. They will be added to the side-effect list as we pass through the
// children and the parent.
// [ts] fn reconcileChildFibers(returnFiber: auto, currentFirstChild: auto, newChild: auto, expirationTime: auto) {
// This function is not recursive.
// If the top level item is an array, we treat it as a set of children,
// not as a fragment. Nested arrays on the other hand will be treated as
// fragment nodes. Recursion happens at the normal flow.
// Handle top level unkeyed fragments as if they were arrays.
// This leads to an ambiguity between <>{[...]}</> and <>...</>.
// We treat the ambiguous cases above the same.
// [ts] let isUnkeyedTopLevelFragment = @typeName(newChild) == "object" && newChild != null && newChild.type == REACT_FRAGMENT_TYPE && newChild.key == null;
// [ts] if (isUnkeyedTopLevelFragment) {
// [ts] newChild = newChild.props.children;
// [ts] } // Handle object types
// [ts] let isObject = @typeName(newChild) == "object" && newChild != null;
// [ts] if (isObject) {
// [ts] switch (newChild.$$typeof) {
// [ts] REACT_ELEMENT_TYPE => {
// [ts] return placeSingleChild(reconcileSingleElement(returnFiber, currentFirstChild, newChild, expirationTime));
// [ts] },
// [ts] REACT_PORTAL_TYPE => {
// [ts] return placeSingleChild(reconcileSinglePortal(returnFiber, currentFirstChild, newChild, expirationTime));
// [ts] }
// [ts] }
// [ts] }
// [ts] if (@typeName(newChild) == "string" || @typeName(newChild) == "number") {
// [ts] return placeSingleChild(reconcileSingleTextNode(returnFiber, currentFirstChild, "" + newChild, expirationTime));
// [ts] }
// [ts] if (isArray$1(newChild)) {
// [ts] return reconcileChildrenArray(returnFiber, currentFirstChild, newChild, expirationTime);
// [ts] }
// [ts] if (getIteratorFn(newChild)) {
// [ts] return reconcileChildrenIterator(returnFiber, currentFirstChild, newChild, expirationTime);
// [ts] }
// [ts] if (isObject) {
// [ts] throwOnInvalidObjectType(returnFiber, newChild);
// [ts] }
// [ts] {
// [ts] if (@typeName(newChild) == "function") {
// [ts] warnOnFunctionType();
// [ts] }
// [ts] }
// [ts] if (@typeName(newChild) == "undefined" && !isUnkeyedTopLevelFragment) {
// If the new child is undefined, and the return fiber is a composite
// component, throw an error. If Fiber return types are disabled,
// we already threw above.
// [ts] switch (returnFiber.tag) {
// [ts] ClassComponent => {
// [ts] {
// [ts] {
// [ts] let instance = returnFiber.stateNode;
// [ts] if (instance.render._isMockFunction) {
// We allow auto-mocks to proceed as if they're returning null.
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// Intentionally fall through to the next case, which handles both
// functions and classes
// eslint-disable-next-lined no-fallthrough
// [ts] },
// [ts] FunctionComponent => {
// [ts] {
// [ts] let Component = returnFiber.type;
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( (Component.displayName || Component.name || 'Component') + "(...): Nothing was returned from render. This usually means a return statement is missing. Or, to render nothing, return null." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] } // Remaining cases are all treated as empty.
// [ts] return deleteRemainingChildren(returnFiber, currentFirstChild);
// [ts] }
// [ts] return reconcileChildFibers;
// [ts] }
// [ts] let reconcileChildFibers = ChildReconciler(true);
// [ts] let mountChildFibers = ChildReconciler(false);
// [ts] fn cloneChildFibers(current: auto, workInProgress: auto) {
// [ts] if (!(current == null || workInProgress.child == current.child)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Resuming work not yet implemented." );)
// [ts] }
// [ts] }
// [ts] if (workInProgress.child == null) {
// [ts] return;
// [ts] }
// [ts] let currentChild = workInProgress.child;
// [ts] let newChild = createWorkInProgress(currentChild, currentChild.pendingProps);
// [ts] workInProgress.child = newChild;
// [ts] newChild.return = workInProgress;
// [ts] while (currentChild.sibling != null) {
// [ts] currentChild = currentChild.sibling;
// [ts] newChild = newChild.sibling = createWorkInProgress(currentChild, currentChild.pendingProps);
// [ts] newChild.return = workInProgress;
// [ts] }
// [ts] newChild.sibling = null;
// [ts] } // Reset a workInProgress child set to prepare it for a second pass.
// [ts] fn resetChildFibers(workInProgress: auto, renderExpirationTime: auto) {
// [ts] let child = workInProgress.child;
// [ts] while (child != null) {
// [ts] resetWorkInProgress(child, renderExpirationTime);
// [ts] child = child.sibling;
// [ts] }
// [ts] }
// [ts] let NO_CONTEXT = {};
// [ts] let contextStackCursor$1 = createCursor(NO_CONTEXT);
// [ts] let contextFiberStackCursor = createCursor(NO_CONTEXT);
// [ts] let rootInstanceStackCursor = createCursor(NO_CONTEXT);
// [ts] fn requiredContext(c: auto) {
// [ts] if (!(c != NO_CONTEXT)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected host context to exist. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] return c;
// [ts] }
// [ts] fn getRootHostContainer() {
// [ts] let rootInstance = requiredContext(rootInstanceStackCursor.current);
// [ts] return rootInstance;
// [ts] }
// [ts] fn pushHostContainer(fiber: auto, nextRootInstance: auto) {
// Push current root instance onto the stack;
// This allows us to reset root when portals are popped.
// [ts] push(rootInstanceStackCursor, nextRootInstance, fiber); // Track the context and the Fiber that provided it.
// This enables us to pop only Fibers that provide unique contexts.
// [ts] push(contextFiberStackCursor, fiber, fiber); // Finally, we need to push the host context to the stack.
// However, we can't just call getRootHostContext() and push it because
// we'd have a different number of entries on the stack depending on
// whether getRootHostContext() throws somewhere in renderer code or not.
// So we push an empty value first. This lets us safely unwind on errors.
// [ts] push(contextStackCursor$1, NO_CONTEXT, fiber);
// [ts] let nextRootContext = getRootHostContext(nextRootInstance); // Now that we know this function doesn't throw, replace it.
// [ts] pop(contextStackCursor$1, fiber);
// [ts] push(contextStackCursor$1, nextRootContext, fiber);
// [ts] }
// [ts] fn popHostContainer(fiber: auto) {
// [ts] pop(contextStackCursor$1, fiber);
// [ts] pop(contextFiberStackCursor, fiber);
// [ts] pop(rootInstanceStackCursor, fiber);
// [ts] }
// [ts] fn getHostContext() {
// [ts] let context = requiredContext(contextStackCursor$1.current);
// [ts] return context;
// [ts] }
// [ts] fn pushHostContext(fiber: auto) {
// [ts] let rootInstance = requiredContext(rootInstanceStackCursor.current);
// [ts] let context = requiredContext(contextStackCursor$1.current);
// [ts] let nextContext = getChildHostContext(context, fiber.type); // Don't push this Fiber's context unless it's unique.
// [ts] if (context == nextContext) {
// [ts] return;
// [ts] } // Track the context and the Fiber that provided it.
// This enables us to pop only Fibers that provide unique contexts.
// [ts] push(contextFiberStackCursor, fiber, fiber);
// [ts] push(contextStackCursor$1, nextContext, fiber);
// [ts] }
// [ts] fn popHostContext(fiber: auto) {
// Do not pop unless this Fiber provided the current context.
// pushHostContext() only pushes Fibers that provide unique contexts.
// [ts] if (contextFiberStackCursor.current != fiber) {
// [ts] return;
// [ts] }
// [ts] pop(contextStackCursor$1, fiber);
// [ts] pop(contextFiberStackCursor, fiber);
// [ts] }
// [ts] let DefaultSuspenseContext = 0; // The Suspense Context is split into two parts. The lower bits is
// inherited deeply down the subtree. The upper bits only affect
// this immediate suspense boundary and gets reset each new
// boundary or suspense list.
// [ts] let SubtreeSuspenseContextMask = 1; // Subtree Flags:
// InvisibleParentSuspenseContext indicates that one of our parent Suspense
// boundaries is not currently showing visible main content.
// Either because it is already showing a fallback or is not mounted at all.
// We can use this to determine if it is desirable to trigger a fallback at
// the parent. If not, then we might need to trigger undesirable boundaries
// and/or suspend the commit to avoid hiding the parent content.
// [ts] let InvisibleParentSuspenseContext = 1; // Shallow Flags:
// ForceSuspenseFallback can be used by SuspenseList to force newly added
// items into their fallback state during one of the render passes.
// [ts] let ForceSuspenseFallback = 2;
// [ts] let suspenseStackCursor = createCursor(DefaultSuspenseContext);
// [ts] fn hasSuspenseContext(parentContext: auto, flag: auto) {
// [ts] return (parentContext & flag) != 0;
// [ts] }
// [ts] fn setDefaultShallowSuspenseContext(parentContext: auto) {
// [ts] return parentContext & SubtreeSuspenseContextMask;
// [ts] }
// [ts] fn setShallowSuspenseContext(parentContext: auto, shallowContext: auto) {
// [ts] return parentContext & SubtreeSuspenseContextMask | shallowContext;
// [ts] }
// [ts] fn addSubtreeSuspenseContext(parentContext: auto, subtreeContext: auto) {
// [ts] return parentContext | subtreeContext;
// [ts] }
// [ts] fn pushSuspenseContext(fiber: auto, newContext: auto) {
// [ts] push(suspenseStackCursor, newContext, fiber);
// [ts] }
// [ts] fn popSuspenseContext(fiber: auto) {
// [ts] pop(suspenseStackCursor, fiber);
// [ts] }
// [ts] fn shouldCaptureSuspense(workInProgress: auto, hasInvisibleParent: auto) {
// If it was the primary children that just suspended, capture and render the
// fallback. Otherwise, don't capture and bubble to the next boundary.
// [ts] let nextState = workInProgress.memoizedState;
// [ts] if (nextState != null) {
// [ts] if (nextState.dehydrated != null) {
// A dehydrated boundary always captures.
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] let props = workInProgress.memoizedProps; // In order to capture, the Suspense component must have a fallback prop.
// [ts] if (props.fallback == null) {
// [ts] return false;
// [ts] } // Regular boundaries always capture.
// [ts] if (props.unstable_avoidThisFallback != true) {
// [ts] return true;
// [ts] } // If it's a boundary we should avoid, then we prefer to bubble up to the
// parent boundary if it is currently invisible.
// [ts] if (hasInvisibleParent) {
// [ts] return false;
// [ts] } // If the parent is not able to handle it, we must handle it.
// [ts] return true;
// [ts] }
// [ts] fn findFirstSuspended(row: auto) {
// [ts] let node = row;
// [ts] while (node != null) {
// [ts] if (node.tag == SuspenseComponent) {
// [ts] let state = node.memoizedState;
// [ts] if (state != null) {
// [ts] let dehydrated = state.dehydrated;
// [ts] if (dehydrated == null || isSuspenseInstancePending(dehydrated) || isSuspenseInstanceFallback(dehydrated)) {
// [ts] return node;
// [ts] }
// [ts] }
// [ts] } else if (node.tag == SuspenseListComponent && // revealOrder undefined can't be trusted because it don't
// keep track of whether it suspended or not.
// [ts] node.memoizedProps.revealOrder != null) {
// [ts] let didSuspend = (node.effectTag & DidCapture) != NoEffect;
// [ts] if (didSuspend) {
// [ts] return node;
// [ts] }
// [ts] } else if (node.child != null) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] if (node == row) {
// [ts] return null;
// [ts] }
// [ts] while (node.sibling == null) {
// [ts] if (node.return == null || node.return == row) {
// [ts] return null;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn createDeprecatedResponderListener(responder: auto, props: auto) {
// [ts] let eventResponderListener = {
// [ts] responder: responder,
// [ts] props: props
// [ts] };
// [ts] {
// [ts] Object.freeze(eventResponderListener);
// [ts] }
// [ts] return eventResponderListener;
// [ts] }
// [ts] let HasEffect =
// [ts] /* */
// [ts] 1; // Represents the phase in which the effect (not the clean-up) fires.
// [ts] let Layout =
// [ts] /*    */
// [ts] 2;
// [ts] let Passive$1 =
// [ts] /*   */
// [ts] 4;
// [ts] let ReactCurrentDispatcher = ReactSharedInternals.ReactCurrentDispatcher,
// [ts] ReactCurrentBatchConfig$1 = ReactSharedInternals.ReactCurrentBatchConfig;
// [ts] let didWarnAboutMismatchedHooksForComponent;
// [ts] {
// [ts] didWarnAboutMismatchedHooksForComponent = Set.init();
// [ts] }
// These are set right before calling the component.
// [ts] let renderExpirationTime = NoWork; // The work-in-progress fiber. I've named it differently to distinguish it from
// the work-in-progress hook.
// [ts] let currentlyRenderingFiber$1 = null; // Hooks are stored as a linked list on the fiber's memoizedState field. The
// current hook list is the list that belongs to the current fiber. The
// work-in-progress hook list is a new list that will be added to the
// work-in-progress fiber.
// [ts] let currentHook = null;
// [ts] let workInProgressHook = null; // Whether an update was scheduled at any point during the render phase. This
// does not get reset if we do another render pass; only when we're completely
// finished evaluating this component. This is an optimization so we know
// whether we need to clear render phase updates after a throw.
// [ts] let didScheduleRenderPhaseUpdate = false;
// [ts] let RE_RENDER_LIMIT = 25; // In DEV, this is the name of the currently executing primitive hook
// [ts] let currentHookNameInDev = null; // In DEV, this list ensures that hooks are called in the same order between renders.
// The list stores the order of hooks used during the initial render (mount).
// Subsequent renders (updates) reference this list.
// [ts] let hookTypesDev = null;
// [ts] let hookTypesUpdateIndexDev = -1; // In DEV, this tracks whether currently rendering component needs to ignore
// the dependencies for Hooks that need them (e.g. useEffect or useMemo).
// When true, such Hooks will always be "remounted". Only used during hot reload.
// [ts] let ignorePreviousDependencies = false;
// [ts] fn mountHookTypesDev() {
// [ts] {
// [ts] let hookName = currentHookNameInDev;
// [ts] if (hookTypesDev == null) {
// [ts] hookTypesDev = [hookName];
// [ts] } else {
// [ts] hookTypesDev.push(hookName);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn updateHookTypesDev() {
// [ts] {
// [ts] let hookName = currentHookNameInDev;
// [ts] if (hookTypesDev != null) {
// [ts] hookTypesUpdateIndexDev++;
// [ts] if (hookTypesDev[hookTypesUpdateIndexDev] != hookName) {
// [ts] warnOnHookMismatchInDev(hookName);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn checkDepsAreArrayDev(deps: auto) {
// [ts] {
// [ts] if (deps != null && deps != null && !Array.isArray(deps)) {
// Verify deps, but only on mount to avoid extra checks.
// It's unlikely their type would change as usually you define them inline.
// [ts] error("%s received a final argument that is not an array (instead, received `%s`). When " + "specified, the final argument must be an array.", currentHookNameInDev, @typeName(deps));
// [ts] }
// [ts] }
// [ts] }
// [ts] fn warnOnHookMismatchInDev(currentHookName: auto) {
// [ts] {
// [ts] let componentName = getComponentName(currentlyRenderingFiber$1.type);
// [ts] if (!didWarnAboutMismatchedHooksForComponent.has(componentName)) {
// [ts] didWarnAboutMismatchedHooksForComponent.add(componentName);
// [ts] if (hookTypesDev != null) {
// [ts] let table = "";
// [ts] let secondColumnStart = 30;
// [ts] let i = 0;
// [ts] while (i <= hookTypesUpdateIndexDev) { // [cap-migrate] for-step: i++
// [ts] let oldHookName = hookTypesDev[i];
// [ts] let newHookName = if (i == hookTypesUpdateIndexDev) currentHookName else oldHookName;
// [ts] let row = i + 1 + ". " + oldHookName; // Extra space so second column lines up
// lol @ IE not supporting String#repeat
// [ts] while (row.length < secondColumnStart) {
// [ts] row += " ";
// [ts] }
// [ts] row += newHookName + "\n";
// [ts] table += row;
// [ts] }
// [ts] error("React has detected a change in the order of Hooks called by %s. " + "This will lead to bugs and errors if not fixed. " + "For more information, read the Rules of Hooks: https://fb.me/rules-of-hooks\n\n" + "   Previous render            Next render\n" + "   ------------------------------------------------------\n" + "%s" + "   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", componentName, table);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn throwInvalidHookError() {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Invalid hook call. Hooks can only be called inside of the body of a function component. This could happen for one of the following reasons:\n1. You might have mismatching versions of React and the renderer (such as React DOM)\n2. You might be breaking the Rules of Hooks\n3. You might have more than one copy of React in the same app\nSee https://fb.me/react-invalid-hook-call for tips about how to debug and fix this problem." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn areHookInputsEqual(nextDeps: auto, prevDeps: auto) {
// [ts] {
// [ts] if (ignorePreviousDependencies) {
// Only true when this component is being hot reloaded.
// [ts] return false;
// [ts] }
// [ts] }
// [ts] if (prevDeps == null) {
// [ts] {
// [ts] error("%s received a final argument during this render, but not during " + "the previous render. Even though the final argument is optional, " + "its type cannot change between renders.", currentHookNameInDev);
// [ts] }
// [ts] return false;
// [ts] }
// [ts] {
// Don't bother comparing lengths in prod because these arrays should be
// passed inline.
// [ts] if (nextDeps.length != prevDeps.length) {
// [ts] error("The final argument passed to %s changed size between renders. The " + "order and size of this array must remain constant.\n\n" + "Previous: %s\n" + "Incoming: %s", currentHookNameInDev, "[" + prevDeps.join(", ") + "]", "[" + nextDeps.join(", ") + "]");
// [ts] }
// [ts] }
// [ts] let i = 0;
// [ts] while (i < prevDeps.length && i < nextDeps.length) { // [cap-migrate] for-step: i++
// [ts] if (objectIs(nextDeps[i], prevDeps[i])) {
// [ts] continue;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn renderWithHooks(current: auto, workInProgress: auto, Component: auto, props: auto, secondArg: auto, nextRenderExpirationTime: auto) {
// [ts] renderExpirationTime = nextRenderExpirationTime;
// [ts] currentlyRenderingFiber$1 = workInProgress;
// [ts] {
// [ts] hookTypesDev = if (current != null) current._debugHookTypes else null;
// [ts] hookTypesUpdateIndexDev = -1; // Used for hot reloading:
// [ts] ignorePreviousDependencies = current != null && current.type != workInProgress.type;
// [ts] }
// [ts] workInProgress.memoizedState = null;
// [ts] workInProgress.updateQueue = null;
// [ts] workInProgress.expirationTime = NoWork; // The following should have already been reset
// currentHook = null;
// workInProgressHook = null;
// didScheduleRenderPhaseUpdate = false;
// TODO Warn if no hooks are used at all during mount, then some are used during update.
// Currently we will identify the update render as a mount because memoizedState === null.
// This is tricky because it's valid for certain types of components (e.g. React.lazy)
// Using memoizedState to differentiate between mount/update only works if at least one stateful hook is used.
// Non-stateful hooks (e.g. context) don't get added to memoizedState,
// so memoizedState would be null during updates and mounts.
// [ts] {
// [ts] if (current != null && current.memoizedState != null) {
// [ts] ReactCurrentDispatcher.current = HooksDispatcherOnUpdateInDEV;
// [ts] } else if (hookTypesDev != null) {
// This dispatcher handles an edge case where a component is updating,
// but no stateful hooks have been used.
// We want to match the production code behavior (which will use HooksDispatcherOnMount),
// but with the extra DEV validation to ensure hooks ordering hasn't changed.
// This dispatcher does that.
// [ts] ReactCurrentDispatcher.current = HooksDispatcherOnMountWithHookTypesInDEV;
// [ts] } else {
// [ts] ReactCurrentDispatcher.current = HooksDispatcherOnMountInDEV;
// [ts] }
// [ts] }
// [ts] let children = Component(props, secondArg); // Check if there was a render phase update
// [ts] if (workInProgress.expirationTime == renderExpirationTime) {
// Keep rendering in a loop for as long as render phase updates continue to
// be scheduled. Use a counter to prevent infinite loops.
// [ts] let numberOfReRenders = 0;
// [ts] do {
// [ts] workInProgress.expirationTime = NoWork;
// [ts] if (!(numberOfReRenders < RE_RENDER_LIMIT)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Too many re-renders. React limits the number of renders to prevent an infinite loop." );)
// [ts] }
// [ts] }
// [ts] numberOfReRenders += 1;
// [ts] {
// Even when hot reloading, allow dependencies to stabilize
// after first render to prevent infinite render phase updates.
// [ts] ignorePreviousDependencies = false;
// [ts] } // Start over from the beginning of the list
// [ts] currentHook = null;
// [ts] workInProgressHook = null;
// [ts] workInProgress.updateQueue = null;
// [ts] {
// Also validate hook order for cascading updates.
// [ts] hookTypesUpdateIndexDev = -1;
// [ts] }
// [ts] ReactCurrentDispatcher.current =  HooksDispatcherOnRerenderInDEV ;
// [ts] children = Component(props, secondArg);
// [ts] } while (workInProgress.expirationTime == renderExpirationTime);
// [ts] } // We can assume the previous dispatcher is always this one, since we set it
// at the beginning of the render phase and there's no re-entrancy.
// [ts] ReactCurrentDispatcher.current = ContextOnlyDispatcher;
// [ts] {
// [ts] workInProgress._debugHookTypes = hookTypesDev;
// [ts] } // This check uses currentHook so that it works the same in DEV and prod bundles.
// hookTypesDev could catch more cases (e.g. context) but only in DEV bundles.
// [ts] let didRenderTooFewHooks = currentHook != null && currentHook.next != null;
// [ts] renderExpirationTime = NoWork;
// [ts] currentlyRenderingFiber$1 = null;
// [ts] currentHook = null;
// [ts] workInProgressHook = null;
// [ts] {
// [ts] currentHookNameInDev = null;
// [ts] hookTypesDev = null;
// [ts] hookTypesUpdateIndexDev = -1;
// [ts] }
// [ts] didScheduleRenderPhaseUpdate = false;
// [ts] if (!!didRenderTooFewHooks) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Rendered fewer hooks than expected. This may be caused by an accidental early return statement." );)
// [ts] }
// [ts] }
// [ts] return children;
// [ts] }
// [ts] fn bailoutHooks(current: auto, workInProgress: auto, expirationTime: auto) {
// [ts] workInProgress.updateQueue = current.updateQueue;
// [ts] workInProgress.effectTag &= ~(Passive | Update);
// [ts] if (current.expirationTime <= expirationTime) {
// [ts] current.expirationTime = NoWork;
// [ts] }
// [ts] }
// [ts] fn resetHooksAfterThrow() {
// We can assume the previous dispatcher is always this one, since we set it
// at the beginning of the render phase and there's no re-entrancy.
// [ts] ReactCurrentDispatcher.current = ContextOnlyDispatcher;
// [ts] if (didScheduleRenderPhaseUpdate) {
// There were render phase updates. These are only valid for this render
// phase, which we are now aborting. Remove the updates from the queues so
// they do not persist to the next render. Do not remove updates from hooks
// that weren't processed.
//
// Only reset the updates from the queue if it has a clone. If it does
// not have a clone, that means it wasn't processed, and the updates were
// scheduled before we entered the render phase.
// [ts] let hook = currentlyRenderingFiber$1.memoizedState;
// [ts] while (hook != null) {
// [ts] let queue = hook.queue;
// [ts] if (queue != null) {
// [ts] queue.pending = null;
// [ts] }
// [ts] hook = hook.next;
// [ts] }
// [ts] }
// [ts] renderExpirationTime = NoWork;
// [ts] currentlyRenderingFiber$1 = null;
// [ts] currentHook = null;
// [ts] workInProgressHook = null;
// [ts] {
// [ts] hookTypesDev = null;
// [ts] hookTypesUpdateIndexDev = -1;
// [ts] currentHookNameInDev = null;
// [ts] }
// [ts] didScheduleRenderPhaseUpdate = false;
// [ts] }
// [ts] fn mountWorkInProgressHook() {
// [ts] let hook = {
// [ts] memoizedState: null,
// [ts] baseState: null,
// [ts] baseQueue: null,
// [ts] queue: null,
// [ts] next: null
// [ts] };
// [ts] if (workInProgressHook == null) {
// This is the first hook in the list
// [ts] currentlyRenderingFiber$1.memoizedState = workInProgressHook = hook;
// [ts] } else {
// Append to the end of the list
// [ts] workInProgressHook = workInProgressHook.next = hook;
// [ts] }
// [ts] return workInProgressHook;
// [ts] }
// [ts] fn updateWorkInProgressHook() {
// This function is used both for updates and for re-renders triggered by a
// render phase update. It assumes there is either a current hook we can
// clone, or a work-in-progress hook from a previous render pass that we can
// use as a base. When we reach the end of the base list, we must switch to
// the dispatcher used for mounts.
// [ts] let nextCurrentHook;
// [ts] if (currentHook == null) {
// [ts] let current = currentlyRenderingFiber$1.alternate;
// [ts] if (current != null) {
// [ts] nextCurrentHook = current.memoizedState;
// [ts] } else {
// [ts] nextCurrentHook = null;
// [ts] }
// [ts] } else {
// [ts] nextCurrentHook = currentHook.next;
// [ts] }
// [ts] let nextWorkInProgressHook;
// [ts] if (workInProgressHook == null) {
// [ts] nextWorkInProgressHook = currentlyRenderingFiber$1.memoizedState;
// [ts] } else {
// [ts] nextWorkInProgressHook = workInProgressHook.next;
// [ts] }
// [ts] if (nextWorkInProgressHook != null) {
// There's already a work-in-progress. Reuse it.
// [ts] workInProgressHook = nextWorkInProgressHook;
// [ts] nextWorkInProgressHook = workInProgressHook.next;
// [ts] currentHook = nextCurrentHook;
// [ts] } else {
// Clone from the current hook.
// [ts] if (!(nextCurrentHook != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Rendered more hooks than during the previous render." );)
// [ts] }
// [ts] }
// [ts] currentHook = nextCurrentHook;
// [ts] let newHook = {
// [ts] memoizedState: currentHook.memoizedState,
// [ts] baseState: currentHook.baseState,
// [ts] baseQueue: currentHook.baseQueue,
// [ts] queue: currentHook.queue,
// [ts] next: null
// [ts] };
// [ts] if (workInProgressHook == null) {
// This is the first hook in the list.
// [ts] currentlyRenderingFiber$1.memoizedState = workInProgressHook = newHook;
// [ts] } else {
// Append to the end of the list.
// [ts] workInProgressHook = workInProgressHook.next = newHook;
// [ts] }
// [ts] }
// [ts] return workInProgressHook;
// [ts] }
// [ts] fn createFunctionComponentUpdateQueue() {
// [ts] return {
// [ts] lastEffect: null
// [ts] };
// [ts] }
// [ts] fn basicStateReducer(state: auto, action: auto) {
// $FlowFixMe: Flow doesn't like mixed types
// [ts] return if (@typeName(action) == "function") action(state) else action;
// [ts] }
// [ts] fn mountReducer(reducer: auto, initialArg: auto, init: auto) {
// [ts] let hook = mountWorkInProgressHook();
// [ts] let initialState;
// [ts] if (init != null) {
// [ts] initialState = init(initialArg);
// [ts] } else {
// [ts] initialState = initialArg;
// [ts] }
// [ts] hook.memoizedState = hook.baseState = initialState;
// [ts] let queue = hook.queue = {
// [ts] pending: null,
// [ts] dispatch: null,
// [ts] lastRenderedReducer: reducer,
// [ts] lastRenderedState: initialState
// [ts] };
// [ts] let dispatch = queue.dispatch = dispatchAction.bind(null, currentlyRenderingFiber$1, queue);
// [ts] return [hook.memoizedState, dispatch];
// [ts] }
// [ts] fn updateReducer(reducer: auto, initialArg: auto, init: auto) {
// [ts] let hook = updateWorkInProgressHook();
// [ts] let queue = hook.queue;
// [ts] if (!(queue != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should have a queue. This is likely a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] queue.lastRenderedReducer = reducer;
// [ts] let current = currentHook; // The last rebase update that is NOT part of the base state.
// [ts] let baseQueue = current.baseQueue; // The last pending update that hasn't been processed yet.
// [ts] let pendingQueue = queue.pending;
// [ts] if (pendingQueue != null) {
// We have new updates that haven't been processed yet.
// We'll add them to the base queue.
// [ts] if (baseQueue != null) {
// Merge the pending queue and the base queue.
// [ts] let baseFirst = baseQueue.next;
// [ts] let pendingFirst = pendingQueue.next;
// [ts] baseQueue.next = pendingFirst;
// [ts] pendingQueue.next = baseFirst;
// [ts] }
// [ts] current.baseQueue = baseQueue = pendingQueue;
// [ts] queue.pending = null;
// [ts] }
// [ts] if (baseQueue != null) {
// We have a queue to process.
// [ts] let first = baseQueue.next;
// [ts] let newState = current.baseState;
// [ts] let newBaseState = null;
// [ts] let newBaseQueueFirst = null;
// [ts] let newBaseQueueLast = null;
// [ts] let update = first;
// [ts] do {
// [ts] let updateExpirationTime = update.expirationTime;
// [ts] if (updateExpirationTime < renderExpirationTime) {
// Priority is insufficient. Skip this update. If this is the first
// skipped update, the previous update/state is the new base
// update/state.
// [ts] let clone = {
// [ts] expirationTime: update.expirationTime,
// [ts] suspenseConfig: update.suspenseConfig,
// [ts] action: update.action,
// [ts] eagerReducer: update.eagerReducer,
// [ts] eagerState: update.eagerState,
// [ts] next: null
// [ts] };
// [ts] if (newBaseQueueLast == null) {
// [ts] newBaseQueueFirst = newBaseQueueLast = clone;
// [ts] newBaseState = newState;
// [ts] } else {
// [ts] newBaseQueueLast = newBaseQueueLast.next = clone;
// [ts] } // Update the remaining priority in the queue.
// [ts] if (updateExpirationTime > currentlyRenderingFiber$1.expirationTime) {
// [ts] currentlyRenderingFiber$1.expirationTime = updateExpirationTime;
// [ts] markUnprocessedUpdateTime(updateExpirationTime);
// [ts] }
// [ts] } else {
// This update does have sufficient priority.
// [ts] if (newBaseQueueLast != null) {
// [ts] let _clone = {
// [ts] expirationTime: Sync,
// This update is going to be committed so we never want uncommit it.
// [ts] suspenseConfig: update.suspenseConfig,
// [ts] action: update.action,
// [ts] eagerReducer: update.eagerReducer,
// [ts] eagerState: update.eagerState,
// [ts] next: null
// [ts] };
// [ts] newBaseQueueLast = newBaseQueueLast.next = _clone;
// [ts] } // Mark the event time of this update as relevant to this render pass.
// TODO: This should ideally use the true event time of this update rather than
// its priority which is a derived and not reverseable value.
// TODO: We should skip this update if it was already committed but currently
// we have no way of detecting the difference between a committed and suspended
// update here.
// [ts] markRenderEventTimeAndConfig(updateExpirationTime, update.suspenseConfig); // Process this update.
// [ts] if (update.eagerReducer == reducer) {
// If this update was processed eagerly, and its reducer matches the
// current reducer, we can use the eagerly computed state.
// [ts] newState = update.eagerState;
// [ts] } else {
// [ts] let action = update.action;
// [ts] newState = reducer(newState, action);
// [ts] }
// [ts] }
// [ts] update = update.next;
// [ts] } while (update != null && update != first);
// [ts] if (newBaseQueueLast == null) {
// [ts] newBaseState = newState;
// [ts] } else {
// [ts] newBaseQueueLast.next = newBaseQueueFirst;
// [ts] } // Mark that the fiber performed work, but only if the new state is
// different from the current state.
// [ts] if (!objectIs(newState, hook.memoizedState)) {
// [ts] markWorkInProgressReceivedUpdate();
// [ts] }
// [ts] hook.memoizedState = newState;
// [ts] hook.baseState = newBaseState;
// [ts] hook.baseQueue = newBaseQueueLast;
// [ts] queue.lastRenderedState = newState;
// [ts] }
// [ts] let dispatch = queue.dispatch;
// [ts] return [hook.memoizedState, dispatch];
// [ts] }
// [ts] fn rerenderReducer(reducer: auto, initialArg: auto, init: auto) {
// [ts] let hook = updateWorkInProgressHook();
// [ts] let queue = hook.queue;
// [ts] if (!(queue != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should have a queue. This is likely a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] queue.lastRenderedReducer = reducer; // This is a re-render. Apply the new render phase updates to the previous
// work-in-progress hook.
// [ts] let dispatch = queue.dispatch;
// [ts] let lastRenderPhaseUpdate = queue.pending;
// [ts] let newState = hook.memoizedState;
// [ts] if (lastRenderPhaseUpdate != null) {
// The queue doesn't persist past this render pass.
// [ts] queue.pending = null;
// [ts] let firstRenderPhaseUpdate = lastRenderPhaseUpdate.next;
// [ts] let update = firstRenderPhaseUpdate;
// [ts] do {
// Process this render phase update. We don't have to check the
// priority because it will always be the same as the current
// render's.
// [ts] let action = update.action;
// [ts] newState = reducer(newState, action);
// [ts] update = update.next;
// [ts] } while (update != firstRenderPhaseUpdate); // Mark that the fiber performed work, but only if the new state is
// different from the current state.
// [ts] if (!objectIs(newState, hook.memoizedState)) {
// [ts] markWorkInProgressReceivedUpdate();
// [ts] }
// [ts] hook.memoizedState = newState; // Don't persist the state accumulated from the render phase updates to
// the base state unless the queue is empty.
// TODO: Not sure if this is the desired semantics, but it's what we
// do for gDSFP. I can't remember why.
// [ts] if (hook.baseQueue == null) {
// [ts] hook.baseState = newState;
// [ts] }
// [ts] queue.lastRenderedState = newState;
// [ts] }
// [ts] return [newState, dispatch];
// [ts] }
// [ts] fn mountState(initialState: auto) {
// [ts] let hook = mountWorkInProgressHook();
// [ts] if (@typeName(initialState) == "function") {
// $FlowFixMe: Flow doesn't like mixed types
// [ts] initialState = initialState();
// [ts] }
// [ts] hook.memoizedState = hook.baseState = initialState;
// [ts] let queue = hook.queue = {
// [ts] pending: null,
// [ts] dispatch: null,
// [ts] lastRenderedReducer: basicStateReducer,
// [ts] lastRenderedState: initialState
// [ts] };
// [ts] let dispatch = queue.dispatch = dispatchAction.bind(null, currentlyRenderingFiber$1, queue);
// [ts] return [hook.memoizedState, dispatch];
// [ts] }
// [ts] fn updateState(initialState: auto) {
// [ts] return updateReducer(basicStateReducer);
// [ts] }
// [ts] fn rerenderState(initialState: auto) {
// [ts] return rerenderReducer(basicStateReducer);
// [ts] }
// [ts] fn pushEffect(tag: auto, create: auto, destroy: auto, deps: auto) {
// [ts] let effect = {
// [ts] tag: tag,
// [ts] create: create,
// [ts] destroy: destroy,
// [ts] deps: deps,
// Circular
// [ts] next: null
// [ts] };
// [ts] let componentUpdateQueue = currentlyRenderingFiber$1.updateQueue;
// [ts] if (componentUpdateQueue == null) {
// [ts] componentUpdateQueue = createFunctionComponentUpdateQueue();
// [ts] currentlyRenderingFiber$1.updateQueue = componentUpdateQueue;
// [ts] componentUpdateQueue.lastEffect = effect.next = effect;
// [ts] } else {
// [ts] let lastEffect = componentUpdateQueue.lastEffect;
// [ts] if (lastEffect == null) {
// [ts] componentUpdateQueue.lastEffect = effect.next = effect;
// [ts] } else {
// [ts] let firstEffect = lastEffect.next;
// [ts] lastEffect.next = effect;
// [ts] effect.next = firstEffect;
// [ts] componentUpdateQueue.lastEffect = effect;
// [ts] }
// [ts] }
// [ts] return effect;
// [ts] }
// [ts] fn mountRef(initialValue: auto) {
// [ts] let hook = mountWorkInProgressHook();
// [ts] let ref = {
// [ts] current: initialValue
// [ts] };
// [ts] {
// [ts] Object.seal(ref);
// [ts] }
// [ts] hook.memoizedState = ref;
// [ts] return ref;
// [ts] }
// [ts] fn updateRef(initialValue: auto) {
// [ts] let hook = updateWorkInProgressHook();
// [ts] return hook.memoizedState;
// [ts] }
// [ts] fn mountEffectImpl(fiberEffectTag: auto, hookEffectTag: auto, create: auto, deps: auto) {
// [ts] let hook = mountWorkInProgressHook();
// [ts] let nextDeps = if (deps == null) null else deps;
// [ts] currentlyRenderingFiber$1.effectTag |= fiberEffectTag;
// [ts] hook.memoizedState = pushEffect(HasEffect | hookEffectTag, create, null, nextDeps);
// [ts] }
// [ts] fn updateEffectImpl(fiberEffectTag: auto, hookEffectTag: auto, create: auto, deps: auto) {
// [ts] let hook = updateWorkInProgressHook();
// [ts] let nextDeps = if (deps == null) null else deps;
// [ts] let destroy = null;
// [ts] if (currentHook != null) {
// [ts] let prevEffect = currentHook.memoizedState;
// [ts] destroy = prevEffect.destroy;
// [ts] if (nextDeps != null) {
// [ts] let prevDeps = prevEffect.deps;
// [ts] if (areHookInputsEqual(nextDeps, prevDeps)) {
// [ts] pushEffect(hookEffectTag, create, destroy, nextDeps);
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] currentlyRenderingFiber$1.effectTag |= fiberEffectTag;
// [ts] hook.memoizedState = pushEffect(HasEffect | hookEffectTag, create, destroy, nextDeps);
// [ts] }
// [ts] fn mountEffect(create: auto, deps: auto) {
// [ts] {
// $FlowExpectedError - jest isn't a global, and isn't recognized outside of tests
// [ts] if ("undefined" != @typeName(jest)) {
// [ts] warnIfNotCurrentlyActingEffectsInDEV(currentlyRenderingFiber$1);
// [ts] }
// [ts] }
// [ts] return mountEffectImpl(Update | Passive, Passive$1, create, deps);
// [ts] }
// [ts] fn updateEffect(create: auto, deps: auto) {
// [ts] {
// $FlowExpectedError - jest isn't a global, and isn't recognized outside of tests
// [ts] if ("undefined" != @typeName(jest)) {
// [ts] warnIfNotCurrentlyActingEffectsInDEV(currentlyRenderingFiber$1);
// [ts] }
// [ts] }
// [ts] return updateEffectImpl(Update | Passive, Passive$1, create, deps);
// [ts] }
// [ts] fn mountLayoutEffect(create: auto, deps: auto) {
// [ts] return mountEffectImpl(Update, Layout, create, deps);
// [ts] }
// [ts] fn updateLayoutEffect(create: auto, deps: auto) {
// [ts] return updateEffectImpl(Update, Layout, create, deps);
// [ts] }
// [ts] fn imperativeHandleEffect(create: auto, ref: auto) {
// [ts] if (@typeName(ref) == "function") {
// [ts] let refCallback = ref;
// [ts] let _inst = create();
// [ts] refCallback(_inst);
// [ts] return () => {
// [ts] refCallback(null);
// [ts] };
// [ts] } else if (ref != null && ref != null) {
// [ts] let refObject = ref;
// [ts] {
// [ts] if (!refObject.hasOwnProperty("current")) {
// [ts] error("Expected useImperativeHandle() first argument to either be a " + "ref callback or React.createRef() object. Instead received: %s.", "an object with keys {" + Object.keys(refObject).join(", ") + "}");
// [ts] }
// [ts] }
// [ts] let _inst2 = create();
// [ts] refObject.current = _inst2;
// [ts] return () => {
// [ts] refObject.current = null;
// [ts] };
// [ts] }
// [ts] }
// [ts] fn mountImperativeHandle(ref: auto, create: auto, deps: auto) {
// [ts] {
// [ts] if (@typeName(create) != "function") {
// [ts] error(if ("Expected useImperativeHandle() second argument to be a function " + "that creates a handle. Instead received: %s.", create != null) @typeName(create) else "null");
// [ts] }
// [ts] } // TODO: If deps are provided, should we skip comparing the ref itself?
// [ts] let effectDeps = if (deps != null && deps != null) deps.concat([ref]) else null;
// [ts] return mountEffectImpl(Update, Layout, imperativeHandleEffect.bind(null, create, ref), effectDeps);
// [ts] }
// [ts] fn updateImperativeHandle(ref: auto, create: auto, deps: auto) {
// [ts] {
// [ts] if (@typeName(create) != "function") {
// [ts] error(if ("Expected useImperativeHandle() second argument to be a function " + "that creates a handle. Instead received: %s.", create != null) @typeName(create) else "null");
// [ts] }
// [ts] } // TODO: If deps are provided, should we skip comparing the ref itself?
// [ts] let effectDeps = if (deps != null && deps != null) deps.concat([ref]) else null;
// [ts] return updateEffectImpl(Update, Layout, imperativeHandleEffect.bind(null, create, ref), effectDeps);
// [ts] }
// [ts] fn mountDebugValue(value: auto, formatterFn: auto) {// This hook is normally a no-op.
// The react-debug-hooks package injects its own implementation
// so that e.g. DevTools can display custom hook values.
// [ts] }
// [ts] let updateDebugValue = mountDebugValue;
// [ts] fn mountCallback(callback: auto, deps: auto) {
// [ts] let hook = mountWorkInProgressHook();
// [ts] let nextDeps = if (deps == null) null else deps;
// [ts] hook.memoizedState = [callback, nextDeps];
// [ts] return callback;
// [ts] }
// [ts] fn updateCallback(callback: auto, deps: auto) {
// [ts] let hook = updateWorkInProgressHook();
// [ts] let nextDeps = if (deps == null) null else deps;
// [ts] let prevState = hook.memoizedState;
// [ts] if (prevState != null) {
// [ts] if (nextDeps != null) {
// [ts] let prevDeps = prevState[1];
// [ts] if (areHookInputsEqual(nextDeps, prevDeps)) {
// [ts] return prevState[0];
// [ts] }
// [ts] }
// [ts] }
// [ts] hook.memoizedState = [callback, nextDeps];
// [ts] return callback;
// [ts] }
// [ts] fn mountMemo(nextCreate: auto, deps: auto) {
// [ts] let hook = mountWorkInProgressHook();
// [ts] let nextDeps = if (deps == null) null else deps;
// [ts] let nextValue = nextCreate();
// [ts] hook.memoizedState = [nextValue, nextDeps];
// [ts] return nextValue;
// [ts] }
// [ts] fn updateMemo(nextCreate: auto, deps: auto) {
// [ts] let hook = updateWorkInProgressHook();
// [ts] let nextDeps = if (deps == null) null else deps;
// [ts] let prevState = hook.memoizedState;
// [ts] if (prevState != null) {
// Assume these are defined. If they're not, areHookInputsEqual will warn.
// [ts] if (nextDeps != null) {
// [ts] let prevDeps = prevState[1];
// [ts] if (areHookInputsEqual(nextDeps, prevDeps)) {
// [ts] return prevState[0];
// [ts] }
// [ts] }
// [ts] }
// [ts] let nextValue = nextCreate();
// [ts] hook.memoizedState = [nextValue, nextDeps];
// [ts] return nextValue;
// [ts] }
// [ts] fn mountDeferredValue(value: auto, config: auto) {
// [ts] let _mountState = mountState(value),
// [ts] prevValue = _mountState[0],
// [ts] setValue = _mountState[1];
// [ts] mountEffect(() => {
// [ts] let previousConfig = ReactCurrentBatchConfig$1.suspense;
// [ts] ReactCurrentBatchConfig$1.suspense = if (config == null) null else config;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] setValue(value);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentBatchConfig$1.suspense = previousConfig;
// [ts] }
// [ts] }, [value, config]);
// [ts] return prevValue;
// [ts] }
// [ts] fn updateDeferredValue(value: auto, config: auto) {
// [ts] let _updateState = updateState(),
// [ts] prevValue = _updateState[0],
// [ts] setValue = _updateState[1];
// [ts] updateEffect(() => {
// [ts] let previousConfig = ReactCurrentBatchConfig$1.suspense;
// [ts] ReactCurrentBatchConfig$1.suspense = if (config == null) null else config;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] setValue(value);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentBatchConfig$1.suspense = previousConfig;
// [ts] }
// [ts] }, [value, config]);
// [ts] return prevValue;
// [ts] }
// [ts] fn rerenderDeferredValue(value: auto, config: auto) {
// [ts] let _rerenderState = rerenderState(),
// [ts] prevValue = _rerenderState[0],
// [ts] setValue = _rerenderState[1];
// [ts] updateEffect(() => {
// [ts] let previousConfig = ReactCurrentBatchConfig$1.suspense;
// [ts] ReactCurrentBatchConfig$1.suspense = if (config == null) null else config;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] setValue(value);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentBatchConfig$1.suspense = previousConfig;
// [ts] }
// [ts] }, [value, config]);
// [ts] return prevValue;
// [ts] }
// [ts] fn startTransition(setPending: auto, config: auto, callback: auto) {
// [ts] let priorityLevel = getCurrentPriorityLevel();
// [ts] runWithPriority$1(priorityLevel < UserBlockingPriority$1 ? UserBlockingPriority$1 : priorityLevel, () => {
// [ts] setPending(true);
// [ts] });
// [ts] runWithPriority$1(priorityLevel > NormalPriority ? NormalPriority : priorityLevel, () => {
// [ts] let previousConfig = ReactCurrentBatchConfig$1.suspense;
// [ts] ReactCurrentBatchConfig$1.suspense = if (config == null) null else config;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] setPending(false);
// [ts] callback();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentBatchConfig$1.suspense = previousConfig;
// [ts] }
// [ts] });
// [ts] }
// [ts] fn mountTransition(config: auto) {
// [ts] let _mountState2 = mountState(false),
// [ts] isPending = _mountState2[0],
// [ts] setPending = _mountState2[1];
// [ts] let start = mountCallback(startTransition.bind(null, setPending, config), [setPending, config]);
// [ts] return [start, isPending];
// [ts] }
// [ts] fn updateTransition(config: auto) {
// [ts] let _updateState2 = updateState(),
// [ts] isPending = _updateState2[0],
// [ts] setPending = _updateState2[1];
// [ts] let start = updateCallback(startTransition.bind(null, setPending, config), [setPending, config]);
// [ts] return [start, isPending];
// [ts] }
// [ts] fn rerenderTransition(config: auto) {
// [ts] let _rerenderState2 = rerenderState(),
// [ts] isPending = _rerenderState2[0],
// [ts] setPending = _rerenderState2[1];
// [ts] let start = updateCallback(startTransition.bind(null, setPending, config), [setPending, config]);
// [ts] return [start, isPending];
// [ts] }
// [ts] fn dispatchAction(fiber: auto, queue: auto, action: auto) {
// [ts] {
// [ts] if (@typeName(arguments[3]) == "function") {
// [ts] error("State updates from the useState() and useReducer() Hooks don't support the " + "second callback argument. To execute a side effect after " + "rendering, declare it in the component body with useEffect().");
// [ts] }
// [ts] }
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] let suspenseConfig = requestCurrentSuspenseConfig();
// [ts] let expirationTime = computeExpirationForFiber(currentTime, fiber, suspenseConfig);
// [ts] let update = {
// [ts] expirationTime: expirationTime,
// [ts] suspenseConfig: suspenseConfig,
// [ts] action: action,
// [ts] eagerReducer: null,
// [ts] eagerState: null,
// [ts] next: null
// [ts] };
// [ts] {
// [ts] update.priority = getCurrentPriorityLevel();
// [ts] } // Append the update to the end of the list.
// [ts] let pending = queue.pending;
// [ts] if (pending == null) {
// This is the first update. Create a circular list.
// [ts] update.next = update;
// [ts] } else {
// [ts] update.next = pending.next;
// [ts] pending.next = update;
// [ts] }
// [ts] queue.pending = update;
// [ts] let alternate = fiber.alternate;
// [ts] if (fiber == currentlyRenderingFiber$1 || alternate != null && alternate == currentlyRenderingFiber$1) {
// This is a render phase update. Stash it in a lazily-created map of
// queue -> linked list of updates. After this render pass, we'll restart
// and apply the stashed updates on top of the work-in-progress hook.
// [ts] didScheduleRenderPhaseUpdate = true;
// [ts] update.expirationTime = renderExpirationTime;
// [ts] currentlyRenderingFiber$1.expirationTime = renderExpirationTime;
// [ts] } else {
// [ts] if (fiber.expirationTime == NoWork && (alternate == null || alternate.expirationTime == NoWork)) {
// The queue is currently empty, which means we can eagerly compute the
// next state before entering the render phase. If the new state is the
// same as the current state, we may be able to bail out entirely.
// [ts] let lastRenderedReducer = queue.lastRenderedReducer;
// [ts] if (lastRenderedReducer != null) {
// [ts] let prevDispatcher;
// [ts] {
// [ts] prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let currentState = queue.lastRenderedState;
// [ts] let eagerState = lastRenderedReducer(currentState, action); // Stash the eagerly computed state, and the reducer used to compute
// it, on the update object. If the reducer hasn't changed by the
// time we enter the render phase, then the eager state can be used
// without calling the reducer again.
// [ts] update.eagerReducer = lastRenderedReducer;
// [ts] update.eagerState = eagerState;
// [ts] if (objectIs(eagerState, currentState)) {
// Fast path. We can bail out without scheduling React to re-render.
// It's still possible that we'll need to rebase this update later,
// if the component re-renders for a different reason and by that
// time the reducer has changed.
// [ts] return;
// [ts] }
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {// Suppress the error. It will throw again in the render phase.)
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] {
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// $FlowExpectedError - jest isn't a global, and isn't recognized outside of tests
// [ts] if ("undefined" != @typeName(jest)) {
// [ts] warnIfNotScopedWithMatchingAct(fiber);
// [ts] warnIfNotCurrentlyActingUpdatesInDev(fiber);
// [ts] }
// [ts] }
// [ts] scheduleWork(fiber, expirationTime);
// [ts] }
// [ts] }
// [ts] let ContextOnlyDispatcher = {
// [ts] readContext: readContext,
// [ts] useCallback: throwInvalidHookError,
// [ts] useContext: throwInvalidHookError,
// [ts] useEffect: throwInvalidHookError,
// [ts] useImperativeHandle: throwInvalidHookError,
// [ts] useLayoutEffect: throwInvalidHookError,
// [ts] useMemo: throwInvalidHookError,
// [ts] useReducer: throwInvalidHookError,
// [ts] useRef: throwInvalidHookError,
// [ts] useState: throwInvalidHookError,
// [ts] useDebugValue: throwInvalidHookError,
// [ts] useResponder: throwInvalidHookError,
// [ts] useDeferredValue: throwInvalidHookError,
// [ts] useTransition: throwInvalidHookError
// [ts] };
// [ts] let HooksDispatcherOnMountInDEV = null;
// [ts] let HooksDispatcherOnMountWithHookTypesInDEV = null;
// [ts] let HooksDispatcherOnUpdateInDEV = null;
// [ts] let HooksDispatcherOnRerenderInDEV = null;
// [ts] let InvalidNestedHooksDispatcherOnMountInDEV = null;
// [ts] let InvalidNestedHooksDispatcherOnUpdateInDEV = null;
// [ts] let InvalidNestedHooksDispatcherOnRerenderInDEV = null;
// [ts] {
// [ts] let warnInvalidContextAccess = () => {
// [ts] error("Context can only be read while React is rendering. " + "In classes, you can read it in the render method or getDerivedStateFromProps. " + "In function components, you can read it directly in the function body, but not " + "inside Hooks like useReducer() or useMemo().");
// [ts] };
// [ts] let warnInvalidHookAccess = () => {
// [ts] error("Do not call Hooks inside useEffect(...), useMemo(...), or other built-in Hooks. " + "You can only call Hooks at the top level of your React function. " + "For more information, see " + "https://fb.me/rules-of-hooks");
// [ts] };
// [ts] HooksDispatcherOnMountInDEV = {
// [ts] readContext: (context, observedBits) => {
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useCallback: (callback, deps) => {
// [ts] currentHookNameInDev = "useCallback";
// [ts] mountHookTypesDev();
// [ts] checkDepsAreArrayDev(deps);
// [ts] return mountCallback(callback, deps);
// [ts] },
// [ts] useContext: (context, observedBits) => {
// [ts] currentHookNameInDev = "useContext";
// [ts] mountHookTypesDev();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useEffect";
// [ts] mountHookTypesDev();
// [ts] checkDepsAreArrayDev(deps);
// [ts] return mountEffect(create, deps);
// [ts] },
// [ts] useImperativeHandle: (ref, create, deps) => {
// [ts] currentHookNameInDev = "useImperativeHandle";
// [ts] mountHookTypesDev();
// [ts] checkDepsAreArrayDev(deps);
// [ts] return mountImperativeHandle(ref, create, deps);
// [ts] },
// [ts] useLayoutEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useLayoutEffect";
// [ts] mountHookTypesDev();
// [ts] checkDepsAreArrayDev(deps);
// [ts] return mountLayoutEffect(create, deps);
// [ts] },
// [ts] useMemo: (create, deps) => {
// [ts] currentHookNameInDev = "useMemo";
// [ts] mountHookTypesDev();
// [ts] checkDepsAreArrayDev(deps);
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountMemo(create, deps);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useReducer: (reducer, initialArg, init) => {
// [ts] currentHookNameInDev = "useReducer";
// [ts] mountHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountReducer(reducer, initialArg, init);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useRef: (initialValue) => {
// [ts] currentHookNameInDev = "useRef";
// [ts] mountHookTypesDev();
// [ts] return mountRef(initialValue);
// [ts] },
// [ts] useState: (initialState) => {
// [ts] currentHookNameInDev = "useState";
// [ts] mountHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountState(initialState);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useDebugValue: (value, formatterFn) => {
// [ts] currentHookNameInDev = "useDebugValue";
// [ts] mountHookTypesDev();
// [ts] return mountDebugValue();
// [ts] },
// [ts] useResponder: (responder, props) => {
// [ts] currentHookNameInDev = "useResponder";
// [ts] mountHookTypesDev();
// [ts] return createDeprecatedResponderListener(responder, props);
// [ts] },
// [ts] useDeferredValue: (value, config) => {
// [ts] currentHookNameInDev = "useDeferredValue";
// [ts] mountHookTypesDev();
// [ts] return mountDeferredValue(value, config);
// [ts] },
// [ts] useTransition: (config) => {
// [ts] currentHookNameInDev = "useTransition";
// [ts] mountHookTypesDev();
// [ts] return mountTransition(config);
// [ts] }
// [ts] };
// [ts] HooksDispatcherOnMountWithHookTypesInDEV = {
// [ts] readContext: (context, observedBits) => {
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useCallback: (callback, deps) => {
// [ts] currentHookNameInDev = "useCallback";
// [ts] updateHookTypesDev();
// [ts] return mountCallback(callback, deps);
// [ts] },
// [ts] useContext: (context, observedBits) => {
// [ts] currentHookNameInDev = "useContext";
// [ts] updateHookTypesDev();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useEffect";
// [ts] updateHookTypesDev();
// [ts] return mountEffect(create, deps);
// [ts] },
// [ts] useImperativeHandle: (ref, create, deps) => {
// [ts] currentHookNameInDev = "useImperativeHandle";
// [ts] updateHookTypesDev();
// [ts] return mountImperativeHandle(ref, create, deps);
// [ts] },
// [ts] useLayoutEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useLayoutEffect";
// [ts] updateHookTypesDev();
// [ts] return mountLayoutEffect(create, deps);
// [ts] },
// [ts] useMemo: (create, deps) => {
// [ts] currentHookNameInDev = "useMemo";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountMemo(create, deps);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useReducer: (reducer, initialArg, init) => {
// [ts] currentHookNameInDev = "useReducer";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountReducer(reducer, initialArg, init);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useRef: (initialValue) => {
// [ts] currentHookNameInDev = "useRef";
// [ts] updateHookTypesDev();
// [ts] return mountRef(initialValue);
// [ts] },
// [ts] useState: (initialState) => {
// [ts] currentHookNameInDev = "useState";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountState(initialState);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useDebugValue: (value, formatterFn) => {
// [ts] currentHookNameInDev = "useDebugValue";
// [ts] updateHookTypesDev();
// [ts] return mountDebugValue();
// [ts] },
// [ts] useResponder: (responder, props) => {
// [ts] currentHookNameInDev = "useResponder";
// [ts] updateHookTypesDev();
// [ts] return createDeprecatedResponderListener(responder, props);
// [ts] },
// [ts] useDeferredValue: (value, config) => {
// [ts] currentHookNameInDev = "useDeferredValue";
// [ts] updateHookTypesDev();
// [ts] return mountDeferredValue(value, config);
// [ts] },
// [ts] useTransition: (config) => {
// [ts] currentHookNameInDev = "useTransition";
// [ts] updateHookTypesDev();
// [ts] return mountTransition(config);
// [ts] }
// [ts] };
// [ts] HooksDispatcherOnUpdateInDEV = {
// [ts] readContext: (context, observedBits) => {
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useCallback: (callback, deps) => {
// [ts] currentHookNameInDev = "useCallback";
// [ts] updateHookTypesDev();
// [ts] return updateCallback(callback, deps);
// [ts] },
// [ts] useContext: (context, observedBits) => {
// [ts] currentHookNameInDev = "useContext";
// [ts] updateHookTypesDev();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useEffect";
// [ts] updateHookTypesDev();
// [ts] return updateEffect(create, deps);
// [ts] },
// [ts] useImperativeHandle: (ref, create, deps) => {
// [ts] currentHookNameInDev = "useImperativeHandle";
// [ts] updateHookTypesDev();
// [ts] return updateImperativeHandle(ref, create, deps);
// [ts] },
// [ts] useLayoutEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useLayoutEffect";
// [ts] updateHookTypesDev();
// [ts] return updateLayoutEffect(create, deps);
// [ts] },
// [ts] useMemo: (create, deps) => {
// [ts] currentHookNameInDev = "useMemo";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return updateMemo(create, deps);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useReducer: (reducer, initialArg, init) => {
// [ts] currentHookNameInDev = "useReducer";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return updateReducer(reducer, initialArg, init);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useRef: (initialValue) => {
// [ts] currentHookNameInDev = "useRef";
// [ts] updateHookTypesDev();
// [ts] return updateRef();
// [ts] },
// [ts] useState: (initialState) => {
// [ts] currentHookNameInDev = "useState";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return updateState(initialState);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useDebugValue: (value, formatterFn) => {
// [ts] currentHookNameInDev = "useDebugValue";
// [ts] updateHookTypesDev();
// [ts] return updateDebugValue();
// [ts] },
// [ts] useResponder: (responder, props) => {
// [ts] currentHookNameInDev = "useResponder";
// [ts] updateHookTypesDev();
// [ts] return createDeprecatedResponderListener(responder, props);
// [ts] },
// [ts] useDeferredValue: (value, config) => {
// [ts] currentHookNameInDev = "useDeferredValue";
// [ts] updateHookTypesDev();
// [ts] return updateDeferredValue(value, config);
// [ts] },
// [ts] useTransition: (config) => {
// [ts] currentHookNameInDev = "useTransition";
// [ts] updateHookTypesDev();
// [ts] return updateTransition(config);
// [ts] }
// [ts] };
// [ts] HooksDispatcherOnRerenderInDEV = {
// [ts] readContext: (context, observedBits) => {
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useCallback: (callback, deps) => {
// [ts] currentHookNameInDev = "useCallback";
// [ts] updateHookTypesDev();
// [ts] return updateCallback(callback, deps);
// [ts] },
// [ts] useContext: (context, observedBits) => {
// [ts] currentHookNameInDev = "useContext";
// [ts] updateHookTypesDev();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useEffect";
// [ts] updateHookTypesDev();
// [ts] return updateEffect(create, deps);
// [ts] },
// [ts] useImperativeHandle: (ref, create, deps) => {
// [ts] currentHookNameInDev = "useImperativeHandle";
// [ts] updateHookTypesDev();
// [ts] return updateImperativeHandle(ref, create, deps);
// [ts] },
// [ts] useLayoutEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useLayoutEffect";
// [ts] updateHookTypesDev();
// [ts] return updateLayoutEffect(create, deps);
// [ts] },
// [ts] useMemo: (create, deps) => {
// [ts] currentHookNameInDev = "useMemo";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnRerenderInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return updateMemo(create, deps);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useReducer: (reducer, initialArg, init) => {
// [ts] currentHookNameInDev = "useReducer";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnRerenderInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return rerenderReducer(reducer, initialArg, init);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useRef: (initialValue) => {
// [ts] currentHookNameInDev = "useRef";
// [ts] updateHookTypesDev();
// [ts] return updateRef();
// [ts] },
// [ts] useState: (initialState) => {
// [ts] currentHookNameInDev = "useState";
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnRerenderInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return rerenderState(initialState);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useDebugValue: (value, formatterFn) => {
// [ts] currentHookNameInDev = "useDebugValue";
// [ts] updateHookTypesDev();
// [ts] return updateDebugValue();
// [ts] },
// [ts] useResponder: (responder, props) => {
// [ts] currentHookNameInDev = "useResponder";
// [ts] updateHookTypesDev();
// [ts] return createDeprecatedResponderListener(responder, props);
// [ts] },
// [ts] useDeferredValue: (value, config) => {
// [ts] currentHookNameInDev = "useDeferredValue";
// [ts] updateHookTypesDev();
// [ts] return rerenderDeferredValue(value, config);
// [ts] },
// [ts] useTransition: (config) => {
// [ts] currentHookNameInDev = "useTransition";
// [ts] updateHookTypesDev();
// [ts] return rerenderTransition(config);
// [ts] }
// [ts] };
// [ts] InvalidNestedHooksDispatcherOnMountInDEV = {
// [ts] readContext: (context, observedBits) => {
// [ts] warnInvalidContextAccess();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useCallback: (callback, deps) => {
// [ts] currentHookNameInDev = "useCallback";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return mountCallback(callback, deps);
// [ts] },
// [ts] useContext: (context, observedBits) => {
// [ts] currentHookNameInDev = "useContext";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useEffect";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return mountEffect(create, deps);
// [ts] },
// [ts] useImperativeHandle: (ref, create, deps) => {
// [ts] currentHookNameInDev = "useImperativeHandle";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return mountImperativeHandle(ref, create, deps);
// [ts] },
// [ts] useLayoutEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useLayoutEffect";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return mountLayoutEffect(create, deps);
// [ts] },
// [ts] useMemo: (create, deps) => {
// [ts] currentHookNameInDev = "useMemo";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountMemo(create, deps);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useReducer: (reducer, initialArg, init) => {
// [ts] currentHookNameInDev = "useReducer";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountReducer(reducer, initialArg, init);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useRef: (initialValue) => {
// [ts] currentHookNameInDev = "useRef";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return mountRef(initialValue);
// [ts] },
// [ts] useState: (initialState) => {
// [ts] currentHookNameInDev = "useState";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnMountInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return mountState(initialState);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useDebugValue: (value, formatterFn) => {
// [ts] currentHookNameInDev = "useDebugValue";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return mountDebugValue();
// [ts] },
// [ts] useResponder: (responder, props) => {
// [ts] currentHookNameInDev = "useResponder";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return createDeprecatedResponderListener(responder, props);
// [ts] },
// [ts] useDeferredValue: (value, config) => {
// [ts] currentHookNameInDev = "useDeferredValue";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return mountDeferredValue(value, config);
// [ts] },
// [ts] useTransition: (config) => {
// [ts] currentHookNameInDev = "useTransition";
// [ts] warnInvalidHookAccess();
// [ts] mountHookTypesDev();
// [ts] return mountTransition(config);
// [ts] }
// [ts] };
// [ts] InvalidNestedHooksDispatcherOnUpdateInDEV = {
// [ts] readContext: (context, observedBits) => {
// [ts] warnInvalidContextAccess();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useCallback: (callback, deps) => {
// [ts] currentHookNameInDev = "useCallback";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateCallback(callback, deps);
// [ts] },
// [ts] useContext: (context, observedBits) => {
// [ts] currentHookNameInDev = "useContext";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useEffect";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateEffect(create, deps);
// [ts] },
// [ts] useImperativeHandle: (ref, create, deps) => {
// [ts] currentHookNameInDev = "useImperativeHandle";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateImperativeHandle(ref, create, deps);
// [ts] },
// [ts] useLayoutEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useLayoutEffect";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateLayoutEffect(create, deps);
// [ts] },
// [ts] useMemo: (create, deps) => {
// [ts] currentHookNameInDev = "useMemo";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return updateMemo(create, deps);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useReducer: (reducer, initialArg, init) => {
// [ts] currentHookNameInDev = "useReducer";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return updateReducer(reducer, initialArg, init);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useRef: (initialValue) => {
// [ts] currentHookNameInDev = "useRef";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateRef();
// [ts] },
// [ts] useState: (initialState) => {
// [ts] currentHookNameInDev = "useState";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return updateState(initialState);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useDebugValue: (value, formatterFn) => {
// [ts] currentHookNameInDev = "useDebugValue";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateDebugValue();
// [ts] },
// [ts] useResponder: (responder, props) => {
// [ts] currentHookNameInDev = "useResponder";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return createDeprecatedResponderListener(responder, props);
// [ts] },
// [ts] useDeferredValue: (value, config) => {
// [ts] currentHookNameInDev = "useDeferredValue";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateDeferredValue(value, config);
// [ts] },
// [ts] useTransition: (config) => {
// [ts] currentHookNameInDev = "useTransition";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateTransition(config);
// [ts] }
// [ts] };
// [ts] InvalidNestedHooksDispatcherOnRerenderInDEV = {
// [ts] readContext: (context, observedBits) => {
// [ts] warnInvalidContextAccess();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useCallback: (callback, deps) => {
// [ts] currentHookNameInDev = "useCallback";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateCallback(callback, deps);
// [ts] },
// [ts] useContext: (context, observedBits) => {
// [ts] currentHookNameInDev = "useContext";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return readContext(context, observedBits);
// [ts] },
// [ts] useEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useEffect";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateEffect(create, deps);
// [ts] },
// [ts] useImperativeHandle: (ref, create, deps) => {
// [ts] currentHookNameInDev = "useImperativeHandle";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateImperativeHandle(ref, create, deps);
// [ts] },
// [ts] useLayoutEffect: (create, deps) => {
// [ts] currentHookNameInDev = "useLayoutEffect";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateLayoutEffect(create, deps);
// [ts] },
// [ts] useMemo: (create, deps) => {
// [ts] currentHookNameInDev = "useMemo";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return updateMemo(create, deps);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useReducer: (reducer, initialArg, init) => {
// [ts] currentHookNameInDev = "useReducer";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return rerenderReducer(reducer, initialArg, init);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useRef: (initialValue) => {
// [ts] currentHookNameInDev = "useRef";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateRef();
// [ts] },
// [ts] useState: (initialState) => {
// [ts] currentHookNameInDev = "useState";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] let prevDispatcher = ReactCurrentDispatcher.current;
// [ts] ReactCurrentDispatcher.current = InvalidNestedHooksDispatcherOnUpdateInDEV;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return rerenderState(initialState);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentDispatcher.current = prevDispatcher;
// [ts] }
// [ts] },
// [ts] useDebugValue: (value, formatterFn) => {
// [ts] currentHookNameInDev = "useDebugValue";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return updateDebugValue();
// [ts] },
// [ts] useResponder: (responder, props) => {
// [ts] currentHookNameInDev = "useResponder";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return createDeprecatedResponderListener(responder, props);
// [ts] },
// [ts] useDeferredValue: (value, config) => {
// [ts] currentHookNameInDev = "useDeferredValue";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return rerenderDeferredValue(value, config);
// [ts] },
// [ts] useTransition: (config) => {
// [ts] currentHookNameInDev = "useTransition";
// [ts] warnInvalidHookAccess();
// [ts] updateHookTypesDev();
// [ts] return rerenderTransition(config);
// [ts] }
// [ts] };
// [ts] }
// [ts] let now$1 = unstable_now;
// [ts] let commitTime = 0;
// [ts] let profilerStartTime = -1;
// [ts] fn getCommitTime() {
// [ts] return commitTime;
// [ts] }
// [ts] fn recordCommitTime() {
// [ts] commitTime = now$1();
// [ts] }
// [ts] fn startProfilerTimer(fiber: auto) {
// [ts] profilerStartTime = now$1();
// [ts] if (fiber.actualStartTime < 0) {
// [ts] fiber.actualStartTime = now$1();
// [ts] }
// [ts] }
// [ts] fn stopProfilerTimerIfRunning(fiber: auto) {
// [ts] profilerStartTime = -1;
// [ts] }
// [ts] fn stopProfilerTimerIfRunningAndRecordDelta(fiber: auto, overrideBaseTime: auto) {
// [ts] if (profilerStartTime >= 0) {
// [ts] let elapsedTime = now$1() - profilerStartTime;
// [ts] fiber.actualDuration += elapsedTime;
// [ts] if (overrideBaseTime) {
// [ts] fiber.selfBaseDuration = elapsedTime;
// [ts] }
// [ts] profilerStartTime = -1;
// [ts] }
// [ts] }
// This may have been an insertion or a hydration.
// [ts] let hydrationParentFiber = null;
// [ts] let nextHydratableInstance = null;
// [ts] let isHydrating = false;
// [ts] fn enterHydrationState(fiber: auto) {
// [ts] let parentInstance = fiber.stateNode.containerInfo;
// [ts] nextHydratableInstance = getFirstHydratableChild(parentInstance);
// [ts] hydrationParentFiber = fiber;
// [ts] isHydrating = true;
// [ts] return true;
// [ts] }
// [ts] fn deleteHydratableInstance(returnFiber: auto, instance: auto) {
// [ts] {
// [ts] switch (returnFiber.tag) {
// [ts] HostRoot => {
// [ts] didNotHydrateContainerInstance(returnFiber.stateNode.containerInfo, instance);
// break;
// [ts] },
// [ts] HostComponent => {
// [ts] didNotHydrateInstance(returnFiber.type, returnFiber.memoizedProps, returnFiber.stateNode, instance);
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] let childToDelete = createFiberFromHostInstanceForDeletion();
// [ts] childToDelete.stateNode = instance;
// [ts] childToDelete.return = returnFiber;
// [ts] childToDelete.effectTag = Deletion; // This might seem like it belongs on progressedFirstDeletion. However,
// these children are not part of the reconciliation list of children.
// Even if we abort and rereconcile the children, that will try to hydrate
// again and the nodes are still in the host tree so these will be
// recreated.
// [ts] if (returnFiber.lastEffect != null) {
// [ts] returnFiber.lastEffect.nextEffect = childToDelete;
// [ts] returnFiber.lastEffect = childToDelete;
// [ts] } else {
// [ts] returnFiber.firstEffect = returnFiber.lastEffect = childToDelete;
// [ts] }
// [ts] }
// [ts] fn insertNonHydratedInstance(returnFiber: auto, fiber: auto) {
// [ts] fiber.effectTag = fiber.effectTag & ~Hydrating | Placement;
// [ts] {
// [ts] switch (returnFiber.tag) {
// [ts] HostRoot => {
// [ts] {
// [ts] let parentContainer = returnFiber.stateNode.containerInfo;
// [ts] switch (fiber.tag) {
// [ts] HostComponent => {
// [ts] let type = fiber.type;
// [ts] let props = fiber.pendingProps;
// [ts] didNotFindHydratableContainerInstance(parentContainer, type);
// break;
// [ts] },
// [ts] HostText => {
// [ts] let text = fiber.pendingProps;
// [ts] didNotFindHydratableContainerTextInstance(parentContainer, text);
// break;
// [ts] }
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] HostComponent => {
// [ts] {
// [ts] let parentType = returnFiber.type;
// [ts] let parentProps = returnFiber.memoizedProps;
// [ts] let parentInstance = returnFiber.stateNode;
// [ts] switch (fiber.tag) {
// [ts] HostComponent => {
// [ts] let _type = fiber.type;
// [ts] let _props = fiber.pendingProps;
// [ts] didNotFindHydratableInstance(parentType, parentProps, parentInstance, _type);
// break;
// [ts] },
// [ts] HostText => {
// [ts] let _text = fiber.pendingProps;
// [ts] didNotFindHydratableTextInstance(parentType, parentProps, parentInstance, _text);
// break;
// [ts] },
// [ts] SuspenseComponent => {
// [ts] didNotFindHydratableSuspenseInstance(parentType, parentProps);
// break;
// [ts] }
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] else => {
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn tryHydrate(fiber: auto, nextInstance: auto) {
// [ts] switch (fiber.tag) {
// [ts] HostComponent => {
// [ts] {
// [ts] let type = fiber.type;
// [ts] let props = fiber.pendingProps;
// [ts] let instance = canHydrateInstance(nextInstance, type);
// [ts] if (instance != null) {
// [ts] fiber.stateNode = instance;
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] },
// [ts] HostText => {
// [ts] {
// [ts] let text = fiber.pendingProps;
// [ts] let textInstance = canHydrateTextInstance(nextInstance, text);
// [ts] if (textInstance != null) {
// [ts] fiber.stateNode = textInstance;
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] },
// [ts] SuspenseComponent => {
// [ts] {
// [ts] return false;
// [ts] }
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn tryToClaimNextHydratableInstance(fiber: auto) {
// [ts] if (!isHydrating) {
// [ts] return;
// [ts] }
// [ts] let nextInstance = nextHydratableInstance;
// [ts] if (!nextInstance) {
// Nothing to hydrate. Make it an insertion.
// [ts] insertNonHydratedInstance(hydrationParentFiber, fiber);
// [ts] isHydrating = false;
// [ts] hydrationParentFiber = fiber;
// [ts] return;
// [ts] }
// [ts] let firstAttemptedInstance = nextInstance;
// [ts] if (!tryHydrate(fiber, nextInstance)) {
// If we can't hydrate this instance let's try the next one.
// We use this as a heuristic. It's based on intuition and not data so it
// might be flawed or unnecessary.
// [ts] nextInstance = getNextHydratableSibling(firstAttemptedInstance);
// [ts] if (!nextInstance || !tryHydrate(fiber, nextInstance)) {
// Nothing to hydrate. Make it an insertion.
// [ts] insertNonHydratedInstance(hydrationParentFiber, fiber);
// [ts] isHydrating = false;
// [ts] hydrationParentFiber = fiber;
// [ts] return;
// [ts] } // We matched the next one, we'll now assume that the first one was
// superfluous and we'll delete it. Since we can't eagerly delete it
// we'll have to schedule a deletion. To do that, this node needs a dummy
// fiber associated with it.
// [ts] deleteHydratableInstance(hydrationParentFiber, firstAttemptedInstance);
// [ts] }
// [ts] hydrationParentFiber = fiber;
// [ts] nextHydratableInstance = getFirstHydratableChild(nextInstance);
// [ts] }
// [ts] fn prepareToHydrateHostInstance(fiber: auto, rootContainerInstance: auto, hostContext: auto) {
// [ts] let instance = fiber.stateNode;
// [ts] let updatePayload = hydrateInstance(instance, fiber.type, fiber.memoizedProps, rootContainerInstance, hostContext, fiber); // TODO: Type this specific to this type of component.
// [ts] fiber.updateQueue = updatePayload; // If the update payload indicates that there is a change or if there
// is a new ref we mark this as an update.
// [ts] if (updatePayload != null) {
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn prepareToHydrateHostTextInstance(fiber: auto) {
// [ts] let textInstance = fiber.stateNode;
// [ts] let textContent = fiber.memoizedProps;
// [ts] let shouldUpdate = hydrateTextInstance(textInstance, textContent, fiber);
// [ts] {
// [ts] if (shouldUpdate) {
// We assume that prepareToHydrateHostTextInstance is called in a context where the
// hydration parent is the parent host component of this host text.
// [ts] let returnFiber = hydrationParentFiber;
// [ts] if (returnFiber != null) {
// [ts] switch (returnFiber.tag) {
// [ts] HostRoot => {
// [ts] {
// [ts] let parentContainer = returnFiber.stateNode.containerInfo;
// [ts] didNotMatchHydratedContainerTextInstance(parentContainer, textInstance, textContent);
// [ts] break;
// [ts] }
// [ts] },
// [ts] HostComponent => {
// [ts] {
// [ts] let parentType = returnFiber.type;
// [ts] let parentProps = returnFiber.memoizedProps;
// [ts] let parentInstance = returnFiber.stateNode;
// [ts] didNotMatchHydratedTextInstance(parentType, parentProps, parentInstance, textInstance, textContent);
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return shouldUpdate;
// [ts] }
// [ts] fn skipPastDehydratedSuspenseInstance(fiber: auto) {
// [ts] let suspenseState = fiber.memoizedState;
// [ts] let suspenseInstance = if (suspenseState != null) suspenseState.dehydrated else null;
// [ts] if (!suspenseInstance) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected to have a hydrated suspense instance. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] return getNextHydratableInstanceAfterSuspenseInstance(suspenseInstance);
// [ts] }
// [ts] fn popToNextHostParent(fiber: auto) {
// [ts] let parent = fiber.return;
// [ts] while (parent != null && parent.tag != HostComponent && parent.tag != HostRoot && parent.tag != SuspenseComponent) {
// [ts] parent = parent.return;
// [ts] }
// [ts] hydrationParentFiber = parent;
// [ts] }
// [ts] fn popHydrationState(fiber: auto) {
// [ts] if (fiber != hydrationParentFiber) {
// We're deeper than the current hydration context, inside an inserted
// tree.
// [ts] return false;
// [ts] }
// [ts] if (!isHydrating) {
// If we're not currently hydrating but we're in a hydration context, then
// we were an insertion and now need to pop up reenter hydration of our
// siblings.
// [ts] popToNextHostParent(fiber);
// [ts] isHydrating = true;
// [ts] return false;
// [ts] }
// [ts] let type = fiber.type; // If we have any remaining hydratable nodes, we need to delete them now.
// We only do this deeper than head and body since they tend to have random
// other nodes in them. We also ignore components with pure text content in
// side of them.
// TODO: Better heuristic.
// [ts] if (fiber.tag != HostComponent || type != "head" && type != "body" && !shouldSetTextContent(type, fiber.memoizedProps)) {
// [ts] let nextInstance = nextHydratableInstance;
// [ts] while (nextInstance) {
// [ts] deleteHydratableInstance(fiber, nextInstance);
// [ts] nextInstance = getNextHydratableSibling(nextInstance);
// [ts] }
// [ts] }
// [ts] popToNextHostParent(fiber);
// [ts] if (fiber.tag == SuspenseComponent) {
// [ts] nextHydratableInstance = skipPastDehydratedSuspenseInstance(fiber);
// [ts] } else {
// [ts] nextHydratableInstance = if (hydrationParentFiber) getNextHydratableSibling(fiber.stateNode) else null;
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn resetHydrationState() {
// [ts] hydrationParentFiber = null;
// [ts] nextHydratableInstance = null;
// [ts] isHydrating = false;
// [ts] }
// [ts] let ReactCurrentOwner$1 = ReactSharedInternals.ReactCurrentOwner;
// [ts] let didReceiveUpdate = false;
// [ts] let didWarnAboutBadClass;
// [ts] let didWarnAboutModulePatternComponent;
// [ts] let didWarnAboutContextTypeOnFunctionComponent;
// [ts] let didWarnAboutGetDerivedStateOnFunctionComponent;
// [ts] let didWarnAboutFunctionRefs;
// [ts] let didWarnAboutReassigningProps;
// [ts] let didWarnAboutRevealOrder;
// [ts] let didWarnAboutTailOptions;
// [ts] {
// [ts] didWarnAboutBadClass = {};
// [ts] didWarnAboutModulePatternComponent = {};
// [ts] didWarnAboutContextTypeOnFunctionComponent = {};
// [ts] didWarnAboutGetDerivedStateOnFunctionComponent = {};
// [ts] didWarnAboutFunctionRefs = {};
// [ts] didWarnAboutReassigningProps = false;
// [ts] didWarnAboutRevealOrder = {};
// [ts] didWarnAboutTailOptions = {};
// [ts] }
// [ts] fn reconcileChildren(current: auto, workInProgress: auto, nextChildren: auto, renderExpirationTime: auto) {
// [ts] if (current == null) {
// If this is a fresh new component that hasn't been rendered yet, we
// won't update its child set by applying minimal side-effects. Instead,
// we will add them all to the child before it gets rendered. That means
// we can optimize this reconciliation pass by not tracking side-effects.
// [ts] workInProgress.child = mountChildFibers(workInProgress, null, nextChildren, renderExpirationTime);
// [ts] } else {
// If the current child is the same as the work in progress, it means that
// we haven't yet started any work on these children. Therefore, we use
// the clone algorithm to create a copy of all the current children.
// If we had any progressed work already, that is invalid at this point so
// let's throw it out.
// [ts] workInProgress.child = reconcileChildFibers(workInProgress, current.child, nextChildren, renderExpirationTime);
// [ts] }
// [ts] }
// [ts] fn forceUnmountCurrentAndReconcile(current: auto, workInProgress: auto, nextChildren: auto, renderExpirationTime: auto) {
// This function is fork of reconcileChildren. It's used in cases where we
// want to reconcile without matching against the existing set. This has the
// effect of all current children being unmounted; even if the type and key
// are the same, the old child is unmounted and a new child is created.
//
// To do this, we're going to go through the reconcile algorithm twice. In
// the first pass, we schedule a deletion for all the current children by
// passing null.
// [ts] workInProgress.child = reconcileChildFibers(workInProgress, current.child, null, renderExpirationTime); // In the second pass, we mount the new children. The trick here is that we
// pass null in place of where we usually pass the current child set. This has
// the effect of remounting all children regardless of whether their
// identities match.
// [ts] workInProgress.child = reconcileChildFibers(workInProgress, null, nextChildren, renderExpirationTime);
// [ts] }
// [ts] fn updateForwardRef(current: auto, workInProgress: auto, Component: auto, nextProps: auto, renderExpirationTime: auto) {
// TODO: current can be non-null here even if the component
// hasn't yet mounted. This happens after the first render suspends.
// We'll need to figure out if this is fine or can cause issues.
// [ts] {
// [ts] if (workInProgress.type != workInProgress.elementType) {
// Lazy component props can't be validated in createElement
// because they're only guaranteed to be resolved here.
// [ts] let innerPropTypes = Component.propTypes;
// [ts] if (innerPropTypes) {
// [ts] checkPropTypes_1(innerPropTypes, nextProps, // Resolved props
// [ts] "prop", getComponentName(Component), getCurrentFiberStackInDev);
// [ts] }
// [ts] }
// [ts] }
// [ts] let render = Component.render;
// [ts] let ref = workInProgress.ref; // The rest is a fork of updateFunctionComponent
// [ts] let nextChildren;
// [ts] prepareToReadContext(workInProgress, renderExpirationTime);
// [ts] {
// [ts] ReactCurrentOwner$1.current = workInProgress;
// [ts] setIsRendering(true);
// [ts] nextChildren = renderWithHooks(current, workInProgress, render, nextProps, ref, renderExpirationTime);
// [ts] if ( workInProgress.mode & StrictMode) {
// Only double-render components with Hooks
// [ts] if (workInProgress.memoizedState != null) {
// [ts] nextChildren = renderWithHooks(current, workInProgress, render, nextProps, ref, renderExpirationTime);
// [ts] }
// [ts] }
// [ts] setIsRendering(false);
// [ts] }
// [ts] if (current != null && !didReceiveUpdate) {
// [ts] bailoutHooks(current, workInProgress, renderExpirationTime);
// [ts] return bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] } // React DevTools reads this flag.
// [ts] workInProgress.effectTag |= PerformedWork;
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn updateMemoComponent(current: auto, workInProgress: auto, Component: auto, nextProps: auto, updateExpirationTime: auto, renderExpirationTime: auto) {
// [ts] if (current == null) {
// [ts] let type = Component.type;
// [ts] if (isSimpleFunctionComponent(type) && Component.compare == null && // SimpleMemoComponent codepath doesn't resolve outer props either.
// [ts] Component.defaultProps == null) {
// [ts] let resolvedType = type;
// [ts] {
// [ts] resolvedType = resolveFunctionForHotReloading(type);
// [ts] } // If this is a plain function component without default props,
// and with only the default shallow comparison, we upgrade it
// to a SimpleMemoComponent to allow fast path updates.
// [ts] workInProgress.tag = SimpleMemoComponent;
// [ts] workInProgress.type = resolvedType;
// [ts] {
// [ts] validateFunctionComponentInDev(workInProgress, type);
// [ts] }
// [ts] return updateSimpleMemoComponent(current, workInProgress, resolvedType, nextProps, updateExpirationTime, renderExpirationTime);
// [ts] }
// [ts] {
// [ts] let innerPropTypes = type.propTypes;
// [ts] if (innerPropTypes) {
// Inner memo component props aren't currently validated in createElement.
// We could move it there, but we'd still need this for lazy code path.
// [ts] checkPropTypes_1(innerPropTypes, nextProps, // Resolved props
// [ts] "prop", getComponentName(type), getCurrentFiberStackInDev);
// [ts] }
// [ts] }
// [ts] let child = createFiberFromTypeAndProps(Component.type, null, nextProps, null, workInProgress.mode, renderExpirationTime);
// [ts] child.ref = workInProgress.ref;
// [ts] child.return = workInProgress;
// [ts] workInProgress.child = child;
// [ts] return child;
// [ts] }
// [ts] {
// [ts] let _type = Component.type;
// [ts] let _innerPropTypes = _type.propTypes;
// [ts] if (_innerPropTypes) {
// Inner memo component props aren't currently validated in createElement.
// We could move it there, but we'd still need this for lazy code path.
// [ts] checkPropTypes_1(_innerPropTypes, nextProps, // Resolved props
// [ts] "prop", getComponentName(_type), getCurrentFiberStackInDev);
// [ts] }
// [ts] }
// [ts] let currentChild = current.child; // This is always exactly one child
// [ts] if (updateExpirationTime < renderExpirationTime) {
// This will be the props with resolved defaultProps,
// unlike current.memoizedProps which will be the unresolved ones.
// [ts] let prevProps = currentChild.memoizedProps; // Default to shallow comparison
// [ts] let compare = Component.compare;
// [ts] compare = if (compare != null) compare else shallowEqual;
// [ts] if (compare(prevProps, nextProps) && current.ref == workInProgress.ref) {
// [ts] return bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] }
// [ts] } // React DevTools reads this flag.
// [ts] workInProgress.effectTag |= PerformedWork;
// [ts] let newChild = createWorkInProgress(currentChild, nextProps);
// [ts] newChild.ref = workInProgress.ref;
// [ts] newChild.return = workInProgress;
// [ts] workInProgress.child = newChild;
// [ts] return newChild;
// [ts] }
// [ts] fn updateSimpleMemoComponent(current: auto, workInProgress: auto, Component: auto, nextProps: auto, updateExpirationTime: auto, renderExpirationTime: auto) {
// TODO: current can be non-null here even if the component
// hasn't yet mounted. This happens when the inner render suspends.
// We'll need to figure out if this is fine or can cause issues.
// [ts] {
// [ts] if (workInProgress.type != workInProgress.elementType) {
// Lazy component props can't be validated in createElement
// because they're only guaranteed to be resolved here.
// [ts] let outerMemoType = workInProgress.elementType;
// [ts] if (outerMemoType.$$typeof == REACT_LAZY_TYPE) {
// We warn when you define propTypes on lazy()
// so let's just skip over it to find memo() outer wrapper.
// Inner props for memo are validated later.
// [ts] outerMemoType = refineResolvedLazyComponent(outerMemoType);
// [ts] }
// [ts] let outerPropTypes = outerMemoType && outerMemoType.propTypes;
// [ts] if (outerPropTypes) {
// [ts] checkPropTypes_1(outerPropTypes, nextProps, // Resolved (SimpleMemoComponent has no defaultProps)
// [ts] "prop", getComponentName(outerMemoType), getCurrentFiberStackInDev);
// [ts] } // Inner propTypes will be validated in the function component path.
// [ts] }
// [ts] }
// [ts] if (current != null) {
// [ts] let prevProps = current.memoizedProps;
// [ts] if (shallowEqual(prevProps, nextProps) && current.ref == workInProgress.ref && ( // Prevent bailout if the implementation changed due to hot reload.
// [ts] workInProgress.type == current.type )) {
// [ts] didReceiveUpdate = false;
// [ts] if (updateExpirationTime < renderExpirationTime) {
// The pending update priority was cleared at the beginning of
// beginWork. We're about to bail out, but there might be additional
// updates at a lower priority. Usually, the priority level of the
// remaining updates is accumlated during the evaluation of the
// component (i.e. when processing the update queue). But since since
// we're bailing out early *without* evaluating the component, we need
// to account for it here, too. Reset to the value of the current fiber.
// NOTE: This only applies to SimpleMemoComponent, not MemoComponent,
// because a MemoComponent fiber does not have hooks or an update queue;
// rather, it wraps around an inner component, which may or may not
// contains hooks.
// TODO: Move the reset at in beginWork out of the common path so that
// this is no longer necessary.
// [ts] workInProgress.expirationTime = current.expirationTime;
// [ts] return bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] }
// [ts] }
// [ts] }
// [ts] return updateFunctionComponent(current, workInProgress, Component, nextProps, renderExpirationTime);
// [ts] }
// [ts] fn updateFragment(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] let nextChildren = workInProgress.pendingProps;
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn updateMode(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] let nextChildren = workInProgress.pendingProps.children;
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn updateProfiler(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] {
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] let nextProps = workInProgress.pendingProps;
// [ts] let nextChildren = nextProps.children;
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn markRef(current: auto, workInProgress: auto) {
// [ts] let ref = workInProgress.ref;
// [ts] if (current == null && ref != null || current != null && current.ref != ref) {
// Schedule a Ref effect
// [ts] workInProgress.effectTag |= Ref;
// [ts] }
// [ts] }
// [ts] fn updateFunctionComponent(current: auto, workInProgress: auto, Component: auto, nextProps: auto, renderExpirationTime: auto) {
// [ts] {
// [ts] if (workInProgress.type != workInProgress.elementType) {
// Lazy component props can't be validated in createElement
// because they're only guaranteed to be resolved here.
// [ts] let innerPropTypes = Component.propTypes;
// [ts] if (innerPropTypes) {
// [ts] checkPropTypes_1(innerPropTypes, nextProps, // Resolved props
// [ts] "prop", getComponentName(Component), getCurrentFiberStackInDev);
// [ts] }
// [ts] }
// [ts] }
// [ts] let context;
// [ts] {
// [ts] let unmaskedContext = getUnmaskedContext(workInProgress, Component, true);
// [ts] context = getMaskedContext(workInProgress, unmaskedContext);
// [ts] }
// [ts] let nextChildren;
// [ts] prepareToReadContext(workInProgress, renderExpirationTime);
// [ts] {
// [ts] ReactCurrentOwner$1.current = workInProgress;
// [ts] setIsRendering(true);
// [ts] nextChildren = renderWithHooks(current, workInProgress, Component, nextProps, context, renderExpirationTime);
// [ts] if ( workInProgress.mode & StrictMode) {
// Only double-render components with Hooks
// [ts] if (workInProgress.memoizedState != null) {
// [ts] nextChildren = renderWithHooks(current, workInProgress, Component, nextProps, context, renderExpirationTime);
// [ts] }
// [ts] }
// [ts] setIsRendering(false);
// [ts] }
// [ts] if (current != null && !didReceiveUpdate) {
// [ts] bailoutHooks(current, workInProgress, renderExpirationTime);
// [ts] return bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] } // React DevTools reads this flag.
// [ts] workInProgress.effectTag |= PerformedWork;
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn updateClassComponent(current: auto, workInProgress: auto, Component: auto, nextProps: auto, renderExpirationTime: auto) {
// [ts] {
// [ts] if (workInProgress.type != workInProgress.elementType) {
// Lazy component props can't be validated in createElement
// because they're only guaranteed to be resolved here.
// [ts] let innerPropTypes = Component.propTypes;
// [ts] if (innerPropTypes) {
// [ts] checkPropTypes_1(innerPropTypes, nextProps, // Resolved props
// [ts] "prop", getComponentName(Component), getCurrentFiberStackInDev);
// [ts] }
// [ts] }
// [ts] } // Push context providers early to prevent context stack mismatches.
// During mounting we don't know the child context yet as the instance doesn't exist.
// We will invalidate the child context in finishClassComponent() right after rendering.
// [ts] let hasContext;
// [ts] if (isContextProvider(Component)) {
// [ts] hasContext = true;
// [ts] pushContextProvider(workInProgress);
// [ts] } else {
// [ts] hasContext = false;
// [ts] }
// [ts] prepareToReadContext(workInProgress, renderExpirationTime);
// [ts] let instance = workInProgress.stateNode;
// [ts] let shouldUpdate;
// [ts] if (instance == null) {
// [ts] if (current != null) {
// A class component without an instance only mounts if it suspended
// inside a non-concurrent tree, in an inconsistent state. We want to
// treat it like a new mount, even though an empty version of it already
// committed. Disconnect the alternate pointers.
// [ts] current.alternate = null;
// [ts] workInProgress.alternate = null; // Since this is conceptually a new fiber, schedule a Placement effect
// [ts] workInProgress.effectTag |= Placement;
// [ts] } // In the initial pass we might need to construct the instance.
// [ts] constructClassInstance(workInProgress, Component, nextProps);
// [ts] mountClassInstance(workInProgress, Component, nextProps, renderExpirationTime);
// [ts] shouldUpdate = true;
// [ts] } else if (current == null) {
// In a resume, we'll already have an instance we can reuse.
// [ts] shouldUpdate = resumeMountClassInstance(workInProgress, Component, nextProps, renderExpirationTime);
// [ts] } else {
// [ts] shouldUpdate = updateClassInstance(current, workInProgress, Component, nextProps, renderExpirationTime);
// [ts] }
// [ts] let nextUnitOfWork = finishClassComponent(current, workInProgress, Component, shouldUpdate, hasContext, renderExpirationTime);
// [ts] {
// [ts] let inst = workInProgress.stateNode;
// [ts] if (inst.props != nextProps) {
// [ts] if (!didWarnAboutReassigningProps) {
// [ts] error("It looks like %s is reassigning its own `this.props` while rendering. " + "This is not supported and can lead to confusing bugs.", getComponentName(workInProgress.type) || "a component");
// [ts] }
// [ts] didWarnAboutReassigningProps = true;
// [ts] }
// [ts] }
// [ts] return nextUnitOfWork;
// [ts] }
// [ts] fn finishClassComponent(current: auto, workInProgress: auto, Component: auto, shouldUpdate: auto, hasContext: auto, renderExpirationTime: auto) {
// Refs should update even if shouldComponentUpdate returns false
// [ts] markRef(current, workInProgress);
// [ts] let didCaptureError = (workInProgress.effectTag & DidCapture) != NoEffect;
// [ts] if (!shouldUpdate && !didCaptureError) {
// Context providers should defer to sCU for rendering
// [ts] if (hasContext) {
// [ts] invalidateContextProvider(workInProgress, Component, false);
// [ts] }
// [ts] return bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] }
// [ts] let instance = workInProgress.stateNode; // Rerender
// [ts] ReactCurrentOwner$1.current = workInProgress;
// [ts] let nextChildren;
// [ts] if (didCaptureError && @typeName(Component.getDerivedStateFromError) != "function") {
// If we captured an error, but getDerivedStateFromError is not defined,
// unmount all the children. componentDidCatch will schedule an update to
// re-render a fallback. This is temporary until we migrate everyone to
// the new API.
// TODO: Warn in a future release.
// [ts] nextChildren = null;
// [ts] {
// [ts] stopProfilerTimerIfRunning();
// [ts] }
// [ts] } else {
// [ts] {
// [ts] setIsRendering(true);
// [ts] nextChildren = instance.render();
// [ts] if ( workInProgress.mode & StrictMode) {
// [ts] instance.render();
// [ts] }
// [ts] setIsRendering(false);
// [ts] }
// [ts] } // React DevTools reads this flag.
// [ts] workInProgress.effectTag |= PerformedWork;
// [ts] if (current != null && didCaptureError) {
// If we're recovering from an error, reconcile without reusing any of
// the existing children. Conceptually, the normal children and the children
// that are shown on error are two different sets, so we shouldn't reuse
// normal children even if their identities match.
// [ts] forceUnmountCurrentAndReconcile(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] } else {
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] } // Memoize state using the values we just used to render.
// TODO: Restructure so we never read values from the instance.
// [ts] workInProgress.memoizedState = instance.state; // The context might have changed so we need to recalculate it.
// [ts] if (hasContext) {
// [ts] invalidateContextProvider(workInProgress, Component, true);
// [ts] }
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn pushHostRootContext(workInProgress: auto) {
// [ts] let root = workInProgress.stateNode;
// [ts] if (root.pendingContext) {
// [ts] pushTopLevelContextObject(workInProgress, root.pendingContext, root.pendingContext != root.context);
// [ts] } else if (root.context) {
// Should always be set
// [ts] pushTopLevelContextObject(workInProgress, root.context, false);
// [ts] }
// [ts] pushHostContainer(workInProgress, root.containerInfo);
// [ts] }
// [ts] fn updateHostRoot(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] pushHostRootContext(workInProgress);
// [ts] let updateQueue = workInProgress.updateQueue;
// [ts] if (!(current != null && updateQueue != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "If the root does not have an updateQueue, we should have already bailed out. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] let nextProps = workInProgress.pendingProps;
// [ts] let prevState = workInProgress.memoizedState;
// [ts] let prevChildren = if (prevState != null) prevState.element else null;
// [ts] cloneUpdateQueue(current, workInProgress);
// [ts] processUpdateQueue(workInProgress, nextProps, null, renderExpirationTime);
// [ts] let nextState = workInProgress.memoizedState; // Caution: React DevTools currently depends on this property
// being called "element".
// [ts] let nextChildren = nextState.element;
// [ts] if (nextChildren == prevChildren) {
// If the state is the same as before, that's a bailout because we had
// no work that expires at this time.
// [ts] resetHydrationState();
// [ts] return bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] }
// [ts] let root = workInProgress.stateNode;
// [ts] if (root.hydrate && enterHydrationState(workInProgress)) {
// If we don't have any current children this might be the first pass.
// We always try to hydrate. If this isn't a hydration pass there won't
// be any children to hydrate which is effectively the same thing as
// not hydrating.
// [ts] let child = mountChildFibers(workInProgress, null, nextChildren, renderExpirationTime);
// [ts] workInProgress.child = child;
// [ts] let node = child;
// [ts] while (node) {
// Mark each child as hydrating. This is a fast path to know whether this
// tree is part of a hydrating tree. This is used to determine if a child
// node has fully mounted yet, and for scheduling event replaying.
// Conceptually this is similar to Placement in that a new subtree is
// inserted into the React tree here. It just happens to not need DOM
// mutations because it already exists.
// [ts] node.effectTag = node.effectTag & ~Placement | Hydrating;
// [ts] node = node.sibling;
// [ts] }
// [ts] } else {
// Otherwise reset hydration state in case we aborted and resumed another
// root.
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] resetHydrationState();
// [ts] }
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn updateHostComponent(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] pushHostContext(workInProgress);
// [ts] if (current == null) {
// [ts] tryToClaimNextHydratableInstance(workInProgress);
// [ts] }
// [ts] let type = workInProgress.type;
// [ts] let nextProps = workInProgress.pendingProps;
// [ts] let prevProps = if (current != null) current.memoizedProps else null;
// [ts] let nextChildren = nextProps.children;
// [ts] let isDirectTextChild = shouldSetTextContent(type, nextProps);
// [ts] if (isDirectTextChild) {
// We special case a direct text child of a host node. This is a common
// case. We won't handle it as a reified child. We will instead handle
// this in the host environment that also has access to this prop. That
// avoids allocating another HostText fiber and traversing it.
// [ts] nextChildren = null;
// [ts] } else if (prevProps != null && shouldSetTextContent(type, prevProps)) {
// If we're switching from a direct text child to a normal child, or to
// empty, we need to schedule the text content to be reset.
// [ts] workInProgress.effectTag |= ContentReset;
// [ts] }
// [ts] markRef(current, workInProgress); // Check the host config to see if the children are offscreen/hidden.
// [ts] if (workInProgress.mode & ConcurrentMode && renderExpirationTime != Never && shouldDeprioritizeSubtree(type, nextProps)) {
// [ts] {
// [ts] markSpawnedWork(Never);
// [ts] } // Schedule this fiber to re-render at offscreen priority. Then bailout.
// [ts] workInProgress.expirationTime = workInProgress.childExpirationTime = Never;
// [ts] return null;
// [ts] }
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn updateHostText(current: auto, workInProgress: auto) {
// [ts] if (current == null) {
// [ts] tryToClaimNextHydratableInstance(workInProgress);
// [ts] } // Nothing to do here. This is terminal. We'll do the completion step
// immediately after.
// [ts] return null;
// [ts] }
// [ts] fn mountLazyComponent(_current: auto, workInProgress: auto, elementType: auto, updateExpirationTime: auto, renderExpirationTime: auto) {
// [ts] if (_current != null) {
// A lazy component only mounts if it suspended inside a non-
// concurrent tree, in an inconsistent state. We want to treat it like
// a new mount, even though an empty version of it already committed.
// Disconnect the alternate pointers.
// [ts] _current.alternate = null;
// [ts] workInProgress.alternate = null; // Since this is conceptually a new fiber, schedule a Placement effect
// [ts] workInProgress.effectTag |= Placement;
// [ts] }
// [ts] let props = workInProgress.pendingProps; // We can't start a User Timing measurement with correct label yet.
// Cancel and resume right after we know the tag.
// [ts] cancelWorkTimer(workInProgress);
// [ts] let Component = readLazyComponentType(elementType); // Store the unwrapped component in the type.
// [ts] workInProgress.type = Component;
// [ts] let resolvedTag = workInProgress.tag = resolveLazyComponentTag(Component);
// [ts] startWorkTimer(workInProgress);
// [ts] let resolvedProps = resolveDefaultProps(Component, props);
// [ts] let child;
// [ts] switch (resolvedTag) {
// [ts] FunctionComponent => {
// [ts] {
// [ts] {
// [ts] validateFunctionComponentInDev(workInProgress, Component);
// [ts] workInProgress.type = Component = resolveFunctionForHotReloading(Component);
// [ts] }
// [ts] child = updateFunctionComponent(null, workInProgress, Component, resolvedProps, renderExpirationTime);
// [ts] return child;
// [ts] }
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] {
// [ts] workInProgress.type = Component = resolveClassForHotReloading(Component);
// [ts] }
// [ts] child = updateClassComponent(null, workInProgress, Component, resolvedProps, renderExpirationTime);
// [ts] return child;
// [ts] }
// [ts] },
// [ts] ForwardRef => {
// [ts] {
// [ts] {
// [ts] workInProgress.type = Component = resolveForwardRefForHotReloading(Component);
// [ts] }
// [ts] child = updateForwardRef(null, workInProgress, Component, resolvedProps, renderExpirationTime);
// [ts] return child;
// [ts] }
// [ts] },
// [ts] MemoComponent => {
// [ts] {
// [ts] {
// [ts] if (workInProgress.type != workInProgress.elementType) {
// [ts] let outerPropTypes = Component.propTypes;
// [ts] if (outerPropTypes) {
// [ts] checkPropTypes_1(outerPropTypes, resolvedProps, // Resolved for outer only
// [ts] "prop", getComponentName(Component), getCurrentFiberStackInDev);
// [ts] }
// [ts] }
// [ts] }
// [ts] child = updateMemoComponent(null, workInProgress, Component, resolveDefaultProps(Component.type, resolvedProps), // The inner type can have defaults too
// [ts] updateExpirationTime, renderExpirationTime);
// [ts] return child;
// [ts] }
// [ts] }
// [ts] }
// [ts] let hint = "";
// [ts] {
// [ts] if (Component != null && @typeName(Component) == "object" && Component.$$typeof == REACT_LAZY_TYPE) {
// [ts] hint = " Did you wrap a component in React.lazy() more than once?";
// [ts] }
// [ts] } // This message intentionally doesn't mention ForwardRef or MemoComponent
// because the fact that it's a separate type of work is an
// implementation detail.
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Element type is invalid. Received a promise that resolves to: " + Component + ". Lazy element type must resolve to a class or function." + hint );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn mountIncompleteClassComponent(_current: auto, workInProgress: auto, Component: auto, nextProps: auto, renderExpirationTime: auto) {
// [ts] if (_current != null) {
// An incomplete component only mounts if it suspended inside a non-
// concurrent tree, in an inconsistent state. We want to treat it like
// a new mount, even though an empty version of it already committed.
// Disconnect the alternate pointers.
// [ts] _current.alternate = null;
// [ts] workInProgress.alternate = null; // Since this is conceptually a new fiber, schedule a Placement effect
// [ts] workInProgress.effectTag |= Placement;
// [ts] } // Promote the fiber to a class and try rendering again.
// [ts] workInProgress.tag = ClassComponent; // The rest of this function is a fork of `updateClassComponent`
// Push context providers early to prevent context stack mismatches.
// During mounting we don't know the child context yet as the instance doesn't exist.
// We will invalidate the child context in finishClassComponent() right after rendering.
// [ts] let hasContext;
// [ts] if (isContextProvider(Component)) {
// [ts] hasContext = true;
// [ts] pushContextProvider(workInProgress);
// [ts] } else {
// [ts] hasContext = false;
// [ts] }
// [ts] prepareToReadContext(workInProgress, renderExpirationTime);
// [ts] constructClassInstance(workInProgress, Component, nextProps);
// [ts] mountClassInstance(workInProgress, Component, nextProps, renderExpirationTime);
// [ts] return finishClassComponent(null, workInProgress, Component, true, hasContext, renderExpirationTime);
// [ts] }
// [ts] fn mountIndeterminateComponent(_current: auto, workInProgress: auto, Component: auto, renderExpirationTime: auto) {
// [ts] if (_current != null) {
// An indeterminate component only mounts if it suspended inside a non-
// concurrent tree, in an inconsistent state. We want to treat it like
// a new mount, even though an empty version of it already committed.
// Disconnect the alternate pointers.
// [ts] _current.alternate = null;
// [ts] workInProgress.alternate = null; // Since this is conceptually a new fiber, schedule a Placement effect
// [ts] workInProgress.effectTag |= Placement;
// [ts] }
// [ts] let props = workInProgress.pendingProps;
// [ts] let context;
// [ts] {
// [ts] let unmaskedContext = getUnmaskedContext(workInProgress, Component, false);
// [ts] context = getMaskedContext(workInProgress, unmaskedContext);
// [ts] }
// [ts] prepareToReadContext(workInProgress, renderExpirationTime);
// [ts] let value;
// [ts] {
// [ts] if (Component.prototype && @typeName(Component.prototype.render) == "function") {
// [ts] let componentName = getComponentName(Component) || "Unknown";
// [ts] if (!didWarnAboutBadClass[componentName]) {
// [ts] error("The <%s /> component appears to have a render method, but doesn't extend React.Component. " + "This is likely to cause errors. Change %s to extend React.Component instead.", componentName, componentName);
// [ts] didWarnAboutBadClass[componentName] = true;
// [ts] }
// [ts] }
// [ts] if (workInProgress.mode & StrictMode) {
// [ts] ReactStrictModeWarnings.recordLegacyContextWarning(workInProgress, null);
// [ts] }
// [ts] setIsRendering(true);
// [ts] ReactCurrentOwner$1.current = workInProgress;
// [ts] value = renderWithHooks(null, workInProgress, Component, props, context, renderExpirationTime);
// [ts] setIsRendering(false);
// [ts] } // React DevTools reads this flag.
// [ts] workInProgress.effectTag |= PerformedWork;
// [ts] if (@typeName(value) == "object" && value != null && @typeName(value.render) == "function" && value.$$typeof == null) {
// [ts] {
// [ts] let _componentName = getComponentName(Component) || "Unknown";
// [ts] if (!didWarnAboutModulePatternComponent[_componentName]) {
// [ts] error("The <%s /> component appears to be a function component that returns a class instance. " + "Change %s to a class that extends React.Component instead. " + "If you can't use a class try assigning the prototype on the function as a workaround. " + "`%s.prototype = React.Component.prototype`. Don't use an arrow function since it " + "cannot be called with `new` by React.", _componentName, _componentName, _componentName);
// [ts] didWarnAboutModulePatternComponent[_componentName] = true;
// [ts] }
// [ts] } // Proceed under the assumption that this is a class instance
// [ts] workInProgress.tag = ClassComponent; // Throw out any hooks that were used.
// [ts] workInProgress.memoizedState = null;
// [ts] workInProgress.updateQueue = null; // Push context providers early to prevent context stack mismatches.
// During mounting we don't know the child context yet as the instance doesn't exist.
// We will invalidate the child context in finishClassComponent() right after rendering.
// [ts] let hasContext = false;
// [ts] if (isContextProvider(Component)) {
// [ts] hasContext = true;
// [ts] pushContextProvider(workInProgress);
// [ts] } else {
// [ts] hasContext = false;
// [ts] }
// [ts] workInProgress.memoizedState = if (value.state != null && value.state != null) value.state else null;
// [ts] initializeUpdateQueue(workInProgress);
// [ts] let getDerivedStateFromProps = Component.getDerivedStateFromProps;
// [ts] if (@typeName(getDerivedStateFromProps) == "function") {
// [ts] applyDerivedStateFromProps(workInProgress, Component, getDerivedStateFromProps, props);
// [ts] }
// [ts] adoptClassInstance(workInProgress, value);
// [ts] mountClassInstance(workInProgress, Component, props, renderExpirationTime);
// [ts] return finishClassComponent(null, workInProgress, Component, true, hasContext, renderExpirationTime);
// [ts] } else {
// Proceed under the assumption that this is a function component
// [ts] workInProgress.tag = FunctionComponent;
// [ts] {
// [ts] if ( workInProgress.mode & StrictMode) {
// Only double-render components with Hooks
// [ts] if (workInProgress.memoizedState != null) {
// [ts] value = renderWithHooks(null, workInProgress, Component, props, context, renderExpirationTime);
// [ts] }
// [ts] }
// [ts] }
// [ts] reconcileChildren(null, workInProgress, value, renderExpirationTime);
// [ts] {
// [ts] validateFunctionComponentInDev(workInProgress, Component);
// [ts] }
// [ts] return workInProgress.child;
// [ts] }
// [ts] }
// [ts] fn validateFunctionComponentInDev(workInProgress: auto, Component: auto) {
// [ts] {
// [ts] if (Component) {
// [ts] if (Component.childContextTypes) {
// [ts] error("%s(...): childContextTypes cannot be defined on a function component.", Component.displayName || Component.name || "Component");
// [ts] }
// [ts] }
// [ts] if (workInProgress.ref != null) {
// [ts] let info = "";
// [ts] let ownerName = getCurrentFiberOwnerNameInDevOrNull();
// [ts] if (ownerName) {
// [ts] info += "\n\nCheck the render method of `" + ownerName + "`.";
// [ts] }
// [ts] let warningKey = ownerName || workInProgress._debugID || "";
// [ts] let debugSource = workInProgress._debugSource;
// [ts] if (debugSource) {
// [ts] warningKey = debugSource.fileName + ":" + debugSource.lineNumber;
// [ts] }
// [ts] if (!didWarnAboutFunctionRefs[warningKey]) {
// [ts] didWarnAboutFunctionRefs[warningKey] = true;
// [ts] error("Function components cannot be given refs. " + "Attempts to access this ref will fail. " + "Did you mean to use React.forwardRef()?%s", info);
// [ts] }
// [ts] }
// [ts] if (@typeName(Component.getDerivedStateFromProps) == "function") {
// [ts] let _componentName2 = getComponentName(Component) || "Unknown";
// [ts] if (!didWarnAboutGetDerivedStateOnFunctionComponent[_componentName2]) {
// [ts] error("%s: Function components do not support getDerivedStateFromProps.", _componentName2);
// [ts] didWarnAboutGetDerivedStateOnFunctionComponent[_componentName2] = true;
// [ts] }
// [ts] }
// [ts] if (@typeName(Component.contextType) == "object" && Component.contextType != null) {
// [ts] let _componentName3 = getComponentName(Component) || "Unknown";
// [ts] if (!didWarnAboutContextTypeOnFunctionComponent[_componentName3]) {
// [ts] error("%s: Function components do not support contextType.", _componentName3);
// [ts] didWarnAboutContextTypeOnFunctionComponent[_componentName3] = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let SUSPENDED_MARKER = {
// [ts] dehydrated: null,
// [ts] retryTime: NoWork
// [ts] };
// [ts] fn shouldRemainOnFallback(suspenseContext: auto, current: auto, workInProgress: auto) {
// If the context is telling us that we should show a fallback, and we're not
// already showing content, then we should show the fallback instead.
// [ts] return hasSuspenseContext(suspenseContext, ForceSuspenseFallback) && (current == null || current.memoizedState != null);
// [ts] }
// [ts] fn updateSuspenseComponent(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] let mode = workInProgress.mode;
// [ts] let nextProps = workInProgress.pendingProps; // This is used by DevTools to force a boundary to suspend.
// [ts] {
// [ts] if (shouldSuspend(workInProgress)) {
// [ts] workInProgress.effectTag |= DidCapture;
// [ts] }
// [ts] }
// [ts] let suspenseContext = suspenseStackCursor.current;
// [ts] let nextDidTimeout = false;
// [ts] let didSuspend = (workInProgress.effectTag & DidCapture) != NoEffect;
// [ts] if (didSuspend || shouldRemainOnFallback(suspenseContext, current)) {
// Something in this boundary's subtree already suspended. Switch to
// rendering the fallback children.
// [ts] nextDidTimeout = true;
// [ts] workInProgress.effectTag &= ~DidCapture;
// [ts] } else {
// Attempting the main content
// [ts] if (current == null || current.memoizedState != null) {
// This is a new mount or this boundary is already showing a fallback state.
// Mark this subtree context as having at least one invisible parent that could
// handle the fallback state.
// Boundaries without fallbacks or should be avoided are not considered since
// they cannot handle preferred fallback states.
// [ts] if (nextProps.fallback != null && nextProps.unstable_avoidThisFallback != true) {
// [ts] suspenseContext = addSubtreeSuspenseContext(suspenseContext, InvisibleParentSuspenseContext);
// [ts] }
// [ts] }
// [ts] }
// [ts] suspenseContext = setDefaultShallowSuspenseContext(suspenseContext);
// [ts] pushSuspenseContext(workInProgress, suspenseContext); // This next part is a bit confusing. If the children timeout, we switch to
// showing the fallback children in place of the "primary" children.
// However, we don't want to delete the primary children because then their
// state will be lost (both the React state and the host state, e.g.
// uncontrolled form inputs). Instead we keep them mounted and hide them.
// Both the fallback children AND the primary children are rendered at the
// same time. Once the primary children are un-suspended, we can delete
// the fallback children — don't need to preserve their state.
//
// The two sets of children are siblings in the host environment, but
// semantically, for purposes of reconciliation, they are two separate sets.
// So we store them using two fragment fibers.
//
// However, we want to avoid allocating extra fibers for every placeholder.
// They're only necessary when the children time out, because that's the
// only time when both sets are mounted.
//
// So, the extra fragment fibers are only used if the children time out.
// Otherwise, we render the primary children directly. This requires some
// custom reconciliation logic to preserve the state of the primary
// children. It's essentially a very basic form of re-parenting.
// [ts] if (current == null) {
// If we're currently hydrating, try to hydrate this boundary.
// But only if this has a fallback.
// [ts] if (nextProps.fallback != null) {
// [ts] tryToClaimNextHydratableInstance(workInProgress); // This could've been a dehydrated suspense component.
// [ts] } // This is the initial mount. This branch is pretty simple because there's
// no previous state that needs to be preserved.
// [ts] if (nextDidTimeout) {
// Mount separate fragments for primary and fallback children.
// [ts] let nextFallbackChildren = nextProps.fallback;
// [ts] let primaryChildFragment = createFiberFromFragment(null, mode, NoWork, null);
// [ts] primaryChildFragment.return = workInProgress;
// [ts] if ((workInProgress.mode & BlockingMode) == NoMode) {
// Outside of blocking mode, we commit the effects from the
// partially completed, timed-out tree, too.
// [ts] let progressedState = workInProgress.memoizedState;
// [ts] let progressedPrimaryChild = if (progressedState != null) workInProgress.child.child else workInProgress.child;
// [ts] primaryChildFragment.child = progressedPrimaryChild;
// [ts] let progressedChild = progressedPrimaryChild;
// [ts] while (progressedChild != null) {
// [ts] progressedChild.return = primaryChildFragment;
// [ts] progressedChild = progressedChild.sibling;
// [ts] }
// [ts] }
// [ts] let fallbackChildFragment = createFiberFromFragment(nextFallbackChildren, mode, renderExpirationTime, null);
// [ts] fallbackChildFragment.return = workInProgress;
// [ts] primaryChildFragment.sibling = fallbackChildFragment; // Skip the primary children, and continue working on the
// fallback children.
// [ts] workInProgress.memoizedState = SUSPENDED_MARKER;
// [ts] workInProgress.child = primaryChildFragment;
// [ts] return fallbackChildFragment;
// [ts] } else {
// Mount the primary children without an intermediate fragment fiber.
// [ts] let nextPrimaryChildren = nextProps.children;
// [ts] workInProgress.memoizedState = null;
// [ts] return workInProgress.child = mountChildFibers(workInProgress, null, nextPrimaryChildren, renderExpirationTime);
// [ts] }
// [ts] } else {
// This is an update. This branch is more complicated because we need to
// ensure the state of the primary children is preserved.
// [ts] let prevState = current.memoizedState;
// [ts] if (prevState != null) {
// wrapped in a fragment fiber.
// [ts] let currentPrimaryChildFragment = current.child;
// [ts] let currentFallbackChildFragment = currentPrimaryChildFragment.sibling;
// [ts] if (nextDidTimeout) {
// Still timed out. Reuse the current primary children by cloning
// its fragment. We're going to skip over these entirely.
// [ts] let _nextFallbackChildren2 = nextProps.fallback;
// [ts] let _primaryChildFragment2 = createWorkInProgress(currentPrimaryChildFragment, currentPrimaryChildFragment.pendingProps);
// [ts] _primaryChildFragment2.return = workInProgress;
// [ts] if ((workInProgress.mode & BlockingMode) == NoMode) {
// Outside of blocking mode, we commit the effects from the
// partially completed, timed-out tree, too.
// [ts] let _progressedState = workInProgress.memoizedState;
// [ts] let _progressedPrimaryChild = if (_progressedState != null) workInProgress.child.child else workInProgress.child;
// [ts] if (_progressedPrimaryChild != currentPrimaryChildFragment.child) {
// [ts] _primaryChildFragment2.child = _progressedPrimaryChild;
// [ts] let _progressedChild2 = _progressedPrimaryChild;
// [ts] while (_progressedChild2 != null) {
// [ts] _progressedChild2.return = _primaryChildFragment2;
// [ts] _progressedChild2 = _progressedChild2.sibling;
// [ts] }
// [ts] }
// [ts] } // Because primaryChildFragment is a new fiber that we're inserting as the
// parent of a new tree, we need to set its treeBaseDuration.
// [ts] if ( workInProgress.mode & ProfileMode) {
// treeBaseDuration is the sum of all the child tree base durations.
// [ts] let _treeBaseDuration = 0;
// [ts] let _hiddenChild = _primaryChildFragment2.child;
// [ts] while (_hiddenChild != null) {
// [ts] _treeBaseDuration += _hiddenChild.treeBaseDuration;
// [ts] _hiddenChild = _hiddenChild.sibling;
// [ts] }
// [ts] _primaryChildFragment2.treeBaseDuration = _treeBaseDuration;
// [ts] } // Clone the fallback child fragment, too. These we'll continue
// working on.
// [ts] let _fallbackChildFragment2 = createWorkInProgress(currentFallbackChildFragment, _nextFallbackChildren2);
// [ts] _fallbackChildFragment2.return = workInProgress;
// [ts] _primaryChildFragment2.sibling = _fallbackChildFragment2;
// [ts] _primaryChildFragment2.childExpirationTime = NoWork; // Skip the primary children, and continue working on the
// fallback children.
// [ts] workInProgress.memoizedState = SUSPENDED_MARKER;
// [ts] workInProgress.child = _primaryChildFragment2;
// [ts] return _fallbackChildFragment2;
// [ts] } else {
// No longer suspended. Switch back to showing the primary children,
// and remove the intermediate fragment fiber.
// [ts] let _nextPrimaryChildren = nextProps.children;
// [ts] let currentPrimaryChild = currentPrimaryChildFragment.child;
// [ts] let primaryChild = reconcileChildFibers(workInProgress, currentPrimaryChild, _nextPrimaryChildren, renderExpirationTime); // If this render doesn't suspend, we need to delete the fallback
// children. Wait until the complete phase, after we've confirmed the
// fallback is no longer needed.
// TODO: Would it be better to store the fallback fragment on
// the stateNode?
// Continue rendering the children, like we normally do.
// [ts] workInProgress.memoizedState = null;
// [ts] return workInProgress.child = primaryChild;
// [ts] }
// [ts] } else {
// The current tree has not already timed out. That means the primary
// children are not wrapped in a fragment fiber.
// [ts] let _currentPrimaryChild = current.child;
// [ts] if (nextDidTimeout) {
// Timed out. Wrap the children in a fragment fiber to keep them
// separate from the fallback children.
// [ts] let _nextFallbackChildren3 = nextProps.fallback;
// [ts] let _primaryChildFragment3 = createFiberFromFragment( // It shouldn't matter what the pending props are because we aren't
// going to render this fragment.
// [ts] null, mode, NoWork, null);
// [ts] _primaryChildFragment3.return = workInProgress;
// [ts] _primaryChildFragment3.child = _currentPrimaryChild;
// [ts] if (_currentPrimaryChild != null) {
// [ts] _currentPrimaryChild.return = _primaryChildFragment3;
// [ts] } // Even though we're creating a new fiber, there are no new children,
// because we're reusing an already mounted tree. So we don't need to
// schedule a placement.
// primaryChildFragment.effectTag |= Placement;
// [ts] if ((workInProgress.mode & BlockingMode) == NoMode) {
// Outside of blocking mode, we commit the effects from the
// partially completed, timed-out tree, too.
// [ts] let _progressedState2 = workInProgress.memoizedState;
// [ts] let _progressedPrimaryChild2 = if (_progressedState2 != null) workInProgress.child.child else workInProgress.child;
// [ts] _primaryChildFragment3.child = _progressedPrimaryChild2;
// [ts] let _progressedChild3 = _progressedPrimaryChild2;
// [ts] while (_progressedChild3 != null) {
// [ts] _progressedChild3.return = _primaryChildFragment3;
// [ts] _progressedChild3 = _progressedChild3.sibling;
// [ts] }
// [ts] } // Because primaryChildFragment is a new fiber that we're inserting as the
// parent of a new tree, we need to set its treeBaseDuration.
// [ts] if ( workInProgress.mode & ProfileMode) {
// treeBaseDuration is the sum of all the child tree base durations.
// [ts] let _treeBaseDuration2 = 0;
// [ts] let _hiddenChild2 = _primaryChildFragment3.child;
// [ts] while (_hiddenChild2 != null) {
// [ts] _treeBaseDuration2 += _hiddenChild2.treeBaseDuration;
// [ts] _hiddenChild2 = _hiddenChild2.sibling;
// [ts] }
// [ts] _primaryChildFragment3.treeBaseDuration = _treeBaseDuration2;
// [ts] } // Create a fragment from the fallback children, too.
// [ts] let _fallbackChildFragment3 = createFiberFromFragment(_nextFallbackChildren3, mode, renderExpirationTime, null);
// [ts] _fallbackChildFragment3.return = workInProgress;
// [ts] _primaryChildFragment3.sibling = _fallbackChildFragment3;
// [ts] _fallbackChildFragment3.effectTag |= Placement;
// [ts] _primaryChildFragment3.childExpirationTime = NoWork; // Skip the primary children, and continue working on the
// fallback children.
// [ts] workInProgress.memoizedState = SUSPENDED_MARKER;
// [ts] workInProgress.child = _primaryChildFragment3;
// [ts] return _fallbackChildFragment3;
// [ts] } else {
// Still haven't timed out. Continue rendering the children, like we
// normally do.
// [ts] workInProgress.memoizedState = null;
// [ts] let _nextPrimaryChildren2 = nextProps.children;
// [ts] return workInProgress.child = reconcileChildFibers(workInProgress, _currentPrimaryChild, _nextPrimaryChildren2, renderExpirationTime);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn scheduleWorkOnFiber(fiber: auto, renderExpirationTime: auto) {
// [ts] if (fiber.expirationTime < renderExpirationTime) {
// [ts] fiber.expirationTime = renderExpirationTime;
// [ts] }
// [ts] let alternate = fiber.alternate;
// [ts] if (alternate != null && alternate.expirationTime < renderExpirationTime) {
// [ts] alternate.expirationTime = renderExpirationTime;
// [ts] }
// [ts] scheduleWorkOnParentPath(fiber.return, renderExpirationTime);
// [ts] }
// [ts] fn propagateSuspenseContextChange(workInProgress: auto, firstChild: auto, renderExpirationTime: auto) {
// Mark any Suspense boundaries with fallbacks as having work to do.
// If they were previously forced into fallbacks, they may now be able
// to unblock.
// [ts] let node = firstChild;
// [ts] while (node != null) {
// [ts] if (node.tag == SuspenseComponent) {
// [ts] let state = node.memoizedState;
// [ts] if (state != null) {
// [ts] scheduleWorkOnFiber(node, renderExpirationTime);
// [ts] }
// [ts] } else if (node.tag == SuspenseListComponent) {
// If the tail is hidden there might not be an Suspense boundaries
// to schedule work on. In this case we have to schedule it on the
// list itself.
// We don't have to traverse to the children of the list since
// the list will propagate the change when it rerenders.
// [ts] scheduleWorkOnFiber(node, renderExpirationTime);
// [ts] } else if (node.child != null) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] if (node == workInProgress) {
// [ts] return;
// [ts] }
// [ts] while (node.sibling == null) {
// [ts] if (node.return == null || node.return == workInProgress) {
// [ts] return;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] }
// [ts] }
// [ts] fn findLastContentRow(firstChild: auto) {
// This is going to find the last row among these children that is already
// showing content on the screen, as opposed to being in fallback state or
// new. If a row has multiple Suspense boundaries, any of them being in the
// fallback state, counts as the whole row being in a fallback state.
// Note that the "rows" will be workInProgress, but any nested children
// will still be current since we haven't rendered them yet. The mounted
// order may not be the same as the new order. We use the new order.
// [ts] let row = firstChild;
// [ts] let lastContentRow = null;
// [ts] while (row != null) {
// [ts] let currentRow = row.alternate; // New rows can't be content rows.
// [ts] if (currentRow != null && findFirstSuspended(currentRow) == null) {
// [ts] lastContentRow = row;
// [ts] }
// [ts] row = row.sibling;
// [ts] }
// [ts] return lastContentRow;
// [ts] }
// [ts] fn validateRevealOrder(revealOrder: auto) {
// [ts] {
// [ts] if (revealOrder != null && revealOrder != "forwards" && revealOrder != "backwards" && revealOrder != "together" && !didWarnAboutRevealOrder[revealOrder]) {
// [ts] didWarnAboutRevealOrder[revealOrder] = true;
// [ts] if (@typeName(revealOrder) == "string") {
// [ts] switch (revealOrder.toLowerCase()) {
// [ts] "together", "forwards", "backwards" => {
// [ts] {
// [ts] error("\"%s\" is not a valid value for revealOrder on <SuspenseList />. " + "Use lowercase \"%s\" instead.", revealOrder, revealOrder.toLowerCase());
// [ts] break;
// [ts] }
// [ts] },
// [ts] "forward", "backward" => {
// [ts] {
// [ts] error("\"%s\" is not a valid value for revealOrder on <SuspenseList />. " + "React uses the -s suffix in the spelling. Use \"%ss\" instead.", revealOrder, revealOrder.toLowerCase());
// [ts] break;
// [ts] }
// [ts] },
// [ts] else => {
// [ts] error("\"%s\" is not a supported revealOrder on <SuspenseList />. " + "Did you mean \"together\", \"forwards\" or \"backwards\"?", revealOrder);
// break;
// [ts] }
// [ts] }
// [ts] } else {
// [ts] error("%s is not a supported value for revealOrder on <SuspenseList />. " + "Did you mean \"together\", \"forwards\" or \"backwards\"?", revealOrder);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn validateTailOptions(tailMode: auto, revealOrder: auto) {
// [ts] {
// [ts] if (tailMode != null && !didWarnAboutTailOptions[tailMode]) {
// [ts] if (tailMode != "collapsed" && tailMode != "hidden") {
// [ts] didWarnAboutTailOptions[tailMode] = true;
// [ts] error("\"%s\" is not a supported value for tail on <SuspenseList />. " + "Did you mean \"collapsed\" or \"hidden\"?", tailMode);
// [ts] } else if (revealOrder != "forwards" && revealOrder != "backwards") {
// [ts] didWarnAboutTailOptions[tailMode] = true;
// [ts] error("<SuspenseList tail=\"%s\" /> is only valid if revealOrder is " + "\"forwards\" or \"backwards\". " + "Did you mean to specify revealOrder=\"forwards\"?", tailMode);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn validateSuspenseListNestedChild(childSlot: auto, index: auto) {
// [ts] {
// [ts] let isArray = Array.isArray(childSlot);
// [ts] let isIterable = !isArray && @typeName(getIteratorFn)(childSlot) == "function";
// [ts] if (isArray || isIterable) {
// [ts] let type = if (isArray) "array" else "iterable";
// [ts] error("A nested %s was passed to row #%s in <SuspenseList />. Wrap it in " + "an additional SuspenseList to configure its revealOrder: " + "<SuspenseList revealOrder=...> ... " + "<SuspenseList revealOrder=...>{%s}</SuspenseList> ... " + "</SuspenseList>", type, index, type);
// [ts] return false;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn validateSuspenseListChildren(children: auto, revealOrder: auto) {
// [ts] {
// [ts] if ((revealOrder == "forwards" || revealOrder == "backwards") && children != null && children != null && children != false) {
// [ts] if (Array.isArray(children)) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] if (!validateSuspenseListNestedChild(children[i], i)) {
// [ts] return;
// [ts] }
// [ts] }
// [ts] } else {
// [ts] let iteratorFn = getIteratorFn(children);
// [ts] if (@typeName(iteratorFn) == "function") {
// [ts] let childrenIterator = iteratorFn.call(children);
// [ts] if (childrenIterator) {
// [ts] let step = childrenIterator.next();
// [ts] let _i = 0;
// [ts] while (!step.done) { // [cap-migrate] for-step: step = childrenIterator.next()
// [ts] if (!validateSuspenseListNestedChild(step.value, _i)) {
// [ts] return;
// [ts] }
// [ts] _i++;
// [ts] }
// [ts] }
// [ts] } else {
// [ts] error("A single row was passed to a <SuspenseList revealOrder=\"%s\" />. " + "This is not useful since it needs multiple rows. " + "Did you mean to pass multiple children or an array?", revealOrder);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn initSuspenseListRenderState(workInProgress: auto, isBackwards: auto, tail: auto, lastContentRow: auto, tailMode: auto, lastEffectBeforeRendering: auto) {
// [ts] let renderState = workInProgress.memoizedState;
// [ts] if (renderState == null) {
// [ts] workInProgress.memoizedState = {
// [ts] isBackwards: isBackwards,
// [ts] rendering: null,
// [ts] renderingStartTime: 0,
// [ts] last: lastContentRow,
// [ts] tail: tail,
// [ts] tailExpiration: 0,
// [ts] tailMode: tailMode,
// [ts] lastEffect: lastEffectBeforeRendering
// [ts] };
// [ts] } else {
// We can reuse the existing object from previous renders.
// [ts] renderState.isBackwards = isBackwards;
// [ts] renderState.rendering = null;
// [ts] renderState.renderingStartTime = 0;
// [ts] renderState.last = lastContentRow;
// [ts] renderState.tail = tail;
// [ts] renderState.tailExpiration = 0;
// [ts] renderState.tailMode = tailMode;
// [ts] renderState.lastEffect = lastEffectBeforeRendering;
// [ts] }
// [ts] } // This can end up rendering this component multiple passes.
// The first pass splits the children fibers into two sets. A head and tail.
// We first render the head. If anything is in fallback state, we do another
// pass through beginWork to rerender all children (including the tail) with
// the force suspend context. If the first render didn't have anything in
// in fallback state. Then we render each row in the tail one-by-one.
// That happens in the completeWork phase without going back to beginWork.
// [ts] fn updateSuspenseListComponent(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] let nextProps = workInProgress.pendingProps;
// [ts] let revealOrder = nextProps.revealOrder;
// [ts] let tailMode = nextProps.tail;
// [ts] let newChildren = nextProps.children;
// [ts] validateRevealOrder(revealOrder);
// [ts] validateTailOptions(tailMode, revealOrder);
// [ts] validateSuspenseListChildren(newChildren, revealOrder);
// [ts] reconcileChildren(current, workInProgress, newChildren, renderExpirationTime);
// [ts] let suspenseContext = suspenseStackCursor.current;
// [ts] let shouldForceFallback = hasSuspenseContext(suspenseContext, ForceSuspenseFallback);
// [ts] if (shouldForceFallback) {
// [ts] suspenseContext = setShallowSuspenseContext(suspenseContext, ForceSuspenseFallback);
// [ts] workInProgress.effectTag |= DidCapture;
// [ts] } else {
// [ts] let didSuspendBefore = current != null && (current.effectTag & DidCapture) != NoEffect;
// [ts] if (didSuspendBefore) {
// If we previously forced a fallback, we need to schedule work
// on any nested boundaries to let them know to try to render
// again. This is the same as context updating.
// [ts] propagateSuspenseContextChange(workInProgress, workInProgress.child, renderExpirationTime);
// [ts] }
// [ts] suspenseContext = setDefaultShallowSuspenseContext(suspenseContext);
// [ts] }
// [ts] pushSuspenseContext(workInProgress, suspenseContext);
// [ts] if ((workInProgress.mode & BlockingMode) == NoMode) {
// Outside of blocking mode, SuspenseList doesn't work so we just
// use make it a noop by treating it as the default revealOrder.
// [ts] workInProgress.memoizedState = null;
// [ts] } else {
// [ts] switch (revealOrder) {
// [ts] "forwards" => {
// [ts] {
// [ts] let lastContentRow = findLastContentRow(workInProgress.child);
// [ts] let tail;
// [ts] if (lastContentRow == null) {
// The whole list is part of the tail.
// TODO: We could fast path by just rendering the tail now.
// [ts] tail = workInProgress.child;
// [ts] workInProgress.child = null;
// [ts] } else {
// Disconnect the tail rows after the content row.
// We're going to render them separately later.
// [ts] tail = lastContentRow.sibling;
// [ts] lastContentRow.sibling = null;
// [ts] }
// [ts] initSuspenseListRenderState(workInProgress, false, // isBackwards
// [ts] tail, lastContentRow, tailMode, workInProgress.lastEffect);
// [ts] break;
// [ts] }
// [ts] },
// [ts] "backwards" => {
// [ts] {
// We're going to find the first row that has existing content.
// At the same time we're going to reverse the list of everything
// we pass in the meantime. That's going to be our tail in reverse
// order.
// [ts] let _tail = null;
// [ts] let row = workInProgress.child;
// [ts] workInProgress.child = null;
// [ts] while (row != null) {
// [ts] let currentRow = row.alternate; // New rows can't be content rows.
// [ts] if (currentRow != null && findFirstSuspended(currentRow) == null) {
// This is the beginning of the main content.
// [ts] workInProgress.child = row;
// [ts] break;
// [ts] }
// [ts] let nextRow = row.sibling;
// [ts] row.sibling = _tail;
// [ts] _tail = row;
// [ts] row = nextRow;
// [ts] } // TODO: If workInProgress.child is null, we can continue on the tail immediately.
// [ts] initSuspenseListRenderState(workInProgress, true, // isBackwards
// [ts] _tail, null, // last
// [ts] tailMode, workInProgress.lastEffect);
// [ts] break;
// [ts] }
// [ts] },
// [ts] "together" => {
// [ts] {
// [ts] initSuspenseListRenderState(workInProgress, false, // isBackwards
// [ts] null, // tail
// [ts] null, // last
// [ts] null, workInProgress.lastEffect);
// [ts] break;
// [ts] }
// [ts] },
// [ts] else => {
// [ts] {
// The default reveal order is the same as not having
// a boundary.
// [ts] workInProgress.memoizedState = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn updatePortalComponent(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] pushHostContainer(workInProgress, workInProgress.stateNode.containerInfo);
// [ts] let nextChildren = workInProgress.pendingProps;
// [ts] if (current == null) {
// Portals are special because we don't append the children during mount
// but at commit. Therefore we need to track insertions which the normal
// flow doesn't do during mount. This doesn't happen at the root because
// the root always starts with a "current" with a null child.
// TODO: Consider unifying this with how the root works.
// [ts] workInProgress.child = reconcileChildFibers(workInProgress, null, nextChildren, renderExpirationTime);
// [ts] } else {
// [ts] reconcileChildren(current, workInProgress, nextChildren, renderExpirationTime);
// [ts] }
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn updateContextProvider(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] let providerType = workInProgress.type;
// [ts] let context = providerType._context;
// [ts] let newProps = workInProgress.pendingProps;
// [ts] let oldProps = workInProgress.memoizedProps;
// [ts] let newValue = newProps.value;
// [ts] {
// [ts] let providerPropTypes = workInProgress.type.propTypes;
// [ts] if (providerPropTypes) {
// [ts] checkPropTypes_1(providerPropTypes, newProps, "prop", "Context.Provider", getCurrentFiberStackInDev);
// [ts] }
// [ts] }
// [ts] pushProvider(workInProgress, newValue);
// [ts] if (oldProps != null) {
// [ts] let oldValue = oldProps.value;
// [ts] let changedBits = calculateChangedBits(context, newValue, oldValue);
// [ts] if (changedBits == 0) {
// No change. Bailout early if children are the same.
// [ts] if (oldProps.children == newProps.children && !hasContextChanged()) {
// [ts] return bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] }
// [ts] } else {
// The context value changed. Search for matching consumers and schedule
// them to update.
// [ts] propagateContextChange(workInProgress, context, changedBits, renderExpirationTime);
// [ts] }
// [ts] }
// [ts] let newChildren = newProps.children;
// [ts] reconcileChildren(current, workInProgress, newChildren, renderExpirationTime);
// [ts] return workInProgress.child;
// [ts] }
// [ts] let hasWarnedAboutUsingContextAsConsumer = false;
// [ts] fn updateContextConsumer(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] let context = workInProgress.type; // The logic below for Context differs depending on PROD or DEV mode. In
// DEV mode, we create a separate object for Context.Consumer that acts
// like a proxy to Context. This proxy object adds unnecessary code in PROD
// so we use the old behaviour (Context.Consumer references Context) to
// reduce size and overhead. The separate object references context via
// a property called "_context", which also gives us the ability to check
// in DEV mode if this property exists or not and warn if it does not.
// [ts] {
// [ts] if (context._context == null) {
// This may be because it's a Context (rather than a Consumer).
// Or it may be because it's older React where they're the same thing.
// We only want to warn if we're sure it's a new React.
// [ts] if (context != context.Consumer) {
// [ts] if (!hasWarnedAboutUsingContextAsConsumer) {
// [ts] hasWarnedAboutUsingContextAsConsumer = true;
// [ts] error("Rendering <Context> directly is not supported and will be removed in " + "a future major release. Did you mean to render <Context.Consumer> instead?");
// [ts] }
// [ts] }
// [ts] } else {
// [ts] context = context._context;
// [ts] }
// [ts] }
// [ts] let newProps = workInProgress.pendingProps;
// [ts] let render = newProps.children;
// [ts] {
// [ts] if (@typeName(render) != "function") {
// [ts] error("A context consumer was rendered with multiple children, or a child " + "that isn't a function. A context consumer expects a single child " + "that is a function. If you did pass a function, make sure there " + "is no trailing or leading whitespace around it.");
// [ts] }
// [ts] }
// [ts] prepareToReadContext(workInProgress, renderExpirationTime);
// [ts] let newValue = readContext(context, newProps.unstable_observedBits);
// [ts] let newChildren;
// [ts] {
// [ts] ReactCurrentOwner$1.current = workInProgress;
// [ts] setIsRendering(true);
// [ts] newChildren = render(newValue);
// [ts] setIsRendering(false);
// [ts] } // React DevTools reads this flag.
// [ts] workInProgress.effectTag |= PerformedWork;
// [ts] reconcileChildren(current, workInProgress, newChildren, renderExpirationTime);
// [ts] return workInProgress.child;
// [ts] }
// [ts] fn markWorkInProgressReceivedUpdate() {
// [ts] didReceiveUpdate = true;
// [ts] }
// [ts] fn bailoutOnAlreadyFinishedWork(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] cancelWorkTimer(workInProgress);
// [ts] if (current != null) {
// Reuse previous dependencies
// [ts] workInProgress.dependencies = current.dependencies;
// [ts] }
// [ts] {
// Don't update "base" render times for bailouts.
// [ts] stopProfilerTimerIfRunning();
// [ts] }
// [ts] let updateExpirationTime = workInProgress.expirationTime;
// [ts] if (updateExpirationTime != NoWork) {
// [ts] markUnprocessedUpdateTime(updateExpirationTime);
// [ts] } // Check if the children have any pending work.
// [ts] let childExpirationTime = workInProgress.childExpirationTime;
// [ts] if (childExpirationTime < renderExpirationTime) {
// The children don't have any work either. We can skip them.
// TODO: Once we add back resuming, we should check if the children are
// a work-in-progress set. If so, we need to transfer their effects.
// [ts] return null;
// [ts] } else {
// This fiber doesn't have work, but its subtree does. Clone the child
// fibers and continue.
// [ts] cloneChildFibers(current, workInProgress);
// [ts] return workInProgress.child;
// [ts] }
// [ts] }
// [ts] fn remountFiber(current: auto, oldWorkInProgress: auto, newWorkInProgress: auto) {
// [ts] {
// [ts] let returnFiber = oldWorkInProgress.return;
// [ts] if (returnFiber == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Cannot swap the root fiber.');)
// [ts] } // Disconnect from the old current.
// It will get deleted.
// [ts] current.alternate = null;
// [ts] oldWorkInProgress.alternate = null; // Connect to the new tree.
// [ts] newWorkInProgress.index = oldWorkInProgress.index;
// [ts] newWorkInProgress.sibling = oldWorkInProgress.sibling;
// [ts] newWorkInProgress.return = oldWorkInProgress.return;
// [ts] newWorkInProgress.ref = oldWorkInProgress.ref; // Replace the child/sibling pointers above it.
// [ts] if (oldWorkInProgress == returnFiber.child) {
// [ts] returnFiber.child = newWorkInProgress;
// [ts] } else {
// [ts] let prevSibling = returnFiber.child;
// [ts] if (prevSibling == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Expected parent to have a child.');)
// [ts] }
// [ts] while (prevSibling.sibling != oldWorkInProgress) {
// [ts] prevSibling = prevSibling.sibling;
// [ts] if (prevSibling == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Expected to find the previous sibling.');)
// [ts] }
// [ts] }
// [ts] prevSibling.sibling = newWorkInProgress;
// [ts] } // Delete the old fiber and place the new one.
// Since the old fiber is disconnected, we have to schedule it manually.
// [ts] let last = returnFiber.lastEffect;
// [ts] if (last != null) {
// [ts] last.nextEffect = current;
// [ts] returnFiber.lastEffect = current;
// [ts] } else {
// [ts] returnFiber.firstEffect = returnFiber.lastEffect = current;
// [ts] }
// [ts] current.nextEffect = null;
// [ts] current.effectTag = Deletion;
// [ts] newWorkInProgress.effectTag |= Placement; // Restart work from the new fiber.
// [ts] return newWorkInProgress;
// [ts] }
// [ts] }
// [ts] fn beginWork(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] let updateExpirationTime = workInProgress.expirationTime;
// [ts] {
// [ts] if (workInProgress._debugNeedsRemount && current != null) {
// This will restart the begin phase with a new fiber.
// [ts] return remountFiber(current, workInProgress, createFiberFromTypeAndProps(workInProgress.type, workInProgress.key, workInProgress.pendingProps, workInProgress._debugOwner || null, workInProgress.mode, workInProgress.expirationTime));
// [ts] }
// [ts] }
// [ts] if (current != null) {
// [ts] let oldProps = current.memoizedProps;
// [ts] let newProps = workInProgress.pendingProps;
// [ts] if (oldProps != newProps || hasContextChanged() || ( // Force a re-render if the implementation changed due to hot reload:
// [ts] workInProgress.type != current.type )) {
// If props or context changed, mark the fiber as having performed work.
// This may be unset if the props are determined to be equal later (memo).
// [ts] didReceiveUpdate = true;
// [ts] } else if (updateExpirationTime < renderExpirationTime) {
// [ts] didReceiveUpdate = false; // This fiber does not have any pending work. Bailout without entering
// the begin phase. There's still some bookkeeping we that needs to be done
// in this optimized path, mostly pushing stuff onto the stack.
// [ts] switch (workInProgress.tag) {
// [ts] HostRoot => {
// [ts] pushHostRootContext(workInProgress);
// [ts] resetHydrationState();
// break;
// [ts] },
// [ts] HostComponent => {
// [ts] pushHostContext(workInProgress);
// [ts] if (workInProgress.mode & ConcurrentMode && renderExpirationTime != Never && shouldDeprioritizeSubtree(workInProgress.type, newProps)) {
// [ts] {
// [ts] markSpawnedWork(Never);
// [ts] } // Schedule this fiber to re-render at offscreen priority. Then bailout.
// [ts] workInProgress.expirationTime = workInProgress.childExpirationTime = Never;
// [ts] return null;
// [ts] }
// break;
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] let Component = workInProgress.type;
// [ts] if (isContextProvider(Component)) {
// [ts] pushContextProvider(workInProgress);
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] HostPortal => {
// [ts] pushHostContainer(workInProgress, workInProgress.stateNode.containerInfo);
// break;
// [ts] },
// [ts] ContextProvider => {
// [ts] {
// [ts] let newValue = workInProgress.memoizedProps.value;
// [ts] pushProvider(workInProgress, newValue);
// [ts] break;
// [ts] }
// [ts] },
// [ts] Profiler => {
// [ts] {
// Profiler should only call onRender when one of its descendants actually rendered.
// [ts] let hasChildWork = workInProgress.childExpirationTime >= renderExpirationTime;
// [ts] if (hasChildWork) {
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] }
// break;
// [ts] },
// [ts] SuspenseComponent => {
// [ts] {
// [ts] let state = workInProgress.memoizedState;
// [ts] if (state != null) {
// whether to retry the primary children, or to skip over it and
// go straight to the fallback. Check the priority of the primary
// child fragment.
// [ts] let primaryChildFragment = workInProgress.child;
// [ts] let primaryChildExpirationTime = primaryChildFragment.childExpirationTime;
// [ts] if (primaryChildExpirationTime != NoWork && primaryChildExpirationTime >= renderExpirationTime) {
// The primary children have pending work. Use the normal path
// to attempt to render the primary children again.
// [ts] return updateSuspenseComponent(current, workInProgress, renderExpirationTime);
// [ts] } else {
// [ts] pushSuspenseContext(workInProgress, setDefaultShallowSuspenseContext(suspenseStackCursor.current)); // The primary children do not have pending work with sufficient
// priority. Bailout.
// [ts] let child = bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] if (child != null) {
// The fallback children have pending work. Skip over the
// primary children and work on the fallback.
// [ts] return child.sibling;
// [ts] } else {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] } else {
// [ts] pushSuspenseContext(workInProgress, setDefaultShallowSuspenseContext(suspenseStackCursor.current));
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] SuspenseListComponent => {
// [ts] {
// [ts] let didSuspendBefore = (current.effectTag & DidCapture) != NoEffect;
// [ts] let _hasChildWork = workInProgress.childExpirationTime >= renderExpirationTime;
// [ts] if (didSuspendBefore) {
// [ts] if (_hasChildWork) {
// If something was in fallback state last time, and we have all the
// same children then we're still in progressive loading state.
// Something might get unblocked by state updates or retries in the
// tree which will affect the tail. So we need to use the normal
// path to compute the correct tail.
// [ts] return updateSuspenseListComponent(current, workInProgress, renderExpirationTime);
// [ts] } // If none of the children had any work, that means that none of
// them got retried so they'll still be blocked in the same way
// as before. We can fast bail out.
// [ts] workInProgress.effectTag |= DidCapture;
// [ts] } // If nothing suspended before and we're rendering the same children,
// then the tail doesn't matter. Anything new that suspends will work
// in the "together" mode, so we can continue from the state we had.
// [ts] let renderState = workInProgress.memoizedState;
// [ts] if (renderState != null) {
// Reset to the "together" mode in case we've started a different
// update in the past but didn't complete it.
// [ts] renderState.rendering = null;
// [ts] renderState.tail = null;
// [ts] }
// [ts] pushSuspenseContext(workInProgress, suspenseStackCursor.current);
// [ts] if (_hasChildWork) {
// [ts] break;
// [ts] } else {
// If none of the children had any work, that means that none of
// them got retried so they'll still be blocked in the same way
// as before. We can fast bail out.
// [ts] return null;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return bailoutOnAlreadyFinishedWork(current, workInProgress, renderExpirationTime);
// [ts] } else {
// An update was scheduled on this fiber, but there are no new props
// nor legacy context. Set this to false. If an update queue or context
// consumer produces a changed value, it will set this to true. Otherwise,
// the component will assume the children have not changed and bail out.
// [ts] didReceiveUpdate = false;
// [ts] }
// [ts] } else {
// [ts] didReceiveUpdate = false;
// [ts] } // Before entering the begin phase, clear pending update priority.
// TODO: This assumes that we're about to evaluate the component and process
// the update queue. However, there's an exception: SimpleMemoComponent
// sometimes bails out later in the begin phase. This indicates that we should
// move this assignment out of the common path and into each branch.
// [ts] workInProgress.expirationTime = NoWork;
// [ts] switch (workInProgress.tag) {
// [ts] IndeterminateComponent => {
// [ts] {
// [ts] return mountIndeterminateComponent(current, workInProgress, workInProgress.type, renderExpirationTime);
// [ts] }
// [ts] },
// [ts] LazyComponent => {
// [ts] {
// [ts] let elementType = workInProgress.elementType;
// [ts] return mountLazyComponent(current, workInProgress, elementType, updateExpirationTime, renderExpirationTime);
// [ts] }
// [ts] },
// [ts] FunctionComponent => {
// [ts] {
// [ts] let _Component = workInProgress.type;
// [ts] let unresolvedProps = workInProgress.pendingProps;
// [ts] let resolvedProps = if (workInProgress.elementType == _Component) unresolvedProps else resolveDefaultProps(_Component, unresolvedProps);
// [ts] return updateFunctionComponent(current, workInProgress, _Component, resolvedProps, renderExpirationTime);
// [ts] }
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] let _Component2 = workInProgress.type;
// [ts] let _unresolvedProps = workInProgress.pendingProps;
// [ts] let _resolvedProps = if (workInProgress.elementType == _Component2) _unresolvedProps else resolveDefaultProps(_Component2, _unresolvedProps);
// [ts] return updateClassComponent(current, workInProgress, _Component2, _resolvedProps, renderExpirationTime);
// [ts] }
// [ts] },
// [ts] HostRoot => {
// [ts] return updateHostRoot(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] HostComponent => {
// [ts] return updateHostComponent(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] HostText => {
// [ts] return updateHostText(current, workInProgress);
// [ts] },
// [ts] SuspenseComponent => {
// [ts] return updateSuspenseComponent(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] HostPortal => {
// [ts] return updatePortalComponent(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] ForwardRef => {
// [ts] {
// [ts] let type = workInProgress.type;
// [ts] let _unresolvedProps2 = workInProgress.pendingProps;
// [ts] let _resolvedProps2 = if (workInProgress.elementType == type) _unresolvedProps2 else resolveDefaultProps(type, _unresolvedProps2);
// [ts] return updateForwardRef(current, workInProgress, type, _resolvedProps2, renderExpirationTime);
// [ts] }
// [ts] },
// [ts] Fragment => {
// [ts] return updateFragment(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] Mode => {
// [ts] return updateMode(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] Profiler => {
// [ts] return updateProfiler(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] ContextProvider => {
// [ts] return updateContextProvider(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] ContextConsumer => {
// [ts] return updateContextConsumer(current, workInProgress, renderExpirationTime);
// [ts] },
// [ts] MemoComponent => {
// [ts] {
// [ts] let _type2 = workInProgress.type;
// [ts] let _unresolvedProps3 = workInProgress.pendingProps; // Resolve outer props first, then resolve inner props.
// [ts] let _resolvedProps3 = resolveDefaultProps(_type2, _unresolvedProps3);
// [ts] {
// [ts] if (workInProgress.type != workInProgress.elementType) {
// [ts] let outerPropTypes = _type2.propTypes;
// [ts] if (outerPropTypes) {
// [ts] checkPropTypes_1(outerPropTypes, _resolvedProps3, // Resolved for outer only
// [ts] "prop", getComponentName(_type2), getCurrentFiberStackInDev);
// [ts] }
// [ts] }
// [ts] }
// [ts] _resolvedProps3 = resolveDefaultProps(_type2.type, _resolvedProps3);
// [ts] return updateMemoComponent(current, workInProgress, _type2, _resolvedProps3, updateExpirationTime, renderExpirationTime);
// [ts] }
// [ts] },
// [ts] SimpleMemoComponent => {
// [ts] {
// [ts] return updateSimpleMemoComponent(current, workInProgress, workInProgress.type, workInProgress.pendingProps, updateExpirationTime, renderExpirationTime);
// [ts] }
// [ts] },
// [ts] IncompleteClassComponent => {
// [ts] {
// [ts] let _Component3 = workInProgress.type;
// [ts] let _unresolvedProps4 = workInProgress.pendingProps;
// [ts] let _resolvedProps4 = if (workInProgress.elementType == _Component3) _unresolvedProps4 else resolveDefaultProps(_Component3, _unresolvedProps4);
// [ts] return mountIncompleteClassComponent(current, workInProgress, _Component3, _resolvedProps4, renderExpirationTime);
// [ts] }
// [ts] },
// [ts] SuspenseListComponent => {
// [ts] {
// [ts] return updateSuspenseListComponent(current, workInProgress, renderExpirationTime);
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unknown unit of work tag (" + workInProgress.tag + "). This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markUpdate(workInProgress: auto) {
// Tag the fiber with an update effect. This turns a Placement into
// a PlacementAndUpdate.
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] (workInProgress) => {
// [ts] workInProgress.effectTag |= Ref;
// [ts] }
// [ts] let appendAllChildren;
// [ts] let updateHostContainer;
// [ts] let updateHostComponent$1;
// [ts] let updateHostText$1;
// [ts] {
// Mutation mode
// [ts] appendAllChildren = (parent, workInProgress, needsVisibilityToggle, isHidden) => {
// We only have the top Fiber that was created but we need recurse down its
// children to find all the terminal nodes.
// [ts] let node = workInProgress.child;
// [ts] while (node != null) {
// [ts] if (node.tag == HostComponent || node.tag == HostText) {
// [ts] appendInitialChild(parent, node.stateNode);
// [ts] } else if (node.tag == HostPortal) ; else if (node.child != null) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] if (node == workInProgress) {
// [ts] return;
// [ts] }
// [ts] while (node.sibling == null) {
// [ts] if (node.return == null || node.return == workInProgress) {
// [ts] return;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] }
// [ts] };
// [ts] updateHostContainer = (workInProgress) => {// Noop
// [ts] };
// [ts] updateHostComponent$1 = (current, workInProgress, type, newProps, rootContainerInstance) => {
// If we have an alternate, that means this is an update and we need to
// schedule a side-effect to do the updates.
// [ts] let oldProps = current.memoizedProps;
// [ts] if (oldProps == newProps) {
// In mutation mode, this is sufficient for a bailout because
// we won't touch this node even if children changed.
// [ts] return;
// [ts] } // If we get updated because one of our children updated, we don't
// have newProps so we'll have to reuse them.
// TODO: Split the update API as separate for the props vs. children.
// Even better would be if children weren't special cased at all tho.
// [ts] let instance = workInProgress.stateNode;
// [ts] let currentHostContext = getHostContext(); // TODO: Experiencing an error where oldProps is null. Suggests a host
// component is hitting the resume path. Figure out why. Possibly
// related to `hidden`.
// [ts] let updatePayload = prepareUpdate(instance, type, oldProps, newProps, rootContainerInstance, currentHostContext); // TODO: Type this specific to this type of component.
// [ts] workInProgress.updateQueue = updatePayload; // If the update payload indicates that there is a change or if there
// is a new ref we mark this as an update. All the work is done in commitWork.
// [ts] if (updatePayload) {
// [ts] markUpdate(workInProgress);
// [ts] }
// [ts] };
// [ts] updateHostText$1 = (current, workInProgress, oldText, newText) => {
// If the text differs, mark it as an update. All the work in done in commitWork.
// [ts] if (oldText != newText) {
// [ts] markUpdate(workInProgress);
// [ts] }
// [ts] };
// [ts] }
// [ts] fn cutOffTailIfNeeded(renderState: auto, hasRenderedATailFallback: auto) {
// [ts] switch (renderState.tailMode) {
// [ts] "hidden" => {
// [ts] {
// Any insertions at the end of the tail list after this point
// should be invisible. If there are already mounted boundaries
// anything before them are not considered for collapsing.
// Therefore we need to go through the whole tail to find if
// there are any.
// [ts] let tailNode = renderState.tail;
// [ts] let lastTailNode = null;
// [ts] while (tailNode != null) {
// [ts] if (tailNode.alternate != null) {
// [ts] lastTailNode = tailNode;
// [ts] }
// [ts] tailNode = tailNode.sibling;
// [ts] } // Next we're simply going to delete all insertions after the
// last rendered item.
// [ts] if (lastTailNode == null) {
// All remaining items in the tail are insertions.
// [ts] renderState.tail = null;
// [ts] } else {
// Detach the insertion after the last node that was already
// inserted.
// [ts] lastTailNode.sibling = null;
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] "collapsed" => {
// [ts] {
// Any insertions at the end of the tail list after this point
// should be invisible. If there are already mounted boundaries
// anything before them are not considered for collapsing.
// Therefore we need to go through the whole tail to find if
// there are any.
// [ts] let _tailNode = renderState.tail;
// [ts] let _lastTailNode = null;
// [ts] while (_tailNode != null) {
// [ts] if (_tailNode.alternate != null) {
// [ts] _lastTailNode = _tailNode;
// [ts] }
// [ts] _tailNode = _tailNode.sibling;
// [ts] } // Next we're simply going to delete all insertions after the
// last rendered item.
// [ts] if (_lastTailNode == null) {
// All remaining items in the tail are insertions.
// [ts] if (!hasRenderedATailFallback && renderState.tail != null) {
// We suspended during the head. We want to show at least one
// row at the tail. So we'll keep on and cut off the rest.
// [ts] renderState.tail.sibling = null;
// [ts] } else {
// [ts] renderState.tail = null;
// [ts] }
// [ts] } else {
// Detach the insertion after the last node that was already
// inserted.
// [ts] _lastTailNode.sibling = null;
// [ts] }
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn completeWork(current: auto, workInProgress: auto, renderExpirationTime: auto) {
// [ts] let newProps = workInProgress.pendingProps;
// [ts] switch (workInProgress.tag) {
// [ts] IndeterminateComponent, LazyComponent, SimpleMemoComponent, FunctionComponent, ForwardRef, Fragment, Mode, Profiler, ContextConsumer, MemoComponent => {
// [ts] return null;
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] let Component = workInProgress.type;
// [ts] if (isContextProvider(Component)) {
// [ts] popContext(workInProgress);
// [ts] }
// [ts] return null;
// [ts] }
// [ts] },
// [ts] HostRoot => {
// [ts] {
// [ts] popHostContainer(workInProgress);
// [ts] popTopLevelContextObject(workInProgress);
// [ts] let fiberRoot = workInProgress.stateNode;
// [ts] if (fiberRoot.pendingContext) {
// [ts] fiberRoot.context = fiberRoot.pendingContext;
// [ts] fiberRoot.pendingContext = null;
// [ts] }
// [ts] if (current == null || current.child == null) {
// If we hydrated, pop so that we can delete any remaining children
// that weren't hydrated.
// [ts] let wasHydrated = popHydrationState(workInProgress);
// [ts] if (wasHydrated) {
// If we hydrated, then we'll need to schedule an update for
// the commit side-effects on the root.
// [ts] markUpdate(workInProgress);
// [ts] }
// [ts] }
// [ts] updateHostContainer(workInProgress);
// [ts] return null;
// [ts] }
// [ts] },
// [ts] HostComponent => {
// [ts] {
// [ts] popHostContext(workInProgress);
// [ts] let rootContainerInstance = getRootHostContainer();
// [ts] let type = workInProgress.type;
// [ts] if (current != null && workInProgress.stateNode != null) {
// [ts] updateHostComponent$1(current, workInProgress, type, newProps, rootContainerInstance);
// [ts] if (current.ref != workInProgress.ref) {
// [ts] markRef$1(workInProgress);
// [ts] }
// [ts] } else {
// [ts] if (!newProps) {
// [ts] if (!(workInProgress.stateNode != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "We must have new props for new mounts. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] } // This can happen when we abort work.
// [ts] return null;
// [ts] }
// [ts] let currentHostContext = getHostContext(); // TODO: Move createInstance to beginWork and keep it on a context
// "stack" as the parent. Then append children as we go in beginWork
// or completeWork depending on whether we want to add them top->down or
// bottom->up. Top->down is faster in IE11.
// [ts] let _wasHydrated = popHydrationState(workInProgress);
// [ts] if (_wasHydrated) {
// TODO: Move this and createInstance step into the beginPhase
// to consolidate.
// [ts] if (prepareToHydrateHostInstance(workInProgress, rootContainerInstance, currentHostContext)) {
// If changes to the hydrated node need to be applied at the
// commit-phase we mark this as such.
// [ts] markUpdate(workInProgress);
// [ts] }
// [ts] } else {
// [ts] let instance = createInstance(type, newProps, rootContainerInstance, currentHostContext, workInProgress);
// [ts] appendAllChildren(instance, workInProgress, false, false); // This needs to be set before we mount Flare event listeners
// [ts] workInProgress.stateNode = instance;
// (eg DOM renderer supports auto-focus for certain elements).
// Make sure such renderers get scheduled for later work.
// [ts] if (finalizeInitialChildren(instance, type, newProps, rootContainerInstance)) {
// [ts] markUpdate(workInProgress);
// [ts] }
// [ts] }
// [ts] if (workInProgress.ref != null) {
// If there is a ref on a host node we need to schedule a callback
// [ts] markRef$1(workInProgress);
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] },
// [ts] HostText => {
// [ts] {
// [ts] let newText = newProps;
// [ts] if (current && workInProgress.stateNode != null) {
// [ts] let oldText = current.memoizedProps; // If we have an alternate, that means this is an update and we need
// to schedule a side-effect to do the updates.
// [ts] updateHostText$1(current, workInProgress, oldText, newText);
// [ts] } else {
// [ts] if (@typeName(newText) != "string") {
// [ts] if (!(workInProgress.stateNode != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "We must have new props for new mounts. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] } // This can happen when we abort work.
// [ts] }
// [ts] let _rootContainerInstance = getRootHostContainer();
// [ts] let _currentHostContext = getHostContext();
// [ts] let _wasHydrated2 = popHydrationState(workInProgress);
// [ts] if (_wasHydrated2) {
// [ts] if (prepareToHydrateHostTextInstance(workInProgress)) {
// [ts] markUpdate(workInProgress);
// [ts] }
// [ts] } else {
// [ts] workInProgress.stateNode = createTextInstance(newText, _rootContainerInstance, _currentHostContext, workInProgress);
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] },
// [ts] SuspenseComponent => {
// [ts] {
// [ts] popSuspenseContext(workInProgress);
// [ts] let nextState = workInProgress.memoizedState;
// [ts] if ((workInProgress.effectTag & DidCapture) != NoEffect) {
// Something suspended. Re-render with the fallback children.
// [ts] workInProgress.expirationTime = renderExpirationTime; // Do not reset the effect list.
// [ts] return workInProgress;
// [ts] }
// [ts] let nextDidTimeout = nextState != null;
// [ts] let prevDidTimeout = false;
// [ts] if (current == null) {
// [ts] if (workInProgress.memoizedProps.fallback != null) {
// [ts] popHydrationState(workInProgress);
// [ts] }
// [ts] } else {
// [ts] let prevState = current.memoizedState;
// [ts] prevDidTimeout = prevState != null;
// [ts] if (!nextDidTimeout && prevState != null) {
// We just switched from the fallback to the normal children.
// Delete the fallback.
// TODO: Would it be better to store the fallback fragment on
// the stateNode during the begin phase?
// [ts] let currentFallbackChild = current.child.sibling;
// [ts] if (currentFallbackChild != null) {
// Deletions go at the beginning of the return fiber's effect list
// [ts] let first = workInProgress.firstEffect;
// [ts] if (first != null) {
// [ts] workInProgress.firstEffect = currentFallbackChild;
// [ts] currentFallbackChild.nextEffect = first;
// [ts] } else {
// [ts] workInProgress.firstEffect = workInProgress.lastEffect = currentFallbackChild;
// [ts] currentFallbackChild.nextEffect = null;
// [ts] }
// [ts] currentFallbackChild.effectTag = Deletion;
// [ts] }
// [ts] }
// [ts] }
// [ts] if (nextDidTimeout && !prevDidTimeout) {
// If this subtreee is running in blocking mode we can suspend,
// otherwise we won't suspend.
// TODO: This will still suspend a synchronous tree if anything
// in the concurrent tree already suspended during this render.
// This is a known bug.
// [ts] if ((workInProgress.mode & BlockingMode) != NoMode) {
// TODO: Move this back to throwException because this is too late
// if this is a large tree which is common for initial loads. We
// don't know if we should restart a render or not until we get
// this marker, and this is too late.
// If this render already had a ping or lower pri updates,
// and this is the first time we know we're going to suspend we
// should be able to immediately restart from within throwException.
// [ts] let hasInvisibleChildContext = current == null && workInProgress.memoizedProps.unstable_avoidThisFallback != true;
// [ts] if (hasInvisibleChildContext || hasSuspenseContext(suspenseStackCursor.current, InvisibleParentSuspenseContext)) {
// If this was in an invisible tree or a new render, then showing
// this boundary is ok.
// [ts] renderDidSuspend();
// [ts] } else {
// Otherwise, we're going to have to hide content so we should
// suspend for longer if possible.
// [ts] renderDidSuspendDelayIfPossible();
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// TODO: Only schedule updates if these values are non equal, i.e. it changed.
// [ts] if (nextDidTimeout || prevDidTimeout) {
// If this boundary just timed out, schedule an effect to attach a
// retry listener to the promise. This flag is also used to hide the
// primary children. In mutation mode, we also need the flag to
// *unhide* children that were previously hidden, so check if this
// is currently timed out, too.
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] },
// [ts] HostPortal => {
// [ts] popHostContainer(workInProgress);
// [ts] updateHostContainer(workInProgress);
// [ts] return null;
// [ts] },
// Pop provider fiber
// [ts] ContextProvider => {
// [ts] popProvider(workInProgress);
// [ts] return null;
// [ts] },
// [ts] IncompleteClassComponent => {
// [ts] {
// Same as class component case. I put it down here so that the tags are
// sequential to ensure this switch is compiled to a jump table.
// [ts] let _Component = workInProgress.type;
// [ts] if (isContextProvider(_Component)) {
// [ts] popContext(workInProgress);
// [ts] }
// [ts] return null;
// [ts] }
// [ts] },
// [ts] SuspenseListComponent => {
// [ts] {
// [ts] popSuspenseContext(workInProgress);
// [ts] let renderState = workInProgress.memoizedState;
// [ts] if (renderState == null) {
// We're running in the default, "independent" mode.
// We don't do anything in this mode.
// [ts] return null;
// [ts] }
// [ts] let didSuspendAlready = (workInProgress.effectTag & DidCapture) != NoEffect;
// [ts] let renderedTail = renderState.rendering;
// [ts] if (renderedTail == null) {
// We just rendered the head.
// [ts] if (!didSuspendAlready) {
// This is the first pass. We need to figure out if anything is still
// suspended in the rendered set.
// If new content unsuspended, but there's still some content that
// didn't. Then we need to do a second pass that forces everything
// to keep showing their fallbacks.
// We might be suspended if something in this render pass suspended, or
// something in the previous committed pass suspended. Otherwise,
// there's no chance so we can skip the expensive call to
// findFirstSuspended.
// [ts] let cannotBeSuspended = renderHasNotSuspendedYet() && (current == null || (current.effectTag & DidCapture) == NoEffect);
// [ts] if (!cannotBeSuspended) {
// [ts] let row = workInProgress.child;
// [ts] while (row != null) {
// [ts] let suspended = findFirstSuspended(row);
// [ts] if (suspended != null) {
// [ts] didSuspendAlready = true;
// [ts] workInProgress.effectTag |= DidCapture;
// [ts] cutOffTailIfNeeded(renderState, false); // If this is a newly suspended tree, it might not get committed as
// part of the second pass. In that case nothing will subscribe to
// its thennables. Instead, we'll transfer its thennables to the
// SuspenseList so that it can retry if they resolve.
// There might be multiple of these in the list but since we're
// going to wait for all of them anyway, it doesn't really matter
// which ones gets to ping. In theory we could get clever and keep
// track of how many dependencies remain but it gets tricky because
// in the meantime, we can add/remove/change items and dependencies.
// We might bail out of the loop before finding any but that
// doesn't matter since that means that the other boundaries that
// we did find already has their listeners attached.
// [ts] let newThennables = suspended.updateQueue;
// [ts] if (newThennables != null) {
// [ts] workInProgress.updateQueue = newThennables;
// [ts] workInProgress.effectTag |= Update;
// [ts] } // Rerender the whole list, but this time, we'll force fallbacks
// to stay in place.
// Reset the effect list before doing the second pass since that's now invalid.
// [ts] if (renderState.lastEffect == null) {
// [ts] workInProgress.firstEffect = null;
// [ts] }
// [ts] workInProgress.lastEffect = renderState.lastEffect; // Reset the child fibers to their original state.
// [ts] resetChildFibers(workInProgress, renderExpirationTime); // Set up the Suspense Context to force suspense and immediately
// rerender the children.
// [ts] pushSuspenseContext(workInProgress, setShallowSuspenseContext(suspenseStackCursor.current, ForceSuspenseFallback));
// [ts] return workInProgress.child;
// [ts] }
// [ts] row = row.sibling;
// [ts] }
// [ts] }
// [ts] } else {
// [ts] cutOffTailIfNeeded(renderState, false);
// [ts] } // Next we're going to render the tail.
// [ts] } else {
// Append the rendered row to the child list.
// [ts] if (!didSuspendAlready) {
// [ts] let _suspended = findFirstSuspended(renderedTail);
// [ts] if (_suspended != null) {
// [ts] workInProgress.effectTag |= DidCapture;
// [ts] didSuspendAlready = true; // Ensure we transfer the update queue to the parent so that it doesn't
// get lost if this row ends up dropped during a second pass.
// [ts] let _newThennables = _suspended.updateQueue;
// [ts] if (_newThennables != null) {
// [ts] workInProgress.updateQueue = _newThennables;
// [ts] workInProgress.effectTag |= Update;
// [ts] }
// [ts] cutOffTailIfNeeded(renderState, true); // This might have been modified.
// [ts] if (renderState.tail == null && renderState.tailMode == "hidden" && !renderedTail.alternate) {
// We need to delete the row we just rendered.
// Reset the effect list to what it was before we rendered this
// child. The nested children have already appended themselves.
// [ts] let lastEffect = workInProgress.lastEffect = renderState.lastEffect; // Remove any effects that were appended after this point.
// [ts] if (lastEffect != null) {
// [ts] lastEffect.nextEffect = null;
// [ts] } // We're done.
// [ts] return null;
// [ts] }
// [ts] } else if ( // The time it took to render last row is greater than time until
// the expiration.
// [ts] now: () => {
// We have now passed our CPU deadline and we'll just give up further
// attempts to render the main content and only render fallbacks.
// The assumption is that this is usually faster.
// [ts] workInProgress.effectTag |= DidCapture;
// [ts] didSuspendAlready = true;
// [ts] cutOffTailIfNeeded(renderState, false); // Since nothing actually suspended, there will nothing to ping this
// to get it started back up to attempt the next item. If we can show
// them, then they really have the same priority as this render.
// So we'll pick it back up the very next render pass once we've had
// an opportunity to yield for paint.
// [ts] let nextPriority = renderExpirationTime - 1;
// [ts] workInProgress.expirationTime = workInProgress.childExpirationTime = nextPriority;
// [ts] {
// [ts] markSpawnedWork(nextPriority);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (renderState.isBackwards) {
// The effect list of the backwards tail will have been added
// to the end. This breaks the guarantee that life-cycles fire in
// sibling order but that isn't a strong guarantee promised by React.
// Especially since these might also just pop in during future commits.
// Append to the beginning of the list.
// [ts] renderedTail.sibling = workInProgress.child;
// [ts] workInProgress.child = renderedTail;
// [ts] } else {
// [ts] let previousSibling = renderState.last;
// [ts] if (previousSibling != null) {
// [ts] previousSibling.sibling = renderedTail;
// [ts] } else {
// [ts] workInProgress.child = renderedTail;
// [ts] }
// [ts] renderState.last = renderedTail;
// [ts] }
// [ts] }
// [ts] if (renderState.tail != null) {
// We still have tail rows to render.
// [ts] if (renderState.tailExpiration == 0) {
// Heuristic for how long we're willing to spend rendering rows
// until we just give up and show what we have so far.
// [ts] let TAIL_EXPIRATION_TIMEOUT_MS = 500;
// [ts] renderState.tailExpiration = now() + TAIL_EXPIRATION_TIMEOUT_MS; // TODO: This is meant to mimic the train model or JND but this
// is a per component value. It should really be since the start
// of the total render or last commit. Consider using something like
// globalMostRecentFallbackTime. That doesn't account for being
// suspended for part of the time or when it's a new render.
// It should probably use a global start time value instead.
// [ts] } // Pop a row.
// [ts] let next = renderState.tail;
// [ts] renderState.rendering = next;
// [ts] renderState.tail = next.sibling;
// [ts] renderState.lastEffect = workInProgress.lastEffect;
// [ts] renderState.renderingStartTime = now();
// [ts] next.sibling = null; // Restore the context.
// TODO: We can probably just avoid popping it instead and only
// setting it the first time we go from not suspended to suspended.
// [ts] let suspenseContext = suspenseStackCursor.current;
// [ts] if (didSuspendAlready) {
// [ts] suspenseContext = setShallowSuspenseContext(suspenseContext, ForceSuspenseFallback);
// [ts] } else {
// [ts] suspenseContext = setDefaultShallowSuspenseContext(suspenseContext);
// [ts] }
// [ts] pushSuspenseContext(workInProgress, suspenseContext); // Do a pass over the next row.
// [ts] return next;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unknown unit of work tag (" + workInProgress.tag + "). This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn unwindWork(workInProgress: auto, renderExpirationTime: auto) {
// [ts] switch (workInProgress.tag) {
// [ts] ClassComponent => {
// [ts] {
// [ts] let Component = workInProgress.type;
// [ts] if (isContextProvider(Component)) {
// [ts] popContext(workInProgress);
// [ts] }
// [ts] let effectTag = workInProgress.effectTag;
// [ts] if (effectTag & ShouldCapture) {
// [ts] workInProgress.effectTag = effectTag & ~ShouldCapture | DidCapture;
// [ts] return workInProgress;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] },
// [ts] HostRoot => {
// [ts] {
// [ts] popHostContainer(workInProgress);
// [ts] popTopLevelContextObject(workInProgress);
// [ts] let _effectTag = workInProgress.effectTag;
// [ts] if (!((_effectTag & DidCapture) == NoEffect)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "The root failed to unmount after an error. This is likely a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] workInProgress.effectTag = _effectTag & ~ShouldCapture | DidCapture;
// [ts] return workInProgress;
// [ts] }
// [ts] },
// [ts] HostComponent => {
// [ts] {
// TODO: popHydrationState
// [ts] popHostContext(workInProgress);
// [ts] return null;
// [ts] }
// [ts] },
// [ts] SuspenseComponent => {
// [ts] {
// [ts] popSuspenseContext(workInProgress);
// [ts] let _effectTag2 = workInProgress.effectTag;
// [ts] if (_effectTag2 & ShouldCapture) {
// [ts] workInProgress.effectTag = _effectTag2 & ~ShouldCapture | DidCapture; // Captured a suspense effect. Re-render the boundary.
// [ts] return workInProgress;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] },
// [ts] SuspenseListComponent => {
// [ts] {
// [ts] popSuspenseContext(workInProgress); // SuspenseList doesn't actually catch anything. It should've been
// caught by a nested boundary. If not, it should bubble through.
// [ts] return null;
// [ts] }
// [ts] },
// [ts] HostPortal => {
// [ts] popHostContainer(workInProgress);
// [ts] return null;
// [ts] },
// [ts] ContextProvider => {
// [ts] popProvider(workInProgress);
// [ts] return null;
// [ts] },
// [ts] else => {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn unwindInterruptedWork(interruptedWork: auto) {
// [ts] switch (interruptedWork.tag) {
// [ts] ClassComponent => {
// [ts] {
// [ts] let childContextTypes = interruptedWork.type.childContextTypes;
// [ts] if (childContextTypes != null && childContextTypes != null) {
// [ts] popContext(interruptedWork);
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] HostRoot => {
// [ts] {
// [ts] popHostContainer(interruptedWork);
// [ts] popTopLevelContextObject(interruptedWork);
// [ts] break;
// [ts] }
// [ts] },
// [ts] HostComponent => {
// [ts] {
// [ts] popHostContext(interruptedWork);
// [ts] break;
// [ts] }
// [ts] },
// [ts] HostPortal => {
// [ts] popHostContainer(interruptedWork);
// break;
// [ts] },
// [ts] SuspenseComponent => {
// [ts] popSuspenseContext(interruptedWork);
// break;
// [ts] },
// [ts] SuspenseListComponent => {
// [ts] popSuspenseContext(interruptedWork);
// break;
// [ts] },
// [ts] ContextProvider => {
// [ts] popProvider(interruptedWork);
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn createCapturedValue(value: auto, source: auto) {
// If the value is an error, call this function immediately after it is thrown
// so the stack is accurate.
// [ts] return {
// [ts] value: value,
// [ts] source: source,
// [ts] stack: getStackByFiberInDevAndProd(source)
// [ts] };
// [ts] }
// [ts] fn logCapturedError(capturedError: auto) {
// [ts] let error = capturedError.error;
// [ts] {
// [ts] let componentName = capturedError.componentName,
// [ts] componentStack = capturedError.componentStack,
// [ts] errorBoundaryName = capturedError.errorBoundaryName,
// [ts] errorBoundaryFound = capturedError.errorBoundaryFound,
// [ts] willRetry = capturedError.willRetry; // Browsers support silencing uncaught errors by calling
// `preventDefault()` in window `error` handler.
// We record this information as an expando on the error.
// [ts] if (error != null && error._suppressLogging) {
// [ts] if (errorBoundaryFound && willRetry) {
// The error is recoverable and was silenced.
// Ignore it and don't print the stack addendum.
// This is handy for testing error boundaries without noise.
// [ts] return;
// [ts] } // The error is fatal. Since the silencing might have
// been accidental, we'll surface it anyway.
// However, the browser would have silenced the original error
// so we'll print it first, and then print the stack addendum.
// [ts] console["error"](error); // Don't transform to our wrapper
// For a more detailed description of this block, see:
// https://github.com/facebook/react/pull/13384
// [ts] }
// [ts] let componentNameMessage = if (componentName) "The above error occurred in the <" + componentName + "> component:" else "The above error occurred in one of your React components:";
// [ts] let errorBoundaryMessage; // errorBoundaryFound check is sufficient; errorBoundaryName check is to satisfy Flow.
// [ts] if (errorBoundaryFound && errorBoundaryName) {
// [ts] if (willRetry) {
// [ts] errorBoundaryMessage = "React will try to recreate this component tree from scratch " + ("using the error boundary you provided, " + errorBoundaryName + ".");
// [ts] } else {
// [ts] errorBoundaryMessage = "This error was initially handled by the error boundary " + errorBoundaryName + ".\n" + "Recreating the tree from scratch failed so React will unmount the tree.";
// [ts] }
// [ts] } else {
// [ts] errorBoundaryMessage = "Consider adding an error boundary to your tree to customize error handling behavior.\n" + "Visit https://fb.me/react-error-boundaries to learn more about error boundaries.";
// [ts] }
// [ts] let combinedMessage = "" + componentNameMessage + componentStack + "\n\n" + ("" + errorBoundaryMessage); // In development, we provide our own message with just the component stack.
// We don't include the original error message and JS stack because the browser
// has already printed it. Even if the application swallows the error, it is still
// displayed by the browser thanks to the DEV-only fake event trick in ReactErrorUtils.
// [ts] console["error"](combinedMessage); // Don't transform to our wrapper
// [ts] }
// [ts] }
// [ts] let didWarnAboutUndefinedSnapshotBeforeUpdate = null;
// [ts] {
// [ts] didWarnAboutUndefinedSnapshotBeforeUpdate = Set.init();
// [ts] }
// [ts] let PossiblyWeakSet = if (@typeName(WeakSet) == "function") WeakSet else Set;
// [ts] fn logError(boundary: auto, errorInfo: auto) {
// [ts] let source = errorInfo.source;
// [ts] let stack = errorInfo.stack;
// [ts] if (stack == null && source != null) {
// [ts] stack = getStackByFiberInDevAndProd(source);
// [ts] }
// [ts] let capturedError = {
// [ts] componentName: if (source != null) getComponentName(source.type) else null,
// [ts] componentStack: if (stack != null) stack else "",
// [ts] error: errorInfo.value,
// [ts] errorBoundary: null,
// [ts] errorBoundaryName: null,
// [ts] errorBoundaryFound: false,
// [ts] willRetry: false
// [ts] };
// [ts] if (boundary != null && boundary.tag == ClassComponent) {
// [ts] capturedError.errorBoundary = boundary.stateNode;
// [ts] capturedError.errorBoundaryName = getComponentName(boundary.type);
// [ts] capturedError.errorBoundaryFound = true;
// [ts] capturedError.willRetry = true;
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] logCapturedError(capturedError);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// This method must not throw, or React internal state will get messed up.
// If console.error is overridden, or logCapturedError() shows a dialog that throws,
// we want to report this error outside of the normal stack as a last resort.
// https://github.com/facebook/react/issues/13188
// [ts] setTimeout(() => {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw e;)
// [ts] });
// [ts] }
// [ts] }
// [ts] let callComponentWillUnmountWithTimer = (current, instance) => {
// [ts] startPhaseTimer(current, "componentWillUnmount");
// [ts] instance.props = current.memoizedProps;
// [ts] instance.state = current.memoizedState;
// [ts] instance.componentWillUnmount();
// [ts] stopPhaseTimer();
// [ts] }; // Capture errors so they don't interrupt unmounting.
// [ts] fn safelyCallComponentWillUnmount(current: auto, instance: auto) {
// [ts] {
// [ts] invokeGuardedCallback(null, callComponentWillUnmountWithTimer, null, current, instance);
// [ts] if (hasCaughtError()) {
// [ts] let unmountError = clearCaughtError();
// [ts] captureCommitPhaseError(current, unmountError);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn safelyDetachRef(current: auto) {
// [ts] let ref = current.ref;
// [ts] if (ref != null) {
// [ts] if (@typeName(ref) == "function") {
// [ts] {
// [ts] invokeGuardedCallback(null, ref, null, null);
// [ts] if (hasCaughtError()) {
// [ts] let refError = clearCaughtError();
// [ts] captureCommitPhaseError(current, refError);
// [ts] }
// [ts] }
// [ts] } else {
// [ts] ref.current = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn safelyCallDestroy(current: auto, destroy: auto) {
// [ts] {
// [ts] invokeGuardedCallback(null, destroy, null);
// [ts] if (hasCaughtError()) {
// [ts] let error = clearCaughtError();
// [ts] captureCommitPhaseError(current, error);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn commitBeforeMutationLifeCycles(current: auto, finishedWork: auto) {
// [ts] switch (finishedWork.tag) {
// [ts] FunctionComponent, ForwardRef, SimpleMemoComponent, Block => {
// [ts] {
// [ts] return;
// [ts] }
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] if (finishedWork.effectTag & Snapshot) {
// [ts] if (current != null) {
// [ts] let prevProps = current.memoizedProps;
// [ts] let prevState = current.memoizedState;
// [ts] startPhaseTimer(finishedWork, "getSnapshotBeforeUpdate");
// [ts] let instance = finishedWork.stateNode; // We could update instance props and state here,
// but instead we rely on them being set during last render.
// TODO: revisit this when we implement resuming.
// [ts] {
// [ts] if (finishedWork.type == finishedWork.elementType && !didWarnAboutReassigningProps) {
// [ts] if (instance.props != finishedWork.memoizedProps) {
// [ts] error("Expected %s props to match memoized props before " + "getSnapshotBeforeUpdate. " + "This might either be because of a bug in React, or because " + "a component reassigns its own `this.props`. " + "Please file an issue.", getComponentName(finishedWork.type) || "instance");
// [ts] }
// [ts] if (instance.state != finishedWork.memoizedState) {
// [ts] error("Expected %s state to match memoized state before " + "getSnapshotBeforeUpdate. " + "This might either be because of a bug in React, or because " + "a component reassigns its own `this.props`. " + "Please file an issue.", getComponentName(finishedWork.type) || "instance");
// [ts] }
// [ts] }
// [ts] }
// [ts] let snapshot = instance.getSnapshotBeforeUpdate(if (finishedWork.elementType == finishedWork.type) prevProps else resolveDefaultProps(finishedWork.type, prevProps), prevState);
// [ts] {
// [ts] let didWarnSet = didWarnAboutUndefinedSnapshotBeforeUpdate;
// [ts] if (snapshot == null && !didWarnSet.has(finishedWork.type)) {
// [ts] didWarnSet.add(finishedWork.type);
// [ts] error("%s.getSnapshotBeforeUpdate(): A snapshot value (or null) " + "must be returned. You have returned undefined.", getComponentName(finishedWork.type));
// [ts] }
// [ts] }
// [ts] instance.__reactInternalSnapshotBeforeUpdate = snapshot;
// [ts] stopPhaseTimer();
// [ts] }
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// Nothing to do for these component types
// [ts] HostRoot, HostComponent, HostText, HostPortal, IncompleteClassComponent => {
// [ts] return;
// [ts] }
// [ts] }
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "This unit of work tag should not have side-effects. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn commitHookEffectListUnmount(tag: auto, finishedWork: auto) {
// [ts] let updateQueue = finishedWork.updateQueue;
// [ts] let lastEffect = if (updateQueue != null) updateQueue.lastEffect else null;
// [ts] if (lastEffect != null) {
// [ts] let firstEffect = lastEffect.next;
// [ts] let effect = firstEffect;
// [ts] do {
// [ts] if ((effect.tag & tag) == tag) {
// Unmount
// [ts] let destroy = effect.destroy;
// [ts] effect.destroy = null;
// [ts] if (destroy != null) {
// [ts] destroy();
// [ts] }
// [ts] }
// [ts] effect = effect.next;
// [ts] } while (effect != firstEffect);
// [ts] }
// [ts] }
// [ts] fn commitHookEffectListMount(tag: auto, finishedWork: auto) {
// [ts] let updateQueue = finishedWork.updateQueue;
// [ts] let lastEffect = if (updateQueue != null) updateQueue.lastEffect else null;
// [ts] if (lastEffect != null) {
// [ts] let firstEffect = lastEffect.next;
// [ts] let effect = firstEffect;
// [ts] do {
// [ts] if ((effect.tag & tag) == tag) {
// Mount
// [ts] let create = effect.create;
// [ts] effect.destroy = create();
// [ts] {
// [ts] let destroy = effect.destroy;
// [ts] if (destroy != null && @typeName(destroy) != "function") {
// [ts] let addendum = void 0;
// [ts] if (destroy == null) {
// [ts] addendum = " You returned null. If your effect does not require clean " + "up, return undefined (or nothing).";
// [ts] } else if (@typeName(destroy.then) == "function") {
// [ts] addendum = "\n\nIt looks like you wrote useEffect(async () => ...) or returned a Promise. " + "Instead, write the async function inside your effect " + "and call it immediately:\n\n" + "useEffect(() => {\n" + "  async function fetchData() {\n" + "    // You can await here\n" + "    const response = await MyAPI.getData(someId);\n" + "    // ...\n" + "  }\n" + "  fetchData();\n" + "}, [someId]); // Or [] if effect doesn't need props or state\n\n" + "Learn more about data fetching with Hooks: https://fb.me/react-hooks-data-fetching";
// [ts] } else {
// [ts] addendum = " You returned: " + destroy;
// [ts] }
// [ts] error("An effect function must not return anything besides a function, " + "which is used for clean-up.%s%s", addendum, getStackByFiberInDevAndProd(finishedWork));
// [ts] }
// [ts] }
// [ts] }
// [ts] effect = effect.next;
// [ts] } while (effect != firstEffect);
// [ts] }
// [ts] }
// [ts] fn commitPassiveHookEffects(finishedWork: auto) {
// [ts] if ((finishedWork.effectTag & Passive) != NoEffect) {
// [ts] switch (finishedWork.tag) {
// [ts] FunctionComponent, ForwardRef, SimpleMemoComponent, Block => {
// [ts] {
// TODO (#17945) We should call all passive destroy functions (for all fibers)
// before calling any create functions. The current approach only serializes
// these for a single fiber.
// [ts] commitHookEffectListUnmount(Passive$1 | HasEffect, finishedWork);
// [ts] commitHookEffectListMount(Passive$1 | HasEffect, finishedWork);
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn commitLifeCycles(finishedRoot: auto, current: auto, finishedWork: auto, committedExpirationTime: auto) {
// [ts] switch (finishedWork.tag) {
// [ts] FunctionComponent, ForwardRef, SimpleMemoComponent, Block => {
// [ts] {
// At this point layout effects have already been destroyed (during mutation phase).
// This is done to prevent sibling component effects from interfering with each other,
// e.g. a destroy function in one component should never override a ref set
// by a create function in another component during the same commit.
// [ts] commitHookEffectListMount(Layout | HasEffect, finishedWork);
// [ts] return;
// [ts] }
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] let instance = finishedWork.stateNode;
// [ts] if (finishedWork.effectTag & Update) {
// [ts] if (current == null) {
// [ts] startPhaseTimer(finishedWork, "componentDidMount"); // We could update instance props and state here,
// but instead we rely on them being set during last render.
// TODO: revisit this when we implement resuming.
// [ts] {
// [ts] if (finishedWork.type == finishedWork.elementType && !didWarnAboutReassigningProps) {
// [ts] if (instance.props != finishedWork.memoizedProps) {
// [ts] error("Expected %s props to match memoized props before " + "componentDidMount. " + "This might either be because of a bug in React, or because " + "a component reassigns its own `this.props`. " + "Please file an issue.", getComponentName(finishedWork.type) || "instance");
// [ts] }
// [ts] if (instance.state != finishedWork.memoizedState) {
// [ts] error("Expected %s state to match memoized state before " + "componentDidMount. " + "This might either be because of a bug in React, or because " + "a component reassigns its own `this.props`. " + "Please file an issue.", getComponentName(finishedWork.type) || "instance");
// [ts] }
// [ts] }
// [ts] }
// [ts] instance.componentDidMount();
// [ts] stopPhaseTimer();
// [ts] } else {
// [ts] let prevProps = if (finishedWork.elementType == finishedWork.type) current.memoizedProps else resolveDefaultProps(finishedWork.type, current.memoizedProps);
// [ts] let prevState = current.memoizedState;
// [ts] startPhaseTimer(finishedWork, "componentDidUpdate"); // We could update instance props and state here,
// but instead we rely on them being set during last render.
// TODO: revisit this when we implement resuming.
// [ts] {
// [ts] if (finishedWork.type == finishedWork.elementType && !didWarnAboutReassigningProps) {
// [ts] if (instance.props != finishedWork.memoizedProps) {
// [ts] error("Expected %s props to match memoized props before " + "componentDidUpdate. " + "This might either be because of a bug in React, or because " + "a component reassigns its own `this.props`. " + "Please file an issue.", getComponentName(finishedWork.type) || "instance");
// [ts] }
// [ts] if (instance.state != finishedWork.memoizedState) {
// [ts] error("Expected %s state to match memoized state before " + "componentDidUpdate. " + "This might either be because of a bug in React, or because " + "a component reassigns its own `this.props`. " + "Please file an issue.", getComponentName(finishedWork.type) || "instance");
// [ts] }
// [ts] }
// [ts] }
// [ts] instance.componentDidUpdate(prevProps, prevState, instance.__reactInternalSnapshotBeforeUpdate);
// [ts] stopPhaseTimer();
// [ts] }
// [ts] }
// [ts] let updateQueue = finishedWork.updateQueue;
// [ts] if (updateQueue != null) {
// [ts] {
// [ts] if (finishedWork.type == finishedWork.elementType && !didWarnAboutReassigningProps) {
// [ts] if (instance.props != finishedWork.memoizedProps) {
// [ts] error("Expected %s props to match memoized props before " + "processing the update queue. " + "This might either be because of a bug in React, or because " + "a component reassigns its own `this.props`. " + "Please file an issue.", getComponentName(finishedWork.type) || "instance");
// [ts] }
// [ts] if (instance.state != finishedWork.memoizedState) {
// [ts] error("Expected %s state to match memoized state before " + "processing the update queue. " + "This might either be because of a bug in React, or because " + "a component reassigns its own `this.props`. " + "Please file an issue.", getComponentName(finishedWork.type) || "instance");
// [ts] }
// [ts] }
// [ts] } // We could update instance props and state here,
// but instead we rely on them being set during last render.
// TODO: revisit this when we implement resuming.
// [ts] commitUpdateQueue(finishedWork, updateQueue, instance);
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostRoot => {
// [ts] {
// [ts] let _updateQueue = finishedWork.updateQueue;
// [ts] if (_updateQueue != null) {
// [ts] let _instance = null;
// [ts] if (finishedWork.child != null) {
// [ts] switch (finishedWork.child.tag) {
// [ts] HostComponent => {
// [ts] _instance = getPublicInstance(finishedWork.child.stateNode);
// break;
// [ts] },
// [ts] ClassComponent => {
// [ts] _instance = finishedWork.child.stateNode;
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] commitUpdateQueue(finishedWork, _updateQueue, _instance);
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostComponent => {
// [ts] {
// [ts] let _instance2 = finishedWork.stateNode; // Renderers may schedule work to be done after host components are mounted
// (eg DOM renderer may schedule auto-focus for inputs and form controls).
// These effects should only be committed when components are first mounted,
// aka when there is no current/alternate.
// [ts] if (current == null && finishedWork.effectTag & Update) {
// [ts] let type = finishedWork.type;
// [ts] let props = finishedWork.memoizedProps;
// [ts] commitMount(_instance2, type, props);
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostText => {
// [ts] {
// We have no life-cycles associated with text.
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostPortal => {
// [ts] {
// We have no life-cycles associated with portals.
// [ts] return;
// [ts] }
// [ts] },
// [ts] Profiler => {
// [ts] {
// [ts] {
// [ts] let onRender = finishedWork.memoizedProps.onRender;
// [ts] if (@typeName(onRender) == "function") {
// [ts] {
// [ts] onRender(if (finishedWork.memoizedProps.id, current == null) "mount" else "update", finishedWork.actualDuration, finishedWork.treeBaseDuration, finishedWork.actualStartTime, getCommitTime(), finishedRoot.memoizedInteractions);
// [ts] }
// [ts] }
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] SuspenseComponent => {
// [ts] {
// [ts] commitSuspenseHydrationCallbacks(finishedRoot, finishedWork);
// [ts] return;
// [ts] }
// [ts] },
// [ts] SuspenseListComponent, IncompleteClassComponent, FundamentalComponent, ScopeComponent => {
// [ts] return;
// [ts] }
// [ts] }
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "This unit of work tag should not have side-effects. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn hideOrUnhideAllChildren(finishedWork: auto, isHidden: auto) {
// [ts] {
// We only have the top Fiber that was inserted but we need to recurse down its
// children to find all the terminal nodes.
// [ts] let node = finishedWork;
// [ts] while (true) {
// [ts] if (node.tag == HostComponent) {
// [ts] let instance = node.stateNode;
// [ts] if (isHidden) {
// [ts] hideInstance(instance);
// [ts] } else {
// [ts] unhideInstance(node.stateNode, node.memoizedProps);
// [ts] }
// [ts] } else if (node.tag == HostText) {
// [ts] let _instance3 = node.stateNode;
// [ts] if (isHidden) {
// [ts] hideTextInstance(_instance3);
// [ts] } else {
// [ts] unhideTextInstance(_instance3, node.memoizedProps);
// [ts] }
// [ts] } else if (node.tag == SuspenseComponent && node.memoizedState != null && node.memoizedState.dehydrated == null) {
// Found a nested Suspense component that timed out. Skip over the
// primary child fragment, which should remain hidden.
// [ts] let fallbackChildFragment = node.child.sibling;
// [ts] fallbackChildFragment.return = node;
// [ts] node = fallbackChildFragment;
// [ts] continue;
// [ts] } else if (node.child != null) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] if (node == finishedWork) {
// [ts] return;
// [ts] }
// [ts] while (node.sibling == null) {
// [ts] if (node.return == null || node.return == finishedWork) {
// [ts] return;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn commitAttachRef(finishedWork: auto) {
// [ts] let ref = finishedWork.ref;
// [ts] if (ref != null) {
// [ts] let instance = finishedWork.stateNode;
// [ts] let instanceToUse;
// [ts] switch (finishedWork.tag) {
// [ts] HostComponent => {
// [ts] instanceToUse = getPublicInstance(instance);
// break;
// [ts] },
// [ts] else => {
// [ts] instanceToUse = instance;
// [ts] } // Moved outside to ensure DCE works with this flag
// [ts] if (@typeName(ref) == "function") {
// [ts] ref(instanceToUse);
// [ts] } else {
// [ts] {
// [ts] if (!ref.hasOwnProperty("current")) {
// [ts] error("Unexpected ref object provided for %s. " + "Use either a ref-setter function or React.createRef().%s", getComponentName(finishedWork.type), getStackByFiberInDevAndProd(finishedWork));
// [ts] }
// [ts] }
// [ts] ref.current = instanceToUse;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn commitDetachRef(current: auto) {
// [ts] let currentRef = current.ref;
// [ts] if (currentRef != null) {
// [ts] if (@typeName(currentRef) == "function") {
// [ts] currentRef(null);
// [ts] } else {
// [ts] currentRef.current = null;
// [ts] }
// [ts] }
// [ts] } // User-originating errors (lifecycles and refs) should not interrupt
// deletion, so don't let them throw. Host-originating errors should
// interrupt deletion, so it's okay
// [ts] fn commitUnmount(finishedRoot: auto, current: auto, renderPriorityLevel: auto) {
// [ts] onCommitUnmount(current);
// [ts] switch (current.tag) {
// [ts] FunctionComponent, ForwardRef, MemoComponent, SimpleMemoComponent, Block => {
// [ts] {
// [ts] let updateQueue = current.updateQueue;
// [ts] if (updateQueue != null) {
// [ts] let lastEffect = updateQueue.lastEffect;
// [ts] if (lastEffect != null) {
// [ts] let firstEffect = lastEffect.next;
// [ts] {
// When the owner fiber is deleted, the destroy function of a passive
// effect hook is called during the synchronous commit phase. This is
// a concession to implementation complexity. Calling it in the
// passive effect phase (like they usually are, when dependencies
// change during an update) would require either traversing the
// children of the deleted fiber again, or including unmount effects
// as part of the fiber effect list.
//
// Because this is during the sync commit phase, we need to change
// the priority.
//
// TODO: Reconsider this implementation trade off.
// [ts] let priorityLevel = if (renderPriorityLevel > NormalPriority) NormalPriority else renderPriorityLevel;
// [ts] runWithPriority$1(priorityLevel, () => {
// [ts] let effect = firstEffect;
// [ts] do {
// [ts] let _destroy = effect.destroy;
// [ts] if (_destroy != null) {
// [ts] safelyCallDestroy(current, _destroy);
// [ts] }
// [ts] effect = effect.next;
// [ts] } while (effect != firstEffect);
// [ts] });
// [ts] }
// [ts] }
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] safelyDetachRef(current);
// [ts] let instance = current.stateNode;
// [ts] if (@typeName(instance.componentWillUnmount) == "function") {
// [ts] safelyCallComponentWillUnmount(current, instance);
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostComponent => {
// [ts] {
// [ts] safelyDetachRef(current);
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostPortal => {
// [ts] {
// TODO: this is recursive.
// We are also not using this parent because
// the portal will get pushed immediately.
// [ts] {
// [ts] unmountHostComponents(finishedRoot, current, renderPriorityLevel);
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] FundamentalComponent => {
// [ts] {
// [ts] return;
// [ts] }
// [ts] },
// [ts] DehydratedFragment => {
// [ts] {
// [ts] return;
// [ts] }
// [ts] },
// [ts] ScopeComponent => {
// [ts] {
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn commitNestedUnmounts(finishedRoot: auto, root: auto, renderPriorityLevel: auto) {
// While we're inside a removed host node we don't want to call
// removeChild on the inner nodes because they're removed by the top
// call anyway. We also want to call componentWillUnmount on all
// composites before this host node is removed from the tree. Therefore
// we do an inner loop while we're still inside the host node.
// [ts] let node = root;
// [ts] while (true) {
// [ts] commitUnmount(finishedRoot, node, renderPriorityLevel); // Visit children because they may contain more composite or host nodes.
// Skip portals because commitUnmount() currently visits them recursively.
// [ts] if (node.child != null && ( // If we use mutation we drill down into portals using commitUnmount above.
// If we don't use mutation we drill down into portals here instead.
// [ts] node.tag != HostPortal)) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] if (node == root) {
// [ts] return;
// [ts] }
// [ts] while (node.sibling == null) {
// [ts] if (node.return == null || node.return == root) {
// [ts] return;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] }
// [ts] }
// [ts] fn detachFiber(current: auto) {
// [ts] let alternate = current.alternate; // Cut off the return pointers to disconnect it from the tree. Ideally, we
// should clear the child pointer of the parent alternate to let this
// get GC:ed but we don't know which for sure which parent is the current
// one so we'll settle for GC:ing the subtree of this child. This child
// itself will be GC:ed when the parent updates the next time.
// [ts] current.return = null;
// [ts] current.child = null;
// [ts] current.memoizedState = null;
// [ts] current.updateQueue = null;
// [ts] current.dependencies = null;
// [ts] current.alternate = null;
// [ts] current.firstEffect = null;
// [ts] current.lastEffect = null;
// [ts] current.pendingProps = null;
// [ts] current.memoizedProps = null;
// [ts] current.stateNode = null;
// [ts] if (alternate != null) {
// [ts] detachFiber(alternate);
// [ts] }
// [ts] }
// [ts] fn getHostParentFiber(fiber: auto) {
// [ts] let parent = fiber.return;
// [ts] while (parent != null) {
// [ts] if (isHostParent(parent)) {
// [ts] return parent;
// [ts] }
// [ts] parent = parent.return;
// [ts] }
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected to find a host parent. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn isHostParent(fiber: auto) {
// [ts] return fiber.tag == HostComponent || fiber.tag == HostRoot || fiber.tag == HostPortal;
// [ts] }
// [ts] fn getHostSibling(fiber: auto) {
// We're going to search forward into the tree until we find a sibling host
// node. Unfortunately, if multiple insertions are done in a row we have to
// search past them. This leads to exponential search for the next sibling.
// TODO: Find a more efficient way to do this.
// [ts] let node = fiber;
// [ts] siblings: while (true) {
// If we didn't find anything, let's try the next sibling.
// [ts] while (node.sibling == null) {
// [ts] if (node.return == null || isHostParent(node.return)) {
// If we pop out of the root or hit the parent the fiber we are the
// last sibling.
// [ts] return null;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] while (node.tag != HostComponent && node.tag != HostText && node.tag != DehydratedFragment) {
// If it is not host node and, we might have a host node inside it.
// Try to search down until we find one.
// [ts] if (node.effectTag & Placement) {
// If we don't have a child, try the siblings instead.
// [ts] continue siblings;
// [ts] } // If we don't have a child, try the siblings instead.
// We also skip portals because they are not part of this host tree.
// [ts] if (node.child == null || node.tag == HostPortal) {
// [ts] continue siblings;
// [ts] } else {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] }
// [ts] } // Check if this host node is stable or about to be placed.
// [ts] if (!(node.effectTag & Placement)) {
// Found it!
// [ts] return node.stateNode;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn commitPlacement(finishedWork: auto) {
// [ts] let parentFiber = getHostParentFiber(finishedWork); // Note: these two variables *must* always be updated together.
// [ts] let parent;
// [ts] let isContainer;
// [ts] let parentStateNode = parentFiber.stateNode;
// [ts] switch (parentFiber.tag) {
// [ts] HostComponent => {
// [ts] parent = parentStateNode;
// [ts] isContainer = false;
// break;
// [ts] },
// [ts] HostRoot => {
// [ts] parent = parentStateNode.containerInfo;
// [ts] isContainer = true;
// break;
// [ts] },
// [ts] HostPortal => {
// [ts] parent = parentStateNode.containerInfo;
// [ts] isContainer = true;
// break;
// [ts] },
// eslint-disable-next-line-no-fallthrough
// [ts] else => {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Invalid host parent fiber. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (parentFiber.effectTag & ContentReset) {
// Reset the text content of the parent before doing any insertions
// [ts] resetTextContent(parent); // Clear ContentReset from the effect tag
// [ts] parentFiber.effectTag &= ~ContentReset;
// [ts] }
// [ts] let before = getHostSibling(finishedWork); // We only have the top Fiber that was inserted but we need to recurse down its
// children to find all the terminal nodes.
// [ts] if (isContainer) {
// [ts] insertOrAppendPlacementNodeIntoContainer(finishedWork, before, parent);
// [ts] } else {
// [ts] insertOrAppendPlacementNode(finishedWork, before, parent);
// [ts] }
// [ts] }
// [ts] fn insertOrAppendPlacementNodeIntoContainer(node: auto, before: auto, parent: auto) {
// [ts] let tag = node.tag;
// [ts] let isHost = tag == HostComponent || tag == HostText;
// [ts] if (isHost || enableFundamentalAPI ) {
// [ts] let stateNode = if (isHost) node.stateNode else node.stateNode.instance;
// [ts] if (before) {
// [ts] insertInContainerBefore(parent, stateNode, before);
// [ts] } else {
// [ts] appendChildToContainer(parent, stateNode);
// [ts] }
// [ts] } else if (tag == HostPortal) ; else {
// [ts] let child = node.child;
// [ts] if (child != null) {
// [ts] insertOrAppendPlacementNodeIntoContainer(child, before, parent);
// [ts] let sibling = child.sibling;
// [ts] while (sibling != null) {
// [ts] insertOrAppendPlacementNodeIntoContainer(sibling, before, parent);
// [ts] sibling = sibling.sibling;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn insertOrAppendPlacementNode(node: auto, before: auto, parent: auto) {
// [ts] let tag = node.tag;
// [ts] let isHost = tag == HostComponent || tag == HostText;
// [ts] if (isHost || enableFundamentalAPI ) {
// [ts] let stateNode = if (isHost) node.stateNode else node.stateNode.instance;
// [ts] if (before) {
// [ts] insertBefore(parent, stateNode, before);
// [ts] } else {
// [ts] appendChild(parent, stateNode);
// [ts] }
// [ts] } else if (tag == HostPortal) ; else {
// [ts] let child = node.child;
// [ts] if (child != null) {
// [ts] insertOrAppendPlacementNode(child, before, parent);
// [ts] let sibling = child.sibling;
// [ts] while (sibling != null) {
// [ts] insertOrAppendPlacementNode(sibling, before, parent);
// [ts] sibling = sibling.sibling;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn unmountHostComponents(finishedRoot: auto, current: auto, renderPriorityLevel: auto) {
// We only have the top Fiber that was deleted but we need to recurse down its
// children to find all the terminal nodes.
// [ts] let node = current; // Each iteration, currentParent is populated with node's host parent if not
// currentParentIsValid.
// [ts] let currentParentIsValid = false; // Note: these two variables *must* always be updated together.
// [ts] let currentParent;
// [ts] let currentParentIsContainer;
// [ts] while (true) {
// [ts] if (!currentParentIsValid) {
// [ts] let parent = node.return;
// [ts] findParent: while (true) {
// [ts] if (!(parent != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected to find a host parent. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] let parentStateNode = parent.stateNode;
// [ts] switch (parent.tag) {
// [ts] HostComponent => {
// [ts] currentParent = parentStateNode;
// [ts] currentParentIsContainer = false;
// [ts] break findParent;
// [ts] },
// [ts] HostRoot => {
// [ts] currentParent = parentStateNode.containerInfo;
// [ts] currentParentIsContainer = true;
// [ts] break findParent;
// [ts] },
// [ts] HostPortal => {
// [ts] currentParent = parentStateNode.containerInfo;
// [ts] currentParentIsContainer = true;
// [ts] break findParent;
// [ts] }
// [ts] }
// [ts] parent = parent.return;
// [ts] }
// [ts] currentParentIsValid = true;
// [ts] }
// [ts] if (node.tag == HostComponent || node.tag == HostText) {
// [ts] commitNestedUnmounts(finishedRoot, node, renderPriorityLevel); // After all the children have unmounted, it is now safe to remove the
// node from the tree.
// [ts] if (currentParentIsContainer) {
// [ts] removeChildFromContainer(currentParent, node.stateNode);
// [ts] } else {
// [ts] removeChild(currentParent, node.stateNode);
// [ts] } // Don't visit children because we already visited them.
// [ts] } else if (node.tag == HostPortal) {
// [ts] if (node.child != null) {
// When we go into a portal, it becomes the parent to remove from.
// We will reassign it back when we pop the portal on the way up.
// [ts] currentParent = node.stateNode.containerInfo;
// [ts] currentParentIsContainer = true; // Visit children because portals might contain host components.
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] } else {
// [ts] commitUnmount(finishedRoot, node, renderPriorityLevel); // Visit children because we may find more host components below.
// [ts] if (node.child != null) {
// [ts] node.child.return = node;
// [ts] node = node.child;
// [ts] continue;
// [ts] }
// [ts] }
// [ts] if (node == current) {
// [ts] return;
// [ts] }
// [ts] while (node.sibling == null) {
// [ts] if (node.return == null || node.return == current) {
// [ts] return;
// [ts] }
// [ts] node = node.return;
// [ts] if (node.tag == HostPortal) {
// When we go out of the portal, we need to restore the parent.
// Since we don't keep a stack of them, we will search for it.
// [ts] currentParentIsValid = false;
// [ts] }
// [ts] }
// [ts] node.sibling.return = node.return;
// [ts] node = node.sibling;
// [ts] }
// [ts] }
// [ts] fn commitDeletion(finishedRoot: auto, current: auto, renderPriorityLevel: auto) {
// [ts] {
// Recursively delete all host nodes from the parent.
// Detach refs and call componentWillUnmount() on the whole subtree.
// [ts] unmountHostComponents(finishedRoot, current, renderPriorityLevel);
// [ts] }
// [ts] detachFiber(current);
// [ts] }
// [ts] fn commitWork(current: auto, finishedWork: auto) {
// [ts] switch (finishedWork.tag) {
// [ts] FunctionComponent, ForwardRef, MemoComponent, SimpleMemoComponent, Block => {
// [ts] {
// Layout effects are destroyed during the mutation phase so that all
// destroy functions for all fibers are called before any create functions.
// This prevents sibling component effects from interfering with each other,
// e.g. a destroy function in one component should never override a ref set
// by a create function in another component during the same commit.
// [ts] commitHookEffectListUnmount(Layout | HasEffect, finishedWork);
// [ts] return;
// [ts] }
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostComponent => {
// [ts] {
// [ts] let instance = finishedWork.stateNode;
// [ts] if (instance != null) {
// Commit the work prepared earlier.
// [ts] let newProps = finishedWork.memoizedProps; // For hydration we reuse the update path but we treat the oldProps
// as the newProps. The updatePayload will contain the real change in
// this case.
// [ts] let oldProps = if (current != null) current.memoizedProps else newProps;
// [ts] let type = finishedWork.type; // TODO: Type the updateQueue to be specific to host components.
// [ts] let updatePayload = finishedWork.updateQueue;
// [ts] finishedWork.updateQueue = null;
// [ts] if (updatePayload != null) {
// [ts] commitUpdate(instance, updatePayload, type, oldProps, newProps);
// [ts] }
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostText => {
// [ts] {
// [ts] if (!(finishedWork.stateNode != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "This should have a text node initialized. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] let textInstance = finishedWork.stateNode;
// [ts] let newText = finishedWork.memoizedProps; // For hydration we reuse the update path but we treat the oldProps
// as the newProps. The updatePayload will contain the real change in
// this case.
// [ts] let oldText = if (current != null) current.memoizedProps else newText;
// [ts] commitTextUpdate(textInstance, oldText, newText);
// [ts] return;
// [ts] }
// [ts] },
// [ts] HostRoot => {
// [ts] {
// [ts] {
// [ts] let _root = finishedWork.stateNode;
// [ts] if (_root.hydrate) {
// We've just hydrated. No need to hydrate again.
// [ts] _root.hydrate = false;
// [ts] commitHydratedContainer(_root.containerInfo);
// [ts] }
// [ts] }
// [ts] return;
// [ts] }
// [ts] },
// [ts] Profiler => {
// [ts] {
// [ts] return;
// [ts] }
// [ts] },
// [ts] SuspenseComponent => {
// [ts] {
// [ts] commitSuspenseComponent(finishedWork);
// [ts] attachSuspenseRetryListeners(finishedWork);
// [ts] return;
// [ts] }
// [ts] },
// [ts] SuspenseListComponent => {
// [ts] {
// [ts] attachSuspenseRetryListeners(finishedWork);
// [ts] return;
// [ts] }
// [ts] },
// [ts] IncompleteClassComponent => {
// [ts] {
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "This unit of work tag should not have side-effects. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn commitSuspenseComponent(finishedWork: auto) {
// [ts] let newState = finishedWork.memoizedState;
// [ts] let newDidTimeout;
// [ts] let primaryChildParent = finishedWork;
// [ts] if (newState == null) {
// [ts] newDidTimeout = false;
// [ts] } else {
// [ts] newDidTimeout = true;
// [ts] primaryChildParent = finishedWork.child;
// [ts] markCommitTimeOfFallback();
// [ts] }
// [ts] if ( primaryChildParent != null) {
// [ts] hideOrUnhideAllChildren(primaryChildParent, newDidTimeout);
// [ts] }
// [ts] }
// [ts] fn commitSuspenseHydrationCallbacks(finishedRoot: auto, finishedWork: auto) {
// [ts] let newState = finishedWork.memoizedState;
// [ts] if (newState == null) {
// [ts] let current = finishedWork.alternate;
// [ts] if (current != null) {
// [ts] let prevState = current.memoizedState;
// [ts] if (prevState != null) {
// [ts] let suspenseInstance = prevState.dehydrated;
// [ts] if (suspenseInstance != null) {
// [ts] commitHydratedSuspenseInstance(suspenseInstance);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn attachSuspenseRetryListeners(finishedWork: auto) {
// If this boundary just timed out, then it will have a set of thenables.
// For each thenable, attach a listener so that when it resolves, React
// attempts to re-render the boundary in the primary (pre-timeout) state.
// [ts] let thenables = finishedWork.updateQueue;
// [ts] if (thenables != null) {
// [ts] finishedWork.updateQueue = null;
// [ts] let retryCache = finishedWork.stateNode;
// [ts] if (retryCache == null) {
// [ts] retryCache = finishedWork.stateNode = PossiblyWeakSet.init();
// [ts] }
// [ts] thenables.forEach((thenable) => {
// Memoize using the boundary fiber to prevent redundant listeners.
// [ts] let retry = resolveRetryThenable.bind(null, finishedWork, thenable);
// [ts] if (!retryCache.has(thenable)) {
// [ts] {
// [ts] if (thenable.__reactDoNotTraceInteractions != true) {
// [ts] retry = unstable_wrap(retry);
// [ts] }
// [ts] }
// [ts] retryCache.add(thenable);
// [ts] thenable.then(retry, retry);
// [ts] }
// [ts] });
// [ts] }
// [ts] }
// [ts] fn commitResetTextContent(current: auto) {
// [ts] resetTextContent(current.stateNode);
// [ts] }
// [ts] let PossiblyWeakMap$1 = if (@typeName(WeakMap) == "function") WeakMap else Map;
// [ts] fn createRootErrorUpdate(fiber: auto, errorInfo: auto, expirationTime: auto) {
// [ts] let update = createUpdate(expirationTime, null); // Unmount the root by rendering null.
// [ts] update.tag = CaptureUpdate; // Caution: React DevTools currently depends on this property
// being called "element".
// [ts] update.payload = {
// [ts] element: null
// [ts] };
// [ts] let error = errorInfo.value;
// [ts] update.callback = () => {
// [ts] onUncaughtError(error);
// [ts] logError(fiber, errorInfo);
// [ts] };
// [ts] return update;
// [ts] }
// [ts] fn createClassErrorUpdate(fiber: auto, errorInfo: auto, expirationTime: auto) {
// [ts] let update = createUpdate(expirationTime, null);
// [ts] update.tag = CaptureUpdate;
// [ts] let getDerivedStateFromError = fiber.type.getDerivedStateFromError;
// [ts] if (@typeName(getDerivedStateFromError) == "function") {
// [ts] let error$1 = errorInfo.value;
// [ts] update.payload = () => {
// [ts] logError(fiber, errorInfo);
// [ts] return getDerivedStateFromError(error$1);
// [ts] };
// [ts] }
// [ts] let inst = fiber.stateNode;
// [ts] if (inst != null && @typeName(inst.componentDidCatch) == "function") {
// [ts] update.callback = () => {
// [ts] {
// [ts] markFailedErrorBoundaryForHotReloading(fiber);
// [ts] }
// [ts] if (@typeName(getDerivedStateFromError) != "function") {
// To preserve the preexisting retry behavior of error boundaries,
// we keep track of which ones already failed during this batch.
// This gets reset before we yield back to the browser.
// TODO: Warn in strict mode if getDerivedStateFromError is
// not defined.
// [ts] markLegacyErrorBoundaryAsFailed(this); // Only log here if componentDidCatch is the only error boundary method defined
// [ts] logError(fiber, errorInfo);
// [ts] }
// [ts] let error$1 = errorInfo.value;
// [ts] let stack = errorInfo.stack;
// [ts] _.componentDidCatch(error$1, {
// [ts] componentStack: if (stack != null) stack else ""
// [ts] });
// [ts] {
// [ts] if (@typeName(getDerivedStateFromError) != "function") {
// If componentDidCatch is the only error boundary method defined,
// then it needs to call setState to recover from errors.
// If no state update is scheduled then the boundary will swallow the error.
// [ts] if (fiber.expirationTime != Sync) {
// [ts] error("%s: Error boundaries should implement getDerivedStateFromError(). " + "In that method, return a state update to display an error message or fallback UI.", getComponentName(fiber.type) || "Unknown");
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] } else {
// [ts] update.callback = () => {
// [ts] markFailedErrorBoundaryForHotReloading(fiber);
// [ts] };
// [ts] }
// [ts] return update;
// [ts] }
// [ts] fn attachPingListener(root: auto, renderExpirationTime: auto, thenable: auto) {
// Attach a listener to the promise to "ping" the root and retry. But
// only if one does not already exist for the current render expiration
// time (which acts like a "thread ID" here).
// [ts] let pingCache = root.pingCache;
// [ts] let threadIDs;
// [ts] if (pingCache == null) {
// [ts] pingCache = root.pingCache = PossiblyWeakMap$1.init();
// [ts] threadIDs = Set.init();
// [ts] pingCache.set(thenable, threadIDs);
// [ts] } else {
// [ts] threadIDs = pingCache.get(thenable);
// [ts] if (threadIDs == null) {
// [ts] threadIDs = Set.init();
// [ts] pingCache.set(thenable, threadIDs);
// [ts] }
// [ts] }
// [ts] if (!threadIDs.has(renderExpirationTime)) {
// Memoize using the thread ID to prevent redundant listeners.
// [ts] threadIDs.add(renderExpirationTime);
// [ts] let ping = pingSuspendedRoot.bind(null, root, thenable, renderExpirationTime);
// [ts] thenable.then(ping, ping);
// [ts] }
// [ts] }
// [ts] fn throwException(root: auto, returnFiber: auto, sourceFiber: auto, value: auto, renderExpirationTime: auto) {
// The source fiber did not complete.
// [ts] sourceFiber.effectTag |= Incomplete; // Its effect list is no longer valid.
// [ts] sourceFiber.firstEffect = sourceFiber.lastEffect = null;
// [ts] if (value != null && @typeName(value) == "object" && @typeName(value.then) == "function") {
// This is a thenable.
// [ts] let thenable = value;
// [ts] if ((sourceFiber.mode & BlockingMode) == NoMode) {
// Reset the memoizedState to what it was before we attempted
// to render it.
// [ts] let currentSource = sourceFiber.alternate;
// [ts] if (currentSource) {
// [ts] sourceFiber.updateQueue = currentSource.updateQueue;
// [ts] sourceFiber.memoizedState = currentSource.memoizedState;
// [ts] sourceFiber.expirationTime = currentSource.expirationTime;
// [ts] } else {
// [ts] sourceFiber.updateQueue = null;
// [ts] sourceFiber.memoizedState = null;
// [ts] }
// [ts] }
// [ts] let hasInvisibleParentBoundary = hasSuspenseContext(suspenseStackCursor.current, InvisibleParentSuspenseContext); // Schedule the nearest Suspense to re-render the timed out view.
// [ts] let _workInProgress = returnFiber;
// [ts] do {
// [ts] if (_workInProgress.tag == SuspenseComponent && shouldCaptureSuspense(_workInProgress, hasInvisibleParentBoundary)) {
// Found the nearest boundary.
// Stash the promise on the boundary fiber. If the boundary times out, we'll
// attach another listener to flip the boundary back to its normal state.
// [ts] let thenables = _workInProgress.updateQueue;
// [ts] if (thenables == null) {
// [ts] let updateQueue = Set.init();
// [ts] updateQueue.add(thenable);
// [ts] _workInProgress.updateQueue = updateQueue;
// [ts] } else {
// [ts] thenables.add(thenable);
// [ts] } // If the boundary is outside of blocking mode, we should *not*
// suspend the commit. Pretend as if the suspended component rendered
// null and keep rendering. In the commit phase, we'll schedule a
// subsequent synchronous update to re-render the Suspense.
//
// Note: It doesn't matter whether the component that suspended was
// inside a blocking mode tree. If the Suspense is outside of it, we
// should *not* suspend the commit.
// [ts] if ((_workInProgress.mode & BlockingMode) == NoMode) {
// [ts] _workInProgress.effectTag |= DidCapture; // We're going to commit this fiber even though it didn't complete.
// But we shouldn't call any lifecycle methods or callbacks. Remove
// all lifecycle effect tags.
// [ts] sourceFiber.effectTag &= ~(LifecycleEffectMask | Incomplete);
// [ts] if (sourceFiber.tag == ClassComponent) {
// [ts] let currentSourceFiber = sourceFiber.alternate;
// [ts] if (currentSourceFiber == null) {
// This is a new mount. Change the tag so it's not mistaken for a
// completed class component. For example, we should not call
// componentWillUnmount if it is deleted.
// [ts] sourceFiber.tag = IncompleteClassComponent;
// [ts] } else {
// When we try rendering again, we should not reuse the current fiber,
// since it's known to be in an inconsistent state. Use a force update to
// prevent a bail out.
// [ts] let update = createUpdate(Sync, null);
// [ts] update.tag = ForceUpdate;
// [ts] enqueueUpdate(sourceFiber, update);
// [ts] }
// [ts] } // The source fiber did not complete. Mark it with Sync priority to
// indicate that it still has pending work.
// [ts] sourceFiber.expirationTime = Sync; // Exit without suspending.
// [ts] return;
// [ts] } // Confirmed that the boundary is in a concurrent mode tree. Continue
// with the normal suspend path.
//
// After this we'll use a set of heuristics to determine whether this
// render pass will run to completion or restart or "suspend" the commit.
// The actual logic for this is spread out in different places.
//
// This first principle is that if we're going to suspend when we complete
// a root, then we should also restart if we get an update or ping that
// might unsuspend it, and vice versa. The only reason to suspend is
// because you think you might want to restart before committing. However,
// it doesn't make sense to restart only while in the period we're suspended.
//
// Restarting too aggressively is also not good because it starves out any
// intermediate loading state. So we use heuristics to determine when.
// Suspense Heuristics
//
// If nothing threw a Promise or all the same fallbacks are already showing,
// then don't suspend/restart.
//
// If this is an initial render of a new tree of Suspense boundaries and
// those trigger a fallback, then don't suspend/restart. We want to ensure
// that we can show the initial loading state as quickly as possible.
//
// If we hit a "Delayed" case, such as when we'd switch from content back into
// a fallback, then we should always suspend/restart. SuspenseConfig applies to
// this case. If none is defined, JND is used instead.
//
// If we're already showing a fallback and it gets "retried", allowing us to show
// another level, but there's still an inner boundary that would show a fallback,
// then we suspend/restart for 500ms since the last time we showed a fallback
// anywhere in the tree. This effectively throttles progressive loading into a
// consistent train of commits. This also gives us an opportunity to restart to
// get to the completed state slightly earlier.
//
// If there's ambiguity due to batching it's resolved in preference of:
// 1) "delayed", 2) "initial render", 3) "retry".
//
// We want to ensure that a "busy" state doesn't get force committed. We want to
// ensure that new initial loading states can commit as soon as possible.
// [ts] attachPingListener(root, renderExpirationTime, thenable);
// [ts] _workInProgress.effectTag |= ShouldCapture;
// [ts] _workInProgress.expirationTime = renderExpirationTime;
// [ts] return;
// [ts] } // This boundary already captured during this render. Continue to the next
// boundary.
// [ts] _workInProgress = _workInProgress.return;
// [ts] } while (_workInProgress != null); // No boundary was found. Fallthrough to error mode.
// TODO: Use invariant so the message is stripped in prod?
// [ts] value = Error.init((getComponentName(sourceFiber.type) || "A React component") + " suspended while rendering, but no fallback UI was specified.\n" + "\n" + "Add a <Suspense fallback=...> component higher in the tree to " + "provide a loading indicator or placeholder to display." + getStackByFiberInDevAndProd(sourceFiber));
// [ts] } // We didn't find a boundary that could handle this type of exception. Start
// over and traverse parent path again, this time treating the exception
// as an error.
// [ts] renderDidError();
// [ts] value = createCapturedValue(value, sourceFiber);
// [ts] let workInProgress = returnFiber;
// [ts] do {
// [ts] switch (workInProgress.tag) {
// [ts] HostRoot => {
// [ts] {
// [ts] let _errorInfo = value;
// [ts] workInProgress.effectTag |= ShouldCapture;
// [ts] workInProgress.expirationTime = renderExpirationTime;
// [ts] let _update = createRootErrorUpdate(workInProgress, _errorInfo, renderExpirationTime);
// [ts] enqueueCapturedUpdate(workInProgress, _update);
// [ts] return;
// [ts] }
// [ts] },
// Capture and retry
// [ts] ClassComponent => {
// [ts] let errorInfo = value;
// [ts] let ctor = workInProgress.type;
// [ts] let instance = workInProgress.stateNode;
// [ts] if ((workInProgress.effectTag & DidCapture) == NoEffect && (@typeName(ctor.getDerivedStateFromError) == "function" || instance != null && @typeName(instance.componentDidCatch) == "function" && !isAlreadyFailedLegacyErrorBoundary(instance))) {
// [ts] workInProgress.effectTag |= ShouldCapture;
// [ts] workInProgress.expirationTime = renderExpirationTime; // Schedule the error boundary to re-render using updated state
// [ts] let _update2 = createClassErrorUpdate(workInProgress, errorInfo, renderExpirationTime);
// [ts] enqueueCapturedUpdate(workInProgress, _update2);
// [ts] return;
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] workInProgress = workInProgress.return;
// [ts] } while (workInProgress != null);
// [ts] }
// [ts] let ceil = Math.ceil;
// [ts] let ReactCurrentDispatcher$1 = ReactSharedInternals.ReactCurrentDispatcher,
// [ts] ReactCurrentOwner$2 = ReactSharedInternals.ReactCurrentOwner,
// [ts] IsSomeRendererActing = ReactSharedInternals.IsSomeRendererActing;
// [ts] let NoContext =
// [ts] /*                    */
// [ts] 0;
// [ts] let BatchedContext =
// [ts] /*               */
// [ts] 1;
// [ts] let EventContext =
// [ts] /*                 */
// [ts] 2;
// [ts] let DiscreteEventContext =
// [ts] /*         */
// [ts] 4;
// [ts] let LegacyUnbatchedContext =
// [ts] /*       */
// [ts] 8;
// [ts] let RenderContext =
// [ts] /*                */
// [ts] 16;
// [ts] let CommitContext =
// [ts] /*                */
// [ts] 32;
// [ts] let RootIncomplete = 0;
// [ts] let RootFatalErrored = 1;
// [ts] let RootErrored = 2;
// [ts] let RootSuspended = 3;
// [ts] let RootSuspendedWithDelay = 4;
// [ts] let RootCompleted = 5;
// Describes where we are in the React execution stack
// [ts] let executionContext = NoContext; // The root we're working on
// [ts] let workInProgressRoot = null; // The fiber we're working on
// [ts] let workInProgress = null; // The expiration time we're rendering
// [ts] let renderExpirationTime$1 = NoWork; // Whether to root completed, errored, suspended, etc.
// [ts] let workInProgressRootExitStatus = RootIncomplete; // A fatal error, if one is thrown
// [ts] let workInProgressRootFatalError = null; // Most recent event time among processed updates during this render.
// This is conceptually a time stamp but expressed in terms of an ExpirationTime
// because we deal mostly with expiration times in the hot path, so this avoids
// the conversion happening in the hot path.
// [ts] let workInProgressRootLatestProcessedExpirationTime = Sync;
// [ts] let workInProgressRootLatestSuspenseTimeout = Sync;
// [ts] let workInProgressRootCanSuspendUsingConfig = null; // The work left over by components that were visited during this render. Only
// includes unprocessed updates, not work in bailed out children.
// [ts] let workInProgressRootNextUnprocessedUpdateTime = NoWork; // If we're pinged while rendering we don't always restart immediately.
// This flag determines if it might be worthwhile to restart if an opportunity
// happens latere.
// [ts] let workInProgressRootHasPendingPing = false; // The most recent time we committed a fallback. This lets us ensure a train
// model where we don't commit new loading states in too quick succession.
// [ts] let globalMostRecentFallbackTime = 0;
// [ts] let FALLBACK_THROTTLE_MS = 500;
// [ts] let nextEffect = null;
// [ts] let hasUncaughtError = false;
// [ts] let firstUncaughtError = null;
// [ts] let legacyErrorBoundariesThatAlreadyFailed = null;
// [ts] let rootDoesHavePassiveEffects = false;
// [ts] let rootWithPendingPassiveEffects = null;
// [ts] let pendingPassiveEffectsRenderPriority = NoPriority;
// [ts] let pendingPassiveEffectsExpirationTime = NoWork;
// [ts] let rootsWithPendingDiscreteUpdates = null; // Use these to prevent an infinite loop of nested updates
// [ts] let NESTED_UPDATE_LIMIT = 50;
// [ts] let nestedUpdateCount = 0;
// [ts] let rootWithNestedUpdates = null;
// [ts] let NESTED_PASSIVE_UPDATE_LIMIT = 50;
// [ts] let nestedPassiveUpdateCount = 0;
// [ts] let interruptedBy = null; // Marks the need to reschedule pending interactions at these expiration times
// during the commit phase. This enables them to be traced across components
// that spawn new work during render. E.g. hidden boundaries, suspended SSR
// hydration or SuspenseList.
// [ts] let spawnedWorkDuringRender = null; // Expiration times are computed by adding to the current time (the start
// time). However, if two updates are scheduled within the same event, we
// should treat their start times as simultaneous, even if the actual clock
// time has advanced between the first and second call.
// In other words, because expiration times determine how updates are batched,
// we want all updates of like priority that occur within the same event to
// receive the same expiration time. Otherwise we get tearing.
// [ts] let currentEventTime = NoWork;
// [ts] fn requestCurrentTimeForUpdate() {
// [ts] if ((executionContext & (RenderContext | CommitContext)) != NoContext) {
// We're inside React, so it's fine to read the actual time.
// [ts] return msToExpirationTime(now());
// [ts] } // We're not inside React, so we may be in the middle of a browser event.
// [ts] if (currentEventTime != NoWork) {
// Use the same start time for all updates until we enter React again.
// [ts] return currentEventTime;
// [ts] } // This is the first update since React yielded. Compute a new start time.
// [ts] currentEventTime = msToExpirationTime(now());
// [ts] return currentEventTime;
// [ts] }
// [ts] fn getCurrentTime() {
// [ts] return msToExpirationTime(now());
// [ts] }
// [ts] fn computeExpirationForFiber(currentTime: auto, fiber: auto, suspenseConfig: auto) {
// [ts] let mode = fiber.mode;
// [ts] if ((mode & BlockingMode) == NoMode) {
// [ts] return Sync;
// [ts] }
// [ts] let priorityLevel = getCurrentPriorityLevel();
// [ts] if ((mode & ConcurrentMode) == NoMode) {
// [ts] return if (priorityLevel == ImmediatePriority) Sync else Batched;
// [ts] }
// [ts] if ((executionContext & RenderContext) != NoContext) {
// Use whatever time we're already rendering
// TODO: Should there be a way to opt out, like with `runWithPriority`?
// [ts] return renderExpirationTime$1;
// [ts] }
// [ts] let expirationTime;
// [ts] if (suspenseConfig != null) {
// Compute an expiration time based on the Suspense timeout.
// [ts] expirationTime = computeSuspenseExpiration(currentTime, suspenseConfig.timeoutMs | 0 || LOW_PRIORITY_EXPIRATION);
// [ts] } else {
// Compute an expiration time based on the Scheduler priority.
// [ts] switch (priorityLevel) {
// [ts] ImmediatePriority => {
// [ts] expirationTime = Sync;
// break;
// [ts] },
// TODO: Rename this to computeUserBlockingExpiration
// [ts] UserBlockingPriority$1 => {
// [ts] expirationTime = computeInteractiveExpiration(currentTime);
// break;
// [ts] },
// TODO: Handle LowPriority
// TODO: Rename this to... something better.
// [ts] NormalPriority, LowPriority => {
// [ts] expirationTime = computeAsyncExpiration(currentTime);
// break;
// [ts] },
// [ts] IdlePriority => {
// [ts] expirationTime = Idle;
// break;
// [ts] },
// [ts] else => {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Expected a valid priority level" );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] } // If we're in the middle of rendering a tree, do not update at the same
// expiration time that is already rendering.
// TODO: We shouldn't have to do this if the update is on a different root.
// Refactor computeExpirationForFiber + scheduleUpdate so we have access to
// the root when we check for this condition.
// [ts] if (workInProgressRoot != null && expirationTime == renderExpirationTime$1) {
// This is a trick to move this update into a separate batch
// [ts] expirationTime -= 1;
// [ts] }
// [ts] return expirationTime;
// [ts] }
// [ts] fn scheduleUpdateOnFiber(fiber: auto, expirationTime: auto) {
// [ts] checkForNestedUpdates();
// [ts] warnAboutRenderPhaseUpdatesInDEV(fiber);
// [ts] let root = markUpdateTimeFromFiberToRoot(fiber, expirationTime);
// [ts] if (root == null) {
// [ts] warnAboutUpdateOnUnmountedFiberInDEV(fiber);
// [ts] return;
// [ts] }
// [ts] checkForInterruption(fiber, expirationTime);
// [ts] recordScheduleUpdate(); // TODO: computeExpirationForFiber also reads the priority. Pass the
// priority as an argument to that function and this one.
// [ts] let priorityLevel = getCurrentPriorityLevel();
// [ts] if (expirationTime == Sync) {
// [ts] if ( // Check if we're inside unbatchedUpdates
// [ts] (executionContext & LegacyUnbatchedContext) != NoContext && // Check if we're not already rendering
// [ts] (executionContext & (RenderContext | CommitContext)) == NoContext) {
// Register pending interactions on the root to avoid losing traced interaction data.
// [ts] schedulePendingInteractions(root, expirationTime); // This is a legacy edge case. The initial mount of a ReactDOM.render-ed
// root inside of batchedUpdates should be synchronous, but layout updates
// should be deferred until the end of the batch.
// [ts] performSyncWorkOnRoot(root);
// [ts] } else {
// [ts] ensureRootIsScheduled(root);
// [ts] schedulePendingInteractions(root, expirationTime);
// [ts] if (executionContext == NoContext) {
// Flush the synchronous work now, unless we're already working or inside
// a batch. This is intentionally inside scheduleUpdateOnFiber instead of
// scheduleCallbackForFiber to preserve the ability to schedule a callback
// without immediately flushing it. We only do this for user-initiated
// updates, to preserve historical behavior of legacy mode.
// [ts] flushSyncCallbackQueue();
// [ts] }
// [ts] }
// [ts] } else {
// [ts] ensureRootIsScheduled(root);
// [ts] schedulePendingInteractions(root, expirationTime);
// [ts] }
// [ts] if ((executionContext & DiscreteEventContext) != NoContext && ( // Only updates at user-blocking priority or greater are considered
// discrete, even inside a discrete event.
// [ts] priorityLevel == UserBlockingPriority$1 || priorityLevel == ImmediatePriority)) {
// This is the result of a discrete event. Track the lowest priority
// discrete update per root so we can flush them early, if needed.
// [ts] if (rootsWithPendingDiscreteUpdates == null) {
// [ts] rootsWithPendingDiscreteUpdates = Map.init([[root, expirationTime]]);
// [ts] } else {
// [ts] let lastDiscreteTime = rootsWithPendingDiscreteUpdates.get(root);
// [ts] if (lastDiscreteTime == null || lastDiscreteTime > expirationTime) {
// [ts] rootsWithPendingDiscreteUpdates.set(root, expirationTime);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let scheduleWork = scheduleUpdateOnFiber; // This is split into a separate function so we can mark a fiber with pending
// work without treating it as a typical update that originates from an event;
// e.g. retrying a Suspense boundary isn't an update, but it does schedule work
// on a fiber.
// [ts] fn markUpdateTimeFromFiberToRoot(fiber: auto, expirationTime: auto) {
// Update the source fiber's expiration time
// [ts] if (fiber.expirationTime < expirationTime) {
// [ts] fiber.expirationTime = expirationTime;
// [ts] }
// [ts] let alternate = fiber.alternate;
// [ts] if (alternate != null && alternate.expirationTime < expirationTime) {
// [ts] alternate.expirationTime = expirationTime;
// [ts] } // Walk the parent path to the root and update the child expiration time.
// [ts] let node = fiber.return;
// [ts] let root = null;
// [ts] if (node == null && fiber.tag == HostRoot) {
// [ts] root = fiber.stateNode;
// [ts] } else {
// [ts] while (node != null) {
// [ts] alternate = node.alternate;
// [ts] if (node.childExpirationTime < expirationTime) {
// [ts] node.childExpirationTime = expirationTime;
// [ts] if (alternate != null && alternate.childExpirationTime < expirationTime) {
// [ts] alternate.childExpirationTime = expirationTime;
// [ts] }
// [ts] } else if (alternate != null && alternate.childExpirationTime < expirationTime) {
// [ts] alternate.childExpirationTime = expirationTime;
// [ts] }
// [ts] if (node.return == null && node.tag == HostRoot) {
// [ts] root = node.stateNode;
// [ts] break;
// [ts] }
// [ts] node = node.return;
// [ts] }
// [ts] }
// [ts] if (root != null) {
// [ts] if (workInProgressRoot == root) {
// Received an update to a tree that's in the middle of rendering. Mark
// that's unprocessed work on this root.
// [ts] markUnprocessedUpdateTime(expirationTime);
// [ts] if (workInProgressRootExitStatus == RootSuspendedWithDelay) {
// The root already suspended with a delay, which means this render
// definitely won't finish. Since we have a new update, let's mark it as
// suspended now, right before marking the incoming update. This has the
// effect of interrupting the current render and switching to the update.
// TODO: This happens to work when receiving an update during the render
// phase, because of the trick inside computeExpirationForFiber to
// subtract 1 from `renderExpirationTime` to move it into a
// separate bucket. But we should probably model it with an exception,
// using the same mechanism we use to force hydration of a subtree.
// TODO: This does not account for low pri updates that were already
// scheduled before the root started rendering. Need to track the next
// pending expiration time (perhaps by backtracking the return path) and
// then trigger a restart in the `renderDidSuspendDelayIfPossible` path.
// [ts] markRootSuspendedAtTime(root, renderExpirationTime$1);
// [ts] }
// [ts] } // Mark that the root has a pending update.
// [ts] markRootUpdatedAtTime(root, expirationTime);
// [ts] }
// [ts] return root;
// [ts] }
// [ts] fn getNextRootExpirationTimeToWorkOn(root: auto) {
// Determines the next expiration time that the root should render, taking
// into account levels that may be suspended, or levels that may have
// received a ping.
// [ts] let lastExpiredTime = root.lastExpiredTime;
// [ts] if (lastExpiredTime != NoWork) {
// [ts] return lastExpiredTime;
// [ts] } // "Pending" refers to any update that hasn't committed yet, including if it
// suspended. The "suspended" range is therefore a subset.
// [ts] let firstPendingTime = root.firstPendingTime;
// [ts] if (!isRootSuspendedAtTime(root, firstPendingTime)) {
// The highest priority pending time is not suspended. Let's work on that.
// [ts] return firstPendingTime;
// [ts] } // If the first pending time is suspended, check if there's a lower priority
// pending level that we know about. Or check if we received a ping. Work
// on whichever is higher priority.
// [ts] let lastPingedTime = root.lastPingedTime;
// [ts] let nextKnownPendingLevel = root.nextKnownPendingLevel;
// [ts] let nextLevel = if (lastPingedTime > nextKnownPendingLevel) lastPingedTime else nextKnownPendingLevel;
// [ts] if ( nextLevel <= Idle && firstPendingTime != nextLevel) {
// Don't work on Idle/Never priority unless everything else is committed.
// [ts] return NoWork;
// [ts] }
// [ts] return nextLevel;
// [ts] } // Use this function to schedule a task for a root. There's only one task per
// root; if a task was already scheduled, we'll check to make sure the
// expiration time of the existing task is the same as the expiration time of
// the next level that the root has work on. This function is called on every
// update, and right before exiting a task.
// [ts] fn ensureRootIsScheduled(root: auto) {
// [ts] let lastExpiredTime = root.lastExpiredTime;
// [ts] if (lastExpiredTime != NoWork) {
// Special case: Expired work should flush synchronously.
// [ts] root.callbackExpirationTime = Sync;
// [ts] root.callbackPriority = ImmediatePriority;
// [ts] root.callbackNode = scheduleSyncCallback(performSyncWorkOnRoot.bind(null, root));
// [ts] return;
// [ts] }
// [ts] let expirationTime = getNextRootExpirationTimeToWorkOn(root);
// [ts] let existingCallbackNode = root.callbackNode;
// [ts] if (expirationTime == NoWork) {
// There's nothing to work on.
// [ts] if (existingCallbackNode != null) {
// [ts] root.callbackNode = null;
// [ts] root.callbackExpirationTime = NoWork;
// [ts] root.callbackPriority = NoPriority;
// [ts] }
// [ts] return;
// [ts] } // TODO: If this is an update, we already read the current time. Pass the
// time as an argument.
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] let priorityLevel = inferPriorityFromExpirationTime(currentTime, expirationTime); // If there's an existing render task, confirm it has the correct priority and
// expiration time. Otherwise, we'll cancel it and schedule a new one.
// [ts] if (existingCallbackNode != null) {
// [ts] let existingCallbackPriority = root.callbackPriority;
// [ts] let existingCallbackExpirationTime = root.callbackExpirationTime;
// [ts] if ( // Callback must have the exact same expiration time.
// [ts] existingCallbackExpirationTime == expirationTime && // Callback must have greater or equal priority.
// [ts] existingCallbackPriority >= priorityLevel) {
// Existing callback is sufficient.
// [ts] return;
// [ts] } // Need to schedule a new task.
// TODO: Instead of scheduling a new task, we should be able to change the
// priority of the existing one.
// [ts] cancelCallback(existingCallbackNode);
// [ts] }
// [ts] root.callbackExpirationTime = expirationTime;
// [ts] root.callbackPriority = priorityLevel;
// [ts] let callbackNode;
// [ts] if (expirationTime == Sync) {
// Sync React callbacks are scheduled on a special internal queue
// [ts] callbackNode = scheduleSyncCallback(performSyncWorkOnRoot.bind(null, root));
// [ts] } else {
// [ts] callbackNode = scheduleCallback(priorityLevel, performConcurrentWorkOnRoot.bind(null, root), // Compute a task timeout based on the expiration time. This also affects
// ordering because tasks are processed in timeout order.
// [ts] {
// [ts] timeout: expirationTimeToMs(expirationTime) - now()
// [ts] });
// [ts] }
// [ts] root.callbackNode = callbackNode;
// [ts] } // This is the entry point for every concurrent task, i.e. anything that
// goes through Scheduler.
// [ts] fn performConcurrentWorkOnRoot(root: auto, didTimeout: auto) {
// Since we know we're in a React event, we can clear the current
// event time. The next update will compute a new event time.
// [ts] currentEventTime = NoWork;
// [ts] if (didTimeout) {
// The render task took too long to complete. Mark the current time as
// expired to synchronously render all expired work in a single batch.
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] markRootExpiredAtTime(root, currentTime); // This will schedule a synchronous callback.
// [ts] ensureRootIsScheduled(root);
// [ts] return null;
// [ts] } // Determine the next expiration time to work on, using the fields stored
// on the root.
// [ts] let expirationTime = getNextRootExpirationTimeToWorkOn(root);
// [ts] if (expirationTime != NoWork) {
// [ts] let originalCallbackNode = root.callbackNode;
// [ts] if (!((executionContext & (RenderContext | CommitContext)) == NoContext)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should not already be working." );)
// [ts] }
// [ts] }
// [ts] flushPassiveEffects(); // If the root or expiration time have changed, throw out the existing stack
// and prepare a fresh one. Otherwise we'll continue where we left off.
// [ts] if (root != workInProgressRoot || expirationTime != renderExpirationTime$1) {
// [ts] prepareFreshStack(root, expirationTime);
// [ts] startWorkOnPendingInteractions(root, expirationTime);
// [ts] } // If we have a work-in-progress fiber, it means there's still work to do
// in this root.
// [ts] if (workInProgress != null) {
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext |= RenderContext;
// [ts] let prevDispatcher = pushDispatcher();
// [ts] let prevInteractions = pushInteractions(root);
// [ts] startWorkLoopTimer(workInProgress);
// [ts] do {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] workLoopConcurrent();
// [ts] break;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (thrownValue) {)
// [ts] handleError(root, thrownValue);
// [ts] }
// [ts] } while (true);
// [ts] resetContextDependencies();
// [ts] executionContext = prevExecutionContext;
// [ts] popDispatcher(prevDispatcher);
// [ts] {
// [ts] popInteractions(prevInteractions);
// [ts] }
// [ts] if (workInProgressRootExitStatus == RootFatalErrored) {
// [ts] let fatalError = workInProgressRootFatalError;
// [ts] stopInterruptedWorkLoopTimer();
// [ts] prepareFreshStack(root, expirationTime);
// [ts] markRootSuspendedAtTime(root, expirationTime);
// [ts] ensureRootIsScheduled(root);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw fatalError;)
// [ts] }
// [ts] if (workInProgress != null) {
// There's still work left over. Exit without committing.
// [ts] stopInterruptedWorkLoopTimer();
// [ts] } else {
// We now have a consistent tree. The next step is either to commit it,
// or, if something suspended, wait to commit it after a timeout.
// [ts] stopFinishedWorkLoopTimer();
// [ts] let finishedWork = root.finishedWork = root.current.alternate;
// [ts] root.finishedExpirationTime = expirationTime;
// [ts] finishConcurrentRender(root, finishedWork, workInProgressRootExitStatus, expirationTime);
// [ts] }
// [ts] ensureRootIsScheduled(root);
// [ts] if (root.callbackNode == originalCallbackNode) {
// The task node scheduled for this root is the same one that's
// currently executed. Need to return a continuation.
// [ts] return performConcurrentWorkOnRoot.bind(null, root);
// [ts] }
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn finishConcurrentRender(root: auto, finishedWork: auto, exitStatus: auto, expirationTime: auto) {
// Set this to null to indicate there's no in-progress render.
// [ts] workInProgressRoot = null;
// [ts] switch (exitStatus) {
// [ts] RootIncomplete, RootFatalErrored => {
// [ts] {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Root did not complete. This is a bug in React." );)
// [ts] }
// [ts] }
// [ts] }
// Flow knows about invariant, so it complains if I add a break
// statement, but eslint doesn't know about invariant, so it complains
// if I do. eslint-disable-next-line no-fallthrough
// [ts] },
// [ts] RootErrored => {
// [ts] {
// If this was an async render, the error may have happened due to
// a mutation in a concurrent event. Try rendering one more time,
// synchronously, to see if the error goes away. If there are
// lower priority updates, let's include those, too, in case they
// fix the inconsistency. Render at Idle to include all updates.
// If it was Idle or Never or some not-yet-invented time, render
// at that time.
// [ts] markRootExpiredAtTime(if (root, expirationTime > Idle) Idle else expirationTime); // We assume that this second render pass will be synchronous
// and therefore not hit this path again.
// [ts] break;
// [ts] }
// [ts] },
// [ts] RootSuspended => {
// [ts] {
// [ts] markRootSuspendedAtTime(root, expirationTime);
// [ts] let lastSuspendedTime = root.lastSuspendedTime;
// [ts] if (expirationTime == lastSuspendedTime) {
// [ts] root.nextKnownPendingLevel = getRemainingExpirationTime(finishedWork);
// [ts] } // We have an acceptable loading state. We need to figure out if we
// should immediately commit it or wait a bit.
// If we have processed new updates during this render, we may now
// have a new loading state ready. We want to ensure that we commit
// that as soon as possible.
// [ts] let hasNotProcessedNewUpdates = workInProgressRootLatestProcessedExpirationTime == Sync;
// [ts] if (hasNotProcessedNewUpdates && // do not delay if we're inside an act() scope
// [ts] !( IsThisRendererActing.current)) {
// If we have not processed any new updates during this pass, then
// this is either a retry of an existing fallback state or a
// hidden tree. Hidden trees shouldn't be batched with other work
// and after that's fixed it can only be a retry. We're going to
// throttle committing retries so that we don't show too many
// loading states too quickly.
// [ts] let msUntilTimeout = globalMostRecentFallbackTime + FALLBACK_THROTTLE_MS - now(); // Don't bother with a very short suspense time.
// [ts] if (msUntilTimeout > 10) {
// [ts] if (workInProgressRootHasPendingPing) {
// [ts] let lastPingedTime = root.lastPingedTime;
// [ts] if (lastPingedTime == NoWork || lastPingedTime >= expirationTime) {
// This render was pinged but we didn't get to restart
// earlier so try restarting now instead.
// [ts] root.lastPingedTime = expirationTime;
// [ts] prepareFreshStack(root, expirationTime);
// [ts] break;
// [ts] }
// [ts] }
// [ts] let nextTime = getNextRootExpirationTimeToWorkOn(root);
// [ts] if (nextTime != NoWork && nextTime != expirationTime) {
// There's additional work on this root.
// [ts] break;
// [ts] }
// [ts] if (lastSuspendedTime != NoWork && lastSuspendedTime != expirationTime) {
// We should prefer to render the fallback of at the last
// suspended level. Ping the last suspended level to try
// rendering it again.
// [ts] root.lastPingedTime = lastSuspendedTime;
// [ts] break;
// [ts] } // The render is suspended, it hasn't timed out, and there's no
// lower priority work to do. Instead of committing the fallback
// immediately, wait for more data to arrive.
// [ts] root.timeoutHandle = scheduleTimeout(commitRoot.bind(null, root), msUntilTimeout);
// [ts] break;
// [ts] }
// [ts] } // The work expired. Commit immediately.
// [ts] commitRoot(root);
// [ts] break;
// [ts] }
// [ts] },
// [ts] RootSuspendedWithDelay => {
// [ts] {
// [ts] markRootSuspendedAtTime(root, expirationTime);
// [ts] let _lastSuspendedTime = root.lastSuspendedTime;
// [ts] if (expirationTime == _lastSuspendedTime) {
// [ts] root.nextKnownPendingLevel = getRemainingExpirationTime(finishedWork);
// [ts] }
// [ts] if ( // do not delay if we're inside an act() scope
// [ts] !( IsThisRendererActing.current)) {
// We're suspended in a state that should be avoided. We'll try to
// avoid committing it for as long as the timeouts let us.
// [ts] if (workInProgressRootHasPendingPing) {
// [ts] let _lastPingedTime = root.lastPingedTime;
// [ts] if (_lastPingedTime == NoWork || _lastPingedTime >= expirationTime) {
// This render was pinged but we didn't get to restart earlier
// so try restarting now instead.
// [ts] root.lastPingedTime = expirationTime;
// [ts] prepareFreshStack(root, expirationTime);
// [ts] break;
// [ts] }
// [ts] }
// [ts] let _nextTime = getNextRootExpirationTimeToWorkOn(root);
// [ts] if (_nextTime != NoWork && _nextTime != expirationTime) {
// There's additional work on this root.
// [ts] break;
// [ts] }
// [ts] if (_lastSuspendedTime != NoWork && _lastSuspendedTime != expirationTime) {
// We should prefer to render the fallback of at the last
// suspended level. Ping the last suspended level to try
// rendering it again.
// [ts] root.lastPingedTime = _lastSuspendedTime;
// [ts] break;
// [ts] }
// [ts] let _msUntilTimeout;
// [ts] if (workInProgressRootLatestSuspenseTimeout != Sync) {
// We have processed a suspense config whose expiration time we
// can use as the timeout.
// [ts] _msUntilTimeout = expirationTimeToMs(workInProgressRootLatestSuspenseTimeout) - now();
// [ts] } else if (workInProgressRootLatestProcessedExpirationTime == Sync) {
// This should never normally happen because only new updates
// cause delayed states, so we should have processed something.
// However, this could also happen in an offscreen tree.
// [ts] _msUntilTimeout = 0;
// [ts] } else {
// If we don't have a suspense config, we're going to use a
// heuristic to determine how long we can suspend.
// [ts] let eventTimeMs = inferTimeFromExpirationTime(workInProgressRootLatestProcessedExpirationTime);
// [ts] let currentTimeMs = now();
// [ts] let timeUntilExpirationMs = expirationTimeToMs(expirationTime) - currentTimeMs;
// [ts] let timeElapsed = currentTimeMs - eventTimeMs;
// [ts] if (timeElapsed < 0) {
// We get this wrong some time since we estimate the time.
// [ts] timeElapsed = 0;
// [ts] }
// [ts] _msUntilTimeout = jnd(timeElapsed) - timeElapsed; // Clamp the timeout to the expiration time. TODO: Once the
// event time is exact instead of inferred from expiration time
// we don't need this.
// [ts] if (timeUntilExpirationMs < _msUntilTimeout) {
// [ts] _msUntilTimeout = timeUntilExpirationMs;
// [ts] }
// [ts] } // Don't bother with a very short suspense time.
// [ts] if (_msUntilTimeout > 10) {
// The render is suspended, it hasn't timed out, and there's no
// lower priority work to do. Instead of committing the fallback
// immediately, wait for more data to arrive.
// [ts] root.timeoutHandle = scheduleTimeout(commitRoot.bind(null, root), _msUntilTimeout);
// [ts] break;
// [ts] }
// [ts] } // The work expired. Commit immediately.
// [ts] commitRoot(root);
// [ts] break;
// [ts] }
// [ts] },
// [ts] RootCompleted => {
// [ts] {
// The work completed. Ready to commit.
// [ts] if ( // do not delay if we're inside an act() scope
// [ts] !( IsThisRendererActing.current) && workInProgressRootLatestProcessedExpirationTime != Sync && workInProgressRootCanSuspendUsingConfig != null) {
// If we have exceeded the minimum loading delay, which probably
// means we have shown a spinner already, we might have to suspend
// a bit longer to ensure that the spinner is shown for
// enough time.
// [ts] let _msUntilTimeout2 = computeMsUntilSuspenseLoadingDelay(workInProgressRootLatestProcessedExpirationTime, expirationTime, workInProgressRootCanSuspendUsingConfig);
// [ts] if (_msUntilTimeout2 > 10) {
// [ts] markRootSuspendedAtTime(root, expirationTime);
// [ts] root.timeoutHandle = scheduleTimeout(commitRoot.bind(null, root), _msUntilTimeout2);
// [ts] break;
// [ts] }
// [ts] }
// [ts] commitRoot(root);
// [ts] break;
// [ts] }
// [ts] },
// [ts] else => {
// [ts] {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unknown root exit status." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] } // This is the entry point for synchronous tasks that don't go
// through Scheduler
// [ts] fn performSyncWorkOnRoot(root: auto) {
// Check if there's expired work on this root. Otherwise, render at Sync.
// [ts] let lastExpiredTime = root.lastExpiredTime;
// [ts] let expirationTime = if (lastExpiredTime != NoWork) lastExpiredTime else Sync;
// [ts] if (!((executionContext & (RenderContext | CommitContext)) == NoContext)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should not already be working." );)
// [ts] }
// [ts] }
// [ts] flushPassiveEffects(); // If the root or expiration time have changed, throw out the existing stack
// and prepare a fresh one. Otherwise we'll continue where we left off.
// [ts] if (root != workInProgressRoot || expirationTime != renderExpirationTime$1) {
// [ts] prepareFreshStack(root, expirationTime);
// [ts] startWorkOnPendingInteractions(root, expirationTime);
// [ts] } // If we have a work-in-progress fiber, it means there's still work to do
// in this root.
// [ts] if (workInProgress != null) {
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext |= RenderContext;
// [ts] let prevDispatcher = pushDispatcher();
// [ts] let prevInteractions = pushInteractions(root);
// [ts] startWorkLoopTimer(workInProgress);
// [ts] do {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] workLoopSync();
// [ts] break;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (thrownValue) {)
// [ts] handleError(root, thrownValue);
// [ts] }
// [ts] } while (true);
// [ts] resetContextDependencies();
// [ts] executionContext = prevExecutionContext;
// [ts] popDispatcher(prevDispatcher);
// [ts] {
// [ts] popInteractions(prevInteractions);
// [ts] }
// [ts] if (workInProgressRootExitStatus == RootFatalErrored) {
// [ts] let fatalError = workInProgressRootFatalError;
// [ts] stopInterruptedWorkLoopTimer();
// [ts] prepareFreshStack(root, expirationTime);
// [ts] markRootSuspendedAtTime(root, expirationTime);
// [ts] ensureRootIsScheduled(root);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw fatalError;)
// [ts] }
// [ts] if (workInProgress != null) {
// This is a sync render, so we should have finished the whole tree.
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Cannot commit an incomplete root. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] }
// [ts] } else {
// We now have a consistent tree. Because this is a sync render, we
// will commit it even if something suspended.
// [ts] stopFinishedWorkLoopTimer();
// [ts] root.finishedWork = root.current.alternate;
// [ts] root.finishedExpirationTime = expirationTime;
// [ts] finishSyncRender(root);
// [ts] } // Before exiting, make sure there's a callback scheduled for the next
// pending level.
// [ts] ensureRootIsScheduled(root);
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn finishSyncRender(root: auto) {
// Set this to null to indicate there's no in-progress render.
// [ts] workInProgressRoot = null;
// [ts] commitRoot(root);
// [ts] }
// [ts] fn flushDiscreteUpdates() {
// TODO: Should be able to flush inside batchedUpdates, but not inside `act`.
// However, `act` uses `batchedUpdates`, so there's no way to distinguish
// those two cases. Need to fix this before exposing flushDiscreteUpdates
// as a public API.
// [ts] if ((executionContext & (BatchedContext | RenderContext | CommitContext)) != NoContext) {
// [ts] {
// [ts] if ((executionContext & RenderContext) != NoContext) {
// [ts] error("unstable_flushDiscreteUpdates: Cannot flush updates when React is " + "already rendering.");
// [ts] }
// [ts] } // We're already rendering, so we can't synchronously flush pending work.
// This is probably a nested event dispatch triggered by a lifecycle/effect,
// like `el.focus()`. Exit.
// [ts] return;
// [ts] }
// [ts] flushPendingDiscreteUpdates(); // If the discrete updates scheduled passive effects, flush them now so that
// they fire before the next serial event.
// [ts] flushPassiveEffects();
// [ts] }
// [ts] fn syncUpdates(fn: auto, a: auto, b: auto, c: auto) {
// [ts] return runWithPriority$1(ImmediatePriority, fn.bind(null, a, b, c));
// [ts] }
// [ts] fn flushPendingDiscreteUpdates() {
// [ts] if (rootsWithPendingDiscreteUpdates != null) {
// For each root with pending discrete updates, schedule a callback to
// immediately flush them.
// [ts] let roots = rootsWithPendingDiscreteUpdates;
// [ts] rootsWithPendingDiscreteUpdates = null;
// [ts] roots.forEach((expirationTime, root) => {
// [ts] markRootExpiredAtTime(root, expirationTime);
// [ts] ensureRootIsScheduled(root);
// [ts] }); // Now flush the immediate queue.
// [ts] flushSyncCallbackQueue();
// [ts] }
// [ts] }
// [ts] (fn, a) => {
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext |= BatchedContext;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return fn(a);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] executionContext = prevExecutionContext;
// [ts] if (executionContext == NoContext) {
// Flush the immediate callbacks that were scheduled during this batch
// [ts] flushSyncCallbackQueue();
// [ts] }
// [ts] }
// [ts] }
// [ts] (fn, a) => {
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext |= EventContext;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return fn(a);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] executionContext = prevExecutionContext;
// [ts] if (executionContext == NoContext) {
// Flush the immediate callbacks that were scheduled during this batch
// [ts] flushSyncCallbackQueue();
// [ts] }
// [ts] }
// [ts] }
// [ts] (fn, a, b, c, d) => {
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext |= DiscreteEventContext;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Should this
// [ts] return runWithPriority$1(UserBlockingPriority$1, fn.bind(null, a, b, c, d));
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] executionContext = prevExecutionContext;
// [ts] if (executionContext == NoContext) {
// Flush the immediate callbacks that were scheduled during this batch
// [ts] flushSyncCallbackQueue();
// [ts] }
// [ts] }
// [ts] }
// [ts] fn unbatchedUpdates(fn: auto, a: auto) {
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext &= ~BatchedContext;
// [ts] executionContext |= LegacyUnbatchedContext;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return fn(a);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] executionContext = prevExecutionContext;
// [ts] if (executionContext == NoContext) {
// Flush the immediate callbacks that were scheduled during this batch
// [ts] flushSyncCallbackQueue();
// [ts] }
// [ts] }
// [ts] }
// [ts] fn flushSync(fn: auto, a: auto) {
// [ts] if ((executionContext & (RenderContext | CommitContext)) != NoContext) {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "flushSync was called from inside a lifecycle method. It cannot be called when React is already rendering." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext |= BatchedContext;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return runWithPriority$1(ImmediatePriority, fn.bind(null, a));
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] executionContext = prevExecutionContext; // Flush the immediate callbacks that were scheduled during this batch.
// Note that this will happen even if batchedUpdates is higher up
// the stack.
// [ts] flushSyncCallbackQueue();
// [ts] }
// [ts] }
// [ts] fn prepareFreshStack(root: auto, expirationTime: auto) {
// [ts] root.finishedWork = null;
// [ts] root.finishedExpirationTime = NoWork;
// [ts] let timeoutHandle = root.timeoutHandle;
// [ts] if (timeoutHandle != noTimeout) {
// The root previous suspended and scheduled a timeout to commit a fallback
// state. Now that we have additional work, cancel the timeout.
// [ts] root.timeoutHandle = noTimeout; // $FlowFixMe Complains noTimeout is not a TimeoutID, despite the check above
// [ts] cancelTimeout(timeoutHandle);
// [ts] }
// [ts] if (workInProgress != null) {
// [ts] let interruptedWork = workInProgress.return;
// [ts] while (interruptedWork != null) {
// [ts] unwindInterruptedWork(interruptedWork);
// [ts] interruptedWork = interruptedWork.return;
// [ts] }
// [ts] }
// [ts] workInProgressRoot = root;
// [ts] workInProgress = createWorkInProgress(root.current, null);
// [ts] renderExpirationTime$1 = expirationTime;
// [ts] workInProgressRootExitStatus = RootIncomplete;
// [ts] workInProgressRootFatalError = null;
// [ts] workInProgressRootLatestProcessedExpirationTime = Sync;
// [ts] workInProgressRootLatestSuspenseTimeout = Sync;
// [ts] workInProgressRootCanSuspendUsingConfig = null;
// [ts] workInProgressRootNextUnprocessedUpdateTime = NoWork;
// [ts] workInProgressRootHasPendingPing = false;
// [ts] {
// [ts] spawnedWorkDuringRender = null;
// [ts] }
// [ts] {
// [ts] ReactStrictModeWarnings.discardPendingWarnings();
// [ts] }
// [ts] }
// [ts] fn handleError(root: auto, thrownValue: auto) {
// [ts] do {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Reset module-level state that was set during the render phase.
// [ts] resetContextDependencies();
// [ts] resetHooksAfterThrow();
// [ts] resetCurrentFiber();
// [ts] if (workInProgress == null || workInProgress.return == null) {
// Expected to be working on a non-root fiber. This is a fatal error
// because there's no ancestor that can handle it; the root is
// supposed to capture all errors that weren't caught by an error
// boundary.
// [ts] workInProgressRootExitStatus = RootFatalErrored;
// [ts] workInProgressRootFatalError = thrownValue; // Set `workInProgress` to null. This represents advancing to the next
// sibling, or the parent if there are no siblings. But since the root
// has no siblings nor a parent, we set it to null. Usually this is
// handled by `completeUnitOfWork` or `unwindWork`, but since we're
// interntionally not calling those, we need set it here.
// TODO: Consider calling `unwindWork` to pop the contexts.
// [ts] workInProgress = null;
// [ts] return null;
// [ts] }
// [ts] if (enableProfilerTimer && workInProgress.mode & ProfileMode) {
// Record the time spent rendering before an error was thrown. This
// avoids inaccurate Profiler durations in the case of a
// suspended render.
// [ts] stopProfilerTimerIfRunningAndRecordDelta(workInProgress, true);
// [ts] }
// [ts] throwException(root, workInProgress.return, workInProgress, thrownValue, renderExpirationTime$1);
// [ts] workInProgress = completeUnitOfWork(workInProgress);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (yetAnotherThrownValue) {)
// Something in the return path also threw.
// [ts] thrownValue = yetAnotherThrownValue;
// [ts] continue;
// [ts] } // Return to the normal work loop.
// [ts] return;
// [ts] } while (true);
// [ts] }
// [ts] fn pushDispatcher(root: auto) {
// [ts] let prevDispatcher = ReactCurrentDispatcher$1.current;
// [ts] ReactCurrentDispatcher$1.current = ContextOnlyDispatcher;
// [ts] if (prevDispatcher == null) {
// The React isomorphic package does not include a default dispatcher.
// Instead the first renderer will lazily attach one, in order to give
// nicer error messages.
// [ts] return ContextOnlyDispatcher;
// [ts] } else {
// [ts] return prevDispatcher;
// [ts] }
// [ts] }
// [ts] fn popDispatcher(prevDispatcher: auto) {
// [ts] ReactCurrentDispatcher$1.current = prevDispatcher;
// [ts] }
// [ts] fn pushInteractions(root: auto) {
// [ts] {
// [ts] let prevInteractions = __interactionsRef.current;
// [ts] __interactionsRef.current = root.memoizedInteractions;
// [ts] return prevInteractions;
// [ts] }
// [ts] }
// [ts] fn popInteractions(prevInteractions: auto) {
// [ts] {
// [ts] __interactionsRef.current = prevInteractions;
// [ts] }
// [ts] }
// [ts] fn markCommitTimeOfFallback() {
// [ts] globalMostRecentFallbackTime = now();
// [ts] }
// [ts] fn markRenderEventTimeAndConfig(expirationTime: auto, suspenseConfig: auto) {
// [ts] if (expirationTime < workInProgressRootLatestProcessedExpirationTime && expirationTime > Idle) {
// [ts] workInProgressRootLatestProcessedExpirationTime = expirationTime;
// [ts] }
// [ts] if (suspenseConfig != null) {
// [ts] if (expirationTime < workInProgressRootLatestSuspenseTimeout && expirationTime > Idle) {
// [ts] workInProgressRootLatestSuspenseTimeout = expirationTime; // Most of the time we only have one config and getting wrong is not bad.
// [ts] workInProgressRootCanSuspendUsingConfig = suspenseConfig;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markUnprocessedUpdateTime(expirationTime: auto) {
// [ts] if (expirationTime > workInProgressRootNextUnprocessedUpdateTime) {
// [ts] workInProgressRootNextUnprocessedUpdateTime = expirationTime;
// [ts] }
// [ts] }
// [ts] fn renderDidSuspend() {
// [ts] if (workInProgressRootExitStatus == RootIncomplete) {
// [ts] workInProgressRootExitStatus = RootSuspended;
// [ts] }
// [ts] }
// [ts] fn renderDidSuspendDelayIfPossible() {
// [ts] if (workInProgressRootExitStatus == RootIncomplete || workInProgressRootExitStatus == RootSuspended) {
// [ts] workInProgressRootExitStatus = RootSuspendedWithDelay;
// [ts] } // Check if there's a lower priority update somewhere else in the tree.
// [ts] if (workInProgressRootNextUnprocessedUpdateTime != NoWork && workInProgressRoot != null) {
// Mark the current render as suspended, and then mark that there's a
// pending update.
// TODO: This should immediately interrupt the current render, instead
// of waiting until the next time we yield.
// [ts] markRootSuspendedAtTime(workInProgressRoot, renderExpirationTime$1);
// [ts] markRootUpdatedAtTime(workInProgressRoot, workInProgressRootNextUnprocessedUpdateTime);
// [ts] }
// [ts] }
// [ts] fn renderDidError() {
// [ts] if (workInProgressRootExitStatus != RootCompleted) {
// [ts] workInProgressRootExitStatus = RootErrored;
// [ts] }
// [ts] } // Called during render to determine if anything has suspended.
// Returns false if we're not sure.
// [ts] fn renderHasNotSuspendedYet() {
// If something errored or completed, we can't really be sure,
// so those are false.
// [ts] return workInProgressRootExitStatus == RootIncomplete;
// [ts] }
// [ts] fn inferTimeFromExpirationTime(expirationTime: auto) {
// We don't know exactly when the update was scheduled, but we can infer an
// approximate start time from the expiration time.
// [ts] let earliestExpirationTimeMs = expirationTimeToMs(expirationTime);
// [ts] return earliestExpirationTimeMs - LOW_PRIORITY_EXPIRATION;
// [ts] }
// [ts] fn inferTimeFromExpirationTimeWithSuspenseConfig(expirationTime: auto, suspenseConfig: auto) {
// We don't know exactly when the update was scheduled, but we can infer an
// approximate start time from the expiration time by subtracting the timeout
// that was added to the event time.
// [ts] let earliestExpirationTimeMs = expirationTimeToMs(expirationTime);
// [ts] return earliestExpirationTimeMs - (suspenseConfig.timeoutMs | 0 || LOW_PRIORITY_EXPIRATION);
// [ts] } // The work loop is an extremely hot path. Tell Closure not to inline it.
// [ts] /** @noinline */
// [ts] fn workLoopSync() {
// Already timed out, so perform work without checking if we need to yield.
// [ts] while (workInProgress != null) {
// [ts] workInProgress = performUnitOfWork(workInProgress);
// [ts] }
// [ts] }
// [ts] /** @noinline */
// [ts] fn workLoopConcurrent() {
// Perform work until Scheduler asks us to yield
// [ts] while (workInProgress != null && !shouldYield()) {
// [ts] workInProgress = performUnitOfWork(workInProgress);
// [ts] }
// [ts] }
// [ts] fn performUnitOfWork(unitOfWork: auto) {
// The current, flushed, state of this fiber is the alternate. Ideally
// nothing should rely on this, but relying on it here means that we don't
// need an additional field on the work in progress.
// [ts] let current = unitOfWork.alternate;
// [ts] startWorkTimer(unitOfWork);
// [ts] setCurrentFiber(unitOfWork);
// [ts] let next;
// [ts] if ( (unitOfWork.mode & ProfileMode) != NoMode) {
// [ts] startProfilerTimer(unitOfWork);
// [ts] next = beginWork$1(current, unitOfWork, renderExpirationTime$1);
// [ts] stopProfilerTimerIfRunningAndRecordDelta(unitOfWork, true);
// [ts] } else {
// [ts] next = beginWork$1(current, unitOfWork, renderExpirationTime$1);
// [ts] }
// [ts] resetCurrentFiber();
// [ts] unitOfWork.memoizedProps = unitOfWork.pendingProps;
// [ts] if (next == null) {
// If this doesn't spawn new work, complete the current work.
// [ts] next = completeUnitOfWork(unitOfWork);
// [ts] }
// [ts] ReactCurrentOwner$2.current = null;
// [ts] return next;
// [ts] }
// [ts] fn completeUnitOfWork(unitOfWork: auto) {
// Attempt to complete the current unit of work, then move to the next
// sibling. If there are no more siblings, return to the parent fiber.
// [ts] workInProgress = unitOfWork;
// [ts] do {
// The current, flushed, state of this fiber is the alternate. Ideally
// nothing should rely on this, but relying on it here means that we don't
// need an additional field on the work in progress.
// [ts] let current = workInProgress.alternate;
// [ts] let returnFiber = workInProgress.return; // Check if the work completed or if something threw.
// [ts] if ((workInProgress.effectTag & Incomplete) == NoEffect) {
// [ts] setCurrentFiber(workInProgress);
// [ts] let next = void 0;
// [ts] if ( (workInProgress.mode & ProfileMode) == NoMode) {
// [ts] next = completeWork(current, workInProgress, renderExpirationTime$1);
// [ts] } else {
// [ts] startProfilerTimer(workInProgress);
// [ts] next = completeWork(current, workInProgress, renderExpirationTime$1); // Update render duration assuming we didn't error.
// [ts] stopProfilerTimerIfRunningAndRecordDelta(workInProgress, false);
// [ts] }
// [ts] stopWorkTimer(workInProgress);
// [ts] resetCurrentFiber();
// [ts] resetChildExpirationTime(workInProgress);
// [ts] if (next != null) {
// Completing this fiber spawned new work. Work on that next.
// [ts] return next;
// [ts] }
// [ts] if (returnFiber != null && // Do not append effects to parents if a sibling failed to complete
// [ts] (returnFiber.effectTag & Incomplete) == NoEffect) {
// Append all the effects of the subtree and this fiber onto the effect
// list of the parent. The completion order of the children affects the
// side-effect order.
// [ts] if (returnFiber.firstEffect == null) {
// [ts] returnFiber.firstEffect = workInProgress.firstEffect;
// [ts] }
// [ts] if (workInProgress.lastEffect != null) {
// [ts] if (returnFiber.lastEffect != null) {
// [ts] returnFiber.lastEffect.nextEffect = workInProgress.firstEffect;
// [ts] }
// [ts] returnFiber.lastEffect = workInProgress.lastEffect;
// [ts] } // If this fiber had side-effects, we append it AFTER the children's
// side-effects. We can perform certain side-effects earlier if needed,
// by doing multiple passes over the effect list. We don't want to
// schedule our own side-effect on our own list because if end up
// reusing children we'll schedule this effect onto itself since we're
// at the end.
// [ts] let effectTag = workInProgress.effectTag; // Skip both NoWork and PerformedWork tags when creating the effect
// list. PerformedWork effect is read by React DevTools but shouldn't be
// committed.
// [ts] if (effectTag > PerformedWork) {
// [ts] if (returnFiber.lastEffect != null) {
// [ts] returnFiber.lastEffect.nextEffect = workInProgress;
// [ts] } else {
// [ts] returnFiber.firstEffect = workInProgress;
// [ts] }
// [ts] returnFiber.lastEffect = workInProgress;
// [ts] }
// [ts] }
// [ts] } else {
// This fiber did not complete because something threw. Pop values off
// the stack without entering the complete phase. If this is a boundary,
// capture values if possible.
// [ts] let _next = unwindWork(workInProgress); // Because this fiber did not complete, don't reset its expiration time.
// [ts] if ( (workInProgress.mode & ProfileMode) != NoMode) {
// Record the render duration for the fiber that errored.
// [ts] stopProfilerTimerIfRunningAndRecordDelta(workInProgress, false); // Include the time spent working on failed children before continuing.
// [ts] let actualDuration = workInProgress.actualDuration;
// [ts] let child = workInProgress.child;
// [ts] while (child != null) {
// [ts] actualDuration += child.actualDuration;
// [ts] child = child.sibling;
// [ts] }
// [ts] workInProgress.actualDuration = actualDuration;
// [ts] }
// [ts] if (_next != null) {
// If completing this work spawned new work, do that next. We'll come
// back here again.
// Since we're restarting, remove anything that is not a host effect
// from the effect tag.
// TODO: The name stopFailedWorkTimer is misleading because Suspense
// also captures and restarts.
// [ts] stopFailedWorkTimer(workInProgress);
// [ts] _next.effectTag &= HostEffectMask;
// [ts] return _next;
// [ts] }
// [ts] stopWorkTimer(workInProgress);
// [ts] if (returnFiber != null) {
// Mark the parent fiber as incomplete and clear its effect list.
// [ts] returnFiber.firstEffect = returnFiber.lastEffect = null;
// [ts] returnFiber.effectTag |= Incomplete;
// [ts] }
// [ts] }
// [ts] let siblingFiber = workInProgress.sibling;
// [ts] if (siblingFiber != null) {
// If there is more work to do in this returnFiber, do that next.
// [ts] return siblingFiber;
// [ts] } // Otherwise, return to the parent
// [ts] workInProgress = returnFiber;
// [ts] } while (workInProgress != null); // We've reached the root.
// [ts] if (workInProgressRootExitStatus == RootIncomplete) {
// [ts] workInProgressRootExitStatus = RootCompleted;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] fn getRemainingExpirationTime(fiber: auto) {
// [ts] let updateExpirationTime = fiber.expirationTime;
// [ts] let childExpirationTime = fiber.childExpirationTime;
// [ts] return if (updateExpirationTime > childExpirationTime) updateExpirationTime else childExpirationTime;
// [ts] }
// [ts] fn resetChildExpirationTime(completedWork: auto) {
// [ts] if (renderExpirationTime$1 != Never && completedWork.childExpirationTime == Never) {
// The children of this component are hidden. Don't bubble their
// expiration times.
// [ts] return;
// [ts] }
// [ts] let newChildExpirationTime = NoWork; // Bubble up the earliest expiration time.
// [ts] if ( (completedWork.mode & ProfileMode) != NoMode) {
// In profiling mode, resetChildExpirationTime is also used to reset
// profiler durations.
// [ts] let actualDuration = completedWork.actualDuration;
// [ts] let treeBaseDuration = completedWork.selfBaseDuration; // When a fiber is cloned, its actualDuration is reset to 0. This value will
// only be updated if work is done on the fiber (i.e. it doesn't bailout).
// When work is done, it should bubble to the parent's actualDuration. If
// the fiber has not been cloned though, (meaning no work was done), then
// this value will reflect the amount of time spent working on a previous
// render. In that case it should not bubble. We determine whether it was
// cloned by comparing the child pointer.
// [ts] let shouldBubbleActualDurations = completedWork.alternate == null || completedWork.child != completedWork.alternate.child;
// [ts] let child = completedWork.child;
// [ts] while (child != null) {
// [ts] let childUpdateExpirationTime = child.expirationTime;
// [ts] let childChildExpirationTime = child.childExpirationTime;
// [ts] if (childUpdateExpirationTime > newChildExpirationTime) {
// [ts] newChildExpirationTime = childUpdateExpirationTime;
// [ts] }
// [ts] if (childChildExpirationTime > newChildExpirationTime) {
// [ts] newChildExpirationTime = childChildExpirationTime;
// [ts] }
// [ts] if (shouldBubbleActualDurations) {
// [ts] actualDuration += child.actualDuration;
// [ts] }
// [ts] treeBaseDuration += child.treeBaseDuration;
// [ts] child = child.sibling;
// [ts] }
// [ts] completedWork.actualDuration = actualDuration;
// [ts] completedWork.treeBaseDuration = treeBaseDuration;
// [ts] } else {
// [ts] let _child = completedWork.child;
// [ts] while (_child != null) {
// [ts] let _childUpdateExpirationTime = _child.expirationTime;
// [ts] let _childChildExpirationTime = _child.childExpirationTime;
// [ts] if (_childUpdateExpirationTime > newChildExpirationTime) {
// [ts] newChildExpirationTime = _childUpdateExpirationTime;
// [ts] }
// [ts] if (_childChildExpirationTime > newChildExpirationTime) {
// [ts] newChildExpirationTime = _childChildExpirationTime;
// [ts] }
// [ts] _child = _child.sibling;
// [ts] }
// [ts] }
// [ts] completedWork.childExpirationTime = newChildExpirationTime;
// [ts] }
// [ts] fn commitRoot(root: auto) {
// [ts] let renderPriorityLevel = getCurrentPriorityLevel();
// [ts] runWithPriority$1(ImmediatePriority, commitRootImpl.bind(null, root, renderPriorityLevel));
// [ts] return null;
// [ts] }
// [ts] fn commitRootImpl(root: auto, renderPriorityLevel: auto) {
// [ts] do {
// `flushPassiveEffects` will call `flushSyncUpdateQueue` at the end, which
// means `flushPassiveEffects` will sometimes result in additional
// passive effects. So we need to keep flushing in a loop until there are
// no more pending effects.
// TODO: Might be better if `flushPassiveEffects` did not automatically
// flush synchronous work at the end, to avoid factoring hazards like this.
// [ts] flushPassiveEffects();
// [ts] } while (rootWithPendingPassiveEffects != null);
// [ts] flushRenderPhaseStrictModeWarningsInDEV();
// [ts] if (!((executionContext & (RenderContext | CommitContext)) == NoContext)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should not already be working." );)
// [ts] }
// [ts] }
// [ts] let finishedWork = root.finishedWork;
// [ts] let expirationTime = root.finishedExpirationTime;
// [ts] if (finishedWork == null) {
// [ts] return null;
// [ts] }
// [ts] root.finishedWork = null;
// [ts] root.finishedExpirationTime = NoWork;
// [ts] if (!(finishedWork != root.current)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Cannot commit the same tree as before. This error is likely caused by a bug in React. Please file an issue." );)
// [ts] }
// [ts] } // commitRoot never returns a continuation; it always finishes synchronously.
// So we can clear these now to allow a new callback to be scheduled.
// [ts] root.callbackNode = null;
// [ts] root.callbackExpirationTime = NoWork;
// [ts] root.callbackPriority = NoPriority;
// [ts] root.nextKnownPendingLevel = NoWork;
// [ts] startCommitTimer(); // Update the first and last pending times on this root. The new first
// pending time is whatever is left on the root fiber.
// [ts] let remainingExpirationTimeBeforeCommit = getRemainingExpirationTime(finishedWork);
// [ts] markRootFinishedAtTime(root, expirationTime, remainingExpirationTimeBeforeCommit);
// [ts] if (root == workInProgressRoot) {
// We can reset these now that they are finished.
// [ts] workInProgressRoot = null;
// [ts] workInProgress = null;
// [ts] renderExpirationTime$1 = NoWork;
// [ts] } // This indicates that the last root we worked on is not the same one that
// we're committing now. This most commonly happens when a suspended root
// times out.
// Get the list of effects.
// [ts] let firstEffect;
// [ts] if (finishedWork.effectTag > PerformedWork) {
// A fiber's effect list consists only of its children, not itself. So if
// the root has an effect, we need to add it to the end of the list. The
// resulting list is the set that would belong to the root's parent, if it
// had one; that is, all the effects in the tree including the root.
// [ts] if (finishedWork.lastEffect != null) {
// [ts] finishedWork.lastEffect.nextEffect = finishedWork;
// [ts] firstEffect = finishedWork.firstEffect;
// [ts] } else {
// [ts] firstEffect = finishedWork;
// [ts] }
// [ts] } else {
// There is no effect on the root.
// [ts] firstEffect = finishedWork.firstEffect;
// [ts] }
// [ts] if (firstEffect != null) {
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext |= CommitContext;
// [ts] let prevInteractions = pushInteractions(root); // Reset this to null before calling lifecycles
// [ts] ReactCurrentOwner$2.current = null; // The commit phase is broken into several sub-phases. We do a separate pass
// of the effect list for each phase: all mutation effects come before all
// layout effects, and so on.
// The first phase a "before mutation" phase. We use this phase to read the
// state of the host tree right before we mutate it. This is where
// getSnapshotBeforeUpdate is called.
// [ts] startCommitSnapshotEffectsTimer();
// [ts] prepareForCommit(root.containerInfo);
// [ts] nextEffect = firstEffect;
// [ts] do {
// [ts] {
// [ts] invokeGuardedCallback(null, commitBeforeMutationEffects, null);
// [ts] if (hasCaughtError()) {
// [ts] if (!(nextEffect != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should be working on an effect." );)
// [ts] }
// [ts] }
// [ts] let error = clearCaughtError();
// [ts] captureCommitPhaseError(nextEffect, error);
// [ts] nextEffect = nextEffect.nextEffect;
// [ts] }
// [ts] }
// [ts] } while (nextEffect != null);
// [ts] stopCommitSnapshotEffectsTimer();
// [ts] {
// Mark the current commit time to be shared by all Profilers in this
// batch. This enables them to be grouped later.
// [ts] recordCommitTime();
// [ts] } // The next phase is the mutation phase, where we mutate the host tree.
// [ts] startCommitHostEffectsTimer();
// [ts] nextEffect = firstEffect;
// [ts] do {
// [ts] {
// [ts] invokeGuardedCallback(null, commitMutationEffects, null, root, renderPriorityLevel);
// [ts] if (hasCaughtError()) {
// [ts] if (!(nextEffect != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should be working on an effect." );)
// [ts] }
// [ts] }
// [ts] let _error = clearCaughtError();
// [ts] captureCommitPhaseError(nextEffect, _error);
// [ts] nextEffect = nextEffect.nextEffect;
// [ts] }
// [ts] }
// [ts] } while (nextEffect != null);
// [ts] stopCommitHostEffectsTimer();
// [ts] resetAfterCommit(root.containerInfo); // The work-in-progress tree is now the current tree. This must come after
// the mutation phase, so that the previous tree is still current during
// componentWillUnmount, but before the layout phase, so that the finished
// work is current during componentDidMount/Update.
// [ts] root.current = finishedWork; // The next phase is the layout phase, where we call effects that read
// the host tree after it's been mutated. The idiomatic use case for this is
// layout, but class component lifecycles also fire here for legacy reasons.
// [ts] startCommitLifeCyclesTimer();
// [ts] nextEffect = firstEffect;
// [ts] do {
// [ts] {
// [ts] invokeGuardedCallback(null, commitLayoutEffects, null, root, expirationTime);
// [ts] if (hasCaughtError()) {
// [ts] if (!(nextEffect != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should be working on an effect." );)
// [ts] }
// [ts] }
// [ts] let _error2 = clearCaughtError();
// [ts] captureCommitPhaseError(nextEffect, _error2);
// [ts] nextEffect = nextEffect.nextEffect;
// [ts] }
// [ts] }
// [ts] } while (nextEffect != null);
// [ts] stopCommitLifeCyclesTimer();
// [ts] nextEffect = null; // Tell Scheduler to yield at the end of the frame, so the browser has an
// opportunity to paint.
// [ts] requestPaint();
// [ts] {
// [ts] popInteractions(prevInteractions);
// [ts] }
// [ts] executionContext = prevExecutionContext;
// [ts] } else {
// No effects.
// [ts] root.current = finishedWork; // Measure these anyway so the flamegraph explicitly shows that there were
// no effects.
// TODO: Maybe there's a better way to report this.
// [ts] startCommitSnapshotEffectsTimer();
// [ts] stopCommitSnapshotEffectsTimer();
// [ts] {
// [ts] recordCommitTime();
// [ts] }
// [ts] startCommitHostEffectsTimer();
// [ts] stopCommitHostEffectsTimer();
// [ts] startCommitLifeCyclesTimer();
// [ts] stopCommitLifeCyclesTimer();
// [ts] }
// [ts] stopCommitTimer();
// [ts] let rootDidHavePassiveEffects = rootDoesHavePassiveEffects;
// [ts] if (rootDoesHavePassiveEffects) {
// This commit has passive effects. Stash a reference to them. But don't
// schedule a callback until after flushing layout work.
// [ts] rootDoesHavePassiveEffects = false;
// [ts] rootWithPendingPassiveEffects = root;
// [ts] pendingPassiveEffectsExpirationTime = expirationTime;
// [ts] pendingPassiveEffectsRenderPriority = renderPriorityLevel;
// [ts] } else {
// We are done with the effect chain at this point so let's clear the
// nextEffect pointers to assist with GC. If we have passive effects, we'll
// clear this in flushPassiveEffects.
// [ts] nextEffect = firstEffect;
// [ts] while (nextEffect != null) {
// [ts] let nextNextEffect = nextEffect.nextEffect;
// [ts] nextEffect.nextEffect = null;
// [ts] nextEffect = nextNextEffect;
// [ts] }
// [ts] } // Check if there's remaining work on this root
// [ts] let remainingExpirationTime = root.firstPendingTime;
// [ts] if (remainingExpirationTime != NoWork) {
// [ts] {
// [ts] if (spawnedWorkDuringRender != null) {
// [ts] let expirationTimes = spawnedWorkDuringRender;
// [ts] spawnedWorkDuringRender = null;
// [ts] let i = 0;
// [ts] while (i < expirationTimes.length) { // [cap-migrate] for-step: i++
// [ts] scheduleInteractions(root, expirationTimes[i], root.memoizedInteractions);
// [ts] }
// [ts] }
// [ts] schedulePendingInteractions(root, remainingExpirationTime);
// [ts] }
// [ts] } else {
// If there's no remaining work, we can clear the set of already failed
// error boundaries.
// [ts] legacyErrorBoundariesThatAlreadyFailed = null;
// [ts] }
// [ts] {
// [ts] if (!rootDidHavePassiveEffects) {
// If there are no passive effects, then we can complete the pending interactions.
// Otherwise, we'll wait until after the passive effects are flushed.
// Wait to do this until after remaining work has been scheduled,
// so that we don't prematurely signal complete for interactions when there's e.g. hidden work.
// [ts] finishPendingInteractions(root, expirationTime);
// [ts] }
// [ts] }
// [ts] if (remainingExpirationTime == Sync) {
// Count the number of times the root synchronously re-renders without
// finishing. If there are too many, it indicates an infinite update loop.
// [ts] if (root == rootWithNestedUpdates) {
// [ts] nestedUpdateCount++;
// [ts] } else {
// [ts] nestedUpdateCount = 0;
// [ts] rootWithNestedUpdates = root;
// [ts] }
// [ts] } else {
// [ts] nestedUpdateCount = 0;
// [ts] }
// [ts] onCommitRoot(finishedWork.stateNode, expirationTime); // Always call this before exiting `commitRoot`, to ensure that any
// additional work on this root is scheduled.
// [ts] ensureRootIsScheduled(root);
// [ts] if (hasUncaughtError) {
// [ts] hasUncaughtError = false;
// [ts] let _error3 = firstUncaughtError;
// [ts] firstUncaughtError = null;
// [ts] return; // TODO(cap-migrate): throw -> error return (throw _error3;)
// [ts] }
// [ts] if ((executionContext & LegacyUnbatchedContext) != NoContext) {
// This is a legacy edge case. We just committed the initial mount of
// a ReactDOM.render-ed root inside of batchedUpdates. The commit fired
// synchronously, but layout updates should be deferred until the end
// of the batch.
// [ts] return null;
// [ts] } // If layout work was scheduled, flush it now.
// [ts] flushSyncCallbackQueue();
// [ts] return null;
// [ts] }
// [ts] fn commitBeforeMutationEffects() {
// [ts] while (nextEffect != null) {
// [ts] let effectTag = nextEffect.effectTag;
// [ts] if ((effectTag & Snapshot) != NoEffect) {
// [ts] setCurrentFiber(nextEffect);
// [ts] recordEffect();
// [ts] let current = nextEffect.alternate;
// [ts] commitBeforeMutationLifeCycles(current, nextEffect);
// [ts] resetCurrentFiber();
// [ts] }
// [ts] if ((effectTag & Passive) != NoEffect) {
// If there are passive effects, schedule a callback to flush at
// the earliest opportunity.
// [ts] if (!rootDoesHavePassiveEffects) {
// [ts] rootDoesHavePassiveEffects = true;
// [ts] scheduleCallback(NormalPriority, () => {
// [ts] flushPassiveEffects();
// [ts] return null;
// [ts] });
// [ts] }
// [ts] }
// [ts] nextEffect = nextEffect.nextEffect;
// [ts] }
// [ts] }
// [ts] fn commitMutationEffects(root: auto, renderPriorityLevel: auto) {
// TODO: Should probably move the bulk of this function to commitWork.
// [ts] while (nextEffect != null) {
// [ts] setCurrentFiber(nextEffect);
// [ts] let effectTag = nextEffect.effectTag;
// [ts] if (effectTag & ContentReset) {
// [ts] commitResetTextContent(nextEffect);
// [ts] }
// [ts] if (effectTag & Ref) {
// [ts] let current = nextEffect.alternate;
// [ts] if (current != null) {
// [ts] commitDetachRef(current);
// [ts] }
// [ts] } // The following switch statement is only concerned about placement,
// updates, and deletions. To avoid needing to add a case for every possible
// bitmap value, we remove the secondary effects from the effect tag and
// switch on that value.
// [ts] let primaryEffectTag = effectTag & (Placement | Update | Deletion | Hydrating);
// [ts] switch (primaryEffectTag) {
// [ts] Placement => {
// [ts] {
// [ts] commitPlacement(nextEffect); // Clear the "placement" from effect tag so that we know that this is
// inserted, before any life-cycles like componentDidMount gets called.
// TODO: findDOMNode doesn't rely on this any more but isMounted does
// and isMounted is deprecated anyway so we should be able to kill this.
// [ts] nextEffect.effectTag &= ~Placement;
// [ts] break;
// [ts] }
// [ts] },
// [ts] PlacementAndUpdate => {
// [ts] {
// Placement
// [ts] commitPlacement(nextEffect); // Clear the "placement" from effect tag so that we know that this is
// inserted, before any life-cycles like componentDidMount gets called.
// [ts] nextEffect.effectTag &= ~Placement; // Update
// [ts] let _current = nextEffect.alternate;
// [ts] commitWork(_current, nextEffect);
// [ts] break;
// [ts] }
// [ts] },
// [ts] Hydrating => {
// [ts] {
// [ts] nextEffect.effectTag &= ~Hydrating;
// [ts] break;
// [ts] }
// [ts] },
// [ts] HydratingAndUpdate => {
// [ts] {
// [ts] nextEffect.effectTag &= ~Hydrating; // Update
// [ts] let _current2 = nextEffect.alternate;
// [ts] commitWork(_current2, nextEffect);
// [ts] break;
// [ts] }
// [ts] },
// [ts] Update => {
// [ts] {
// [ts] let _current3 = nextEffect.alternate;
// [ts] commitWork(_current3, nextEffect);
// [ts] break;
// [ts] }
// [ts] },
// [ts] Deletion => {
// [ts] {
// [ts] commitDeletion(root, nextEffect, renderPriorityLevel);
// [ts] break;
// [ts] }
// [ts] } // TODO: Only record a mutation effect if primaryEffectTag is non-zero.
// [ts] recordEffect();
// [ts] resetCurrentFiber();
// [ts] nextEffect = nextEffect.nextEffect;
// [ts] }
// [ts] }
// [ts] fn commitLayoutEffects(root: auto, committedExpirationTime: auto) {
// TODO: Should probably move the bulk of this function to commitWork.
// [ts] while (nextEffect != null) {
// [ts] setCurrentFiber(nextEffect);
// [ts] let effectTag = nextEffect.effectTag;
// [ts] if (effectTag & (Update | Callback)) {
// [ts] recordEffect();
// [ts] let current = nextEffect.alternate;
// [ts] commitLifeCycles(root, current, nextEffect);
// [ts] }
// [ts] }
// [ts] if (effectTag & Ref) {
// [ts] recordEffect();
// [ts] commitAttachRef(nextEffect);
// [ts] }
// [ts] resetCurrentFiber();
// [ts] nextEffect = nextEffect.nextEffect;
// [ts] }
// [ts] }
// [ts] fn flushPassiveEffects() {
// [ts] if (pendingPassiveEffectsRenderPriority != NoPriority) {
// [ts] let priorityLevel = if (pendingPassiveEffectsRenderPriority > NormalPriority) NormalPriority else pendingPassiveEffectsRenderPriority;
// [ts] pendingPassiveEffectsRenderPriority = NoPriority;
// [ts] return runWithPriority$1(priorityLevel, flushPassiveEffectsImpl);
// [ts] }
// [ts] }
// [ts] fn flushPassiveEffectsImpl() {
// [ts] if (rootWithPendingPassiveEffects == null) {
// [ts] return false;
// [ts] }
// [ts] let root = rootWithPendingPassiveEffects;
// [ts] let expirationTime = pendingPassiveEffectsExpirationTime;
// [ts] rootWithPendingPassiveEffects = null;
// [ts] pendingPassiveEffectsExpirationTime = NoWork;
// [ts] if (!((executionContext & (RenderContext | CommitContext)) == NoContext)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Cannot flush passive effects while already rendering." );)
// [ts] }
// [ts] }
// [ts] let prevExecutionContext = executionContext;
// [ts] executionContext |= CommitContext;
// [ts] let prevInteractions = pushInteractions(root);
// [ts] {
// Note: This currently assumes there are no passive effects on the root fiber
// because the root is not part of its own effect list.
// This could change in the future.
// [ts] let _effect2 = root.current.firstEffect;
// [ts] while (_effect2 != null) {
// [ts] {
// [ts] setCurrentFiber(_effect2);
// [ts] invokeGuardedCallback(null, commitPassiveHookEffects, null, _effect2);
// [ts] if (hasCaughtError()) {
// [ts] if (!(_effect2 != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Should be working on an effect." );)
// [ts] }
// [ts] }
// [ts] let _error5 = clearCaughtError();
// [ts] captureCommitPhaseError(_effect2, _error5);
// [ts] }
// [ts] resetCurrentFiber();
// [ts] }
// [ts] let nextNextEffect = _effect2.nextEffect; // Remove nextEffect pointer to assist GC
// [ts] _effect2.nextEffect = null;
// [ts] _effect2 = nextNextEffect;
// [ts] }
// [ts] }
// [ts] {
// [ts] popInteractions(prevInteractions);
// [ts] finishPendingInteractions(root, expirationTime);
// [ts] }
// [ts] executionContext = prevExecutionContext;
// [ts] flushSyncCallbackQueue(); // If additional passive effects were scheduled, increment a counter. If this
// exceeds the limit, we'll fire a warning.
// [ts] nestedPassiveUpdateCount = if (rootWithPendingPassiveEffects == null) 0 else nestedPassiveUpdateCount + 1;
// [ts] return true;
// [ts] }
// [ts] fn isAlreadyFailedLegacyErrorBoundary(instance: auto) {
// [ts] return legacyErrorBoundariesThatAlreadyFailed != null && legacyErrorBoundariesThatAlreadyFailed.has(instance);
// [ts] }
// [ts] fn markLegacyErrorBoundaryAsFailed(instance: auto) {
// [ts] if (legacyErrorBoundariesThatAlreadyFailed == null) {
// [ts] legacyErrorBoundariesThatAlreadyFailed = Set.init([instance]);
// [ts] } else {
// [ts] legacyErrorBoundariesThatAlreadyFailed.add(instance);
// [ts] }
// [ts] }
// [ts] fn prepareToThrowUncaughtError(error: auto) {
// [ts] if (!hasUncaughtError) {
// [ts] hasUncaughtError = true;
// [ts] firstUncaughtError = error;
// [ts] }
// [ts] }
// [ts] let onUncaughtError = prepareToThrowUncaughtError;
// [ts] fn captureCommitPhaseErrorOnRoot(rootFiber: auto, sourceFiber: auto, error: auto) {
// [ts] let errorInfo = createCapturedValue(error, sourceFiber);
// [ts] let update = createRootErrorUpdate(rootFiber, errorInfo, Sync);
// [ts] enqueueUpdate(rootFiber, update);
// [ts] let root = markUpdateTimeFromFiberToRoot(rootFiber, Sync);
// [ts] if (root != null) {
// [ts] ensureRootIsScheduled(root);
// [ts] schedulePendingInteractions(root, Sync);
// [ts] }
// [ts] }
// [ts] fn captureCommitPhaseError(sourceFiber: auto, error: auto) {
// [ts] if (sourceFiber.tag == HostRoot) {
// Error was thrown at the root. There is no parent, so the root
// itself should capture it.
// [ts] captureCommitPhaseErrorOnRoot(sourceFiber, sourceFiber, error);
// [ts] return;
// [ts] }
// [ts] let fiber = sourceFiber.return;
// [ts] while (fiber != null) {
// [ts] if (fiber.tag == HostRoot) {
// [ts] captureCommitPhaseErrorOnRoot(fiber, sourceFiber, error);
// [ts] return;
// [ts] } else if (fiber.tag == ClassComponent) {
// [ts] let ctor = fiber.type;
// [ts] let instance = fiber.stateNode;
// [ts] if (@typeName(ctor.getDerivedStateFromError) == "function" || @typeName(instance.componentDidCatch) == "function" && !isAlreadyFailedLegacyErrorBoundary(instance)) {
// [ts] let errorInfo = createCapturedValue(error, sourceFiber);
// [ts] let update = createClassErrorUpdate(fiber, errorInfo, // TODO: This is always sync
// [ts] Sync);
// [ts] enqueueUpdate(fiber, update);
// [ts] let root = markUpdateTimeFromFiberToRoot(fiber, Sync);
// [ts] if (root != null) {
// [ts] ensureRootIsScheduled(root);
// [ts] schedulePendingInteractions(root, Sync);
// [ts] }
// [ts] return;
// [ts] }
// [ts] }
// [ts] fiber = fiber.return;
// [ts] }
// [ts] }
// [ts] fn pingSuspendedRoot(root: auto, thenable: auto, suspendedTime: auto) {
// [ts] let pingCache = root.pingCache;
// [ts] if (pingCache != null) {
// The thenable resolved, so we no longer need to memoize, because it will
// never be thrown again.
// [ts] pingCache.delete(thenable);
// [ts] }
// [ts] if (workInProgressRoot == root && renderExpirationTime$1 == suspendedTime) {
// Received a ping at the same priority level at which we're currently
// rendering. We might want to restart this render. This should mirror
// the logic of whether or not a root suspends once it completes.
// TODO: If we're rendering sync either due to Sync, Batched or expired,
// we should probably never restart.
// If we're suspended with delay, we'll always suspend so we can always
// restart. If we're suspended without any updates, it might be a retry.
// If it's early in the retry we can restart. We can't know for sure
// whether we'll eventually process an update during this render pass,
// but it's somewhat unlikely that we get to a ping before that, since
// getting to the root most update is usually very fast.
// [ts] if (workInProgressRootExitStatus == RootSuspendedWithDelay || workInProgressRootExitStatus == RootSuspended && workInProgressRootLatestProcessedExpirationTime == Sync && now() - globalMostRecentFallbackTime < FALLBACK_THROTTLE_MS) {
// Restart from the root. Don't need to schedule a ping because
// we're already working on this tree.
// [ts] prepareFreshStack(root, renderExpirationTime$1);
// [ts] } else {
// Even though we can't restart right now, we might get an
// opportunity later. So we mark this render as having a ping.
// [ts] workInProgressRootHasPendingPing = true;
// [ts] }
// [ts] return;
// [ts] }
// [ts] if (!isRootSuspendedAtTime(root, suspendedTime)) {
// The root is no longer suspended at this time.
// [ts] return;
// [ts] }
// [ts] let lastPingedTime = root.lastPingedTime;
// [ts] if (lastPingedTime != NoWork && lastPingedTime < suspendedTime) {
// There's already a lower priority ping scheduled.
// [ts] return;
// [ts] } // Mark the time at which this ping was scheduled.
// [ts] root.lastPingedTime = suspendedTime;
// [ts] ensureRootIsScheduled(root);
// [ts] schedulePendingInteractions(root, suspendedTime);
// [ts] }
// [ts] fn retryTimedOutBoundary(boundaryFiber: auto, retryTime: auto) {
// The boundary fiber (a Suspense component or SuspenseList component)
// previously was rendered in its fallback state. One of the promises that
// suspended it has resolved, which means at least part of the tree was
// likely unblocked. Try rendering again, at a new expiration time.
// [ts] if (retryTime == NoWork) {
// [ts] let suspenseConfig = null; // Retries don't carry over the already committed update.
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] retryTime = computeExpirationForFiber(currentTime, boundaryFiber, suspenseConfig);
// [ts] } // TODO: Special case idle priority?
// [ts] let root = markUpdateTimeFromFiberToRoot(boundaryFiber, retryTime);
// [ts] if (root != null) {
// [ts] ensureRootIsScheduled(root);
// [ts] schedulePendingInteractions(root, retryTime);
// [ts] }
// [ts] }
// [ts] fn resolveRetryThenable(boundaryFiber: auto, thenable: auto) {
// [ts] let retryTime = NoWork; // Default
// [ts] let retryCache;
// [ts] {
// [ts] retryCache = boundaryFiber.stateNode;
// [ts] }
// [ts] if (retryCache != null) {
// The thenable resolved, so we no longer need to memoize, because it will
// never be thrown again.
// [ts] retryCache.delete(thenable);
// [ts] }
// [ts] retryTimedOutBoundary(boundaryFiber, retryTime);
// [ts] } // Computes the next Just Noticeable Difference (JND) boundary.
// The theory is that a person can't tell the difference between small differences in time.
// Therefore, if we wait a bit longer than necessary that won't translate to a noticeable
// difference in the experience. However, waiting for longer might mean that we can avoid
// showing an intermediate loading state. The longer we have already waited, the harder it
// is to tell small differences in time. Therefore, the longer we've already waited,
// the longer we can wait additionally. At some point we have to give up though.
// We pick a train model where the next boundary commits at a consistent schedule.
// These particular numbers are vague estimates. We expect to adjust them based on research.
// [ts] fn jnd(timeElapsed: auto) {
// [ts] return if (timeElapsed < 120) 120 else if (timeElapsed < 480) 480 else if (timeElapsed < 1080) 1080 else if (timeElapsed < 1920) 1920 else if (timeElapsed < 3000) 3000 else if (timeElapsed < 4320) 4320 else ceil(timeElapsed / 1960) * 1960;
// [ts] }
// [ts] fn computeMsUntilSuspenseLoadingDelay(mostRecentEventTime: auto, committedExpirationTime: auto, suspenseConfig: auto) {
// [ts] let busyMinDurationMs = suspenseConfig.busyMinDurationMs | 0;
// [ts] if (busyMinDurationMs <= 0) {
// [ts] return 0;
// [ts] }
// [ts] let busyDelayMs = suspenseConfig.busyDelayMs | 0; // Compute the time until this render pass would expire.
// [ts] let currentTimeMs = now();
// [ts] let eventTimeMs = inferTimeFromExpirationTimeWithSuspenseConfig(mostRecentEventTime, suspenseConfig);
// [ts] let timeElapsed = currentTimeMs - eventTimeMs;
// [ts] if (timeElapsed <= busyDelayMs) {
// If we haven't yet waited longer than the initial delay, we don't
// have to wait any additional time.
// [ts] return 0;
// [ts] }
// [ts] let msUntilTimeout = busyDelayMs + busyMinDurationMs - timeElapsed; // This is the value that is passed to `setTimeout`.
// [ts] return msUntilTimeout;
// [ts] }
// [ts] fn checkForNestedUpdates() {
// [ts] if (nestedUpdateCount > NESTED_UPDATE_LIMIT) {
// [ts] nestedUpdateCount = 0;
// [ts] rootWithNestedUpdates = null;
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Maximum update depth exceeded. This can happen when a component repeatedly calls setState inside componentWillUpdate or componentDidUpdate. React limits the number of nested updates to prevent infinite loops." );)
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] if (nestedPassiveUpdateCount > NESTED_PASSIVE_UPDATE_LIMIT) {
// [ts] nestedPassiveUpdateCount = 0;
// [ts] error("Maximum update depth exceeded. This can happen when a component " + "calls setState inside useEffect, but useEffect either doesn't " + "have a dependency array, or one of the dependencies changes on " + "every render.");
// [ts] }
// [ts] }
// [ts] }
// [ts] fn flushRenderPhaseStrictModeWarningsInDEV() {
// [ts] {
// [ts] ReactStrictModeWarnings.flushLegacyContextWarning();
// [ts] {
// [ts] ReactStrictModeWarnings.flushPendingUnsafeLifecycleWarnings();
// [ts] }
// [ts] }
// [ts] }
// [ts] fn stopFinishedWorkLoopTimer() {
// [ts] let didCompleteRoot = true;
// [ts] stopWorkLoopTimer(interruptedBy, didCompleteRoot);
// [ts] interruptedBy = null;
// [ts] }
// [ts] fn stopInterruptedWorkLoopTimer() {
// TODO: Track which fiber caused the interruption.
// [ts] let didCompleteRoot = false;
// [ts] stopWorkLoopTimer(interruptedBy, didCompleteRoot);
// [ts] interruptedBy = null;
// [ts] }
// [ts] fn checkForInterruption(fiberThatReceivedUpdate: auto, updateExpirationTime: auto) {
// [ts] if ( workInProgressRoot != null && updateExpirationTime > renderExpirationTime$1) {
// [ts] interruptedBy = fiberThatReceivedUpdate;
// [ts] }
// [ts] }
// [ts] let didWarnStateUpdateForUnmountedComponent = null;
// [ts] fn warnAboutUpdateOnUnmountedFiberInDEV(fiber: auto) {
// [ts] {
// [ts] let tag = fiber.tag;
// [ts] if (tag != HostRoot && tag != ClassComponent && tag != FunctionComponent && tag != ForwardRef && tag != MemoComponent && tag != SimpleMemoComponent && tag != Block) {
// Only warn for user-defined components, not internal ones like Suspense.
// [ts] return;
// [ts] }
// the problematic code almost always lies inside that component.
// [ts] let componentName = getComponentName(fiber.type) || "ReactComponent";
// [ts] if (didWarnStateUpdateForUnmountedComponent != null) {
// [ts] if (didWarnStateUpdateForUnmountedComponent.has(componentName)) {
// [ts] return;
// [ts] }
// [ts] didWarnStateUpdateForUnmountedComponent.add(componentName);
// [ts] } else {
// [ts] didWarnStateUpdateForUnmountedComponent = Set.init([componentName]);
// [ts] }
// [ts] error(if ("Can't perform a React state update on an unmounted component. This " + "is a no-op, but it indicates a memory leak in your application. To " + "fix, cancel all subscriptions and asynchronous tasks in %s.%s", tag == ClassComponent) "the componentWillUnmount method" else "a useEffect cleanup function", getStackByFiberInDevAndProd(fiber));
// [ts] }
// [ts] }
// [ts] let beginWork$1;
// [ts] {
// [ts] let dummyFiber = null;
// [ts] beginWork$1 = (current, unitOfWork, expirationTime) => {
// If a component throws an error, we replay it again in a synchronously
// dispatched event, so that the debugger will treat it as an uncaught
// error See ReactErrorUtils for more information.
// Before entering the begin phase, copy the work-in-progress onto a dummy
// fiber. If beginWork throws, we'll use this to reset the state.
// [ts] let originalWorkInProgressCopy = assignFiberPropertiesInDEV(dummyFiber, unitOfWork);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return beginWork(current, unitOfWork, expirationTime);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (originalError) {)
// [ts] if (originalError != null && @typeName(originalError) == "object" && @typeName(originalError.then) == "function") {
// Don't replay promises. Treat everything else like an error.
// [ts] return; // TODO(cap-migrate): throw -> error return (throw originalError;)
// [ts] } // Keep this code in sync with handleError; any changes here must have
// corresponding changes there.
// [ts] resetContextDependencies();
// [ts] resetHooksAfterThrow(); // Don't reset current debug fiber, since we're about to work on the
// same fiber again.
// Unwind the failed stack frame
// [ts] unwindInterruptedWork(unitOfWork); // Restore the original properties of the fiber.
// [ts] assignFiberPropertiesInDEV(unitOfWork, originalWorkInProgressCopy);
// [ts] if ( unitOfWork.mode & ProfileMode) {
// Reset the profiler timer.
// [ts] startProfilerTimer(unitOfWork);
// [ts] } // Run beginWork again.
// [ts] invokeGuardedCallback(null, beginWork, null, current, unitOfWork, expirationTime);
// [ts] if (hasCaughtError()) {
// [ts] let replayError = clearCaughtError(); // `invokeGuardedCallback` sometimes sets an expando `_suppressLogging`.
// Rethrow this error instead of the original one.
// [ts] return; // TODO(cap-migrate): throw -> error return (throw replayError;)
// [ts] } else {
// This branch is reachable if the render phase is impure.
// [ts] return; // TODO(cap-migrate): throw -> error return (throw originalError;)
// [ts] }
// [ts] }
// [ts] };
// [ts] }
// [ts] let didWarnAboutUpdateInRender = false;
// [ts] let didWarnAboutUpdateInRenderForAnotherComponent;
// [ts] {
// [ts] didWarnAboutUpdateInRenderForAnotherComponent = Set.init();
// [ts] }
// [ts] fn warnAboutRenderPhaseUpdatesInDEV(fiber: auto) {
// [ts] {
// [ts] if (isRendering && (executionContext & RenderContext) != NoContext) {
// [ts] switch (fiber.tag) {
// [ts] FunctionComponent, ForwardRef, SimpleMemoComponent => {
// [ts] {
// [ts] let renderingComponentName = workInProgress && getComponentName(workInProgress.type) || "Unknown"; // Dedupe by the rendering component because it's the one that needs to be fixed.
// [ts] let dedupeKey = renderingComponentName;
// [ts] if (!didWarnAboutUpdateInRenderForAnotherComponent.has(dedupeKey)) {
// [ts] didWarnAboutUpdateInRenderForAnotherComponent.add(dedupeKey);
// [ts] let setStateComponentName = getComponentName(fiber.type) || "Unknown";
// [ts] error("Cannot update a component (`%s`) while rendering a " + "different component (`%s`). To locate the bad setState() call inside `%s`, " + "follow the stack trace as described in https://fb.me/setstate-in-render", setStateComponentName, renderingComponentName, renderingComponentName);
// [ts] }
// [ts] break;
// [ts] }
// [ts] },
// [ts] ClassComponent => {
// [ts] {
// [ts] if (!didWarnAboutUpdateInRender) {
// [ts] error("Cannot update during an existing state transition (such as " + "within `render`). Render methods should be a pure " + "function of props and state.");
// [ts] didWarnAboutUpdateInRender = true;
// [ts] }
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] } // a 'shared' variable that changes when act() opens/closes in tests.
// [ts] let IsThisRendererActing = {
// [ts] current: false
// [ts] };
// [ts] fn warnIfNotScopedWithMatchingAct(fiber: auto) {
// [ts] {
// [ts] if ( IsSomeRendererActing.current == true && IsThisRendererActing.current != true) {
// [ts] error("It looks like you're using the wrong act() around your test interactions.\n" + "Be sure to use the matching version of act() corresponding to your renderer:\n\n" + "// for react-dom:\n" + "import {act} from 'react-dom/test-utils';\n" + "// ...\n" + "act(() => ...);\n\n" + "// for react-test-renderer:\n" + "import TestRenderer from 'react-test-renderer';\n" + "const {act} = TestRenderer;\n" + "// ...\n" + "act(() => ...);" + "%s", getStackByFiberInDevAndProd(fiber));
// [ts] }
// [ts] }
// [ts] }
// [ts] fn warnIfNotCurrentlyActingEffectsInDEV(fiber: auto) {
// [ts] {
// [ts] if ( (fiber.mode & StrictMode) != NoMode && IsSomeRendererActing.current == false && IsThisRendererActing.current == false) {
// [ts] error("An update to %s ran an effect, but was not wrapped in act(...).\n\n" + "When testing, code that causes React state updates should be " + "wrapped into act(...):\n\n" + "act(() => {\n" + "  /* fire events that update state */\n" + "});\n" + "/* assert on the output */\n\n" + "This ensures that you're testing the behavior the user would see " + "in the browser." + " Learn more at https://fb.me/react-wrap-tests-with-act" + "%s", getComponentName(fiber.type), getStackByFiberInDevAndProd(fiber));
// [ts] }
// [ts] }
// [ts] }
// [ts] fn warnIfNotCurrentlyActingUpdatesInDEV(fiber: auto) {
// [ts] {
// [ts] if ( executionContext == NoContext && IsSomeRendererActing.current == false && IsThisRendererActing.current == false) {
// [ts] error("An update to %s inside a test was not wrapped in act(...).\n\n" + "When testing, code that causes React state updates should be " + "wrapped into act(...):\n\n" + "act(() => {\n" + "  /* fire events that update state */\n" + "});\n" + "/* assert on the output */\n\n" + "This ensures that you're testing the behavior the user would see " + "in the browser." + " Learn more at https://fb.me/react-wrap-tests-with-act" + "%s", getComponentName(fiber.type), getStackByFiberInDevAndProd(fiber));
// [ts] }
// [ts] }
// [ts] }
// [ts] let warnIfNotCurrentlyActingUpdatesInDev = warnIfNotCurrentlyActingUpdatesInDEV; // In tests, we want to enforce a mocked scheduler.
// [ts] let didWarnAboutUnmockedScheduler = false; // TODO Before we release concurrent mode, revisit this and decide whether a mocked
// scheduler is the actual recommendation. The alternative could be a testing build,
// a new lib, or whatever; we dunno just yet. This message is for early adopters
// to get their tests right.
// [ts] fn warnIfUnmockedScheduler(fiber: auto) {
// [ts] {
// [ts] if (didWarnAboutUnmockedScheduler == false && unstable_flushAllWithoutAsserting == null) {
// [ts] if (fiber.mode & BlockingMode || fiber.mode & ConcurrentMode) {
// [ts] didWarnAboutUnmockedScheduler = true;
// [ts] error("In Concurrent or Sync modes, the \"scheduler\" module needs to be mocked " + "to guarantee consistent behaviour across tests and browsers. " + "For example, with jest: \n" + "jest.mock('scheduler', () => require('scheduler/unstable_mock'));\n\n" + "For more info, visit https://fb.me/react-mock-scheduler");
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn computeThreadID(root: auto, expirationTime: auto) {
// Interaction threads are unique per root and expiration time.
// [ts] return expirationTime * 1000 + root.interactionThreadID;
// [ts] }
// [ts] fn markSpawnedWork(expirationTime: auto) {
// [ts] if (spawnedWorkDuringRender == null) {
// [ts] spawnedWorkDuringRender = [expirationTime];
// [ts] } else {
// [ts] spawnedWorkDuringRender.push(expirationTime);
// [ts] }
// [ts] }
// [ts] fn scheduleInteractions(root: auto, expirationTime: auto, interactions: auto) {
// [ts] if (interactions.size > 0) {
// [ts] let pendingInteractionMap = root.pendingInteractionMap;
// [ts] let pendingInteractions = pendingInteractionMap.get(expirationTime);
// [ts] if (pendingInteractions != null) {
// [ts] interactions.forEach((interaction) => {
// [ts] if (!pendingInteractions.has(interaction)) {
// Update the pending async work count for previously unscheduled interaction.
// [ts] interaction.__count++;
// [ts] }
// [ts] pendingInteractions.add(interaction);
// [ts] });
// [ts] } else {
// [ts] pendingInteractionMap.set(expirationTime, Set.init(interactions)); // Update the pending async work count for the current interactions.
// [ts] interactions.forEach((interaction) => {
// [ts] interaction.__count++;
// [ts] });
// [ts] }
// [ts] let subscriber = __subscriberRef.current;
// [ts] if (subscriber != null) {
// [ts] let threadID = computeThreadID(root, expirationTime);
// [ts] subscriber.onWorkScheduled(interactions, threadID);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn schedulePendingInteractions(root: auto, expirationTime: auto) {
// [ts] scheduleInteractions(root, expirationTime, __interactionsRef.current);
// [ts] }
// [ts] fn startWorkOnPendingInteractions(root: auto, expirationTime: auto) {
// we can accurately attribute time spent working on it, And so that cascading
// work triggered during the render phase will be associated with it.
// [ts] let interactions = Set.init();
// [ts] root.pendingInteractionMap.forEach((scheduledInteractions, scheduledExpirationTime) => {
// [ts] if (scheduledExpirationTime >= expirationTime) {
// [ts] scheduledInteractions.forEach((interaction) => {
// [ts] return interactions.add(interaction);
// [ts] });
// [ts] }
// [ts] }); // Store the current set of interactions on the FiberRoot for a few reasons:
// We can re-use it in hot functions like performConcurrentWorkOnRoot()
// without having to recalculate it. We will also use it in commitWork() to
// pass to any Profiler onRender() hooks. This also provides DevTools with a
// way to access it when the onCommitRoot() hook is called.
// [ts] root.memoizedInteractions = interactions;
// [ts] if (interactions.size > 0) {
// [ts] let subscriber = __subscriberRef.current;
// [ts] if (subscriber != null) {
// [ts] let threadID = computeThreadID(root, expirationTime);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber.onWorkStarted(interactions, threadID);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// If the subscriber throws, rethrow it in a separate task
// [ts] scheduleCallback(ImmediatePriority, () => {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] });
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn finishPendingInteractions(root: auto, committedExpirationTime: auto) {
// [ts] let earliestRemainingTimeAfterCommit = root.firstPendingTime;
// [ts] let subscriber;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber = __subscriberRef.current;
// [ts] if (subscriber != null && root.memoizedInteractions.size > 0) {
// [ts] let threadID = computeThreadID(root, committedExpirationTime);
// [ts] subscriber.onWorkStopped(root.memoizedInteractions, threadID);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// If the subscriber throws, rethrow it in a separate task
// [ts] scheduleCallback(ImmediatePriority, () => {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] });
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// Clear completed interactions from the pending Map.
// Unless the render was suspended or cascading work was scheduled,
// In which case– leave pending interactions until the subsequent render.
// [ts] let pendingInteractionMap = root.pendingInteractionMap;
// [ts] pendingInteractionMap.forEach((scheduledInteractions, scheduledExpirationTime) => {
// Only decrement the pending interaction count if we're done.
// If there's still work at the current priority,
// That indicates that we are waiting for suspense data.
// [ts] if (scheduledExpirationTime > earliestRemainingTimeAfterCommit) {
// [ts] pendingInteractionMap.delete(scheduledExpirationTime);
// [ts] scheduledInteractions.forEach((interaction) => {
// [ts] interaction.__count--;
// [ts] if (subscriber != null && interaction.__count == 0) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber.onInteractionScheduledWorkCompleted(interaction);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// If the subscriber throws, rethrow it in a separate task
// [ts] scheduleCallback(ImmediatePriority, () => {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] });
// [ts] }
// [ts] }
// [ts] });
// [ts] }
// [ts] });
// [ts] }
// [ts] }
// [ts] let onScheduleFiberRoot = null;
// [ts] let onCommitFiberRoot = null;
// [ts] let onCommitFiberUnmount = null;
// [ts] let hasLoggedError = false;
// [ts] let isDevToolsPresent = @typeName(__REACT_DEVTOOLS_GLOBAL_HOOK__) != "undefined";
// [ts] fn injectInternals(internals: auto) {
// [ts] if (@typeName(__REACT_DEVTOOLS_GLOBAL_HOOK__) == "undefined") {
// No DevTools
// [ts] return false;
// [ts] }
// [ts] let hook = __REACT_DEVTOOLS_GLOBAL_HOOK__;
// [ts] if (hook.isDisabled) {
// This isn't a real property on the hook, but it can be set to opt out
// of DevTools integration and associated warnings and logs.
// https://github.com/facebook/react/issues/3877
// [ts] return true;
// [ts] }
// [ts] if (!hook.supportsFiber) {
// [ts] {
// [ts] error("The installed version of React DevTools is too old and will not work " + "with the current version of React. Please update React DevTools. " + "https://fb.me/react-devtools");
// [ts] } // DevTools exists, even though it doesn't support Fiber.
// [ts] return true;
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let rendererID = hook.inject(internals); // We have successfully injected, so now it is safe to set up hooks.
// [ts] if (true) {
// Only used by Fast Refresh
// [ts] if (@typeName(hook.onScheduleFiberRoot) == "function") {
// [ts] onScheduleFiberRoot = (root, children) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] hook.onScheduleFiberRoot(rendererID, root, children);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
// [ts] if (true && !hasLoggedError) {
// [ts] hasLoggedError = true;
// [ts] error("React instrumentation encountered an error: %s", err);
// [ts] }
// [ts] }
// [ts] };
// [ts] }
// [ts] }
// [ts] onCommitFiberRoot = (root, expirationTime) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let didError = (root.current.effectTag & DidCapture) == DidCapture;
// [ts] if (enableProfilerTimer) {
// [ts] let currentTime = getCurrentTime();
// [ts] let priorityLevel = inferPriorityFromExpirationTime(currentTime, expirationTime);
// [ts] hook.onCommitFiberRoot(rendererID, root, priorityLevel, didError);
// [ts] } else {
// [ts] hook.onCommitFiberRoot(rendererID, root, null, didError);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
// [ts] if (true) {
// [ts] if (!hasLoggedError) {
// [ts] hasLoggedError = true;
// [ts] error("React instrumentation encountered an error: %s", err);
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] onCommitFiberUnmount = (fiber) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] hook.onCommitFiberUnmount(rendererID, fiber);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
// [ts] if (true) {
// [ts] if (!hasLoggedError) {
// [ts] hasLoggedError = true;
// [ts] error("React instrumentation encountered an error: %s", err);
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
// Catch all errors because it is unsafe to throw during initialization.
// [ts] {
// [ts] error("React instrumentation encountered an error: %s.", err);
// [ts] }
// [ts] } // DevTools exists
// [ts] return true;
// [ts] }
// [ts] fn onScheduleRoot(root: auto, children: auto) {
// [ts] if (@typeName(onScheduleFiberRoot) == "function") {
// [ts] onScheduleFiberRoot(root, children);
// [ts] }
// [ts] }
// [ts] fn onCommitRoot(root: auto, expirationTime: auto) {
// [ts] if (@typeName(onCommitFiberRoot) == "function") {
// [ts] onCommitFiberRoot(root, expirationTime);
// [ts] }
// [ts] }
// [ts] fn onCommitUnmount(fiber: auto) {
// [ts] if (@typeName(onCommitFiberUnmount) == "function") {
// [ts] onCommitFiberUnmount(fiber);
// [ts] }
// [ts] }
// [ts] let hasBadMapPolyfill;
// [ts] {
// [ts] hasBadMapPolyfill = false;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let nonExtensibleObject = Object.preventExtensions({});
// [ts] let testMap = Map.init([[nonExtensibleObject, null]]);
// [ts] let testSet = Set.init([nonExtensibleObject]); // This is necessary for Rollup to not consider these unused.
// https://github.com/rollup/rollup/issues/1771
// TODO: we can remove these if Rollup fixes the bug.
// [ts] testMap.set(0, 0);
// [ts] testSet.add(0);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// TODO: Consider warning about bad polyfills
// [ts] hasBadMapPolyfill = true;
// [ts] }
// [ts] }
// [ts] let debugCounter = 1;
// [ts] fn FiberNode(tag: auto, pendingProps: auto, key: auto, mode: auto) {
// Instance
// [ts] _.tag = tag;
// [ts] _.key = key;
// [ts] _.elementType = null;
// [ts] _.type = null;
// [ts] _.stateNode = null; // Fiber
// [ts] _.return = null;
// [ts] _.child = null;
// [ts] _.sibling = null;
// [ts] _.index = 0;
// [ts] _.ref = null;
// [ts] _.pendingProps = pendingProps;
// [ts] _.memoizedProps = null;
// [ts] _.updateQueue = null;
// [ts] _.memoizedState = null;
// [ts] _.dependencies = null;
// [ts] _.mode = mode; // Effects
// [ts] _.effectTag = NoEffect;
// [ts] _.nextEffect = null;
// [ts] _.firstEffect = null;
// [ts] _.lastEffect = null;
// [ts] _.expirationTime = NoWork;
// [ts] _.childExpirationTime = NoWork;
// [ts] _.alternate = null;
// [ts] {
// Note: The following is done to avoid a v8 performance cliff.
//
// Initializing the fields below to smis and later updating them with
// double values will cause Fibers to end up having separate shapes.
// This behavior/bug has something to do with Object.preventExtension().
// Fortunately this only impacts DEV builds.
// Unfortunately it makes React unusably slow for some applications.
// To work around this, initialize the fields below with doubles.
//
// Learn more about this here:
// https://github.com/facebook/react/issues/14365
// https://bugs.chromium.org/p/v8/issues/detail?id=8538
// [ts] _.actualDuration = Number.NaN;
// [ts] _.actualStartTime = Number.NaN;
// [ts] _.selfBaseDuration = Number.NaN;
// [ts] _.treeBaseDuration = Number.NaN; // It's okay to replace the initial doubles with smis after initialization.
// This won't trigger the performance cliff mentioned above,
// and it simplifies other profiler code (including DevTools).
// [ts] _.actualDuration = 0;
// [ts] _.actualStartTime = -1;
// [ts] _.selfBaseDuration = 0;
// [ts] _.treeBaseDuration = 0;
// [ts] } // This is normally DEV-only except www when it adds listeners.
// TODO: remove the User Timing integration in favor of Root Events.
// [ts] {
// [ts] _._debugID = debugCounter++;
// [ts] _._debugIsCurrentlyTiming = false;
// [ts] }
// [ts] {
// [ts] _._debugSource = null;
// [ts] _._debugOwner = null;
// [ts] _._debugNeedsRemount = false;
// [ts] _._debugHookTypes = null;
// [ts] if (!hasBadMapPolyfill && @typeName(Object.preventExtensions) == "function") {
// [ts] Object.preventExtensions(this);
// [ts] }
// [ts] }
// [ts] } // This is a constructor function, rather than a POJO constructor, still
// please ensure we do the following:
// 1) Nobody should add any instance methods on this. Instance methods can be
//    more difficult to predict when they get optimized and they are almost
//    never inlined properly in static compilers.
// 2) Nobody should rely on `instanceof Fiber` for type testing. We should
//    always know when it is a fiber.
// 3) We might want to experiment with using numeric keys since they are easier
//    to optimize in a non-JIT environment.
// 4) We can easily go from a constructor to a createFiber object literal if that
//    is faster.
// 5) It should be easy to port this to a C struct and keep a C implementation
//    compatible.
// [ts] let createFiber = (tag, pendingProps, key, mode) => {
// $FlowFixMe: the shapes are exact here but Flow doesn't like constructors
// [ts] return FiberNode.init(tag, pendingProps, key, mode);
// [ts] };
// [ts] fn shouldConstruct(Component: auto) {
// [ts] let prototype = Component.prototype;
// [ts] return !!(prototype && prototype.isReactComponent);
// [ts] }
// [ts] fn isSimpleFunctionComponent(type: auto) {
// [ts] return @typeName(type) == "function" && !shouldConstruct(type) && type.defaultProps == null;
// [ts] }
// [ts] fn resolveLazyComponentTag(Component: auto) {
// [ts] if (@typeName(Component) == "function") {
// [ts] return if (shouldConstruct(Component)) ClassComponent else FunctionComponent;
// [ts] } else if (Component != null && Component != null) {
// [ts] let $$typeof = Component.$$typeof;
// [ts] if ($$typeof == REACT_FORWARD_REF_TYPE) {
// [ts] return ForwardRef;
// [ts] }
// [ts] if ($$typeof == REACT_MEMO_TYPE) {
// [ts] return MemoComponent;
// [ts] }
// [ts] }
// [ts] return IndeterminateComponent;
// [ts] } // This is used to create an alternate fiber to do work on.
// [ts] fn createWorkInProgress(current: auto, pendingProps: auto) {
// [ts] let workInProgress = current.alternate;
// [ts] if (workInProgress == null) {
// We use a double buffering pooling technique because we know that we'll
// only ever need at most two versions of a tree. We pool the "other" unused
// node that we're free to reuse. This is lazily created to avoid allocating
// extra objects for things that are never updated. It also allow us to
// reclaim the extra memory if needed.
// [ts] workInProgress = createFiber(current.tag, pendingProps, current.key, current.mode);
// [ts] workInProgress.elementType = current.elementType;
// [ts] workInProgress.type = current.type;
// [ts] workInProgress.stateNode = current.stateNode;
// [ts] {
// DEV-only fields
// [ts] {
// [ts] workInProgress._debugID = current._debugID;
// [ts] }
// [ts] workInProgress._debugSource = current._debugSource;
// [ts] workInProgress._debugOwner = current._debugOwner;
// [ts] workInProgress._debugHookTypes = current._debugHookTypes;
// [ts] }
// [ts] workInProgress.alternate = current;
// [ts] current.alternate = workInProgress;
// [ts] } else {
// [ts] workInProgress.pendingProps = pendingProps; // We already have an alternate.
// Reset the effect tag.
// [ts] workInProgress.effectTag = NoEffect; // The effect list is no longer valid.
// [ts] workInProgress.nextEffect = null;
// [ts] workInProgress.firstEffect = null;
// [ts] workInProgress.lastEffect = null;
// [ts] {
// We intentionally reset, rather than copy, actualDuration & actualStartTime.
// This prevents time from endlessly accumulating in new commits.
// This has the downside of resetting values for different priority renders,
// But works for yielding (the common case) and should support resuming.
// [ts] workInProgress.actualDuration = 0;
// [ts] workInProgress.actualStartTime = -1;
// [ts] }
// [ts] }
// [ts] workInProgress.childExpirationTime = current.childExpirationTime;
// [ts] workInProgress.expirationTime = current.expirationTime;
// [ts] workInProgress.child = current.child;
// [ts] workInProgress.memoizedProps = current.memoizedProps;
// [ts] workInProgress.memoizedState = current.memoizedState;
// [ts] workInProgress.updateQueue = current.updateQueue; // Clone the dependencies object. This is mutated during the render phase, so
// it cannot be shared with the current fiber.
// [ts] let currentDependencies = current.dependencies;
// [ts] workInProgress.dependencies = if (currentDependencies == null) null else {
// [ts] expirationTime: currentDependencies.expirationTime,
// [ts] firstContext: currentDependencies.firstContext,
// [ts] responders: currentDependencies.responders
// [ts] }; // These will be overridden during the parent's reconciliation
// [ts] workInProgress.sibling = current.sibling;
// [ts] workInProgress.index = current.index;
// [ts] workInProgress.ref = current.ref;
// [ts] {
// [ts] workInProgress.selfBaseDuration = current.selfBaseDuration;
// [ts] workInProgress.treeBaseDuration = current.treeBaseDuration;
// [ts] }
// [ts] {
// [ts] workInProgress._debugNeedsRemount = current._debugNeedsRemount;
// [ts] switch (workInProgress.tag) {
// [ts] IndeterminateComponent, FunctionComponent, SimpleMemoComponent => {
// [ts] workInProgress.type = resolveFunctionForHotReloading(current.type);
// break;
// [ts] },
// [ts] ClassComponent => {
// [ts] workInProgress.type = resolveClassForHotReloading(current.type);
// break;
// [ts] },
// [ts] ForwardRef => {
// [ts] workInProgress.type = resolveForwardRefForHotReloading(current.type);
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] return workInProgress;
// [ts] } // Used to reuse a Fiber for a second pass.
// [ts] fn resetWorkInProgress(workInProgress: auto, renderExpirationTime: auto) {
// This resets the Fiber to what createFiber or createWorkInProgress would
// have set the values to before during the first pass. Ideally this wouldn't
// be necessary but unfortunately many code paths reads from the workInProgress
// when they should be reading from current and writing to workInProgress.
// We assume pendingProps, index, key, ref, return are still untouched to
// avoid doing another reconciliation.
// Reset the effect tag but keep any Placement tags, since that's something
// that child fiber is setting, not the reconciliation.
// [ts] workInProgress.effectTag &= Placement; // The effect list is no longer valid.
// [ts] workInProgress.nextEffect = null;
// [ts] workInProgress.firstEffect = null;
// [ts] workInProgress.lastEffect = null;
// [ts] let current = workInProgress.alternate;
// [ts] if (current == null) {
// Reset to createFiber's initial values.
// [ts] workInProgress.childExpirationTime = NoWork;
// [ts] workInProgress.expirationTime = renderExpirationTime;
// [ts] workInProgress.child = null;
// [ts] workInProgress.memoizedProps = null;
// [ts] workInProgress.memoizedState = null;
// [ts] workInProgress.updateQueue = null;
// [ts] workInProgress.dependencies = null;
// [ts] {
// Note: We don't reset the actualTime counts. It's useful to accumulate
// actual time across multiple render passes.
// [ts] workInProgress.selfBaseDuration = 0;
// [ts] workInProgress.treeBaseDuration = 0;
// [ts] }
// [ts] } else {
// Reset to the cloned values that createWorkInProgress would've.
// [ts] workInProgress.childExpirationTime = current.childExpirationTime;
// [ts] workInProgress.expirationTime = current.expirationTime;
// [ts] workInProgress.child = current.child;
// [ts] workInProgress.memoizedProps = current.memoizedProps;
// [ts] workInProgress.memoizedState = current.memoizedState;
// [ts] workInProgress.updateQueue = current.updateQueue; // Clone the dependencies object. This is mutated during the render phase, so
// it cannot be shared with the current fiber.
// [ts] let currentDependencies = current.dependencies;
// [ts] workInProgress.dependencies = if (currentDependencies == null) null else {
// [ts] expirationTime: currentDependencies.expirationTime,
// [ts] firstContext: currentDependencies.firstContext,
// [ts] responders: currentDependencies.responders
// [ts] };
// [ts] {
// Note: We don't reset the actualTime counts. It's useful to accumulate
// actual time across multiple render passes.
// [ts] workInProgress.selfBaseDuration = current.selfBaseDuration;
// [ts] workInProgress.treeBaseDuration = current.treeBaseDuration;
// [ts] }
// [ts] }
// [ts] return workInProgress;
// [ts] }
// [ts] fn createHostRootFiber(tag: auto) {
// [ts] let mode;
// [ts] if (tag == ConcurrentRoot) {
// [ts] mode = ConcurrentMode | BlockingMode | StrictMode;
// [ts] } else if (tag == BlockingRoot) {
// [ts] mode = BlockingMode | StrictMode;
// [ts] } else {
// [ts] mode = NoMode;
// [ts] }
// [ts] if ( isDevToolsPresent) {
// Always collect profile timings when DevTools are present.
// This enables DevTools to start capturing timing at any point–
// Without some nodes in the tree having empty base times.
// [ts] mode |= ProfileMode;
// [ts] }
// [ts] return createFiber(HostRoot, null, null, mode);
// [ts] }
// [ts] fn createFiberFromTypeAndProps(type: auto, // React$ElementType key: auto, pendingProps: auto, owner: auto, mode: auto, expirationTime: auto) {
// [ts] let fiber;
// [ts] let fiberTag = IndeterminateComponent; // The resolved type is set if we know what the final type will be. I.e. it's not lazy.
// [ts] let resolvedType = type;
// [ts] if (@typeName(type) == "function") {
// [ts] if (shouldConstruct(type)) {
// [ts] fiberTag = ClassComponent;
// [ts] {
// [ts] resolvedType = resolveClassForHotReloading(resolvedType);
// [ts] }
// [ts] } else {
// [ts] {
// [ts] resolvedType = resolveFunctionForHotReloading(resolvedType);
// [ts] }
// [ts] }
// [ts] } else if (@typeName(type) == "string") {
// [ts] fiberTag = HostComponent;
// [ts] } else {
// [ts] getTag: switch (type) {
// [ts] case REACT_FRAGMENT_TYPE:
// [ts] return createFiberFromFragment(pendingProps.children, mode, expirationTime, key);
// [ts] case REACT_CONCURRENT_MODE_TYPE:
// [ts] fiberTag = Mode;
// [ts] mode |= ConcurrentMode | BlockingMode | StrictMode;
// [ts] break;
// [ts] case REACT_STRICT_MODE_TYPE:
// [ts] fiberTag = Mode;
// [ts] mode |= StrictMode;
// [ts] break;
// [ts] case REACT_PROFILER_TYPE:
// [ts] return createFiberFromProfiler(pendingProps, mode, expirationTime, key);
// [ts] case REACT_SUSPENSE_TYPE:
// [ts] return createFiberFromSuspense(pendingProps, mode, expirationTime, key);
// [ts] case REACT_SUSPENSE_LIST_TYPE:
// [ts] return createFiberFromSuspenseList(pendingProps, mode, expirationTime, key);
// [ts] default:
// [ts] {
// [ts] if (@typeName(type) == "object" && type != null) {
// [ts] switch (type.$$typeof) {
// [ts] REACT_PROVIDER_TYPE => {
// [ts] fiberTag = ContextProvider;
// [ts] break getTag;
// [ts] },
// This is a consumer
// [ts] REACT_CONTEXT_TYPE => {
// [ts] fiberTag = ContextConsumer;
// [ts] break getTag;
// [ts] },
// [ts] REACT_FORWARD_REF_TYPE => {
// [ts] fiberTag = ForwardRef;
// [ts] {
// [ts] resolvedType = resolveForwardRefForHotReloading(resolvedType);
// [ts] }
// [ts] break getTag;
// [ts] },
// [ts] REACT_MEMO_TYPE => {
// [ts] fiberTag = MemoComponent;
// [ts] break getTag;
// [ts] },
// [ts] REACT_LAZY_TYPE => {
// [ts] fiberTag = LazyComponent;
// [ts] resolvedType = null;
// [ts] break getTag;
// [ts] },
// [ts] REACT_BLOCK_TYPE => {
// [ts] fiberTag = Block;
// [ts] break getTag;
// [ts] }
// [ts] }
// [ts] }
// [ts] let info = "";
// [ts] {
// [ts] if (type == null || @typeName(type) == "object" && type != null && Object.keys(type).length == 0) {
// [ts] info += " You likely forgot to export your component from the file " + "it's defined in, or you might have mixed up default and " + "named imports.";
// [ts] }
// [ts] let ownerName = if (owner) getComponentName(owner.type) else null;
// [ts] if (ownerName) {
// [ts] info += "\n\nCheck the render method of `" + ownerName + "`.";
// [ts] }
// [ts] }
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Element type is invalid: expected a string (for built-in components) or a class/function (for composite components) but got: " + (type == null ? type : typeof type) + "." + info );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fiber = createFiber(fiberTag, pendingProps, key, mode);
// [ts] fiber.elementType = type;
// [ts] fiber.type = resolvedType;
// [ts] fiber.expirationTime = expirationTime;
// [ts] return fiber;
// [ts] }
// [ts] fn createFiberFromElement(element: auto, mode: auto, expirationTime: auto) {
// [ts] let owner = null;
// [ts] {
// [ts] owner = element._owner;
// [ts] }
// [ts] let type = element.type;
// [ts] let key = element.key;
// [ts] let pendingProps = element.props;
// [ts] let fiber = createFiberFromTypeAndProps(type, key, pendingProps, owner, mode, expirationTime);
// [ts] {
// [ts] fiber._debugSource = element._source;
// [ts] fiber._debugOwner = element._owner;
// [ts] }
// [ts] return fiber;
// [ts] }
// [ts] fn createFiberFromFragment(elements: auto, mode: auto, expirationTime: auto, key: auto) {
// [ts] let fiber = createFiber(Fragment, elements, key, mode);
// [ts] fiber.expirationTime = expirationTime;
// [ts] return fiber;
// [ts] }
// [ts] fn createFiberFromProfiler(pendingProps: auto, mode: auto, expirationTime: auto, key: auto) {
// [ts] {
// [ts] if (@typeName(pendingProps.id) != "string" || @typeName(pendingProps.onRender) != "function") {
// [ts] error("Profiler must specify an \"id\" string and \"onRender\" function as props");
// [ts] }
// [ts] }
// [ts] let fiber = createFiber(Profiler, pendingProps, key, mode | ProfileMode); // TODO: The Profiler fiber shouldn't have a type. It has a tag.
// [ts] fiber.elementType = REACT_PROFILER_TYPE;
// [ts] fiber.type = REACT_PROFILER_TYPE;
// [ts] fiber.expirationTime = expirationTime;
// [ts] return fiber;
// [ts] }
// [ts] fn createFiberFromSuspense(pendingProps: auto, mode: auto, expirationTime: auto, key: auto) {
// [ts] let fiber = createFiber(SuspenseComponent, pendingProps, key, mode); // TODO: The SuspenseComponent fiber shouldn't have a type. It has a tag.
// This needs to be fixed in getComponentName so that it relies on the tag
// instead.
// [ts] fiber.type = REACT_SUSPENSE_TYPE;
// [ts] fiber.elementType = REACT_SUSPENSE_TYPE;
// [ts] fiber.expirationTime = expirationTime;
// [ts] return fiber;
// [ts] }
// [ts] fn createFiberFromSuspenseList(pendingProps: auto, mode: auto, expirationTime: auto, key: auto) {
// [ts] let fiber = createFiber(SuspenseListComponent, pendingProps, key, mode);
// [ts] {
// TODO: The SuspenseListComponent fiber shouldn't have a type. It has a tag.
// This needs to be fixed in getComponentName so that it relies on the tag
// instead.
// [ts] fiber.type = REACT_SUSPENSE_LIST_TYPE;
// [ts] }
// [ts] fiber.elementType = REACT_SUSPENSE_LIST_TYPE;
// [ts] fiber.expirationTime = expirationTime;
// [ts] return fiber;
// [ts] }
// [ts] fn createFiberFromText(content: auto, mode: auto, expirationTime: auto) {
// [ts] let fiber = createFiber(HostText, content, null, mode);
// [ts] fiber.expirationTime = expirationTime;
// [ts] return fiber;
// [ts] }
// [ts] fn createFiberFromHostInstanceForDeletion() {
// [ts] let fiber = createFiber(HostComponent, null, null, NoMode); // TODO: These should not need a type.
// [ts] fiber.elementType = "DELETED";
// [ts] fiber.type = "DELETED";
// [ts] return fiber;
// [ts] }
// [ts] fn createFiberFromPortal(portal: auto, mode: auto, expirationTime: auto) {
// [ts] let pendingProps = if (portal.children != null) portal.children else [];
// [ts] let fiber = createFiber(HostPortal, pendingProps, portal.key, mode);
// [ts] fiber.expirationTime = expirationTime;
// [ts] fiber.stateNode = {
// [ts] containerInfo: portal.containerInfo,
// [ts] pendingChildren: null,
// Used by persistent updates
// [ts] implementation: portal.implementation
// [ts] };
// [ts] return fiber;
// [ts] } // Used for stashing WIP properties to replay failed work in DEV.
// [ts] fn assignFiberPropertiesInDEV(target: auto, source: auto) {
// [ts] if (target == null) {
// This Fiber's initial properties will always be overwritten.
// We only use a Fiber to ensure the same hidden class so DEV isn't slow.
// [ts] target = createFiber(IndeterminateComponent, null, null, NoMode);
// [ts] } // This is intentionally written as a list of all properties.
// We tried to use Object.assign() instead but this is called in
// the hottest path, and Object.assign() was too slow:
// https://github.com/facebook/react/issues/12502
// This code is DEV-only so size is not a concern.
// [ts] target.tag = source.tag;
// [ts] target.key = source.key;
// [ts] target.elementType = source.elementType;
// [ts] target.type = source.type;
// [ts] target.stateNode = source.stateNode;
// [ts] target.return = source.return;
// [ts] target.child = source.child;
// [ts] target.sibling = source.sibling;
// [ts] target.index = source.index;
// [ts] target.ref = source.ref;
// [ts] target.pendingProps = source.pendingProps;
// [ts] target.memoizedProps = source.memoizedProps;
// [ts] target.updateQueue = source.updateQueue;
// [ts] target.memoizedState = source.memoizedState;
// [ts] target.dependencies = source.dependencies;
// [ts] target.mode = source.mode;
// [ts] target.effectTag = source.effectTag;
// [ts] target.nextEffect = source.nextEffect;
// [ts] target.firstEffect = source.firstEffect;
// [ts] target.lastEffect = source.lastEffect;
// [ts] target.expirationTime = source.expirationTime;
// [ts] target.childExpirationTime = source.childExpirationTime;
// [ts] target.alternate = source.alternate;
// [ts] {
// [ts] target.actualDuration = source.actualDuration;
// [ts] target.actualStartTime = source.actualStartTime;
// [ts] target.selfBaseDuration = source.selfBaseDuration;
// [ts] target.treeBaseDuration = source.treeBaseDuration;
// [ts] }
// [ts] {
// [ts] target._debugID = source._debugID;
// [ts] }
// [ts] target._debugSource = source._debugSource;
// [ts] target._debugOwner = source._debugOwner;
// [ts] target._debugIsCurrentlyTiming = source._debugIsCurrentlyTiming;
// [ts] target._debugNeedsRemount = source._debugNeedsRemount;
// [ts] target._debugHookTypes = source._debugHookTypes;
// [ts] return target;
// [ts] }
// [ts] fn FiberRootNode(containerInfo: auto, tag: auto, hydrate: auto) {
// [ts] _.tag = tag;
// [ts] _.current = null;
// [ts] _.containerInfo = containerInfo;
// [ts] _.pendingChildren = null;
// [ts] _.pingCache = null;
// [ts] _.finishedExpirationTime = NoWork;
// [ts] _.finishedWork = null;
// [ts] _.timeoutHandle = noTimeout;
// [ts] _.context = null;
// [ts] _.pendingContext = null;
// [ts] _.hydrate = hydrate;
// [ts] _.callbackNode = null;
// [ts] _.callbackPriority = NoPriority;
// [ts] _.firstPendingTime = NoWork;
// [ts] _.firstSuspendedTime = NoWork;
// [ts] _.lastSuspendedTime = NoWork;
// [ts] _.nextKnownPendingLevel = NoWork;
// [ts] _.lastPingedTime = NoWork;
// [ts] _.lastExpiredTime = NoWork;
// [ts] {
// [ts] _.interactionThreadID = unstable_getThreadID();
// [ts] _.memoizedInteractions = Set.init();
// [ts] _.pendingInteractionMap = Map.init();
// [ts] }
// [ts] }
// [ts] fn createFiberRoot(containerInfo: auto, tag: auto, hydrate: auto, hydrationCallbacks: auto) {
// [ts] let root = FiberRootNode.init(containerInfo, tag, hydrate);
// stateNode is any.
// [ts] let uninitializedFiber = createHostRootFiber(tag);
// [ts] root.current = uninitializedFiber;
// [ts] uninitializedFiber.stateNode = root;
// [ts] initializeUpdateQueue(uninitializedFiber);
// [ts] return root;
// [ts] }
// [ts] fn isRootSuspendedAtTime(root: auto, expirationTime: auto) {
// [ts] let firstSuspendedTime = root.firstSuspendedTime;
// [ts] let lastSuspendedTime = root.lastSuspendedTime;
// [ts] return firstSuspendedTime != NoWork && firstSuspendedTime >= expirationTime && lastSuspendedTime <= expirationTime;
// [ts] }
// [ts] fn markRootSuspendedAtTime(root: auto, expirationTime: auto) {
// [ts] let firstSuspendedTime = root.firstSuspendedTime;
// [ts] let lastSuspendedTime = root.lastSuspendedTime;
// [ts] if (firstSuspendedTime < expirationTime) {
// [ts] root.firstSuspendedTime = expirationTime;
// [ts] }
// [ts] if (lastSuspendedTime > expirationTime || firstSuspendedTime == NoWork) {
// [ts] root.lastSuspendedTime = expirationTime;
// [ts] }
// [ts] if (expirationTime <= root.lastPingedTime) {
// [ts] root.lastPingedTime = NoWork;
// [ts] }
// [ts] if (expirationTime <= root.lastExpiredTime) {
// [ts] root.lastExpiredTime = NoWork;
// [ts] }
// [ts] }
// [ts] fn markRootUpdatedAtTime(root: auto, expirationTime: auto) {
// Update the range of pending times
// [ts] let firstPendingTime = root.firstPendingTime;
// [ts] if (expirationTime > firstPendingTime) {
// [ts] root.firstPendingTime = expirationTime;
// [ts] } // Update the range of suspended times. Treat everything lower priority or
// equal to this update as unsuspended.
// [ts] let firstSuspendedTime = root.firstSuspendedTime;
// [ts] if (firstSuspendedTime != NoWork) {
// [ts] if (expirationTime >= firstSuspendedTime) {
// The entire suspended range is now unsuspended.
// [ts] root.firstSuspendedTime = root.lastSuspendedTime = root.nextKnownPendingLevel = NoWork;
// [ts] } else if (expirationTime >= root.lastSuspendedTime) {
// [ts] root.lastSuspendedTime = expirationTime + 1;
// [ts] } // This is a pending level. Check if it's higher priority than the next
// known pending level.
// [ts] if (expirationTime > root.nextKnownPendingLevel) {
// [ts] root.nextKnownPendingLevel = expirationTime;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markRootFinishedAtTime(root: auto, finishedExpirationTime: auto, remainingExpirationTime: auto) {
// Update the range of pending times
// [ts] root.firstPendingTime = remainingExpirationTime; // Update the range of suspended times. Treat everything higher priority or
// equal to this update as unsuspended.
// [ts] if (finishedExpirationTime <= root.lastSuspendedTime) {
// The entire suspended range is now unsuspended.
// [ts] root.firstSuspendedTime = root.lastSuspendedTime = root.nextKnownPendingLevel = NoWork;
// [ts] } else if (finishedExpirationTime <= root.firstSuspendedTime) {
// Part of the suspended range is now unsuspended. Narrow the range to
// include everything between the unsuspended time (non-inclusive) and the
// last suspended time.
// [ts] root.firstSuspendedTime = finishedExpirationTime - 1;
// [ts] }
// [ts] if (finishedExpirationTime <= root.lastPingedTime) {
// Clear the pinged time
// [ts] root.lastPingedTime = NoWork;
// [ts] }
// [ts] if (finishedExpirationTime <= root.lastExpiredTime) {
// Clear the expired time
// [ts] root.lastExpiredTime = NoWork;
// [ts] }
// [ts] }
// [ts] fn markRootExpiredAtTime(root: auto, expirationTime: auto) {
// [ts] let lastExpiredTime = root.lastExpiredTime;
// [ts] if (lastExpiredTime == NoWork || lastExpiredTime > expirationTime) {
// [ts] root.lastExpiredTime = expirationTime;
// [ts] }
// [ts] }
// [ts] let didWarnAboutNestedUpdates;
// [ts] let didWarnAboutFindNodeInStrictMode;
// [ts] {
// [ts] didWarnAboutNestedUpdates = false;
// [ts] didWarnAboutFindNodeInStrictMode = {};
// [ts] }
// [ts] fn getContextForSubtree(parentComponent: auto) {
// [ts] if (!parentComponent) {
// [ts] return emptyContextObject;
// [ts] }
// [ts] let fiber = get(parentComponent);
// [ts] let parentContext = findCurrentUnmaskedContext(fiber);
// [ts] if (fiber.tag == ClassComponent) {
// [ts] let Component = fiber.type;
// [ts] if (isContextProvider(Component)) {
// [ts] return processChildContext(fiber, Component, parentContext);
// [ts] }
// [ts] }
// [ts] return parentContext;
// [ts] }
// [ts] fn findHostInstanceWithWarning(component: auto, methodName: auto) {
// [ts] {
// [ts] let fiber = get(component);
// [ts] if (fiber == null) {
// [ts] if (@typeName(component.render) == "function") {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Unable to find node on an unmounted component." );)
// [ts] }
// [ts] }
// [ts] } else {
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Argument appears to not be a ReactComponent. Keys: " + Object.keys(component) );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let hostFiber = findCurrentHostFiber(fiber);
// [ts] if (hostFiber == null) {
// [ts] return null;
// [ts] }
// [ts] if (hostFiber.mode & StrictMode) {
// [ts] let componentName = getComponentName(fiber.type) || "Component";
// [ts] if (!didWarnAboutFindNodeInStrictMode[componentName]) {
// [ts] didWarnAboutFindNodeInStrictMode[componentName] = true;
// [ts] if (fiber.mode & StrictMode) {
// [ts] error("%s is deprecated in StrictMode. " + "%s was passed an instance of %s which is inside StrictMode. " + "Instead, add a ref directly to the element you want to reference. " + "Learn more about using refs safely here: " + "https://fb.me/react-strict-mode-find-node%s", methodName, methodName, componentName, getStackByFiberInDevAndProd(hostFiber));
// [ts] } else {
// [ts] error("%s is deprecated in StrictMode. " + "%s was passed an instance of %s which renders StrictMode children. " + "Instead, add a ref directly to the element you want to reference. " + "Learn more about using refs safely here: " + "https://fb.me/react-strict-mode-find-node%s", methodName, methodName, componentName, getStackByFiberInDevAndProd(hostFiber));
// [ts] }
// [ts] }
// [ts] }
// [ts] return hostFiber.stateNode;
// [ts] }
// [ts] }
// [ts] fn createContainer(containerInfo: auto, tag: auto, hydrate: auto, hydrationCallbacks: auto) {
// [ts] return createFiberRoot(containerInfo, tag, hydrate);
// [ts] }
// [ts] fn updateContainer(element: auto, container: auto, parentComponent: auto, callback: auto) {
// [ts] {
// [ts] onScheduleRoot(container, element);
// [ts] }
// [ts] let current$1 = container.current;
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] {
// $FlowExpectedError - jest isn't a global, and isn't recognized outside of tests
// [ts] if ("undefined" != @typeName(jest)) {
// [ts] warnIfUnmockedScheduler(current$1);
// [ts] warnIfNotScopedWithMatchingAct(current$1);
// [ts] }
// [ts] }
// [ts] let suspenseConfig = requestCurrentSuspenseConfig();
// [ts] let expirationTime = computeExpirationForFiber(currentTime, current$1, suspenseConfig);
// [ts] let context = getContextForSubtree(parentComponent);
// [ts] if (container.context == null) {
// [ts] container.context = context;
// [ts] } else {
// [ts] container.pendingContext = context;
// [ts] }
// [ts] {
// [ts] if (isRendering && current != null && !didWarnAboutNestedUpdates) {
// [ts] didWarnAboutNestedUpdates = true;
// [ts] error("Render methods should be a pure function of props and state; " + "triggering nested component updates from render is not allowed. " + "If necessary, trigger nested updates in componentDidUpdate.\n\n" + "Check the render method of %s.", getComponentName(current.type) || "Unknown");
// [ts] }
// [ts] }
// [ts] let update = createUpdate(expirationTime, suspenseConfig); // Caution: React DevTools currently depends on this property
// being called "element".
// [ts] update.payload = {
// [ts] element: element
// [ts] };
// [ts] callback = if (callback == null) null else callback;
// [ts] if (callback != null) {
// [ts] {
// [ts] if (@typeName(callback) != "function") {
// [ts] error("render(...): Expected the last optional `callback` argument to be a " + "function. Instead received: %s.", callback);
// [ts] }
// [ts] }
// [ts] update.callback = callback;
// [ts] }
// [ts] enqueueUpdate(current$1, update);
// [ts] scheduleWork(current$1, expirationTime);
// [ts] return expirationTime;
// [ts] }
// [ts] fn getPublicRootInstance(container: auto) {
// [ts] let containerFiber = container.current;
// [ts] if (!containerFiber.child) {
// [ts] return null;
// [ts] }
// [ts] switch (containerFiber.child.tag) {
// [ts] HostComponent => {
// [ts] return getPublicInstance(containerFiber.child.stateNode);
// [ts] },
// [ts] else => {
// [ts] return containerFiber.child.stateNode;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markRetryTimeImpl(fiber: auto, retryTime: auto) {
// [ts] let suspenseState = fiber.memoizedState;
// [ts] if (suspenseState != null && suspenseState.dehydrated != null) {
// [ts] if (suspenseState.retryTime < retryTime) {
// [ts] suspenseState.retryTime = retryTime;
// [ts] }
// [ts] }
// [ts] } // Increases the priority of thennables when they resolve within this boundary.
// [ts] fn markRetryTimeIfNotHydrated(fiber: auto, retryTime: auto) {
// [ts] markRetryTimeImpl(fiber, retryTime);
// [ts] let alternate = fiber.alternate;
// [ts] if (alternate) {
// [ts] markRetryTimeImpl(alternate, retryTime);
// [ts] }
// [ts] }
// [ts] (fiber) => {
// [ts] if (fiber.tag != SuspenseComponent) {
// We ignore HostRoots here because we can't increase
// their priority and they should not suspend on I/O,
// since you have to wrap anything that might suspend in
// Suspense.
// [ts] return;
// [ts] }
// [ts] let expTime = computeInteractiveExpiration(requestCurrentTimeForUpdate());
// [ts] scheduleWork(fiber, expTime);
// [ts] markRetryTimeIfNotHydrated(fiber, expTime);
// [ts] }
// [ts] (fiber) => {
// [ts] if (fiber.tag != SuspenseComponent) {
// We ignore HostRoots here because we can't increase
// their priority and they should not suspend on I/O,
// since you have to wrap anything that might suspend in
// Suspense.
// [ts] return;
// [ts] }
// [ts] scheduleWork(fiber, ContinuousHydration);
// [ts] markRetryTimeIfNotHydrated(fiber, ContinuousHydration);
// [ts] }
// [ts] (fiber) => {
// [ts] if (fiber.tag != SuspenseComponent) {
// We ignore HostRoots here because we can't increase
// their priority other than synchronously flush it.
// [ts] return;
// [ts] }
// [ts] let currentTime = requestCurrentTimeForUpdate();
// [ts] let expTime = computeExpirationForFiber(currentTime, fiber, null);
// [ts] scheduleWork(fiber, expTime);
// [ts] markRetryTimeIfNotHydrated(fiber, expTime);
// [ts] }
// [ts] fn findHostInstanceWithNoPortals(fiber: auto) {
// [ts] let hostFiber = findCurrentHostFiberWithNoPortals(fiber);
// [ts] if (hostFiber == null) {
// [ts] return null;
// [ts] }
// [ts] if (hostFiber.tag == FundamentalComponent) {
// [ts] return hostFiber.stateNode.instance;
// [ts] }
// [ts] return hostFiber.stateNode;
// [ts] }
// [ts] let shouldSuspendImpl = (fiber) => {
// [ts] return false;
// [ts] };
// [ts] fn shouldSuspend(fiber: auto) {
// [ts] return shouldSuspendImpl(fiber);
// [ts] }
// [ts] let overrideHookState = null;
// [ts] let overrideProps = null;
// [ts] let scheduleUpdate = null;
// [ts] let setSuspenseHandler = null;
// [ts] {
// [ts] let copyWithSetImpl = (obj, path, idx, value) => {
// [ts] if (idx >= path.length) {
// [ts] return value;
// [ts] }
// [ts] let key = path[idx];
// [ts] let updated = if (Array.isArray(obj)) obj.slice() else _assign({}, obj); // $FlowFixMe number or string is fine here
// [ts] updated[key] = copyWithSetImpl(obj[key], path, idx + 1, value);
// [ts] return updated;
// [ts] };
// [ts] let copyWithSet = (obj, path, value) => {
// [ts] return copyWithSetImpl(obj, path, 0, value);
// [ts] }; // Support DevTools editable values for useState and useReducer.
// [ts] overrideHookState = (fiber, id, path, value) => {
// For now, the "id" of stateful hooks is just the stateful hook index.
// This may change in the future with e.g. nested hooks.
// [ts] let currentHook = fiber.memoizedState;
// [ts] while (currentHook != null && id > 0) {
// [ts] currentHook = currentHook.next;
// [ts] id--;
// [ts] }
// [ts] if (currentHook != null) {
// [ts] let newState = copyWithSet(currentHook.memoizedState, path, value);
// [ts] currentHook.memoizedState = newState;
// [ts] currentHook.baseState = newState; // We aren't actually adding an update to the queue,
// because there is no update we can add for useReducer hooks that won't trigger an error.
// (There's no appropriate action type for DevTools overrides.)
// As a result though, React will see the scheduled update as a noop and bailout.
// Shallow cloning props works as a workaround for now to bypass the bailout check.
// [ts] fiber.memoizedProps = _assign({}, fiber.memoizedProps);
// [ts] scheduleWork(fiber, Sync);
// [ts] }
// [ts] }; // Support DevTools props for function components, forwardRef, memo, host components, etc.
// [ts] overrideProps = (fiber, path, value) => {
// [ts] fiber.pendingProps = copyWithSet(fiber.memoizedProps, path, value);
// [ts] if (fiber.alternate) {
// [ts] fiber.alternate.pendingProps = fiber.pendingProps;
// [ts] }
// [ts] scheduleWork(fiber, Sync);
// [ts] };
// [ts] scheduleUpdate = (fiber) => {
// [ts] scheduleWork(fiber, Sync);
// [ts] };
// [ts] setSuspenseHandler = (newShouldSuspendImpl) => {
// [ts] shouldSuspendImpl = newShouldSuspendImpl;
// [ts] };
// [ts] }
// [ts] fn injectIntoDevTools(devToolsConfig: auto) {
// [ts] let findFiberByHostInstance = devToolsConfig.findFiberByHostInstance;
// [ts] let ReactCurrentDispatcher = ReactSharedInternals.ReactCurrentDispatcher;
// [ts] return injectInternals(_assign({}, devToolsConfig, {
// [ts] overrideHookState: overrideHookState,
// [ts] overrideProps: overrideProps,
// [ts] setSuspenseHandler: setSuspenseHandler,
// [ts] scheduleUpdate: scheduleUpdate,
// [ts] currentDispatcherRef: ReactCurrentDispatcher,
// [ts] findHostInstanceByFiber: (fiber) => {
// [ts] let hostFiber = findCurrentHostFiber(fiber);
// [ts] if (hostFiber == null) {
// [ts] return null;
// [ts] }
// [ts] return hostFiber.stateNode;
// [ts] },
// [ts] findFiberByHostInstance: (instance) => {
// [ts] if (!findFiberByHostInstance) {
// Might not be implemented by the renderer.
// [ts] return null;
// [ts] }
// [ts] return findFiberByHostInstance(instance);
// [ts] },
// React Refresh
// [ts] findHostInstancesForRefresh:  findHostInstancesForRefresh ,
// [ts] scheduleRefresh:  scheduleRefresh ,
// [ts] scheduleRoot:  scheduleRoot ,
// [ts] setRefreshHandler:  setRefreshHandler ,
// Enables DevTools to append owner stacks to error messages in DEV mode.
// [ts] getCurrentFiber:  () => {
// [ts] return current;
// [ts] }
// [ts] }));
// [ts] }
// [ts] let IsSomeRendererActing$1 = ReactSharedInternals.IsSomeRendererActing;
// [ts] fn ReactDOMRoot(container: auto, options: auto) {
// [ts] _._internalRoot = createRootImpl(container, ConcurrentRoot, options);
// [ts] }
// [ts] fn ReactDOMBlockingRoot(container: auto, tag: auto, options: auto) {
// [ts] _._internalRoot = createRootImpl(container, tag, options);
// [ts] }
// [ts] ReactDOMRoot.prototype.render = ReactDOMBlockingRoot.prototype.render = (children) => {
// [ts] let root = _._internalRoot;
// [ts] {
// [ts] if (@typeName(arguments[1]) == "function") {
// [ts] error("render(...): does not support the second callback argument. " + "To execute a side effect after rendering, declare it in a component body with useEffect().");
// [ts] }
// [ts] let container = root.containerInfo;
// [ts] if (container.nodeType != COMMENT_NODE) {
// [ts] let hostInstance = findHostInstanceWithNoPortals(root.current);
// [ts] if (hostInstance) {
// [ts] if (hostInstance.parentNode != container) {
// [ts] error("render(...): It looks like the React-rendered content of the " + "root container was removed without using React. This is not " + "supported and will cause errors. Instead, call " + "root.unmount() to empty a root's container.");
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] updateContainer(children, root, null, null);
// [ts] };
// [ts] ReactDOMRoot.prototype.unmount = ReactDOMBlockingRoot.prototype.unmount = () => {
// [ts] {
// [ts] if (@typeName(arguments[0]) == "function") {
// [ts] error("unmount(...): does not support a callback argument. " + "To execute a side effect after rendering, declare it in a component body with useEffect().");
// [ts] }
// [ts] }
// [ts] let root = _._internalRoot;
// [ts] let container = root.containerInfo;
// [ts] updateContainer(null, root, null, () => {
// [ts] unmarkContainerAsRoot(container);
// [ts] });
// [ts] };
// [ts] fn createRootImpl(container: auto, tag: auto, options: auto) {
// Tag is either LegacyRoot or Concurrent Root
// [ts] let hydrate = options != null && options.hydrate == true;
// [ts] let hydrationCallbacks = options != null && options.hydrationOptions || null;
// [ts] let root = createContainer(container, tag, hydrate);
// [ts] markContainerAsRoot(root.current, container);
// [ts] if (hydrate && tag != LegacyRoot) {
// [ts] let doc = if (container.nodeType == DOCUMENT_NODE) container else container.ownerDocument;
// [ts] eagerlyTrapReplayableEvents(container, doc);
// [ts] }
// [ts] return root;
// [ts] }
// [ts] fn createLegacyRoot(container: auto, options: auto) {
// [ts] return ReactDOMBlockingRoot.init(container, LegacyRoot, options);
// [ts] }
// [ts] fn isValidContainer(node: auto) {
// [ts] return !!(node && (node.nodeType == ELEMENT_NODE || node.nodeType == DOCUMENT_NODE || node.nodeType == DOCUMENT_FRAGMENT_NODE || node.nodeType == COMMENT_NODE && node.nodeValue == " react-mount-point-unstable "));
// [ts] }
// [ts] let ReactCurrentOwner$3 = ReactSharedInternals.ReactCurrentOwner;
// [ts] let topLevelUpdateWarnings;
// [ts] let warnedAboutHydrateAPI = false;
// [ts] {
// [ts] topLevelUpdateWarnings = (container) => {
// [ts] if (container._reactRootContainer && container.nodeType != COMMENT_NODE) {
// [ts] let hostInstance = findHostInstanceWithNoPortals(container._reactRootContainer._internalRoot.current);
// [ts] if (hostInstance) {
// [ts] if (hostInstance.parentNode != container) {
// [ts] error("render(...): It looks like the React-rendered content of this " + "container was removed without using React. This is not " + "supported and will cause errors. Instead, call " + "ReactDOM.unmountComponentAtNode to empty a container.");
// [ts] }
// [ts] }
// [ts] }
// [ts] let isRootRenderedBySomeReact = !!container._reactRootContainer;
// [ts] let rootEl = getReactRootElementInContainer(container);
// [ts] let hasNonRootReactChild = !!(rootEl && getInstanceFromNode$1(rootEl));
// [ts] if (hasNonRootReactChild && !isRootRenderedBySomeReact) {
// [ts] error("render(...): Replacing React-rendered children with a new root " + "component. If you intended to update the children of this node, " + "you should instead have the existing children update their state " + "and render the new components instead of calling ReactDOM.render.");
// [ts] }
// [ts] if (container.nodeType == ELEMENT_NODE && container.tagName && container.tagName.toUpperCase() == "BODY") {
// [ts] error("render(): Rendering components directly into document.body is " + "discouraged, since its children are often manipulated by third-party " + "scripts and browser extensions. This may lead to subtle " + "reconciliation issues. Try rendering into a container element created " + "for your app.");
// [ts] }
// [ts] };
// [ts] }
// [ts] fn getReactRootElementInContainer(container: auto) {
// [ts] if (!container) {
// [ts] return null;
// [ts] }
// [ts] if (container.nodeType == DOCUMENT_NODE) {
// [ts] return container.documentElement;
// [ts] } else {
// [ts] return container.firstChild;
// [ts] }
// [ts] }
// [ts] fn shouldHydrateDueToLegacyHeuristic(container: auto) {
// [ts] let rootElement = getReactRootElementInContainer(container);
// [ts] return !!(rootElement && rootElement.nodeType == ELEMENT_NODE && rootElement.hasAttribute(ROOT_ATTRIBUTE_NAME));
// [ts] }
// [ts] fn legacyCreateRootFromDOMContainer(container: auto, forceHydrate: auto) {
// [ts] let shouldHydrate = forceHydrate || shouldHydrateDueToLegacyHeuristic(container); // First clear any existing content.
// [ts] if (!shouldHydrate) {
// [ts] let warned = false;
// [ts] let rootSibling;
// [ts] while (rootSibling = container.lastChild) {
// [ts] {
// [ts] if (!warned && rootSibling.nodeType == ELEMENT_NODE && rootSibling.hasAttribute(ROOT_ATTRIBUTE_NAME)) {
// [ts] warned = true;
// [ts] error("render(): Target node has markup rendered by React, but there " + "are unrelated nodes as well. This is most commonly caused by " + "white-space inserted around server-rendered markup.");
// [ts] }
// [ts] }
// [ts] container.removeChild(rootSibling);
// [ts] }
// [ts] }
// [ts] {
// [ts] if (shouldHydrate && !forceHydrate && !warnedAboutHydrateAPI) {
// [ts] warnedAboutHydrateAPI = true;
// [ts] warn("render(): Calling ReactDOM.render() to hydrate server-rendered markup " + "will stop working in React v17. Replace the ReactDOM.render() call " + "with ReactDOM.hydrate() if you want React to attach to the server HTML.");
// [ts] }
// [ts] }
// [ts] return createLegacyRoot(container, shouldHydrate ? {
// [ts] hydrate: true
// [ts] } : null);
// [ts] }
// [ts] (callback, callerName) => {
// [ts] {
// [ts] if (callback != null && @typeName(callback) != "function") {
// [ts] error("%s(...): Expected the last optional `callback` argument to be a " + "function. Instead received: %s.", callerName, callback);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn legacyRenderSubtreeIntoContainer(parentComponent: auto, children: auto, container: auto, forceHydrate: auto, callback: auto) {
// [ts] {
// [ts] topLevelUpdateWarnings(container);
// [ts] warnOnInvalidCallback$1(if (callback == null) null else callback, "render");
// [ts] } // TODO: Without `any` type, Flow says "Property cannot be accessed on any
// member of intersection type." Whyyyyyy.
// [ts] let root = container._reactRootContainer;
// [ts] let fiberRoot;
// [ts] if (!root) {
// Initial mount
// [ts] root = container._reactRootContainer = legacyCreateRootFromDOMContainer(container, forceHydrate);
// [ts] fiberRoot = root._internalRoot;
// [ts] if (@typeName(callback) == "function") {
// [ts] let originalCallback = callback;
// [ts] callback = () => {
// [ts] let instance = getPublicRootInstance(fiberRoot);
// [ts] originalCallback.call(instance);
// [ts] };
// [ts] } // Initial mount should not be batched.
// [ts] unbatchedUpdates(() => {
// [ts] updateContainer(children, fiberRoot, parentComponent, callback);
// [ts] });
// [ts] } else {
// [ts] fiberRoot = root._internalRoot;
// [ts] if (@typeName(callback) == "function") {
// [ts] let _originalCallback = callback;
// [ts] callback = () => {
// [ts] let instance = getPublicRootInstance(fiberRoot);
// [ts] _originalCallback.call(instance);
// [ts] };
// [ts] } // Update
// [ts] updateContainer(children, fiberRoot, parentComponent, callback);
// [ts] }
// [ts] return getPublicRootInstance(fiberRoot);
// [ts] }
// [ts] fn findDOMNode(componentOrElement: auto) {
// [ts] {
// [ts] let owner = ReactCurrentOwner$3.current;
// [ts] if (owner != null && owner.stateNode != null) {
// [ts] let warnedAboutRefsInRender = owner.stateNode._warnedAboutRefsInRender;
// [ts] if (!warnedAboutRefsInRender) {
// [ts] error("%s is accessing findDOMNode inside its render(). " + "render() should be a pure function of props and state. It should " + "never access something that requires stale data from the previous " + "render, such as refs. Move this logic to componentDidMount and " + "componentDidUpdate instead.", getComponentName(owner.type) || "A component");
// [ts] }
// [ts] owner.stateNode._warnedAboutRefsInRender = true;
// [ts] }
// [ts] }
// [ts] if (componentOrElement == null) {
// [ts] return null;
// [ts] }
// [ts] if (componentOrElement.nodeType == ELEMENT_NODE) {
// [ts] return componentOrElement;
// [ts] }
// [ts] {
// [ts] return findHostInstanceWithWarning(componentOrElement, "findDOMNode");
// [ts] }
// [ts] }
// [ts] fn hydrate(element: auto, container: auto, callback: auto) {
// [ts] if (!isValidContainer(container)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Target container is not a DOM element." );)
// [ts] }
// [ts] }
// [ts] {
// [ts] let isModernRoot = isContainerMarkedAsRoot(container) && container._reactRootContainer == null;
// [ts] if (isModernRoot) {
// [ts] error("You are calling ReactDOM.hydrate() on a container that was previously " + "passed to ReactDOM.createRoot(). This is not supported. " + "Did you mean to call createRoot(container, {hydrate: true}).render(element)?");
// [ts] }
// [ts] } // TODO: throw or warn if we couldn't hydrate?
// [ts] return legacyRenderSubtreeIntoContainer(null, element, container, true, callback);
// [ts] }
// [ts] fn render(element: auto, container: auto, callback: auto) {
// [ts] if (!isValidContainer(container)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Target container is not a DOM element." );)
// [ts] }
// [ts] }
// [ts] {
// [ts] let isModernRoot = isContainerMarkedAsRoot(container) && container._reactRootContainer == null;
// [ts] if (isModernRoot) {
// [ts] error("You are calling ReactDOM.render() on a container that was previously " + "passed to ReactDOM.createRoot(). This is not supported. " + "Did you mean to call root.render(element)?");
// [ts] }
// [ts] }
// [ts] return legacyRenderSubtreeIntoContainer(null, element, container, false, callback);
// [ts] }
// [ts] fn unstable_renderSubtreeIntoContainer(parentComponent: auto, element: auto, containerNode: auto, callback: auto) {
// [ts] if (!isValidContainer(containerNode)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Target container is not a DOM element." );)
// [ts] }
// [ts] }
// [ts] if (!(parentComponent != null && has$1(parentComponent))) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "parentComponent must be a valid React Component" );)
// [ts] }
// [ts] }
// [ts] return legacyRenderSubtreeIntoContainer(parentComponent, element, containerNode, false, callback);
// [ts] }
// [ts] fn unmountComponentAtNode(container: auto) {
// [ts] if (!isValidContainer(container)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "unmountComponentAtNode(...): Target container is not a DOM element." );)
// [ts] }
// [ts] }
// [ts] {
// [ts] let isModernRoot = isContainerMarkedAsRoot(container) && container._reactRootContainer == null;
// [ts] if (isModernRoot) {
// [ts] error("You are calling ReactDOM.unmountComponentAtNode() on a container that was previously " + "passed to ReactDOM.createRoot(). This is not supported. Did you mean to call root.unmount()?");
// [ts] }
// [ts] }
// [ts] if (container._reactRootContainer) {
// [ts] {
// [ts] let rootEl = getReactRootElementInContainer(container);
// [ts] let renderedByDifferentReact = rootEl && !getInstanceFromNode$1(rootEl);
// [ts] if (renderedByDifferentReact) {
// [ts] error("unmountComponentAtNode(): The node you're attempting to unmount " + "was rendered by another copy of React.");
// [ts] }
// [ts] } // Unmount should not be batched.
// [ts] unbatchedUpdates(() => {
// [ts] legacyRenderSubtreeIntoContainer(null, null, container, false, () => {
// $FlowFixMe This should probably use `delete container._reactRootContainer`
// [ts] container._reactRootContainer = null;
// [ts] unmarkContainerAsRoot(container);
// [ts] });
// [ts] }); // If you call unmountComponentAtNode twice in quick succession, you'll
// get `true` twice. That's probably fine?
// [ts] return true;
// [ts] } else {
// [ts] {
// [ts] let _rootEl = getReactRootElementInContainer(container);
// [ts] let hasNonRootReactChild = !!(_rootEl && getInstanceFromNode$1(_rootEl)); // Check if the container itself is a React root node.
// [ts] let isContainerReactRoot = container.nodeType == ELEMENT_NODE && isValidContainer(container.parentNode) && !!container.parentNode._reactRootContainer;
// [ts] if (hasNonRootReactChild) {
// [ts] error(if ("unmountComponentAtNode(): The node you're attempting to unmount " + "was rendered by React and is not a top-level container. %s", isContainerReactRoot) "You may have accidentally passed in a React root node instead " + "of its container." else "Instead, have the parent component update its state and " + "rerender in order to remove this component.");
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] }
// [ts] fn createPortal(children: auto, containerInfo: auto, // TODO: figure out the API for cross-renderer implementation. implementation) {
// [ts] let key = if (arguments.length > 3 && arguments[3] != null) arguments[3] else null;
// [ts] return {
// This tag allow us to uniquely identify this as a React Portal
// [ts] $$typeof: REACT_PORTAL_TYPE,
// [ts] key: if (key == null) null else "" + key,
// [ts] children: children,
// [ts] containerInfo: containerInfo,
// [ts] implementation: implementation
// [ts] };
// [ts] }
// [ts] let ReactVersion = "16.14.0";
// [ts] setAttemptUserBlockingHydration(attemptUserBlockingHydration$1);
// [ts] setAttemptContinuousHydration(attemptContinuousHydration$1);
// [ts] setAttemptHydrationAtCurrentPriority(attemptHydrationAtCurrentPriority$1);
// [ts] let didWarnAboutUnstableCreatePortal = false;
// [ts] {
// [ts] if (@typeName(Map) != "function" || // $FlowIssue Flow incorrectly thinks Map has no prototype
// [ts] Map.prototype == null || @typeName(Map.prototype.forEach) != "function" || @typeName(Set) != "function" || // $FlowIssue Flow incorrectly thinks Set has no prototype
// [ts] Set.prototype == null || @typeName(Set.prototype.clear) != "function" || @typeName(Set.prototype.forEach) != "function") {
// [ts] error("React depends on Map and Set built-in types. Make sure that you load a " + "polyfill in older browsers. https://fb.me/react-polyfills");
// [ts] }
// [ts] }
// [ts] setRestoreImplementation(restoreControlledState$3);
// [ts] setBatchingImplementation(batchedUpdates$1, discreteUpdates$1, flushDiscreteUpdates, batchedEventUpdates$1);
// [ts] (children, container) => {
// [ts] let key = if (arguments.length > 2 && arguments[2] != null) arguments[2] else null;
// [ts] if (!isValidContainer(container)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Target container is not a DOM element." );)
// [ts] }
// [ts] } // TODO: pass ReactDOM portal implementation as third argument
// $FlowFixMe The Flow type is opaque but there's no way to actually create it.
// [ts] return createPortal(children, container, null, key);
// [ts] }
// [ts] fn renderSubtreeIntoContainer(parentComponent: auto, element: auto, containerNode: auto, callback: auto) {
// [ts] return unstable_renderSubtreeIntoContainer(parentComponent, element, containerNode, callback);
// [ts] }
// [ts] fn unstable_createPortal(children: auto, container: auto) {
// [ts] let key = if (arguments.length > 2 && arguments[2] != null) arguments[2] else null;
// [ts] {
// [ts] if (!didWarnAboutUnstableCreatePortal) {
// [ts] didWarnAboutUnstableCreatePortal = true;
// [ts] warn("The ReactDOM.unstable_createPortal() alias has been deprecated, " + "and will be removed in React 17+. Update your code to use " + "ReactDOM.createPortal() instead. It has the exact same API, " + "but without the \"unstable_\" prefix.");
// [ts] }
// [ts] }
// [ts] return createPortal$1(children, container, key);
// [ts] }
// [ts] let Internals = {
// Keep in sync with ReactDOMUnstableNativeDependencies.js
// ReactTestUtils.js, and ReactTestUtilsAct.js. This is an array for better minification.
// [ts] Events: [getInstanceFromNode$1, getNodeFromInstance$1, getFiberCurrentPropsFromNode$1, injectEventPluginsByName, eventNameDispatchConfigs, accumulateTwoPhaseDispatches, accumulateDirectDispatches, enqueueStateRestore, restoreStateIfNeeded, dispatchEvent, runEventsInBatch, flushPassiveEffects, IsThisRendererActing]
// [ts] };
// [ts] let foundDevTools = injectIntoDevTools({
// [ts] findFiberByHostInstance: getClosestInstanceFromNode,
// [ts] bundleType:  1 ,
// [ts] version: ReactVersion,
// [ts] rendererPackageName: "react-dom"
// [ts] });
// [ts] {
// [ts] if (!foundDevTools && canUseDOM && window.top == window.self) {
// If we're in Chrome or Firefox, provide a download link if not installed.
// [ts] if (navigator.userAgent.indexOf("Chrome") > -1 && navigator.userAgent.indexOf("Edge") == -1 || navigator.userAgent.indexOf("Firefox") > -1) {
// [ts] let protocol = window.location.protocol; // Don't warn in exotic cases like chrome-extension://.
// [ts] if ("^(https?|file):$".test(protocol)) {
// eslint-disable-next-line react-internal/no-production-logging
// [ts] @print("%cDownload the React DevTools " + "for a better development experience: " + "https://fb.me/react-devtools" + (if (protocol == "file:") "\nYou might need to use a local HTTP server (instead of file://): " + "https://fb.me/react-devtools-faq" else ""), "font-weight:bold");
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] exports.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED = Internals;
// [ts] exports.createPortal = createPortal$1;
// [ts] exports.findDOMNode = findDOMNode;
// [ts] exports.flushSync = flushSync;
// [ts] exports.hydrate = hydrate;
// [ts] exports.render = render;
// [ts] exports.unmountComponentAtNode = unmountComponentAtNode;
// [ts] exports.unstable_batchedUpdates = batchedUpdates$1;
// [ts] exports.unstable_createPortal = unstable_createPortal;
// [ts] exports.unstable_renderSubtreeIntoContainer = renderSubtreeIntoContainer;
// [ts] exports.version = ReactVersion;
// [ts] })));
