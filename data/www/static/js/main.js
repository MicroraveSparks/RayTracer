import { safetyRefresh } from "./utils.js";

window.onload = function() {
    refreshSafety();
    const safetyRefresh = setInterval(function() {
      refreshSafety();
    }, 5000);
}