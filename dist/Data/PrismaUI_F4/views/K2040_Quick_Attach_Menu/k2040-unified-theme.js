"use strict";
window.applyUnifiedTheme = function(theme) {
  if (!["default","midnight","high-contrast","pipboy"].includes(theme)) return;
  document.documentElement.setAttribute("data-k2040-unified-theme", theme);
};
