// Migrated from TypeScript (src/touch-event-utils.ts) via cap from -ts

/**
 * @license
 * Copyright 2024 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";

pub type ReportedTouch = [f64, f64];
pub struct ReportedTouchEvent {
  changed: []ReportedTouch = [];
  touches: []ReportedTouch = [];
}
pub struct TouchEventReport {
  events: []ReportedTouchEvent = [];
}

pub async fn initializeTouchEventReport(page: Page): TouchEventReport {
    // TODO(cap-migrate): refine TS body
    // [ts] const events: []ReportedTouchEvent = [];
    // [ts] await page.exposeFunction(
    // [ts] "reportTouchEvent",
    // [ts] (event) => {
    // [ts] events.push(event);
    // [ts] },
    // [ts] );
    // [ts] await page.evaluate(() => {
    // [ts] document.body.addEventListener("touchstart", reportTouchEvent);
    // [ts] document.body.addEventListener("touchmove", reportTouchEvent);
    // [ts] document.body.addEventListener("touchend", reportTouchEvent);
    // [ts] fn reportTouchEvent(e: TouchEvent) {
    // [ts] const toReport: ReportedTouchEvent = {
    // [ts] changed: getReportableTouchList(e.changedTouches),
    // [ts] touches: getReportableTouchList(e.touches),
    // [ts] };
    // [ts] (window).reportTouchEvent(toReport);
    // [ts] }
    // [ts] fn getReportableTouchList(list: TouchList): []ReportedTouch {
    // [ts] return [...list].map(t => {
    // [ts] return [t.pageX, t.pageY];
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] return {events};
}
