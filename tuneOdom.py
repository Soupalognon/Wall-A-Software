import tkinter as tk
from tkinter import ttk
from tkinter.scrolledtext import ScrolledText
import threading
import queue
import time
import re
from collections import deque

import serial
import serial.tools.list_ports
import matplotlib
matplotlib.use('TkAgg')
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

MAX_POINTS = 500
DEFAULT_COM_PORT = 'COM31'
BAUD_RATES = ['9600', '19200', '38400', '57600', '115200', '230400', '460800', '921600']
RECONNECT_DELAY = 1.0
PLOT_WINDOW = 3  # secondes visibles sur le graphe (fenêtre glissante)

_KV_RE = re.compile(r'(\w+):([-\d.]+)')


def parse_frame(line: str):
    """Return {domain, subdomain, vars: {name: float}, raw} or None."""
    line = line.strip()
    if not line:
        return None
    parts = line.split(None, 2)
    if len(parts) < 2:
        return None
    domain, subdomain = parts[0], parts[1]
    rest = parts[2] if len(parts) > 2 else ''
    vars_ = {}
    for m in _KV_RE.finditer(rest):
        try:
            vars_[m.group(1)] = float(m.group(2))
        except ValueError:
            pass
    # Fallback: bare numeric value (e.g. "ALT PROXIMITY 0.42")
    if not vars_ and rest:
        try:
            vars_[subdomain] = float(rest.split()[0])
        except (ValueError, IndexError):
            pass
    frame_time_ms = vars_.pop('time', None)
    return {'domain': domain, 'subdomain': subdomain, 'vars': vars_,
            'frame_time_ms': frame_time_ms, 'raw': line}


class DataStore:
    def __init__(self):
        self.history: dict[tuple, deque] = {}
        self.times: dict[tuple, deque] = {}
        self.current: dict[tuple, tuple] = {}  # key -> (value, timestamp_str)
        self._start: float | None = None         # fallback wall-clock origin
        self._frame_start_ms: float | None = None  # premier timestamp embarqué
        self.last_t: float = 0.0                   # dernier t normalisé (secondes)

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


