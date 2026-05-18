import time
from collections import deque

from config import MAX_POINTS


class DataStore:
    def __init__(self):
        self.history: dict[tuple, deque] = {}
        self.times: dict[tuple, deque] = {}
        self.current: dict[tuple, tuple] = {}  # key -> (value, timestamp_str)
        self._start: float | None = None
        self._frame_start_ms: float | None = None
        self.last_t: float = 0.0

    def reset(self):
        self.history.clear()
        self.times.clear()
        self.current.clear()
        self._start = None
        self._frame_start_ms = None
        self.last_t = 0.0

    def update(self, parsed: dict):
        frame_time_ms = parsed.get('frame_time_ms')
        if frame_time_ms is not None:
            if self._frame_start_ms is None:
                self._frame_start_ms = frame_time_ms
            t = (frame_time_ms - self._frame_start_ms) / 1000.0
        else:
            now = time.time()
            if self._start is None:
                self._start = now
            t = now - self._start
        self.last_t = t
        ts_str = time.strftime('%H:%M:%S')
        domain, subdomain = parsed['domain'], parsed['subdomain']
        for var, val in parsed['vars'].items():
            key = (domain, subdomain, var)
            if key not in self.history:
                self.history[key] = deque(maxlen=MAX_POINTS)
                self.times[key] = deque(maxlen=MAX_POINTS)
            self.history[key].append(val)
            self.times[key].append(t)
            self.current[key] = (val, ts_str)
