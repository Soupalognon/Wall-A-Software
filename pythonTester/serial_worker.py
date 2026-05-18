import threading
import queue
import time

import serial

from config import RECONNECT_DELAY


class SerialWorker(threading.Thread):
    def __init__(self, port: str, baud: int, data_queue: queue.Queue, connected_cb):
        super().__init__(daemon=True)
        self._port = port
        self._baud = baud
        self._queue = data_queue
        self._connected_cb = connected_cb
        self._stop = threading.Event()
        self._ser: serial.Serial | None = None
        self._ser_lock = threading.Lock()

    def stop(self):
        self._stop.set()

    def write(self, data: str) -> bool:
        with self._ser_lock:
            if self._ser is None:
                return False
            try:
                self._ser.write(data.encode())
                return True
            except serial.SerialException:
                return False

    def run(self):
        while not self._stop.is_set():
            try:
                with serial.Serial(self._port, self._baud, timeout=1) as ser:
                    with self._ser_lock:
                        self._ser = ser
                    self._connected_cb(True)
                    while not self._stop.is_set():
                        line = ser.readline().decode('utf-8', errors='replace')
                        if line:
                            self._queue.put(line)
            except serial.SerialException:
                with self._ser_lock:
                    self._ser = None
                self._connected_cb(False)
                for _ in range(int(RECONNECT_DELAY * 10)):
                    if self._stop.is_set():
                        return
                    time.sleep(0.1)
        with self._ser_lock:
            self._ser = None
        self._connected_cb(False)
