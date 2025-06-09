from flask import Flask, render_template, request, jsonify 
import serial
import serial.tools.list_ports
import threading
import time
from collections import deque

app = Flask(__name__)

ser = None
spamming = False
spam_thread = None
receive_thread = None
receive_running = False

uart_logs = deque(maxlen=100)  # Lưu 100 dòng log UART gần nhất

def uart_receive_loop():
    global receive_running
    while receive_running and ser:
        try:
            if ser.in_waiting >= 4:
                # 1. Read mode
                mode_byte = ser.read(1)
                if not mode_byte:
                    continue
                mode_val = int.from_bytes(mode_byte, 'big')
                mode = 'Standard' if mode_val == 0 else 'Extended'

                # 2. Read CAN ID
                id_len = 2 if mode == 'Standard' else 4
                can_id_bytes = ser.read(id_len)
                if len(can_id_bytes) != id_len:
                    continue
                can_id = can_id_bytes.hex().upper()

                # 3. Read data length
                length_byte = ser.read(1)
                if not length_byte:
                    continue
                data_len = int.from_bytes(length_byte, 'big')

                # 4. Read data
                data_bytes = ser.read(data_len)
                if len(data_bytes) != data_len:
                    continue

                # Decode data
                try:
                    data = data_bytes.decode('ascii', errors='replace')
                except Exception:
                    data = data_bytes.hex().upper()

                log_line = f"[UART Frame] mode={mode}, can_id={can_id}, data={data}"
                print(log_line)
                uart_logs.appendleft(log_line)

        except Exception as e:
            print("[UART Error]", e)

def start_receive_thread():
    global receive_running, receive_thread
    if ser and ser.is_open:
        receive_running = True
        receive_thread = threading.Thread(target=uart_receive_loop, daemon=True)
        receive_thread.start()

@app.route('/')
def index():
    ports = [port.device for port in serial.tools.list_ports.comports()]
    return render_template('index.html', ports=ports)

@app.route('/connect', methods=['POST'])
def connect():
    global ser
    data = request.get_json()
    port = data.get('port')
    baudrate = int(data.get('baudrate', 115200))
    try:
        if ser and ser.is_open:
            ser.close()
        ser = serial.Serial(port, baudrate, timeout=1)
        start_receive_thread()
        return jsonify({'status': 'connected'})
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)})

@app.route('/uart')
def uart():
    global uart_logs
    if uart_logs:
        logs_str = "\n".join(uart_logs)
        return jsonify({'log': logs_str})
    else:
        return jsonify({'log': ''})

@app.route('/start_spam', methods=['POST'])
def start_spam():
    global spamming, spam_thread
    data = request.get_json()
    cycle = data.get('cycle', 5000)

    if ser:
        if cycle == 0:
            ser.write(b'CAN_ATTACK_FRAME\n')
        else:
            spamming = True

            def spam_loop():
                while spamming:
                    if ser:
                        try:
                            # Dữ liệu mẫu cứng để gửi xuống (thay thế DB)
                            model = 'Standard'
                            model_byte = b'\x00'

                            can_id_hex = '1234'
                            can_id_bytes = bytes.fromhex(can_id_hex)

                            data_str = 'DATA'
                            data_bytes = data_str.encode('utf-8')
                            data_len_byte = len(data_bytes).to_bytes(1, 'big')

                            cyclic_bytes = int(1000).to_bytes(2, 'big')

                            frame = model_byte + can_id_bytes + data_len_byte + data_bytes + cyclic_bytes
                            ser.write(frame)
                            print("Spam frame sent:", frame.hex())
                        except Exception as e:
                            print("[Spam Error]", e)
                    time.sleep(cycle / 1000.0)

            spam_thread = threading.Thread(target=spam_loop, daemon=True)
            spam_thread.start()
    return '', 204

@app.route('/stop_spam', methods=['POST'])
def stop_spam():
    global spamming
    spamming = False
    return '', 204

if __name__ == '__main__':
    app.run(debug=True)
