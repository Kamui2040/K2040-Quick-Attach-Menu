// K2040's Quick Attach Menu shared renderer.
(function(global) {
  "use strict";

  var SVG_NS = "http://www.w3.org/2000/svg";

  function clamp(value, minimum, maximum, fallback) {
    var number = Number(value);
    if (!isFinite(number)) number = fallback;
    return Math.min(maximum, Math.max(minimum, number));
  }

  function hex(value, fallback) {
    var text = String(value || "").trim();
    if (/^#[0-9a-fA-F]{6}$/.test(text)) return text.toLowerCase();
    return fallback;
  }

  function rgb(value) {
    var colour = hex(value, "#000000");
    return {
      r: parseInt(colour.slice(1, 3), 16),
      g: parseInt(colour.slice(3, 5), 16),
      b: parseInt(colour.slice(5, 7), 16)
    };
  }

  function rgba(value, alpha) {
    var colour = rgb(value);
    return "rgba(" + colour.r + "," + colour.g + "," + colour.b + "," + alpha + ")";
  }

  function contrastColour(value) {
    var colour = rgb(value);
    var luminance = colour.r * .299 + colour.g * .587 + colour.b * .114;
    return luminance > 145 ? "#071014" : "#eaf8ee";
  }

  function polar(cx, cy, radius, angle) {
    var radians = angle * Math.PI / 180;
    return { x: cx + radius * Math.cos(radians), y: cy + radius * Math.sin(radians) };
  }

  function annularPath(cx, cy, innerRadius, outerRadius, startAngle, endAngle) {
    var outerStart = polar(cx, cy, outerRadius, startAngle);
    var outerEnd = polar(cx, cy, outerRadius, endAngle);
    var innerEnd = polar(cx, cy, innerRadius, endAngle);
    var innerStart = polar(cx, cy, innerRadius, startAngle);
    var largeArc = endAngle - startAngle > 180 ? 1 : 0;
    return [
      "M", outerStart.x.toFixed(2), outerStart.y.toFixed(2),
      "A", outerRadius, outerRadius, 0, largeArc, 1, outerEnd.x.toFixed(2), outerEnd.y.toFixed(2),
      "L", innerEnd.x.toFixed(2), innerEnd.y.toFixed(2),
      "A", innerRadius, innerRadius, 0, largeArc, 0, innerStart.x.toFixed(2), innerStart.y.toFixed(2),
      "Z"
    ].join(" ");
  }

  function wrapWheelLabel(value, limit, maximumLines) {
    var text = String(value || "").trim();
    if (text.length <= limit) return [text];
    var words = text.split(/\s+/);
    var lines = [];
    var line = "";
    for (var i = 0; i < words.length; i += 1) {
      var candidate = line ? line + " " + words[i] : words[i];
      if (candidate.length > limit && line && lines.length < maximumLines - 1) {
        lines.push(line);
        line = words[i];
      } else {
        line = candidate;
      }
    }
    if (line) lines.push(line);
    return lines;
  }

  function Renderer(root, options) {
    this.root = root;
    this.options = options || {};
    this.labelPortal = null;
    this.statusPortal = null;
    this.requestedScale = 1;
    this.effectiveScale = 1;
    this.previewZoom = 1;
    this.payload = null;
    this.activeCategoryIndex = null;
    this.activePane = "categories";
    this.lastFocusedOptionIndex = 0;
    this.statusMessage = "";
    this.statusClass = "";
    this.viewportFrame = 0;
    this.nativePreviewDisabledSent = false;
  }

  Renderer.prototype.settings = function() {
    return this.payload && this.payload.settings ? this.payload.settings : {};
  };

  Renderer.prototype.bracketedTextVisibility = function() {
    var parser = this.payload && this.payload.parser ? this.payload.parser : {};
    var settings = this.settings();
    if (parser.bracketedTextOverride === "hide") return { prefix: true, infix: true, suffix: true };
    if (parser.bracketedTextOverride === "show") return { prefix: false, infix: false, suffix: false };
    var fallback = !!settings.hideBracketedText;
    return {
      prefix: settings.hideBracketedPrefixes === undefined ? fallback : !!settings.hideBracketedPrefixes,
      infix: settings.hideBracketedInfixes === undefined ? fallback : !!settings.hideBracketedInfixes,
      suffix: settings.hideBracketedSuffixes === undefined ? fallback : !!settings.hideBracketedSuffixes
    };
  };

  Renderer.prototype.displayName = function(value) {
    var original = String(value || "");
    var visibility = this.bracketedTextVisibility();
    var changed = false;
    var result = original.replace(/\[[^\]]*\]/g, function(match, offset) {
      var beforeHasText = original.slice(0, offset).replace(/\[[^\]]*\]/g, "").trim().length > 0;
      var afterHasText = original.slice(offset + match.length).replace(/\[[^\]]*\]/g, "").trim().length > 0;
      var hidden = !beforeHasText ? visibility.prefix : (!afterHasText ? visibility.suffix : visibility.infix);
      if (hidden) changed = true;
      return hidden ? " " : match;
    });
    return changed ? result.replace(/\s+/g, " ").trim() : original;
  };

  Renderer.prototype.visibleCategories = function() {
    var parser = this.payload && this.payload.parser ? this.payload.parser : {};
    return (parser.categories || []).filter(function(category) {
      if (category.userHidden) return false;
      return (category.options || []).some(function(option) {
        if (option.userHidden || !option.isVisible) return false;
        return option.isInstalled || (option.isSelectable && option.isStructurallyValid);
      });
    });
  };

  Renderer.prototype.visibleOptions = function(category) {
    var settings = this.settings();
    return ((category && category.options) || []).filter(function(option) {
      if (option.userHidden || !option.isVisible) return false;
      if (!option.isInstalled && option.status === "inventory-unavailable") return false;
      if (settings.hideInvalidOptions && !option.isInstalled && (!option.isSelectable || !option.isStructurallyValid)) return false;
      return true;
    });
  };

  Renderer.prototype.optionReason = function(option) {
    if (!option) return "Choose an attachment.";
    if (option.isInstalled) return "Currently installed.";
    if (option.isSelectable && option.isStructurallyValid) {
      return "Ready to equip.";
    }
    switch (option.status) {
    case "provider-not-installed": return "Requires another attachment first.";
    case "target-mismatch": return "Not compatible with this weapon.";
    case "inventory-unavailable": return "Required loose mod is not available.";
    case "missing-consumed-ap":
    case "missing-live-attach-point": return "Attachment point is not available.";
    default: return "Unavailable.";
    }
  };

  Renderer.prototype.findCategory = function(categoryIndex) {
    var categories = this.visibleCategories();
    for (var i = 0; i < categories.length; i += 1) {
      if (categories[i].categoryIndex === categoryIndex) return categories[i];
    }
    return null;
  };

  Renderer.prototype.focusedOption = function(category) {
    var options = this.visibleOptions(category);
    for (var i = 0; i < options.length; i += 1) {
      if (options[i].optionIndex === this.lastFocusedOptionIndex) return options[i];
    }
    return options.length ? options[0] : null;
  };

  Renderer.prototype.setPayload = function(payload) {
    this.payload = payload || {};
    var categories = this.visibleCategories();
    if (!this.findCategory(this.activeCategoryIndex)) {
      this.activeCategoryIndex = categories.length ? categories[0].categoryIndex : null;
    }
    var category = this.findCategory(this.activeCategoryIndex);
    var option = this.focusedOption(category);
    this.lastFocusedOptionIndex = option ? option.optionIndex : 0;
    this.render();
  };

  Renderer.prototype.setStatus = function(message, className) {
    this.statusMessage = String(message || "");
    this.statusClass = className || "";
    var status = this.statusPortal ? this.statusPortal.querySelector(".qm-status") : this.root.querySelector(".qm-status");
    if (!status) return;
    status.className = "qm-status" + (this.statusClass ? " " + this.statusClass : "");
    status.textContent = this.statusMessage;
    this.updateScreenStatus();
  };

  Renderer.prototype.ensureLabelPortal = function() {
    var parent = this.root && this.root.parentNode;
    if (!parent) return null;
    if (!this.labelPortal) {
      this.labelPortal = document.createElement("div");
      this.labelPortal.setAttribute("aria-hidden", "true");
    }
    if (this.labelPortal.parentNode !== parent) parent.appendChild(this.labelPortal);
    return this.labelPortal;
  };

  Renderer.prototype.ensureStatusPortal = function() {
    var parent = this.root && this.root.parentNode;
    if (!parent) return null;
    if (!this.statusPortal) {
      this.statusPortal = document.createElement("div");
      this.statusPortal.setAttribute("aria-live", "polite");
      this.statusPortal.setAttribute("aria-atomic", "true");
    }
    if (this.statusPortal.parentNode !== parent) parent.appendChild(this.statusPortal);
    return this.statusPortal;
  };

  Renderer.prototype.adaptiveScaleFloor = function(presentation) {
    var count = this.visibleCategories().length;
    if (presentation === "radial") {
      return clamp(.54 + Math.max(0, count - 10) * .048, .54, .78, .54);
    }
    if (presentation === "hybrid") {
      return clamp(.56 + Math.max(0, count - 6) * .03, .56, .82, .56);
    }
    return .5;
  };

  Renderer.prototype.updateScreenLabels = function() {
    var portal = this.labelPortal;
    var wheel = this.root.querySelector(".qm-wheel");
    if (!portal || !wheel || portal.hidden) return;
    var parent = portal.parentNode;
    var parentRect = parent.getBoundingClientRect();
    var wheelRect = wheel.getBoundingClientRect();
    var parentWidth = Math.max(1, parent.clientWidth || parentRect.width);
    var parentHeight = Math.max(1, parent.clientHeight || parentRect.height);
    var parentScaleX = parentRect.width / parentWidth || 1;
    var parentScaleY = parentRect.height / parentHeight || 1;
    var wheelLeft = (wheelRect.left - parentRect.left) / parentScaleX;
    var wheelTop = (wheelRect.top - parentRect.top) / parentScaleY;
    var wheelWidth = wheelRect.width / parentScaleX;
    var wheelHeight = wheelRect.height / parentScaleY;
    var labels = portal.querySelectorAll(".qm-wheel-html-label");
    var previewFontScale = this.options.preview ? this.previewZoom : 1;
    for (var i = 0; i < labels.length; i += 1) {
      var geometry = labels[i]._qmWheelGeometry;
      if (!geometry) continue;
      labels[i].style.left = (wheelLeft + geometry.x / geometry.viewWidth * wheelWidth).toFixed(2) + "px";
      labels[i].style.top = (wheelTop + geometry.y / geometry.viewHeight * wheelHeight).toFixed(2) + "px";
      labels[i].style.width = Math.max(32, geometry.width / geometry.viewWidth * wheelWidth).toFixed(2) + "px";
      labels[i].style.minHeight = Math.max(24, geometry.height / geometry.viewHeight * wheelHeight).toFixed(2) + "px";
      labels[i].style.fontSize = (geometry.fontSize * previewFontScale).toFixed(2) + "px";
    }
  };

  Renderer.prototype.updateScreenStatus = function() {
    var portal = this.statusPortal;
    var status = portal && portal.querySelector(".qm-status");
    if (!portal || !status || portal.hidden) return;
    var parent = portal.parentNode;
    var parentRect = parent.getBoundingClientRect();
    var rootRect = this.root.getBoundingClientRect();
    var parentWidth = Math.max(1, parent.clientWidth || parentRect.width);
    var parentHeight = Math.max(1, parent.clientHeight || parentRect.height);
    var parentScaleX = parentRect.width / parentWidth || 1;
    var parentScaleY = parentRect.height / parentHeight || 1;
    var rootLeft = (rootRect.left - parentRect.left) / parentScaleX;
    var rootTop = (rootRect.top - parentRect.top) / parentScaleY;
    var rootWidth = rootRect.width / parentScaleX;
    var rootHeight = rootRect.height / parentScaleY;
    status.style.left = (rootLeft + rootWidth / 2).toFixed(2) + "px";
    if (this.root.classList.contains("status-above")) {
      status.style.top = "auto";
      status.style.bottom = Math.max(0, parentHeight - rootTop + 8).toFixed(2) + "px";
    } else {
      status.style.top = (rootTop + rootHeight + 24).toFixed(2) + "px";
      status.style.bottom = "auto";
    }
  };

  Renderer.prototype.applyTheme = function() {
    var settings = this.settings();
    var theme = ["default", "pipboy", "high-contrast", "custom"].indexOf(settings.theme) >= 0 ? settings.theme : "default";
    var opacity = clamp(settings.backgroundOpacity, .25, 1, .92);
    var accent = "#66c4ff";
    var text = "#f4faff";
    var panel = "#0b1218";
    var installed = "#74e398";
    if (theme === "high-contrast") {
      accent = "#00e5ff";
      text = "#ffffff";
      panel = "#000000";
      installed = "#63ff9b";
    } else if (theme === "custom") {
      accent = hex(settings.customAccent, accent);
      text = hex(settings.customText, text);
      panel = hex(settings.customPanel, panel);
      installed = hex(settings.customInstalled, installed);
    } else if (theme === "pipboy") {
      accent = hex(settings.hudColor, "#20ff48");
      text = accent;
      panel = hex(settings.hudBackgroundColor, "#001407");
      installed = accent;
    }

    var targets = [this.root];
    if (this.labelPortal) targets.push(this.labelPortal);
    if (this.statusPortal) targets.push(this.statusPortal);
    for (var i = 0; i < targets.length; i += 1) {
      var target = targets[i];
      target.style.setProperty("--qm-accent", accent);
      target.style.setProperty("--qm-text", text);
      target.style.setProperty("--qm-installed", installed);
      target.style.setProperty("--qm-panel", rgba(panel, opacity));
      target.style.setProperty("--qm-panel-strong", rgba(panel, Math.min(1, opacity + .05)));
      target.style.setProperty("--qm-row", theme === "pipboy" ? "transparent" : rgba(panel, opacity));
      target.style.setProperty("--qm-active", theme === "pipboy" ? accent : rgba(accent, Math.min(.48, .19 + opacity * .22)));
      target.style.setProperty("--qm-line", theme === "pipboy" ? rgba(accent, .84) : rgba(text, .20));
      target.style.setProperty("--qm-line-soft", theme === "pipboy" ? rgba(accent, .28) : rgba(text, .10));
      target.style.setProperty("--qm-muted", theme === "pipboy" ? rgba(accent, .72) : rgba(text, .68));
      target.style.setProperty("--qm-accent-faint", rgba(accent, .12));
      target.style.setProperty("--qm-hud-color", accent);
      target.style.setProperty("--qm-hud-background", panel);
      target.style.setProperty("--qm-hud-muted", rgba(accent, .72));
      target.style.setProperty("--qm-hud-line", rgba(accent, .84));
      target.style.setProperty("--qm-hud-line-soft", rgba(accent, .28));
      target.style.setProperty("--qm-hud-glow", rgba(accent, .42));
      target.style.setProperty("--qm-hud-shadow", rgba(accent, .16));
      target.style.setProperty("--qm-hud-dark", contrastColour(accent));
    }
  };

  Renderer.prototype.setPlacement = function(scale, positionX, positionY) {
    scale = clamp(scale, .5, 1.5, 1);
    positionX = clamp(positionX, .05, .95, .5);
    positionY = clamp(positionY, .05, .95, .5);
    var settings = this.settings();
    var presentation = ["cascade", "radial", "hybrid", "horizontal"].indexOf(settings.presentation) >= 0
      ? settings.presentation : "cascade";
    this.requestedScale = scale;
    this.effectiveScale = Math.max(scale, this.adaptiveScaleFloor(presentation));
    this.previewZoom = this.options.preview ? clamp(this.options.previewZoom, 1, 2, 1) : 1;
    this.root.style.left = String(positionX * 100) + "%";
    this.root.style.top = String(positionY * 100) + "%";
    this.root.style.transform = "translate(-50%, -50%) scale(" + (this.effectiveScale * this.previewZoom) + ")";
    this.root.setAttribute("data-requested-scale", scale.toFixed(3));
    this.root.setAttribute("data-effective-scale", this.effectiveScale.toFixed(3));
    this.root.classList.toggle("status-above", positionY > .72);
    if (this.statusPortal) this.statusPortal.classList.toggle("status-above", positionY > .72);
    this.updateScreenLabels();
    this.updateScreenStatus();
  };

  Renderer.prototype.applyRootState = function() {
    var settings = this.settings();
    var presentation = ["cascade", "radial", "hybrid", "horizontal"].indexOf(settings.presentation) >= 0
      ? settings.presentation : "cascade";
    var scale = clamp(settings.scale, .5, 1.5, 1);
    var positionX = clamp(settings.positionX, .05, .95, .5);
    var positionY = clamp(settings.positionY, .05, .95, .5);
    this.root.className = "k2040-quick-renderer presentation-" + presentation + " theme-" + (settings.theme || "default") +
      (this.options.preview ? " preview-mode" : "") +
      " hints-" + (settings.controlHints || "contextual") +
      (this.options.keyboardActive ? " keyboard-active" : "");
    if (this.labelPortal) {
      this.labelPortal.className = "qm-wheel-screen-labels presentation-" + presentation + " theme-" + (settings.theme || "default") +
        (this.options.preview ? " preview-mode" : "");
      this.labelPortal.hidden = presentation !== "radial" && presentation !== "hybrid";
    }
    if (this.statusPortal) {
      this.statusPortal.className = "qm-status-portal presentation-" + presentation + " theme-" + (settings.theme || "default") +
        (this.options.preview ? " preview-mode" : "");
      this.statusPortal.hidden = false;
    }
    this.setPlacement(scale, positionX, positionY);
    this.applyTheme();
  };

  Renderer.prototype.makeIcon = function(iconClass) {
    return global.K2040IconResources ?
      global.K2040IconResources.createIcon(this.payload && this.payload.iconAssets,
        iconClass, this.payload && this.payload.iconVectors) : null;
  };

  Renderer.prototype.makeCategoryButton = function(category) {
    var self = this;
    var label = this.displayName(category.label) || "Unnamed category";
    var button = document.createElement("button");
    button.type = "button";
    button.className = "qm-category-button" +
      (category.categoryIndex === this.activeCategoryIndex ? " active" : "") +
      (category.categoryIndex === this.activeCategoryIndex && this.activePane === "categories" ? " pane-focus" : "");
    button.dataset.categoryIndex = String(category.categoryIndex);
    var icon = this.makeIcon(category.iconClass);
    if (icon) {
      button.className += " qm-has-icon";
      button.appendChild(icon);
    }
    var name = document.createElement("span");
    name.className = "qm-category-name";
    name.textContent = label;
    name.title = label;
    var arrow = document.createElement("span");
    arrow.className = "qm-category-arrow";
    arrow.textContent = "›";
    button.appendChild(name);
    button.appendChild(arrow);
    button.addEventListener("click", function() {
      self.activePane = "categories";
      self.selectCategory(category.categoryIndex);
    });
    return button;
  };

  Renderer.prototype.makeOptionButton = function(category, option) {
    var self = this;
    var label = this.displayName(option.label) || "Unnamed attachment";
    var ready = option.isSelectable && option.isStructurallyValid && !option.isInstalled;
    var focused = this.activePane === "options" && option.optionIndex === this.lastFocusedOptionIndex;
    var button = document.createElement("button");
    button.type = "button";
    button.className = "qm-option-button" + (option.isInstalled ? " installed" : "") +
      (!ready && !option.isInstalled ? " blocked" : "") + (focused ? " focused-option" : "");
    button.dataset.optionIndex = String(option.optionIndex);
    var icon = this.makeIcon(option.iconClass);
    if (icon) {
      button.className += " qm-has-icon";
      button.appendChild(icon);
    }
    var name = document.createElement("span");
    name.className = "qm-option-name";
    name.textContent = label;
    name.title = label;
    button.appendChild(name);
    button.addEventListener("mouseenter", function() {
      self.lastFocusedOptionIndex = option.optionIndex;
      if (self.options.onHover) self.options.onHover(category, option);
    });
    button.addEventListener("click", function() {
      self.activePane = "options";
      self.lastFocusedOptionIndex = option.optionIndex;
      self.activateOption(category, option);
    });
    return button;
  };

  Renderer.prototype.makeCategoryList = function(categories) {
    var list = document.createElement("div");
    list.className = "qm-category-list";
    for (var i = 0; i < categories.length; i += 1) list.appendChild(this.makeCategoryButton(categories[i]));
    return list;
  };

  Renderer.prototype.makeOptionList = function(category, withHeading) {
    var wrapper = document.createElement("section");
    wrapper.className = "qm-pane";
    if (withHeading) {
      var heading = document.createElement("div");
      heading.className = "qm-option-heading";
      heading.textContent = category ? this.displayName(category.label) : "Attachments";
      wrapper.appendChild(heading);
    }
    var list = document.createElement("div");
    list.className = "qm-option-list";
    var options = this.visibleOptions(category);
    if (!options.length) {
      var empty = document.createElement("div");
      empty.className = "qm-empty";
      empty.textContent = category ? "No available attachments in this category." : "No attachment category is available.";
      list.appendChild(empty);
    } else {
      for (var i = 0; i < options.length; i += 1) list.appendChild(this.makeOptionButton(category, options[i]));
    }
    wrapper.appendChild(list);
    return wrapper;
  };

  Renderer.prototype.appendStatusAndMeta = function(container) {
    var status = document.createElement("div");
    status.className = "qm-status" + (this.statusClass ? " " + this.statusClass : "");
    status.textContent = this.statusMessage;
    status.setAttribute("role", "status");
    (this.statusPortal || container).appendChild(status);
    var hints = document.createElement("div");
    hints.className = "qm-hints";
    hints.textContent = "W/S or ↑/↓ to move · A/D or ←/→ to switch · Enter to apply · opener/Escape to close";
    container.appendChild(hints);
    var version = document.createElement("div");
    version.className = "qm-version";
    version.textContent = "v" + (this.payload && this.payload.pluginVersion ? this.payload.pluginVersion : "unknown");
    container.appendChild(version);
  };

  Renderer.prototype.renderCascade = function(categories, category) {
    var layout = document.createElement("div");
    layout.className = "qm-layout qm-cascade-layout";
    var columns = document.createElement("div");
    columns.className = "qm-cascade-columns";
    var categoryPane = document.createElement("nav");
    categoryPane.className = "qm-pane";
    categoryPane.appendChild(this.makeCategoryList(categories));
    columns.appendChild(categoryPane);
    columns.appendChild(this.makeOptionList(category, false));
    layout.appendChild(columns);
    return layout;
  };

  Renderer.prototype.renderHorizontal = function(categories, category) {
    var layout = document.createElement("div");
    layout.className = "qm-layout qm-horizontal-layout";
    var categoryPane = document.createElement("nav");
    categoryPane.className = "qm-pane qm-horizontal-categories";
    categoryPane.appendChild(this.makeCategoryList(categories));
    layout.appendChild(categoryPane);
    layout.appendChild(this.makeOptionList(category, false));
    return layout;
  };

  Renderer.prototype.appendWheelLabel = function(labelLayer, geometry, label, optionLabel, stateClass) {
    var innerRadius = geometry.innerRadius;
    var outerRadius = geometry.outerRadius;
    var thickness = outerRadius - innerRadius;
    var middleAngle = (geometry.start + geometry.end) / 2;
    var middleRadius = (innerRadius + outerRadius) / 2;
    var menuScale = this.effectiveScale || clamp(this.settings().scale, .5, 1.5, 1);
    var baseSize = optionLabel ? 13 : 13.5;
    var minimumSize = optionLabel ? 12 : 12.5;
    var spanRadians = Math.abs(geometry.end - geometry.start) * Math.PI / 180;
    var availableAtMiddle = Math.max(24, spanRadians * middleRadius - 12);
    var characterLimit = Math.max(6, Math.floor(availableAtMiddle * menuScale / (baseSize * .50)));
    var lines = wrapWheelLabel(label, characterLimit, 3);

    var labelCentre = middleRadius;
    var labelPoint = polar(geometry.cx, geometry.cy, labelCentre, middleAngle);
    var safeArc = Math.max(32, spanRadians * labelCentre - 12);
    var longestLine = 1;
    for (var i = 0; i < lines.length; i += 1) {
      longestLine = Math.max(longestLine, lines[i].length);
    }
    var scaledBaseSize = baseSize * Math.max(1, menuScale);
    var estimatedWidth = Math.max(1, longestLine * scaledBaseSize * .50);
    var fontSize = Math.max(minimumSize, Math.min(scaledBaseSize, scaledBaseSize * safeArc * menuScale / estimatedWidth));
    var rotation = ((middleAngle + 90 + 180) % 360 + 360) % 360 - 180;
    if (rotation > 90) rotation -= 180;
    else if (rotation < -90) rotation += 180;

    var labelBox = document.createElement("div");
    labelBox.className = "qm-wheel-html-label" + (optionLabel ? " option-label" : "") + (stateClass ? " " + stateClass : "");
    labelBox.setAttribute("data-full-label", label);
    labelBox.style.transform = "translate(-50%, -50%) rotate(" + rotation.toFixed(2) + "deg)";
    labelBox._qmWheelGeometry = {
      x: labelPoint.x,
      y: labelPoint.y,
      width: safeArc,
      height: Math.max(24, thickness * .54),
      fontSize: fontSize,
      viewWidth: geometry.viewWidth,
      viewHeight: geometry.viewHeight
    };
    for (var j = 0; j < lines.length; j += 1) {
      var line = document.createElement("span");
      line.textContent = lines[j];
      labelBox.appendChild(line);
    }
    labelLayer.appendChild(labelBox);
    return labelBox;
  };

  Renderer.prototype.makeCategorySegment = function(svg, labelLayer, category, start, end, cx, cy, innerRadius, outerRadius) {
    var self = this;
    var label = this.displayName(category.label) || "Unnamed category";
    var active = category.categoryIndex === this.activeCategoryIndex;
    var stateClass = (active ? "active" : "") + (active && this.activePane === "categories" ? " pane-focus" : "");
    var group = document.createElementNS(SVG_NS, "g");
    group.setAttribute("class", "qm-wheel-segment qm-category-segment" + (stateClass ? " " + stateClass : ""));
    group.setAttribute("data-category-index", String(category.categoryIndex));
    var path = document.createElementNS(SVG_NS, "path");
    path.setAttribute("class", "qm-wheel-path");
    path.setAttribute("d", annularPath(cx, cy, innerRadius, outerRadius, start, end));
    group.appendChild(path);
    var labelBox = this.appendWheelLabel(labelLayer,
      { cx:cx, cy:cy, innerRadius:innerRadius, outerRadius:outerRadius, start:start, end:end, viewWidth:Number(svg.getAttribute("data-view-width")), viewHeight:Number(svg.getAttribute("data-view-height")) },
      label, false, stateClass);
    group.addEventListener("mouseenter", function() { labelBox.classList.add("hovered"); });
    group.addEventListener("mouseleave", function() { labelBox.classList.remove("hovered"); });
    group.addEventListener("click", function() {
      self.activePane = "categories";
      self.selectCategory(category.categoryIndex);
    });
    svg.appendChild(group);
  };

  Renderer.prototype.makeOptionSegment = function(svg, labelLayer, category, option, start, end, cx, cy, innerRadius, outerRadius) {
    var self = this;
    var label = this.displayName(option.label) || "Unnamed attachment";
    var ready = option.isSelectable && option.isStructurallyValid && !option.isInstalled;
    var focused = this.activePane === "options" && option.optionIndex === this.lastFocusedOptionIndex;
    var stateClass = (option.isInstalled ? "installed" : "") +
      (!ready && !option.isInstalled ? " blocked" : "") + (focused ? " pane-focus" : "");
    var group = document.createElementNS(SVG_NS, "g");
    group.setAttribute("class", "qm-wheel-segment qm-wheel-option" + stateClass);
    group.setAttribute("data-option-index", String(option.optionIndex));
    var path = document.createElementNS(SVG_NS, "path");
    path.setAttribute("class", "qm-wheel-path");
    path.setAttribute("d", annularPath(cx, cy, innerRadius, outerRadius, start, end));
    group.appendChild(path);
    var labelBox = this.appendWheelLabel(labelLayer,
      { cx:cx, cy:cy, innerRadius:innerRadius, outerRadius:outerRadius, start:start, end:end, viewWidth:Number(svg.getAttribute("data-view-width")), viewHeight:Number(svg.getAttribute("data-view-height")) },
      label, true, stateClass);
    group.addEventListener("mouseenter", function() {
      labelBox.classList.add("hovered");
      self.lastFocusedOptionIndex = option.optionIndex;
    });
    group.addEventListener("mouseleave", function() { labelBox.classList.remove("hovered"); });
    group.addEventListener("click", function() {
      self.activePane = "options";
      self.lastFocusedOptionIndex = option.optionIndex;
      self.activateOption(category, option);
    });
    svg.appendChild(group);
  };

  Renderer.prototype.renderRadial = function(categories, category) {
    var layout = document.createElement("div");
    layout.className = "qm-layout qm-radial-layout";
    var svg = document.createElementNS(SVG_NS, "svg");
    svg.setAttribute("class", "qm-wheel");
    svg.setAttribute("viewBox", "0 0 600 600");
    svg.setAttribute("data-view-width", "600");
    svg.setAttribute("data-view-height", "600");
    var labelLayer = this.labelPortal;
    var center = document.createElementNS(SVG_NS, "circle");
    center.setAttribute("class", "qm-wheel-center");
    center.setAttribute("cx", "300");
    center.setAttribute("cy", "300");
    center.setAttribute("r", "98");
    svg.appendChild(center);
    var categorySweep = 360 / Math.max(1, categories.length);
    for (var i = 0; i < categories.length; i += 1) {
      var categoryStart = -90 + i * categorySweep + 1.5;
      var categoryEnd = -90 + (i + 1) * categorySweep - 1.5;
      this.makeCategorySegment(svg, labelLayer, categories[i], categoryStart, categoryEnd, 300, 300, 108, 190);
    }
    var options = this.visibleOptions(category);
    if (category && options.length) {
      var categoryPosition = categories.indexOf(category);
      var categoryMiddle = -90 + (categoryPosition + .5) * categorySweep;
      var optionSweep = Math.min(250, Math.max(72, options.length * 38));
      var optionSize = optionSweep / options.length;
      var optionStart = categoryMiddle - optionSweep / 2;
      for (var j = 0; j < options.length; j += 1) {
        this.makeOptionSegment(svg, labelLayer, category, options[j], optionStart + j * optionSize + 1.2,
          optionStart + (j + 1) * optionSize - 1.2, 300, 300, 202, 286);
      }
    }
    layout.appendChild(svg);
    return layout;
  };

  Renderer.prototype.renderHybrid = function(categories, category) {
    var layout = document.createElement("div");
    layout.className = "qm-layout qm-hybrid-layout";
    var wheel = document.createElement("div");
    wheel.className = "qm-hybrid-wheel";
    var svg = document.createElementNS(SVG_NS, "svg");
    svg.setAttribute("class", "qm-wheel");
    svg.setAttribute("viewBox", "0 0 430 430");
    svg.setAttribute("data-view-width", "430");
    svg.setAttribute("data-view-height", "430");
    var labelLayer = this.labelPortal;
    var center = document.createElementNS(SVG_NS, "circle");
    center.setAttribute("class", "qm-wheel-center");
    center.setAttribute("cx", "225");
    center.setAttribute("cy", "215");
    center.setAttribute("r", "98");
    svg.appendChild(center);
    var totalSweep = 166;
    var segmentSweep = totalSweep / Math.max(1, categories.length);
    for (var i = 0; i < categories.length; i += 1) {
      var start = 97 + i * segmentSweep + 1.4;
      var end = 97 + (i + 1) * segmentSweep - 1.4;
      this.makeCategorySegment(svg, labelLayer, categories[i], start, end, 225, 215, 108, 190);
    }
    wheel.appendChild(svg);
    layout.appendChild(wheel);
    var optionsPane = document.createElement("div");
    optionsPane.className = "qm-hybrid-options";
    optionsPane.appendChild(this.makeOptionList(category, true));
    layout.appendChild(optionsPane);
    return layout;
  };

  Renderer.prototype.render = function() {
    if (!this.root) return;
    this.ensureLabelPortal();
    this.ensureStatusPortal();
    if (this.labelPortal) this.labelPortal.textContent = "";
    if (this.statusPortal) this.statusPortal.textContent = "";
    this.applyRootState();
    this.root.textContent = "";
    var categories = this.visibleCategories();
    var category = this.findCategory(this.activeCategoryIndex);
    var settings = this.settings();
    var presentation = settings.presentation || "cascade";
    var layout;
    if (presentation === "radial") layout = this.renderRadial(categories, category);
    else if (presentation === "hybrid") layout = this.renderHybrid(categories, category);
    else if (presentation === "horizontal") layout = this.renderHorizontal(categories, category);
    else layout = this.renderCascade(categories, category);
    this.root.appendChild(layout);
    this.appendStatusAndMeta(this.root);
    this.updateScreenLabels();
    this.updateScreenStatus();
  };

  Renderer.prototype.selectCategory = function(categoryIndex) {
    var category = this.findCategory(categoryIndex);
    if (!category) return;
    this.activeCategoryIndex = category.categoryIndex;
    var option = this.focusedOption(category);
    this.lastFocusedOptionIndex = option ? option.optionIndex : 0;
    this.statusMessage = "";
    this.statusClass = "";
    this.render();
    if (this.options.onCategoryChanged) this.options.onCategoryChanged(category);
  };

  Renderer.prototype.moveCategory = function(delta) {
    var categories = this.visibleCategories();
    if (!categories.length) return;
    var index = -1;
    for (var i = 0; i < categories.length; i += 1) {
      if (categories[i].categoryIndex === this.activeCategoryIndex) index = i;
    }
    index = index < 0 ? 0 : (index + delta + categories.length) % categories.length;
    this.selectCategory(categories[index].categoryIndex);
  };

  Renderer.prototype.moveOption = function(delta) {
    var category = this.findCategory(this.activeCategoryIndex);
    var options = this.visibleOptions(category);
    if (!options.length) return;
    var index = -1;
    for (var i = 0; i < options.length; i += 1) {
      if (options[i].optionIndex === this.lastFocusedOptionIndex) index = i;
    }
    index = index < 0 ? (delta > 0 ? 0 : options.length - 1) : (index + delta + options.length) % options.length;
    this.lastFocusedOptionIndex = options[index].optionIndex;
    this.render();
  };

  Renderer.prototype.switchPane = function(direction) {
    if (direction > 0 && this.activePane === "categories") this.activePane = "options";
    else if (direction < 0 && this.activePane === "options") this.activePane = "categories";
    this.render();
  };

  Renderer.prototype.activateOption = function(category, option) {
    category = category || this.findCategory(this.activeCategoryIndex);
    option = option || this.focusedOption(category);
    if (!category || !option) return;
    var ready = option.isSelectable && option.isStructurallyValid && !option.isInstalled;
    if (ready && this.options.onOptionActivated) this.options.onOptionActivated(category, option);
    else this.setStatus(this.optionReason(option), option.isInstalled ? "warning" : "error");
  };

  Renderer.prototype.activateFocusedOption = function() {
    if (this.activePane !== "options") {
      this.switchPane(1);
      return;
    }
    this.activateOption();
  };

  Renderer.prototype.setKeyboardActive = function(active) {
    this.options.keyboardActive = !!active;
    this.render();
  };

  global.K2040QuickMenuRenderer = {
    create: function(root, options) { return new Renderer(root, options); }
  };
})(window);
