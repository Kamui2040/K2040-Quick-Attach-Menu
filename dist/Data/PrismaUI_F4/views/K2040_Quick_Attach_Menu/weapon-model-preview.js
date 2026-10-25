// Optional PrismaUI 2.2+ ModelPreview v4 adapter. Never changes game objects.
(function(global) {
  "use strict";

  function clamp(value, min, max) { return Math.max(min, Math.min(max, value)); }

  function WeaponModelPreview(host) {
    this.host = host;
    this.active = false;
    this.failed = false;
    this.error = "";
    this.zoom = 1;
    this.yaw = 0;
    this.pitch = 0;
    this.lastRequest = "";
    this.boundBridge = null;
    this.region = null;
    this.nifPath = "";
    this.dragStart = null;
    this.relativePathRetry = false;
    this.nativeRequestedPath = "";
  }

  WeaponModelPreview.prototype.bridge = function() {
    var value = this.host.__prismaUI_modelPreview;
    return value && Number(value.version) >= 4 &&
      typeof value.show === "function" && typeof value.hide === "function" &&
      typeof value.onStatus === "function" ? value : null;
  };

  WeaponModelPreview.prototype.supported = function() { return !!this.bridge(); };

  WeaponModelPreview.prototype.bind = function() {
    var api = this.bridge();
    if (!api || api === this.boundBridge) return api;
    var self = this;
    api.onStatus(function(status) {
      if (!status || status.id !== "k2040-weapon-base" || !self.active) return;
      if (status.nifPath && String(status.nifPath).replace(/\\/g, "/").toLowerCase() !==
          self.nativeRequestedPath.toLowerCase()) return;
      if (status.ok === false) {
        // PrismaUI 2.2.1.1's engine loader logged `meshes/Meshes/...`
        // for documented prefixed requests. Try without the prefix ONCE.
        // Do not continually retry a missing/undrawable model.
        if (!self.relativePathRetry && self.nifPath.indexOf("Meshes/") === 0) {
          self.relativePathRetry = true;
          self.error = "Trying compatible mesh path...";
          self.hideNative();
          self.updateMessage();
          self.sync(self.region, self.nifPath);
          return;
        }
        self.failed = true;
        var reason = String(status.error || "missing or undrawable NIF").slice(0, 150);
        self.error = "3D preview unavailable: " + reason + " [" + self.nifPath + "]";
        self.hideNative();
      } else if (status.phase === "loaded" || status.phase === "textured") {
        self.error = "";
      }
      self.updateMessage();
    });
    this.boundBridge = api;
    return api;
  };

  WeaponModelPreview.prototype.hideNative = function() {
    var api = this.bridge() || this.boundBridge;
    if (api) {
      try { api.hide({ id: "k2040-weapon-base" }); } catch (error) { /* fail closed */ }
    }
    this.lastRequest = "";
  };

  WeaponModelPreview.prototype.stop = function() {
    this.hideNative();
    this.active = false;
    this.failed = false;
    this.error = "";
    this.dragStart = null;
    this.region = null;
    this.nifPath = "";
    this.nativeRequestedPath = "";
    this.relativePathRetry = false;
  };

  WeaponModelPreview.prototype.toggle = function() {
    if (this.active) { this.stop(); return false; }
    if (!this.supported()) return false;
    this.active = true;
    this.failed = false;
    this.error = "";
    this.zoom = 1;
    this.yaw = 0;
    this.pitch = 0;
    return true;
  };

  WeaponModelPreview.prototype.updateMessage = function() {
    if (!this.region) return;
    var message = this.region.parentNode &&
      this.region.parentNode.querySelector(".qm-weapon-preview-message");
    if (message) message.textContent = this.error ||
      "Receiver/base mesh only — attachments may differ. Drag to rotate; scroll to zoom.";
  };

  WeaponModelPreview.prototype.sync = function(region, modelPath) {
    if (!this.active) return;
    var api = this.bind();
    if (!api || !region || !modelPath || !/^Meshes\/.+\.nif$/i.test(modelPath)) {
      this.stop();
      return;
    }
    this.region = region;
    if (this.nifPath !== modelPath) {
      this.nifPath = modelPath;
      this.relativePathRetry = false;
      this.lastRequest = "";
    }
    this.updateMessage();
    if (this.failed) return;
    var r = region.getBoundingClientRect();
    var rect = [r.left, r.top, r.width, r.height].map(Math.round);
    if (rect[2] < 60 || rect[3] < 60) { this.hideNative(); return; }
    var requestedPath = this.relativePathRetry ? modelPath.slice("Meshes/".length) : modelPath;
    var request = JSON.stringify([requestedPath, rect, this.zoom, this.yaw, this.pitch]);
    if (request === this.lastRequest) return;
    try {
      api.show({
        id: "k2040-weapon-base", nifPath: requestedPath, formType: "WEAP",
        x: rect[0], y: rect[1], w: rect[2], h: rect[3],
        clipX: rect[0], clipY: rect[1], clipW: rect[2], clipH: rect[3],
        zoom: this.zoom, yaw: this.yaw, pitch: this.pitch, brightness: 1
      });
      this.lastRequest = request;
      this.nativeRequestedPath = requestedPath;
    } catch (error) {
      this.failed = true;
      this.error = "3D preview could not start.";
      this.hideNative();
      this.updateMessage();
    }
  };

  WeaponModelPreview.prototype.beginDrag = function(x, y) {
    if (this.active) this.dragStart = { x: x, y: y };
  };
  WeaponModelPreview.prototype.drag = function(x, y) {
    if (!this.active || !this.dragStart) return;
    this.yaw = (this.yaw + (x - this.dragStart.x) * 0.6 + 360) % 360;
    this.pitch = clamp(this.pitch + (y - this.dragStart.y) * 0.4, -70, 70);
    this.dragStart = { x: x, y: y };
    this.sync(this.region, this.nifPath);
  };
  WeaponModelPreview.prototype.endDrag = function() { this.dragStart = null; };
  WeaponModelPreview.prototype.scroll = function(deltaY) {
    if (!this.active) return;
    this.zoom = clamp(this.zoom + (deltaY < 0 ? 0.1 : -0.1), 0.6, 2.5);
    this.sync(this.region, this.nifPath);
  };

  global.K2040WeaponModelPreview = {
    create: function(host) { return new WeaponModelPreview(host); }
  };
  if (typeof module !== "undefined" && module.exports) module.exports = global.K2040WeaponModelPreview;
})(typeof window !== "undefined" ? window : globalThis);