class SerialWorker(threading.Thread):
    def __init__(self, port: str, baud: int, data_queue: queue.Queue, connected_cb):
        super().__init__(daemon=True)
        self._port = port
        self._baud = baud
        self._queue = data_queue
        self._connected_cb = connected_cb
        self._stop = threading.Event()

    def stop(self):
        self._stop.set()

    def run(self):
        while not self._stop.is_set():
            try:
                with serial.Serial(self._port, self._baud, timeout=1) as ser:
                    self._connected_cb(True)
                    while not self._stop.is_set():
                        line = ser.readline().decode('utf-8', errors='replace')
                        if line:
                            self._queue.put(line)
            except serial.SerialException:
                self._connected_cb(False)
                for _ in range(int(RECONNECT_DELAY * 10)):
                    if self._stop.is_set():
                        return
                    time.sleep(0.1)
        self._connected_cb(False)


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title('Moniteur Série Wall-A')
        self.geometry('960x720')
        self.minsize(700, 500)

        self._queue: queue.Queue = queue.Queue()
        self._store = DataStore()
        self._worker: SerialWorker | None = None
        self._tree_ids: dict[tuple, str] = {}
        self._plot_lines: dict = {}

        self._build_ui()
        self._refresh_ports()
        self.after(100, self._poll)
        self.protocol('WM_DELETE_WINDOW', self._on_close)

    # ── Build UI ──────────────────────────────────────────────────────

    def _build_ui(self):
        self._build_toolbar()
        pane = tk.PanedWindow(self, orient=tk.VERTICAL, sashrelief=tk.RAISED, sashwidth=5)
        pane.pack(fill=tk.BOTH, expand=True, padx=4, pady=(0, 4))
        self._build_table(pane)
        self._build_plot(pane)
        self._build_console(pane)

    def _build_toolbar(self):
        bar = tk.Frame(self, pady=4, padx=4)
        bar.pack(side=tk.TOP, fill=tk.X)

        tk.Label(bar, text='Port:').pack(side=tk.LEFT)
        self._port_cb = ttk.Combobox(bar, width=10, state='readonly')
        self._port_cb.pack(side=tk.LEFT, padx=2)

        tk.Label(bar, text='Baud:').pack(side=tk.LEFT, padx=(8, 0))
        self._baud_cb = ttk.Combobox(bar, values=BAUD_RATES, width=9, state='readonly')
        self._baud_cb.set('115200')
        self._baud_cb.pack(side=tk.LEFT, padx=2)

        ttk.Button(bar, text='⟳', width=3, command=self._refresh_ports).pack(side=tk.LEFT, padx=(4, 2))

        self._conn_btn = ttk.Button(bar, text='Connecter', command=self._toggle_connect)
        self._conn_btn.pack(side=tk.LEFT, padx=6)

        self._led = tk.Canvas(bar, width=14, height=14, highlightthickness=0)
        self._led.pack(side=tk.LEFT)
        self._led_oval = self._led.create_oval(2, 2, 12, 12, fill='#cc3333', outline='#888')

        self._status_lbl = tk.Label(bar, text='Déconnecté', fg='#cc3333')
        self._status_lbl.pack(side=tk.LEFT, padx=4)

        ttk.Button(bar, text='Effacer historique', command=self._clear_history).pack(side=tk.RIGHT)

    def _build_table(self, pane: tk.PanedWindow):
        frame = tk.Frame(pane)
        pane.add(frame, minsize=80, height=160)
        cols = ('domain', 'subdomain', 'variable', 'valeur', 'delta', 'heure')
        labels = ('Domaine', 'Sous-dom.', 'Variable', 'Valeur', 'Δ fenêtre', 'Heure')
        widths = (80, 90, 120, 90, 85, 80)
        self._tree = ttk.Treeview(frame, columns=cols, show='headings', height=6)
        for col, lbl, w in zip(cols, labels, widths):
            self._tree.heading(col, text=lbl)
            self._tree.column(col, width=w, anchor=tk.CENTER, minwidth=50)
        vsb = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=self._tree.yview)
        self._tree.configure(yscrollcommand=vsb.set)
        vsb.pack(side=tk.RIGHT, fill=tk.Y)
        self._tree.pack(fill=tk.BOTH, expand=True)

    def _build_plot(self, pane: tk.PanedWindow):
        frame = tk.Frame(pane)
        pane.add(frame, minsize=150, height=300)
        self._fig = Figure(dpi=95)
        self._ax = self._fig.add_subplot(111)
        self._ax.set_xlabel('Temps (s)', fontsize=8)
        self._ax.set_ylabel('Valeur', fontsize=8)
        self._ax.tick_params(labelsize=7)
        self._ax.set_xlim(0, PLOT_WINDOW)
        self._fig.tight_layout(pad=1.8)
        self._canvas = FigureCanvasTkAgg(self._fig, master=frame)
        self._canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    def _build_console(self, pane: tk.PanedWindow):
        frame = tk.Frame(pane)
        pane.add(frame, minsize=60, height=120)
        self._console = ScrolledText(
            frame, height=5, bg='#1e1e1e', fg='#d4d4d4',
            font=('Consolas', 9), state=tk.DISABLED
        )
        self._console.pack(fill=tk.BOTH, expand=True)
        self._console.tag_config('warn', foreground='#dcdcaa')
        self._console.tag_config('alt', foreground='#ce9178')
        self._console.tag_config('info', foreground='#9cdcfe')

    # ── Port / connection ─────────────────────────────────────────────

    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self._port_cb['values'] = ports
        if DEFAULT_COM_PORT in ports:
            self._port_cb.set(DEFAULT_COM_PORT)
        elif ports and not self._port_cb.get():
            self._port_cb.set(ports[0])

    def _toggle_connect(self):
        if self._worker and self._worker.is_alive():
            self._worker.stop()
            self._worker = None
            self._conn_btn.config(text='Connecter')
        else:
            port = self._port_cb.get()
            if not port:
                return
            baud = int(self._baud_cb.get())
            self._worker = SerialWorker(port, baud, self._queue, self._on_connected)
            self._worker.start()
            self._conn_btn.config(text='Déconnecter')

    def _on_connected(self, state: bool):
        self.after(0, self._update_led, state)

    def _update_led(self, state: bool):
        color = '#4ec94e' if state else '#cc3333'
        label = 'Connecté' if state else 'Reconnexion...'
        self._led.itemconfig(self._led_oval, fill=color)
        self._status_lbl.config(text=label, fg=color)

    # ── Main poll loop ────────────────────────────────────────────────

    def _poll(self):
        try:
            while True:
                line = self._queue.get_nowait()
                parsed = parse_frame(line)
                if parsed and parsed['vars']:
                    self._store.update(parsed)
                    self._refresh_table()
                    self._refresh_plot()
                else:
                    self._append_console(line.strip() if parsed is None else parsed['raw'])
        except queue.Empty:
            pass
        self.after(100, self._poll)

    # ── Table ─────────────────────────────────────────────────────────

    def _refresh_table(self):
        t_now = self._store.last_t if (self._store._frame_start_ms is not None or self._store._start is not None) else None
        for key, (val, ts) in self._store.current.items():
            domain, subdomain, var = key
            if t_now is not None:
                t_min = t_now - PLOT_WINDOW
                visible = [v for t, v in zip(self._store.times[key], self._store.history[key]) if t >= t_min]
                delta = f'{max(visible) - min(visible):.5g}' if visible else '—'
            else:
                delta = '—'
            row = (domain, subdomain, var, f'{val:.5g}', delta, ts)
            if key in self._tree_ids:
                self._tree.item(self._tree_ids[key], values=row)
            else:
                iid = self._tree.insert('', tk.END, values=row)
                self._tree_ids[key] = iid

    # ── Plot ──────────────────────────────────────────────────────────

    def _refresh_plot(self):
        need_legend = False
        for key, vals in self._store.history.items():
            ts = self._store.times[key]
            if len(vals) < 2:
                continue
            domain, subdomain, var = key
            label = f'{domain}.{subdomain}.{var}'
            xdata = list(ts)
            ydata = list(vals)
            if key not in self._plot_lines:
                (ln,) = self._ax.plot(xdata, ydata, label=label, linewidth=1.3)
                self._plot_lines[key] = ln
                need_legend = True
            else:
                self._plot_lines[key].set_xdata(xdata)
                self._plot_lines[key].set_ydata(ydata)
        if self._plot_lines:
            self._ax.relim()
            self._ax.autoscale_view(scalex=False)  # Y seulement, X géré manuellement
            if self._store._frame_start_ms is not None or self._store._start is not None:
                t_now = self._store.last_t
                self._ax.set_xlim(max(0.0, t_now - PLOT_WINDOW), t_now)
        if need_legend:
            self._ax.legend(fontsize=7, loc='upper left')
        self._canvas.draw_idle()

    # ── Console ───────────────────────────────────────────────────────

    def _append_console(self, text: str):
        if not text:
            return
        upper = text.upper()
        if 'LOG WARN' in upper:
            tag = 'warn'
        elif upper.startswith(('ALT', 'HLT')):
            tag = 'alt'
        else:
            tag = 'info'
        prefix = time.strftime('[%H:%M:%S] ')
        self._console.config(state=tk.NORMAL)
        self._console.insert(tk.END, prefix + text + '\n', tag)
        line_count = int(self._console.index('end-1c').split('.')[0])
        if line_count > 200:
            self._console.delete('1.0', f'{line_count - 200}.0')
        self._console.see(tk.END)
        self._console.config(state=tk.DISABLED)

    # ── Clear history ─────────────────────────────────────────────────

    def _clear_history(self):
        self._store.reset()
        self._ax.cla()
        self._ax.set_xlabel('Temps (s)', fontsize=8)
        self._ax.set_ylabel('Valeur', fontsize=8)
        self._ax.tick_params(labelsize=7)
        self._plot_lines.clear()
        self._canvas.draw_idle()

    def _on_close(self):
        if self._worker:
            self._worker.stop()
        self.destroy()


if __name__ == '__main__':
    app = App()
    app.mainloop()
