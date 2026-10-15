// Shared controller focus/navigation for Builder and Settings.
// Navigation never changes a preference: actions require explicit Confirm.
// Dropdowns and confirmation dialogs take priority over page navigation.
(function(global) {
  "use strict";
  function create(mode, options) {
    options = options || {};
    var confirmButton = "A";
    var cancelButton = "B";
    var secondaryButton = "";
    var lastStickDirection = "";
    var lastStickAt = 0;
    var stickNeedsNeutral = false;
    var lastDpadAt = 0;
    var sliderEditing = false;
    var active = false;

    function editingText() {
      var input = document.activeElement;
      return input && input.tagName === "INPUT" &&
        (input.type === "text" || input.type === "search");
    }

    function visible(element) {
      if (!element || element.disabled) return false;
      for (var node = element; node && node !== document; node = node.parentElement) {
        if (node.hidden || (node.getAttribute && node.getAttribute("aria-hidden") === "true")) return false;
      }
      return true;
    }
    function selectable(selector, root) {
      return Array.prototype.filter.call((root || document).querySelectorAll(selector), visible);
    }
    function focus(element) {
      if (!visible(element)) return false;
      element.focus();
      active = true;
      if (element.scrollIntoView) {
        try { element.scrollIntoView({ block: "nearest", inline: "nearest" }); }
        catch (error) { element.scrollIntoView(false); }
      }
      return true;
    }
    function primary(selector, root) { return selectable(selector, root)[0] || null; }
    function current() { return document.activeElement; }
    function pageNav() { return selectable(".workspace nav .nav-button"); }
    function pageControls() {
      return selectable("#pages .page:not([hidden]) button:not(.keybind), #pages .page:not([hidden]) input[type=range]");
    }
    function builderGroups() {
      return [
        selectable("#builder > header button"),
        selectable("#builder .weapon-settings button, #builder .weapon-settings input[type=checkbox]"),
        selectable("#categories .row"),
        selectable("#options .row")
      ];
    }
    function groupOf(groups, node) {
      for (var i = 0; i < groups.length; ++i) if (groups[i].indexOf(node) >= 0) return i;
      return -1;
    }
    function shiftGroup(delta) {
      var groups = builderGroups();
      var index = groupOf(groups, current());
      if (index < 0) index = 2;
      for (var n = 1; n <= groups.length; n++) {
        var target = (index + delta * n + groups.length * n) % groups.length;
        if (groups[target].length) {
          focus(groups[target][0]);
          return;
        }
      }
    }
    function moveIn(list, direction) {
      if (!list.length) return false;
      var index = list.indexOf(current());
      var next = index < 0 ? 0 : (index + direction + list.length) % list.length;
      return focus(list[next]);
    }
    function expandedDropdown() {
      var choices = selectable(".choice-options[role=listbox]");
      for (var i = 0; i < choices.length; i++) {
        if (!choices[i].hidden) return choices[i];
      }
      return null;
    }
    function dropdownTrigger(panel) {
      return panel && panel.id ? document.getElementById(panel.id.replace(/Options$/, "Trigger")) : null;
    }
    function dismissDropdown(panel) {
      var trigger = dropdownTrigger(panel);
      if (!trigger) return;
      if (trigger.getAttribute("aria-expanded") === "true") trigger.click();
      focus(trigger);
    }
    function modal() {
      var ids = mode === "builder" ? ["profileDialog"] : ["confirmOverlay"];
      for (var i = 0; i < ids.length; i++) {
        var e = document.getElementById(ids[i]);
        if (visible(e)) return e;
      }
      return null;
    }
    function modalCancel(element) {
      var button = primary("#closeProfileDialog, #cancelReset", element);
      if (button) button.click();
      else return false;
      return true;
    }
    function confirm() {
      var dialog = modal();
      if (dialog) {
        if (!dialog.contains(current())) {
          focus(primary("button", dialog));
          return;
        }
        if (current() && current().tagName === "BUTTON") current().click();
        return;
      }
      var open = expandedDropdown();
      if (open) {
        var selected = open.contains(current()) ? current() :
          primary('.choice-option[aria-selected="true"], .choice-option', open);
        if (selected) selected.click();
        return;
      }
      var element = current();
      if (!visible(element)) return;
      if (element.tagName === "INPUT" && element.type === "range") {
        sliderEditing = !sliderEditing;
        element.classList.toggle("controller-editing", sliderEditing);
      } else if (element.tagName === "BUTTON" && !element.classList.contains("keybind")) {
        // No inferred clicks on focus, including reset, import, or toggles.
        element.click();
        var newPanel = expandedDropdown();
        if (newPanel) focus(primary('.choice-option[aria-selected="true"], .choice-option', newPanel));
      } else if (element.tagName === "INPUT" && element.type === "checkbox") {
        element.click();
      }
    }
    function cancel() {
      var dialog = modal();
      if (dialog) { modalCancel(dialog); return; }
      var open = expandedDropdown();
      if (open) { dismissDropdown(open); return; }
      if (sliderEditing) {
        sliderEditing = false;
        if (current()) current().classList.remove("controller-editing");
        return;
      }
      if (mode === "builder") {
        if (selectable("#options .row").indexOf(current()) >= 0) {
          focus(primary("#categories .row"));
        } else if (typeof global.k2040CloseRequested === "function") {
          global.k2040CloseRequested("builder");
        }
      } else {
        var nav = pageNav();
        if (nav.indexOf(current()) >= 0) {
          var back = document.getElementById("back");
          if (back) back.click();
        } else {
          focus(primary(".workspace nav .nav-button.active") || nav[0]);
        }
      }
    }
    function adjustSlider(delta) {
      var e = current();
      if (!sliderEditing || !e || e.type !== "range") return false;
      var step = Number(e.step) || 1;
      var min = Number(e.min) || 0;
      var max = Number(e.max) || 100;
      var value = Math.max(min, Math.min(max, Number(e.value) + delta * step));
      if (value !== Number(e.value)) {
        e.value = String(value);
        e.dispatchEvent(new Event("input", { bubbles: true }));
        e.dispatchEvent(new Event("change", { bubbles: true }));
      }
      return true;
    }
    function settingsHeader() {
      return selectable("#shell > header button");
    }
    function settingsGridDirection(node, direction) {
      // Theme/presentation choices form a two-column grid. Use axes as
      // displayed rather than moving focus sequentially down the DOM.
      var grid = node && node.closest && node.closest(".choice-grid");
      if (!grid) return false;
      var choices = selectable(".choice", grid);
      var index = choices.indexOf(node);
      if (index < 0) return false;
      var columns = 2; // Matches the settings .choice-grid CSS.
      var col = index % columns;
      var target = -1;
      if (direction === "left" && col > 0) target = index - 1;
      else if (direction === "right" && col < columns - 1) target = index + 1;
      else if (direction === "up") target = index - columns;
      else if (direction === "down") target = index + columns;
      if (target >= 0 && target < choices.length) {
        focus(choices[target]);
        return true;
      }
      // Do not wrap the right edge into the next row. Left from the
      // first column may return to the Settings section navigation.
      return direction === "right";
    }
    function settingsRowDirection(node, direction) {
      // Several Settings controls share one horizontal row, e.g. the
      // controller shortcut Modifier / Button / Apply controls and
      // Reset Builder / Reset Settings. Follow their physical axis.
      if (direction !== "left" && direction !== "right") return false;
      var row = node && node.closest && node.closest(".setting-row");
      if (!row) return false;
      var siblings = selectable("button, input[type=range]", row);
      if (siblings.length < 2) return false;
      var index = siblings.indexOf(node);
      if (index < 0) return false;
      var next = index + (direction === "left" ? -1 : 1);
      if (next >= 0 && next < siblings.length) {
        focus(siblings[next]);
        return true;
      }
      // Left from the first setting control returns to the side nav.
      return direction === "right";
    }
    function settingsDirection(direction) {
      var nav = pageNav();
      var header = settingsHeader();
      var node = current();
      if (header.indexOf(node) >= 0) {
        if (direction === "left" || direction === "right") {
          moveIn(header, direction === "left" ? -1 : 1);
        } else if (direction === "down") {
          focus(primary(".workspace nav .nav-button.active") || nav[0]);
        }
        return;
      }
      if (nav.indexOf(node) >= 0) {
        if (direction === "left") {
          focus(header[0]);
        } else if (direction === "right") {
          focus(pageControls()[0] || node);
        } else if (direction === "up" || direction === "down") {
          moveIn(nav, direction === "up" ? -1 : 1);
        }
        return;
      }
      if ((direction === "left" || direction === "right") &&
          adjustSlider(direction === "left" ? -1 : 1)) return;
      if (settingsGridDirection(node, direction)) return;
      if (settingsRowDirection(node, direction)) return;
      if (direction === "left") {
        focus(primary(".workspace nav .nav-button.active") || nav[0]);
      } else if (direction === "up" || direction === "down") {
        moveIn(pageControls(), direction === "up" ? -1 : 1);
      }
    }
    function builderDirection(direction) {
      var groups = builderGroups();
      var index = groupOf(groups, current());
      if (index < 0) index = 2;
      if (index <= 1) {
        // Header actions and weapon controls are horizontal. Left/Right
        // should follow the row, not require the vertical list controls.
        if (direction === "left" || direction === "right") {
          moveIn(groups[index], direction === "left" ? -1 : 1);
        } else if (direction === "up" && index === 1) {
          focus(groups[0][0]);
        } else if (direction === "down") {
          focus(groups[index + 1][0]);
        }
        return;
      }
      if (direction === "left" && index === 3) {
        focus(primary("#categories .row"));
      } else if (direction === "right" && index === 2) {
        focus(primary("#options .row"));
      } else if (direction === "up" || direction === "down") {
        moveIn(groups[index], direction === "up" ? -1 : 1);
        // Browsing category rows changes the visible option pane, not
        // category visibility or any persistent preference.
        var row = current();
        if (index === 2 && row) row.click();
      }
    }
    function direction(name) {
      var dialog = modal();
      if (dialog) {
        // Settings Reset confirmation buttons are side-by-side. Builder
        // profile import choices, by contrast, are vertically stacked.
        var horizontal = mode === "settings";
        if (horizontal && (name === "left" || name === "right")) {
          moveIn(selectable("button", dialog), name === "left" ? -1 : 1);
        } else if (!horizontal && (name === "up" || name === "down")) {
          moveIn(selectable("button", dialog), name === "up" ? -1 : 1);
        }
        return;
      }
      var open = expandedDropdown();
      if (open) {
        if (name === "up" || name === "down")
          moveIn(selectable(".choice-option", open), name === "up" ? -1 : 1);
        return;
      }
      if (mode === "settings") settingsDirection(name);
      else builderDirection(name);
    }
    function shoulder(delta) {
      if (modal() || expandedDropdown() || sliderEditing) return;
      if (mode === "builder") {
        shiftGroup(delta);
      } else {
        var nav = pageNav();
        if (!nav.length) return;
        var selected = primary(".workspace nav .nav-button.active") || nav[0];
        var index = nav.indexOf(selected);
        var next = nav[(index + delta + nav.length) % nav.length];
        next.click();
        focus(next);
      }
    }
    function secondary() {
      if (mode !== "builder" || modal() || expandedDropdown()) return;
      var e = current();
      if (selectable("#categories .row").indexOf(e) >= 0) {
        // The Builder already interprets a click on the toggle icon as
        // the explicit category visibility command. Do not simulate it
        // while merely moving category focus.
        var icon = e.querySelector(".toggle");
        if (icon) icon.click();
      } else if (selectable("#options .row").indexOf(e) >= 0) {
        e.click();
      }
    }
    function button(event) {
      var detail = event && event.detail;
      if (!detail || (detail.state !== "pressed" && detail.state !== "repeat")) return;
      var code = String(detail.button || "");
      var repeat = detail.state === "repeat";
      if (options.isKeyCaptureActive && options.isKeyCaptureActive()) {
        // A controller press during keyboard hotkey capture must not
        // select another settings control and leave capture stranded.
        if (!repeat && code === cancelButton && options.cancelKeyCapture) {
          options.cancelKeyCapture();
        }
        return;
      }
      // A keyboard text field must not unexpectedly commit on blur because
      // the controller moved focus while someone was editing a name/hex code.
      if (editingText()) return;
      if (secondaryButton && code === secondaryButton) {
        if (repeat) return;
        secondary();
      } else if (code === confirmButton || code === cancelButton) {
        if (repeat) return;
        if (code === confirmButton) confirm();
        else cancel();
      } else if (code === "LB" || code === "RB") {
        if (repeat) return;
        shoulder(code === "LB" ? -1 : 1);
      } else if (["DUp", "DDown", "DLeft", "DRight"].indexOf(code) >= 0) {
        lastDpadAt = Date.now();
        stickNeedsNeutral = true;
        direction({ DUp:"up", DDown:"down", DLeft:"left", DRight:"right" }[code]);
      } else return;
      active = true;
      if (event.stopImmediatePropagation) event.stopImmediatePropagation();
    }
    function stick(value) {
      var sector = Number(value);
      if (!Number.isInteger(sector) || sector < -1 || sector >= 72) return;
      if ((options.isKeyCaptureActive && options.isKeyCaptureActive()) || editingText()) return;
      if (sector === -1) {
        lastStickDirection = "";
        stickNeedsNeutral = false;
        return;
      }
      if (stickNeedsNeutral || Date.now() - lastDpadAt < 260) return;
      var name = ["up", "right", "down", "left"][Math.floor((sector + 9) / 18) % 4];
      if (name === lastStickDirection && Date.now() - lastStickAt < 185) return;
      lastStickDirection = name;
      lastStickAt = Date.now();
      direction(name);
      active = true;
    }
    global.k2040ControllerConfirmButton = function(value) {
      confirmButton = String(value || "A");
    };
    global.k2040ControllerCancelButton = function(value) {
      cancelButton = String(value || "B");
    };
    global.k2040ControllerSecondaryButton = function(value) {
      secondaryButton = String(value || "");
    };
    global.k2040ControllerStickSector = stick;
    global.addEventListener("prisma-controller-action", button);
    var style = document.createElement("style");
    style.textContent = [
      "body .k2040-controller-focus:focus { outline: 3px solid #86c9fc !important;",
      "  outline-offset: 3px; box-shadow: 0 0 0 2px #142b40 !important; }",
      "body .controller-editing { outline-color: #ffd078 !important; }"
    ].join("\\n");
    document.head.appendChild(style);
    document.addEventListener("focusin", function(event) {
      if (event.target && event.target.matches &&
          event.target.matches("button, input[type=range], input[type=checkbox]"))
        event.target.classList.add("k2040-controller-focus");
    });
    return { direction:direction, confirm:confirm, cancel:cancel,
      button:button, stick:stick, focus:focus, shoulder:shoulder,
      isActive:function() { return active; } };
  }
  global.K2040ControllerPages = { create:create };
})(window);
