'use strict';

/**
 * Loader for the cw_keyer_pin native addon.
 *
 * Returns the binding object on success, or an empty object when the addon
 * could not be loaded (non-Linux platform, missing toolchain at install time,
 * or ABI mismatch). Callers must handle the missing `setPin` case by falling
 * back to serialport's set() implementation.
 */

let binding = null;

// Native addon is only meaningful on Linux (uses TIOCMBIS/TIOCMBIC ioctls).
// On other platforms, return empty object so callers fall back to serialport.
if (process.platform === 'linux') {
  try {
    binding = require('./build/Release/cw_keyer_pin.node');
  } catch (releaseErr) {
    try {
      binding = require('./build/Debug/cw_keyer_pin.node');
    } catch (debugErr) {
      binding = null;
    }
  }
}

if (binding && typeof binding.setPin !== 'function') {
  binding = null;
}

module.exports = binding || {};
