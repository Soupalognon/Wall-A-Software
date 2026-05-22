import threading
import time

try:
    import pygame
    _PYGAME_AVAILABLE = True
except ImportError:
    _PYGAME_AVAILABLE = False

from config import (VEL_V_MIN, VEL_V_MAX, VEL_W_MIN, VEL_W_MAX,
                    GAMEPAD_V_MAX_NORMAL, GAMEPAD_W_MAX_NORMAL)


def _apply_deadzone(value: float, deadzone: float) -> float:
    if abs(value) < deadzone:
        return 0.0
    sign = 1.0 if value > 0 else -1.0
    return sign * (abs(value) - deadzone) / (1.0 - deadzone)


class GamepadWorker(threading.Thread):
    _pygame_inited = False
    _init_lock = threading.Lock()

    def __init__(self, joystick_index: int, axis_v: int, axis_w: int,
                 deadzone: float, callback, poll_hz: float = 20,
                 axis_trigger: int = 5,
                 v_max_normal: float = GAMEPAD_V_MAX_NORMAL,
                 w_max_normal: float = GAMEPAD_W_MAX_NORMAL):
        super().__init__(daemon=True)
        self._joystick_index = joystick_index
        self._axis_v = axis_v
        self._axis_w = axis_w
        self._deadzone = deadzone
        self._callback = callback
        self._poll_hz = poll_hz
        self._axis_trigger = axis_trigger
        self._v_max_normal = v_max_normal
        self._w_max_normal = w_max_normal
        self._stop_event = threading.Event()

    @staticmethod
    def is_available() -> bool:
        return _PYGAME_AVAILABLE

    @staticmethod
    def _ensure_init():
        with GamepadWorker._init_lock:
            if not GamepadWorker._pygame_inited:
                pygame.init()
                pygame.joystick.init()
                GamepadWorker._pygame_inited = True

    @staticmethod
    def list_joysticks() -> list[str]:
        if not _PYGAME_AVAILABLE:
            return []
        GamepadWorker._ensure_init()
        pygame.joystick.quit()
        pygame.joystick.init()
        return [pygame.joystick.Joystick(i).get_name()
                for i in range(pygame.joystick.get_count())]

    def run(self):
        if not _PYGAME_AVAILABLE:
            return
        GamepadWorker._ensure_init()

        try:
            joy = pygame.joystick.Joystick(self._joystick_index)
            joy.init()
        except Exception:
            self._callback(None, None)  # signal d'erreur
            return

        interval = 1.0 / max(1, self._poll_hz)

        while not self._stop_event.is_set():
            try:
                pygame.event.pump()
                raw_v = joy.get_axis(self._axis_v) if self._axis_v < joy.get_numaxes() else 0.0
                raw_w = joy.get_axis(self._axis_w) if self._axis_w < joy.get_numaxes() else 0.0
            except Exception:
                break

            dz_v = _apply_deadzone(raw_v, self._deadzone)
            dz_w = _apply_deadzone(raw_w, self._deadzone)

            # Gâchette droite : -1.0 (relâchée) → +1.0 (enfoncée)
            trigger = joy.get_axis(self._axis_trigger) if self._axis_trigger < joy.get_numaxes() else -1.0
            boost = trigger > 0.0

            v_limit = VEL_V_MAX if boost else self._v_max_normal
            w_limit = VEL_W_MAX if boost else self._w_max_normal

            # L'axe vertical est inversé sur la plupart des manettes (haut = -1)
            v = -dz_v * v_limit
            w = dz_w * w_limit

            v = max(-v_limit, min(v_limit, v))
            w = max(-w_limit, min(w_limit, w))

            self._callback(round(v, 3), round(w, 3))
            time.sleep(interval)

        joy.quit()

    def stop(self):
        self._stop_event.set()
