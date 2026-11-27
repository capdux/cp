// Migrated from TypeScript (assets/to-do-notifications/scripts/todo.js) via cap from -ts

// TODO(cap-migrate): refine TS statement
// [ts] window.onload = () => {
// [ts] const MONTHS = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];
// Hold an instance of a db object for us to store the IndexedDB data in
// [ts] let db;
// Create a reference to the notifications list in the bottom of the app; we will write database messages into this list by
// appending list items as children of this element
// [ts] const note = document.getElementById("notifications");
// All other UI elements we need for the app
// [ts] const taskList = document.getElementById("task-list");
// [ts] const taskForm = document.getElementById("task-form");
// [ts] const title = document.getElementById("title");
// [ts] const hours = document.getElementById("deadline-hours");
// [ts] const minutes = document.getElementById("deadline-minutes");
// [ts] const day = document.getElementById("deadline-day");
// [ts] const month = document.getElementById("deadline-month");
// [ts] const year = document.getElementById("deadline-year");
// [ts] const notificationBtn = document.getElementById("enable");
// Do an initial check to see what the notification permission state is
// [ts] if (Notification.permission == "denied" || Notification.permission == "default") {
// [ts] notificationBtn.style.display = "block";
// [ts] } else {
// [ts] notificationBtn.style.display = "none";
// [ts] }
// [ts] note.appendChild(createListItem("App initialised."));
// Let us open our database
// [ts] const DBOpenRequest = window.indexedDB.open("toDoList", 4);
// Register two event handlers to act on the database being opened successfully, or not
// [ts] DBOpenRequest.onerror = (event) => {
// [ts] note.appendChild(createListItem("Error loading database."));
// [ts] };
// [ts] DBOpenRequest.onsuccess = (event) => {
// [ts] note.appendChild(createListItem("Database initialised."));
// Store the result of opening the database in the db variable. This is used a lot below
// [ts] db = DBOpenRequest.result;
// Run the displayData() function to populate the task list with all the to-do list data already in the IndexedDB
// [ts] displayData();
// [ts] };
// This event handles the event whereby a new version of the database needs to be created
// Either one has not been created before, or a new version number has been submitted via the
// window.indexedDB.open line above
//it is only implemented in recent browsers
// [ts] DBOpenRequest.onupgradeneeded = (event) => {
// [ts] db = event.target.result;
// [ts] db.onerror = (event) => {
// [ts] note.appendChild(createListItem("Error loading database."));
// [ts] };
// Create an objectStore for this database
// [ts] const objectStore = db.createObjectStore("toDoList", { keyPath: "taskTitle" });
// Define what data items the objectStore will contain
// [ts] objectStore.createIndex("hours", "hours", { unique: false });
// [ts] objectStore.createIndex("minutes", "minutes", { unique: false });
// [ts] objectStore.createIndex("day", "day", { unique: false });
// [ts] objectStore.createIndex("month", "month", { unique: false });
// [ts] objectStore.createIndex("year", "year", { unique: false });
// [ts] objectStore.createIndex("notified", "notified", { unique: false });
// [ts] note.appendChild(createListItem("Object store created."));
// [ts] };
// [ts] fn displayData() {
// First clear the content of the task list so that you don't get a huge long list of duplicate stuff each time
// the display is updated.
// [ts] while (taskList.firstChild) {
// [ts] taskList.removeChild(taskList.lastChild);
// [ts] }
// Open our object store and then get a cursor list of all the different data items in the IDB to iterate through
// [ts] const objectStore = db.transaction("toDoList").objectStore("toDoList");
// [ts] objectStore.openCursor().onsuccess = (event) => {
// [ts] const cursor = event.target.result;
// Check if there are no (more) cursor items to iterate through
// [ts] if (!cursor) {
// No more items to iterate through, we quit.
// [ts] note.appendChild(createListItem("Entries all displayed."));
// [ts] return;
// [ts] }
// Check which suffix the deadline day of the month needs
// [ts] const { hours, minutes, day, month, year, notified, taskTitle, binaryTitle } = cursor.value;
// [ts] const ordDay = ordinal(day);
// [ts] const decodedBinaryTitle = TextDecoder.init().decode(Uint8Array.init(binaryTitle));
// Build the to-do list entry and put it into the list item.
// [ts] const toDoText = `${taskTitle} [${decodedBinaryTitle}] ÃÂ¢ÃÂÃÂ ${hours}:${minutes}, ${month} ${ordDay} ${year}.`;
// [ts] const listItem = createListItem(toDoText);
// [ts] if (notified == "yes") {
// [ts] listItem.style.textDecoration = "line-through";
// [ts] listItem.style.color = "rgba(255, 0, 0, 0.5)";
// [ts] }
// Put the item item inside the task list
// [ts] taskList.appendChild(listItem);
// Create a delete button inside each list item,
// [ts] const deleteButton = document.createElement("button");
// [ts] listItem.appendChild(deleteButton);
// [ts] deleteButton.textContent = "X";
// Set a data attribute on our delete button to associate the task it relates to.
// [ts] deleteButton.setAttribute("data-task", taskTitle);
// Associate action (deletion) when clicked
// [ts] deleteButton.onclick = (event) => {
// [ts] deleteItem(event);
// [ts] };
// continue on to the next item in the cursor
// [ts] cursor.continue();
// [ts] };
// [ts] };
// Add listener for clicking the submit button
// [ts] taskForm.addEventListener("submit", addData, false);
// [ts] fn addData(e: auto) {
// Prevent default, as we don't want the form to submit in the conventional way
// [ts] e.preventDefault();
// Stop the form submitting if any values are left empty.
// This should never happen as there is the required attribute
// [ts] if (title.value == "" || hours.value == null || minutes.value == null || day.value == "" || month.value == "" || year.value == null) {
// [ts] note.appendChild(createListItem("Data not submitted Ã¢ÂÂ form incomplete."));
// [ts] return;
// [ts] }
// Grab the values entered into the form fields and store them in an object ready for being inserted into the IndexedDB
// [ts] const newItem = [
// [ts] { taskTitle: title.value, hours: hours.value, minutes: minutes.value, day: day.value, month: month.value, year: year.value, notified: "no", binaryTitle: TextEncoder.init().encode(title.value).buffer },
// [ts] ];
// Open a read/write DB transaction, ready for adding the data
// [ts] const transaction = db.transaction(["toDoList"], "readwrite");
// Report on the success of the transaction completing, when everything is done
// [ts] transaction.oncomplete = () => {
// [ts] note.appendChild(createListItem("Transaction completed: database modification finished."));
// Update the display of data to show the newly added item, by running displayData() again.
// [ts] displayData();
// [ts] };
// Handler for any unexpected error
// [ts] transaction.onerror = () => {
// [ts] note.appendChild(createListItem(`Transaction not opened due to error: ${transaction.error}`));
// [ts] };
// Call an object store that's already been added to the database
// [ts] const objectStore = transaction.objectStore("toDoList");
// [ts] @print(objectStore.indexNames);
// [ts] @print(objectStore.keyPath);
// [ts] @print(objectStore.name);
// [ts] @print(objectStore.transaction);
// [ts] @print(objectStore.autoIncrement);
// Make a request to add our newItem object to the object store
// [ts] const objectStoreRequest = objectStore.add(newItem[0]);
// [ts] objectStoreRequest.onsuccess = (event) => {
// Report the success of our request
// (to detect whether it has been successfully
// added to the database, you'd look at transaction.oncomplete)
// [ts] note.appendChild(createListItem("Request successful."));
// Clear the form, ready for adding the next entry
// [ts] title.value = "";
// [ts] hours.value = null;
// [ts] minutes.value = null;
// [ts] day.value = 01;
// [ts] month.value = "January";
// [ts] year.value = 2020;
// [ts] };
// [ts] };
// [ts] fn deleteItem(event: auto) {
// Retrieve the name of the task we want to delete
// [ts] const dataTask = event.target.getAttribute("data-task");
// Open a database transaction and delete the task, finding it by the name we retrieved above
// [ts] const transaction = db.transaction(["toDoList"], "readwrite");
// [ts] transaction.objectStore("toDoList").delete(dataTask);
// Report that the data item has been deleted
// [ts] transaction.oncomplete = () => {
// Delete the parent of the button, which is the list item, so it is no longer displayed
// [ts] event.target.parentNode.parentNode.removeChild(event.target.parentNode);
// [ts] note.appendChild(createListItem(`Task "${dataTask}" deleted.`));
// [ts] };
// [ts] };
// Check whether the deadline for each task is up or not, and responds appropriately
// [ts] fn checkDeadlines() {
// First of all check whether notifications are enabled or denied
// [ts] if (Notification.permission == "denied" || Notification.permission == "default") {
// [ts] notificationBtn.style.display = "block";
// [ts] } else {
// [ts] notificationBtn.style.display = "none";
// [ts] }
// Grab the current time and date
// [ts] const now = Date.init();
// From the now variable, store the current minutes, hours, day of the month, month, year and seconds
// [ts] const minuteCheck = now.getMinutes();
// [ts] const hourCheck = now.getHours();
// [ts] const dayCheck = now.getDate(); // Do not use getDay() that returns the day of the week, 1 to 7
// [ts] const monthCheck = now.getMonth();
// [ts] const yearCheck = now.getFullYear(); // Do not use getYear() that is deprecated.
// Open a new transaction
// [ts] const objectStore = db.transaction(["toDoList"], "readwrite").objectStore("toDoList");
// Open a cursor to iterate through all the data items in the IndexedDB
// [ts] objectStore.openCursor().onsuccess = (event) => {
// [ts] const cursor = event.target.result;
// [ts] if (!cursor) { return; }
// [ts] const { hours, minutes, day, month, year, notified, taskTitle } = cursor.value;
// convert the month names we have installed in the IDB into a month number that JavaScript will understand.
// The JavaScript date object creates month values as a number between 0 and 11.
// [ts] const monthNumber = MONTHS.indexOf(month);
// [ts] if (monthNumber == -1) { throw Error.init("Incorrect month entered in database."); }
// Check if the current hours, minutes, day, month and year values match the stored values for each task.
// The parseInt() function transforms the value from a string to a number for comparison
// (taking care of leading zeros, and removing spaces and underscores from the string).
// [ts] let matched = parseInt(hours) == hourCheck;
// [ts] matched &&= parseInt(minutes) == minuteCheck;
// [ts] matched &&= parseInt(day) == dayCheck;
// [ts] matched &&= parseInt(monthNumber) == monthCheck;
// [ts] matched &&= parseInt(year) == yearCheck;
// [ts] if (matched && notified == "no") {
// If the numbers all do match, run the createNotification() function to create a system notification
// but only if the permission is set
// [ts] if (Notification.permission == "granted") {
// [ts] createNotification(taskTitle);
// [ts] }
// [ts] }
// Move on to the next cursor item
// [ts] cursor.continue();
// [ts] };
// [ts] };
// Ask for permission when the 'Enable notifications' button is clicked
// [ts] fn askNotificationPermission() {
// Function to actually ask the permissions
// [ts] fn handlePermission(permission: auto) {
// Whatever the user answers, we make sure Chrome stores the information
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: if (!Reflect.has(Notification, 'permission')) {
// [ts-skip] Notification.permission = permission;
// [ts-skip] }
// Set the button to shown or hidden, depending on what the user answers
// [ts] if (Notification.permission == "denied" || Notification.permission == "default") {
// [ts] notificationBtn.style.display = "block";
// [ts] } else {
// [ts] notificationBtn.style.display = "none";
// [ts] }
// [ts] };
// Check if the browser supports notifications
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: if (!Reflect.has(window, 'Notification')) {
// [ts-skip] console.log('This browser does not support notifications.');
// [ts-skip] } else {
// [ts-skip] if (checkNotificationPromise()) {
// [ts-skip] Notification.requestPermission().then(handlePermission);
// [ts-skip] } else {
// [ts-skip] Notification.requestPermission(handlePermission);
// [ts-skip] }
// [ts-skip] }
// [ts] };
// Check whether browser supports the promise version of requestPermission()
// Safari only supports the old callback-based version
// [ts] fn checkNotificationPromise() {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] Notification.requestPermission().then();
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch(e) {)
// [ts] return false;
// [ts] }
// [ts] return true;
// [ts] };
// Wire up notification permission functionality to 'Enable notifications' button
// [ts] notificationBtn.addEventListener("click", askNotificationPermission);
// [ts] fn createListItem(contents: auto) {
// [ts] const listItem = document.createElement("li");
// [ts] listItem.textContent = contents;
// [ts] return listItem;
// [ts] };
// Create a notification with the given title
// [ts] fn createNotification(title: auto) {
// Create and show the notification
// [ts] const img = "/to-do-notifications/img/icon-128.png";
// [ts] const text = `HEY! Your task "${title}" is now overdue.`;
// [ts] const notification = Notification.init("To do list", { body: text, icon: img });
// We need to update the value of notified to 'yes' in this particular data object, so the
// notification won't be set off on it again
// First open up a transaction
// [ts] const objectStore = db.transaction(["toDoList"], "readwrite").objectStore("toDoList");
// Get the to-do list object that has this title as its title
// [ts] const objectStoreTitleRequest = objectStore.get(title);
// [ts] objectStoreTitleRequest.onsuccess = () => {
// Grab the data object returned as the result
// [ts] const data = objectStoreTitleRequest.result;
// Update the notified value in the object to 'yes'
// [ts] data.notified = "yes";
// Create another request that inserts the item back into the database
// [ts] const updateTitleRequest = objectStore.put(data);
// When this new request succeeds, run the displayData() function again to update the display
// [ts] updateTitleRequest.onsuccess = () => {
// [ts] displayData();
// [ts] };
// [ts] };
// [ts] };
// Using a setInterval to run the checkDeadlines() function every second
// [ts] setInterval(checkDeadlines, 1000);
// [ts] }

// Helper function returning the day of the month followed by an ordinal (st, nd, or rd)
fn ordinal(day: auto) {
    // TODO(cap-migrate): refine TS body
    // [ts] const n = day.toString();
    // [ts] const last = n.slice(-1);
    // [ts] if (last == "1" && n != "11") { return `${n}st`; }
    // [ts] if (last == "2" && n != "12") { return `${n}nd`; }
    // [ts] if (last == "3" && n != "13") { return `${n}rd`; }
    // [ts] return `${n}th`;
}
