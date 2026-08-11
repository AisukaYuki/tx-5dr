/**
 * Native N-API addon for direct DTR/RTS pin control via TIOCMBIS/TIOCMBIC.
 *
 * On Linux, the binding's `setPin` drives per-bit modem control ioctls directly
 * on a file descriptor. This works around `@serialport/bindings-cpp`'s
 * `TIOCMGET + TIOCMSET` path returning ENOTTY on some driver combinations
 * (notably CP2105 ECI on kernel 6.17).
 *
 * On non-Linux platforms or when the addon fails to build, the loader returns
 * an empty object and `setPin` is undefined; callers must fall back to
 * `serialport`'s `port.set()` in that case.
 */

export type CwKeyerPin = 'dtr' | 'rts';

export interface CwKeyerPinBinding {
  /**
   * Set or clear the specified modem control bit on the given file descriptor.
   *
   * @param fd   Open file descriptor for the tty device.
   * @param pin  Which control line to drive ('dtr' or 'rts').
   * @param level true to set the bit (TIOCMBIS), false to clear it (TIOCMBIC).
   * @throws Error when the underlying ioctl fails (errno reported in message).
   */
  setPin(fd: number, pin: CwKeyerPin, level: boolean): void;
}

declare const binding: Partial<CwKeyerPinBinding>;

export default binding;
