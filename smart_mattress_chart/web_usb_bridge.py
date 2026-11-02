"""Mở web app đệm thông minh trên máy tính qua cáp USB (không cần đổi WiFi).

Chạy file này (hoặc bấm đúp Mo_web_qua_USB.bat): nó tìm cổng USB của ESP32 (chip CH340),
phục vụ index.html tại http://127.0.0.1:8765 và chuyển /api/state, /api/cmd xuống board
theo cùng giao thức dòng "@cmd {json}" / "@J {json}" của firmware.
"""
import json
import os
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import serial
import serial.tools.list_ports

HERE = os.path.dirname(os.path.abspath(__file__))
PAGE = os.path.join(HERE, 'index.html')
HTTP_PORT = 8765


def find_port():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if args:
        return args[0]
    for p in serial.tools.list_ports.comports():
        if p.vid == 0x1A86 or 'CH340' in (p.description or ''):
            return p.device
    return 'COM11'


name = find_port()
try:
    port = serial.Serial(name, 115200, timeout=0.2)
except serial.SerialException as e:
    print(f'Không mở được {name}: {e}')
    print('Hãy đóng idf.py monitor / chương trình khác đang dùng cổng này rồi chạy lại.')
    input('Nhấn Enter để thoát...')
    sys.exit(1)
port.dtr = False
port.rts = False

latest = {'state': None, 'polled': 0.0}

# Every new reading is also appended to data/log_YYYYMMDD.csv next to this script.
LOG_DIR = os.path.join(HERE, 'data')
LEVEL_KPA = [2, 4, 6, 8, 10, 12, 14, 17]
CSV_HEAD = (['thoi_gian'] + ['P%02d_kPa' % (i + 1) for i in range(13)] +
            ['C%02d_cap' % (i + 1) for i in range(13)] +
            ['tu_dong', 'khoang_dang_chinh', 'hanh_dong', 'van_dang_mo'])


def level(v):
    if v is None:
        return ''
    if v < 1:
        return 0
    return min(range(8), key=lambda i: abs(v - LEVEL_KPA[i])) + 1


def log_state(st):
    try:
        os.makedirs(LOG_DIR, exist_ok=True)
        path = os.path.join(LOG_DIR, time.strftime('log_%Y%m%d.csv'))
        new = not os.path.exists(path)
        a = st.get('auto') or {}
        mask = (st.get('valve') or {}).get('mask', 0)
        act = {'idle': 'giu', 'inflate': 'nap', 'exhaust': 'xa'}.get(a.get('action', ''), '')
        row = ([time.strftime('%Y-%m-%d %H:%M:%S')] +
               ['ERR' if v is None else '%.2f' % v for v in st['p']] +
               [level(v) for v in st['p']] +
               [1 if a.get('on') else 0,
                (a.get('active', -1) + 1) if a.get('active', -1) >= 0 else '', act,
                ' '.join('V%d' % (i + 1) for i in range(32) if (mask >> i) & 1)])
        with open(path, 'a', encoding='utf-8-sig' if new else 'utf-8', newline='') as f:
            if new:
                f.write(','.join(CSV_HEAD) + '\r\n')
            f.write(','.join(str(x) for x in row) + '\r\n')
    except OSError:
        pass
replies = []
write_lock = threading.Lock()


def reader():
    buf = b''
    while True:
        try:
            buf += port.read(4096)
        except serial.SerialException:
            latest['state'] = None
            time.sleep(1)
            continue
        *lines, buf = buf.split(b'\n')
        for raw in lines:
            line = raw.decode('utf-8', 'replace')
            i = line.find('@J ')
            if i < 0:
                continue
            try:
                o = json.loads(line[i + 3:])
            except ValueError:
                continue
            if 'seq' in o:
                prev = latest['state']
                latest['state'] = o
                if prev is None or prev.get('seq') != o.get('seq'):
                    log_state(o)
            else:
                replies.append(o)


def keepalive():
    # Firmware switches every valve off when no page has talked to it for 30 s.
    while True:
        time.sleep(5)
        if time.time() - latest['polled'] < 10:
            with write_lock:
                port.write(b'@cmd {"cmd":"ping"}\n')


class Handler(BaseHTTPRequestHandler):
    def _send(self, code, body, ctype):
        data = body.encode('utf-8') if isinstance(body, str) else body
        self.send_response(code)
        self.send_header('Content-Type', ctype)
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path in ('/', '/index.html'):
            with open(PAGE, 'rb') as f:
                self._send(200, f.read(), 'text/html; charset=utf-8')
        elif self.path.startswith('/api/state'):
            latest['polled'] = time.time()
            st = latest['state']
            self._send(200 if st else 503, json.dumps(st or {'error': 'no data from board'}),
                       'application/json')
        else:
            self._send(404, 'not found', 'text/plain')

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get('Content-Length', 0))).decode('utf-8')
        with write_lock:
            n = len(replies)
            port.write(('@cmd ' + body.replace('\n', ' ').strip() + '\n').encode('utf-8'))
            t0 = time.time()
            while len(replies) == n and time.time() - t0 < 3:
                time.sleep(0.02)
        r = replies[n] if len(replies) > n else {'ok': False, 'msg': 'no reply'}
        self._send(200 if r.get('ok') else 400, json.dumps(r), 'application/json')

    def log_message(self, *args):
        pass


threading.Thread(target=reader, daemon=True).start()
threading.Thread(target=keepalive, daemon=True).start()
url = f'http://127.0.0.1:{HTTP_PORT}/'
print(f'Đã nối board ở {name}. Web app: {url}')
print('Giữ cửa sổ này mở khi dùng web. Đóng cửa sổ để ngắt.')
print('Dữ liệu cảm biến tự lưu vào:', LOG_DIR)
if '--no-browser' not in sys.argv:
    webbrowser.open(url)
ThreadingHTTPServer(('127.0.0.1', HTTP_PORT), Handler).serve_forever()
