import re

_KV_RE = re.compile(r'(\w+):([-\d.]+)')


def parse_frame(line: str):
    """Return {domain, subdomain, vars: {name: float}, frame_time_ms, raw} or None."""
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
    if not vars_ and rest:
        try:
            vars_[subdomain] = float(rest.split()[0])
        except (ValueError, IndexError):
            pass
    frame_time_ms = vars_.pop('time', None)
    return {'domain': domain, 'subdomain': subdomain, 'vars': vars_,
            'frame_time_ms': frame_time_ms, 'raw': line}
