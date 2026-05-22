MAX_POINTS = 500
DEFAULT_COM_PORT = 'COM31'
BAUD_RATES = ['9600', '19200', '38400', '57600', '115200', '230400', '460800', '921600']
RECONNECT_DELAY = 1.0
PLOT_WINDOW = 3  # secondes visibles sur le graphe (fenêtre glissante)

# Plages des sliders MOVE_VEL
VEL_V_MIN = -1.8   # m/s
VEL_V_MAX =  1.8   # m/s
VEL_W_MIN = -20.0   # rad/s
VEL_W_MAX =  20.0   # rad/s

# Manette (gamepad)
GAMEPAD_POLL_HZ  = 20
GAMEPAD_DEADZONE = 0.08   # zone morte normalisée (0–1)
GAMEPAD_AXIS_V   = 1      # stick gauche vertical  (Xbox/PS : axe 1)
GAMEPAD_AXIS_W   = 2      # stick droit horizontal (Xbox/PS : axe 2)
GAMEPAD_AXIS_TRIGGER = 5  # gâchette droite        (Xbox/PS : axe 5)

# Limites en mode normal (sans gâchette)
GAMEPAD_V_MAX_NORMAL = 1.0   # m/s
GAMEPAD_W_MAX_NORMAL = 10.0   # rad/s
# Quand la gâchette droite est enfoncée, les limites passent à VEL_V_MAX / VEL_W_MAX
