/*
 * cw_keyer_pin.c — Native N-API addon for direct DTR/RTS pin control.
 *
 * Background: @serialport/bindings-cpp's set() path uses TIOCMGET + TIOCMSET
 * (read-modify-write of the whole modem bit register). On some Linux/tty
 * driver combinations — observed with CP2105 ECI (interface 1) on kernel
 * 6.17 — TIOCMSET returns ENOTTY even though TIOCMBIS/TIOCMBIC (per-bit
 * set/clear) work fine. pyserial uses TIOCMBIS/TIOCMBIC, which is why its
 * setDTR()/setRTS() succeed on the same device where serialport's set() fails.
 *
 * This addon exposes a minimal setPin(fd, pin, level) entry point that drives
 * TIOCMBIS / TIOCMBIC directly on a file descriptor obtained from serialport.
 * It is Linux-only; on other platforms the loader returns null and the caller
 * falls back to serialport's set().
 */

#include <node_api.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#include <sys/ioctl.h>
#include <termios.h>  /* TIOCMBIS, TIOCMBIC, TIOCM_DTR, TIOCM_RTS */
#define CW_KEYER_PIN_LINUX 1
#else
#define CW_KEYER_PIN_LINUX 0
#endif

/*
 * setPin(fd: number, pin: 'dtr' | 'rts', level: boolean): void
 *
 * Sets or clears the specified modem control bit on the given file descriptor.
 * Throws an Error if the underlying ioctl fails.
 */
static napi_value SetPin(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  if (status != napi_ok || argc < 3) {
    napi_throw_type_error(env, NULL, "Expected 3 arguments: fd, pin, level");
    return NULL;
  }

  int32_t fd = -1;
  status = napi_get_value_int32(env, args[0], &fd);
  if (status != napi_ok) {
    napi_throw_type_error(env, NULL, "fd must be a number");
    return NULL;
  }
  if (fd < 0) {
    napi_throw_type_error(env, NULL, "fd must be a non-negative integer");
    return NULL;
  }

  char pin[8] = {0};
  size_t pin_len = 0;
  status = napi_get_value_string_utf8(env, args[1], NULL, 0, &pin_len);
  if (status != napi_ok) {
    napi_throw_type_error(env, NULL, "pin must be a string");
    return NULL;
  }
  if (pin_len == 0 || pin_len >= sizeof(pin)) {
    napi_throw_type_error(env, NULL, "pin must be 'dtr' or 'rts'");
    return NULL;
  }
  status = napi_get_value_string_utf8(env, args[1], pin, sizeof(pin), &pin_len);
  if (status != napi_ok) {
    napi_throw_type_error(env, NULL, "pin must be a string");
    return NULL;
  }

  bool level = false;
  status = napi_get_value_bool(env, args[2], &level);
  if (status != napi_ok) {
    napi_throw_type_error(env, NULL, "level must be a boolean");
    return NULL;
  }

#if CW_KEYER_PIN_LINUX
  int bit;
  if (strcmp(pin, "dtr") == 0) {
    bit = TIOCM_DTR;
  } else if (strcmp(pin, "rts") == 0) {
    bit = TIOCM_RTS;
  } else {
    napi_throw_type_error(env, NULL, "pin must be 'dtr' or 'rts'");
    return NULL;
  }

  unsigned long cmd = level ? TIOCMBIS : TIOCMBIC;
  int arg = bit;
  if (ioctl(fd, cmd, &arg) == -1) {
    char buf[128];
    snprintf(buf, sizeof(buf), "ioctl %s on fd %d failed: %s",
             level ? "TIOCMBIS" : "TIOCMBIC", fd, strerror(errno));
    napi_throw_error(env, NULL, buf);
    return NULL;
  }
  return NULL;
#else
  napi_throw_error(env, NULL, "cw_keyer_pin native addon is Linux-only");
  return NULL;
#endif
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_value fn;
  napi_status status = napi_create_function(env, "setPin", NAPI_AUTO_LENGTH, SetPin, NULL, &fn);
  if (status != napi_ok) {
    return NULL;
  }
  status = napi_set_named_property(env, exports, "setPin", fn);
  if (status != napi_ok) {
    return NULL;
  }
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
