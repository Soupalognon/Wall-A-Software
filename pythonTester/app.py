import tkinter as tk
from tkinter import ttk
from tkinter.scrolledtext import ScrolledText
import queue
import time

import matplotlib
matplotlib.use('TkAgg')
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

import serial.tools.list_ports

from config import DEFAULT_COM_PORT, BAUD_RATES, PLOT_WINDOW, VEL_V_MIN, VEL_V_MAX, VEL_W_MIN, VEL_W_MAX
from parser import parse_frame
from data_store import DataStore
from serial_worker import SerialWorker


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

        # Variables partagées entre les deux onglets
        self._svar_vel_v   = tk.StringVar(value='0.0')
        self._svar_vel_w   = tk.StringVar(value='0.0')
        self._dvar_vel_v   = tk.DoubleVar(value=0.0)   # valeur slider v (partagée)
        self._dvar_vel_w   = tk.DoubleVar(value=0.0)   # valeur slider w (partagée)
        self._vel_syncing  = False                      # garde contre boucle trace
        self._svar_pose_x  = tk.StringVar(value='0.0')
        self._svar_pose_y  = tk.StringVar(value='0.0')
        self._svar_pose_a  = tk.StringVar(value='0.0')
        self._svar_pid_p   = tk.StringVar(value='0.0')
        self._svar_pid_i   = tk.StringVar(value='0.0')
        self._svar_pid_d   = tk.StringVar(value='0.0')
        self._auto_vel     = tk.BooleanVar(value=False)
        self._svar_auto_hz = tk.StringVar(value='10')
        self._auto_vel_job: str | None = None

        self._dvar_vel_v.trace_add('write', self._on_dbl_vel_v)
        self._dvar_vel_w.trace_add('write', self._on_dbl_vel_w)
        self._svar_vel_v.trace_add('write', self._on_str_vel_v)
        self._svar_vel_w.trace_add('write', self._on_str_vel_w)

        self._build_ui()
        self._refresh_ports()
        self.after(100, self._poll)
        self.protocol('WM_DELETE_WINDOW', self._on_close)

    # ── Build UI ──────────────────────────────────────────────────────

    def _build_ui(self):
        self._build_toolbar()
        self._notebook = ttk.Notebook(self)
        self._notebook.pack(fill=tk.BOTH, expand=True, padx=4, pady=(0, 4))
        self._build_surveillance_tab()
        self._build_command_tab()

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

    def _build_surveillance_tab(self):
        tab = tk.Frame(self._notebook)
        self._notebook.add(tab, text='Surveillance')
        pane = tk.PanedWindow(tab, orient=tk.VERTICAL, sashrelief=tk.RAISED, sashwidth=5)
        pane.pack(fill=tk.BOTH, expand=True)
        self._build_table(pane)
        self._build_plot(pane)
        self._build_main_cmd_panel(pane)

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

    def _build_main_cmd_panel(self, pane: tk.PanedWindow):
        frame = tk.Frame(pane)
        pane.add(frame, minsize=80, height=180)
        self._build_cmd_groups(frame, with_log=False)

    def _build_command_tab(self):
        tab = tk.Frame(self._notebook)
        self._notebook.add(tab, text='Commandes')
        self._build_cmd_groups(tab, with_log=True)

    def _build_cmd_groups(self, parent: tk.Widget, *, with_log: bool):
        pad = {'padx': 8, 'pady': 4}

        # ── MOVE_VEL ─────────────────────────────────────────────────
        grp_vel = ttk.LabelFrame(parent, text='Vitesse  —  CMD MOVE_VEL')
        grp_vel.pack(fill=tk.X, **pad)

        tk.Label(grp_vel, text='v (m/s):').grid(row=0, column=0, padx=6, pady=6)
        ttk.Entry(grp_vel, textvariable=self._svar_vel_v, width=10).grid(row=0, column=1, padx=4)

        tk.Label(grp_vel, text='w (rad/s):').grid(row=0, column=2, padx=6)
        ttk.Entry(grp_vel, textvariable=self._svar_vel_w, width=10).grid(row=0, column=3, padx=4)

        ttk.Button(grp_vel, text='Envoyer', command=self._send_move_vel).grid(row=0, column=4, padx=10)

        tk.Label(grp_vel, text='v:').grid(row=0, column=5, padx=(16, 2))
        tk.Scale(grp_vel, from_=VEL_V_MIN, to=VEL_V_MAX, resolution=0.01, orient=tk.HORIZONTAL,
                 length=180, variable=self._dvar_vel_v).grid(row=0, column=6, padx=4)

        tk.Label(grp_vel, text='w:').grid(row=0, column=7, padx=(8, 2))
        tk.Scale(grp_vel, from_=VEL_W_MIN, to=VEL_W_MAX, resolution=0.05, orient=tk.HORIZONTAL,
                 length=180, variable=self._dvar_vel_w).grid(row=0, column=8, padx=4)

        ttk.Checkbutton(grp_vel, text='Auto envoi', variable=self._auto_vel,
                        command=self._toggle_auto_vel).grid(row=0, column=9, padx=(16, 2))
        tk.Label(grp_vel, text='Hz:').grid(row=0, column=10, padx=(4, 0))
        ttk.Entry(grp_vel, textvariable=self._svar_auto_hz, width=6).grid(row=0, column=11, padx=4)

        # ── MOVE_POSE ────────────────────────────────────────────────
        grp_pose = ttk.LabelFrame(parent, text='Pose cible  —  CMD MOVE_POSE')
        grp_pose.pack(fill=tk.X, **pad)

        tk.Label(grp_pose, text='x (m):').grid(row=0, column=0, padx=6, pady=6)
        ttk.Entry(grp_pose, textvariable=self._svar_pose_x, width=10).grid(row=0, column=1, padx=4)

        tk.Label(grp_pose, text='y (m):').grid(row=0, column=2, padx=6)
        ttk.Entry(grp_pose, textvariable=self._svar_pose_y, width=10).grid(row=0, column=3, padx=4)

        tk.Label(grp_pose, text='angle (rad):').grid(row=0, column=4, padx=6)
        ttk.Entry(grp_pose, textvariable=self._svar_pose_a, width=10).grid(row=0, column=5, padx=4)

        ttk.Button(grp_pose, text='Envoyer', command=self._send_move_pose).grid(row=0, column=6, padx=10)

        # ── MOVE_STOP ────────────────────────────────────────────────
        grp_stop = ttk.LabelFrame(parent, text='Arrêt  —  CMD MOVE_STOP')
        grp_stop.pack(fill=tk.X, **pad)

        ttk.Button(grp_stop, text='STOP', width=12,
                   command=self._send_move_stop).pack(padx=10, pady=8, anchor=tk.W)

        # ── PID ──────────────────────────────────────────────────────
        grp_pid = ttk.LabelFrame(parent, text='Gains PID  —  CMD PID')
        grp_pid.pack(fill=tk.X, **pad)

        tk.Label(grp_pid, text='P:').grid(row=0, column=0, padx=6, pady=6)
        ttk.Entry(grp_pid, textvariable=self._svar_pid_p, width=10).grid(row=0, column=1, padx=4)

        tk.Label(grp_pid, text='I:').grid(row=0, column=2, padx=6)
        ttk.Entry(grp_pid, textvariable=self._svar_pid_i, width=10).grid(row=0, column=3, padx=4)

        tk.Label(grp_pid, text='D:').grid(row=0, column=4, padx=6)
        ttk.Entry(grp_pid, textvariable=self._svar_pid_d, width=10).grid(row=0, column=5, padx=4)

        ttk.Button(grp_pid, text='Envoyer', command=self._send_pid).grid(row=0, column=6, padx=10)

        # ── Journal des envois (onglet Commandes seulement) ───────────
        if with_log:
            grp_log = ttk.LabelFrame(parent, text='Journal des envois')
            grp_log.pack(fill=tk.BOTH, expand=True, **pad)
            self._cmd_log = ScrolledText(
                grp_log, height=8, bg='#1e1e1e', fg='#d4d4d4',
                font=('Consolas', 9), state=tk.DISABLED
            )
            self._cmd_log.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)
            self._cmd_log.tag_config('ok', foreground='#4ec94e')
            self._cmd_log.tag_config('err', foreground='#cc3333')

    # ── Slider ↔ Entry sync ───────────────────────────────────────────

    def _on_dbl_vel_v(self, *_):
        if self._vel_syncing:
            return
        self._vel_syncing = True
        self._svar_vel_v.set(str(round(self._dvar_vel_v.get(), 3)))
        self._vel_syncing = False

    def _on_dbl_vel_w(self, *_):
        if self._vel_syncing:
            return
        self._vel_syncing = True
        self._svar_vel_w.set(str(round(self._dvar_vel_w.get(), 3)))
        self._vel_syncing = False

    def _on_str_vel_v(self, *_):
        if self._vel_syncing:
            return
        try:
            self._vel_syncing = True
            self._dvar_vel_v.set(float(self._svar_vel_v.get()))
        except ValueError:
            pass
        finally:
            self._vel_syncing = False

    def _on_str_vel_w(self, *_):
        if self._vel_syncing:
            return
        try:
            self._vel_syncing = True
            self._dvar_vel_w.set(float(self._svar_vel_w.get()))
        except ValueError:
            pass
        finally:
            self._vel_syncing = False

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

    # ── Send commands ─────────────────────────────────────────────────

    def _send(self, line: str):
        ok = bool(self._worker and self._worker.write(line + '\n'))
        prefix = time.strftime('[%H:%M:%S] ')
        tag = 'ok' if ok else 'err'
        suffix = '' if ok else '  ← non connecté'
        self._cmd_log.config(state=tk.NORMAL)
        self._cmd_log.insert(tk.END, prefix + line + suffix + '\n', tag)
        self._cmd_log.see(tk.END)
        self._cmd_log.config(state=tk.DISABLED)

    def _toggle_auto_vel(self):
        if self._auto_vel.get():
            self._auto_vel_tick()
        elif self._auto_vel_job is not None:
            self.after_cancel(self._auto_vel_job)
            self._auto_vel_job = None

    def _auto_vel_tick(self):
        if not self._auto_vel.get():
            return
        self._send_move_vel()
        try:
            hz = float(self._svar_auto_hz.get())
            interval = max(50, int(1000 / hz))
        except ValueError:
            interval = 100
        self._auto_vel_job = self.after(interval, self._auto_vel_tick)

    def _send_move_vel(self):
        try:
            v = float(self._svar_vel_v.get())
            w = float(self._svar_vel_w.get())
        except ValueError:
            return
        self._send(f'CMD MOVE_VEL {v} {w}')

    def _send_move_pose(self):
        try:
            x = float(self._svar_pose_x.get())
            y = float(self._svar_pose_y.get())
            a = float(self._svar_pose_a.get())
        except ValueError:
            return
        self._send(f'CMD MOVE_POSE {x} {y} {a}')

    def _send_move_stop(self):
        self._svar_vel_v.set('0.0')
        self._svar_vel_w.set('0.0')
        self._send('CMD MOVE_STOP')

    def _send_pid(self):
        try:
            p = float(self._svar_pid_p.get())
            i = float(self._svar_pid_i.get())
            d = float(self._svar_pid_d.get())
        except ValueError:
            return
        self._send(f'CMD PID P:{p} I:{i} D:{d}')

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
            self._ax.autoscale_view(scalex=False)
            if self._store._frame_start_ms is not None or self._store._start is not None:
                t_now = self._store.last_t
                self._ax.set_xlim(max(0.0, t_now - PLOT_WINDOW), t_now)
        if need_legend:
            self._ax.legend(fontsize=7, loc='upper left')
        self._canvas.draw_idle()

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
