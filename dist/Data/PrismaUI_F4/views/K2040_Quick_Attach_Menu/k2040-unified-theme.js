"use strict";
window.applyUnifiedTheme = function(theme) {
  if (!["default","midnight","high-contrast","pipboy"].includes(theme)) return false;
  document.documentElement.setAttribute("data-k2040-unified-theme", theme);
  return true;
};


// The VNext hub supplies independently selected structural theme and palette.
// This is transient CSS state; never overwrite the owner's saved palette.
window.applyUnifiedAppearance = function(raw) {
  let data;
  try { data = JSON.parse(raw); } catch (_) { return false; }
  if (!data || data.schema !== 1 ||
      !["fallout4","fallout76","doom-eternal","fallout3-newvegas"].includes(data.theme) ||
      typeof data.palette !== "string" || !/^[a-z0-9-]{1,48}$/.test(data.palette) ||
      !data.colors || typeof data.colors !== "object" || Array.isArray(data.colors)) return false;
  const keys = ["background","panel","text","accent","muted","border","highlight"];
  if (Object.keys(data.colors).length !== keys.length ||
      !keys.every(key => typeof data.colors[key] === "string" &&
                         /^#[0-9a-fA-F]{6}$/.test(data.colors[key]))) return false;
  const el = document.documentElement;
  const css = {
    "--k2040-shared-accent": data.colors.accent,
    "--k2040-shared-text": data.colors.text,
    "--k2040-shared-muted": data.colors.muted,
    "--k2040-shared-installed": data.colors.highlight,
    "--k2040-shared-panel": data.colors.panel,
    "--k2040-shared-border": data.colors.border
  };
  for (const [name,value] of Object.entries(css)) el.style.setProperty(name,value);
  el.setAttribute("data-k2040-visual-theme",data.theme);
  el.setAttribute("data-k2040-palette",data.palette);
  el.setAttribute("data-k2040-unified-theme","default");
  return true;
};
