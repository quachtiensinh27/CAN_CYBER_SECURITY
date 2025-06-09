from flask import Flask, render_template, request, jsonify
import serial
import serial.tools.list_ports
import mysql.connector
import threading

app = Flask(__name__)

db_config = {
    'host': 'localhost',
    'user': 'root',
    'password': '',
    'database': 'id_des'
}

uart_port = None
uart_baudrate = 115200
ser = None
receive_thread = None
receive_running = False

def connect_uart(port, baudrate):
    global ser, receive_running, receive_thread
    if ser and ser.is_open:
        ser.close()
    ser = serial.Serial(port, baudrate, timeout=1)
    receive_running = True
    receive_thread = threading.Thread(target=uart_receive_loop)
    receive_thread.start()

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

                # 5. Read attack flash
                attack_flash_byte = ser.read(1)
                if not attack_flash_byte:
                    continue
                attack_flash = attack_flash_byte.hex().upper()

                # Decode data
                try:
                    data = data_bytes.decode('ascii', errors='replace')
                except Exception:
                    data = data_bytes.hex().upper()

                print(f"[UART Frame] mode={mode}, can_id={can_id}, data={data}, attack_flash={attack_flash}")

                # Save to DB
                conn = mysql.connector.connect(**db_config)
                cursor = conn.cursor()
                cursor.execute("SELECT description FROM can WHERE can_id = %s", (can_id,))
                result = cursor.fetchone()
                description = result[0] if result else ''
                cursor.execute("""
                    INSERT INTO receive (model, can_id, data, description, direction, timestamp)
                    VALUES (%s, %s, %s, %s, 'Rx', NOW())
                """, (mode, can_id, data, description))
                conn.commit()
                cursor.close()
                conn.close()

        except Exception as e:
            print("[UART Error]", e)

@app.route('/uart_status')
def uart_status():
    global ser
    return jsonify({'connected': ser is not None and ser.is_open})

@app.route('/')
def index():
    conn = mysql.connector.connect(**db_config)
    cursor = conn.cursor()
    cursor.execute("SELECT can_id FROM can")
    can_ids = [row[0] for row in cursor.fetchall()]

    cursor.execute("SELECT id, model, can_id, data, description, direction, cyclic FROM transmit")
    transmit_rows = cursor.fetchall()

    #cursor.execute("SELECT id, timestamp, model, can_id, data, description, direction FROM receive")
    cursor.execute("SELECT id, timestamp, model, can_id, data, description, direction FROM receive ORDER BY timestamp DESC")

    receive_rows = cursor.fetchall()

    cursor.close()
    conn.close()

    ports = [port.device for port in serial.tools.list_ports.comports()]
    baudrates = [115200, 19200, 38400, 57600, 9600]

    return render_template('index.html', ports=ports, baudrates=baudrates,
                           can_ids=can_ids, transmit_rows=transmit_rows, receive_rows=receive_rows)

@app.route('/connect_uart', methods=['POST'])
def connect_uart_route():
    global uart_port, uart_baudrate
    uart_port = request.form['port']
    uart_baudrate = int(request.form['baudrate'])
    connect_uart(uart_port, uart_baudrate)
    return jsonify({'status': 'connected'})

@app.route('/add_transmit', methods=['POST'])
def add_transmit():
    can_id = request.form['can_id']
    data = request.form['data'][:256]
    cyclic = int(request.form['cyclic'])

    conn = mysql.connector.connect(**db_config)
    cursor = conn.cursor()
    cursor.execute("SELECT model, description FROM can WHERE can_id = %s", (can_id,))
    result = cursor.fetchone()
    if result:
        model, description = result
        cursor.execute("""
            INSERT INTO transmit (model, can_id, data, description, direction, cyclic)
            VALUES (%s, %s, %s, %s, 'Tx', %s)
        """, (model, can_id, data, description, cyclic))
        conn.commit()
    cursor.close()
    conn.close()
    return jsonify({'status': 'added'})

@app.route('/send_frame/<int:transmit_id>', methods=['POST'])
def send_frame(transmit_id):
    conn = mysql.connector.connect(**db_config)
    cursor = conn.cursor(dictionary=True)
    cursor.execute("SELECT * FROM transmit WHERE id = %s", (transmit_id,))
    row = cursor.fetchone()
    if row:
        model = row['model']
        model_byte = b'\x00' if model == 'Standard' else b'\x01'
        can_id_bytes = bytes.fromhex(row['can_id'].zfill(8)) if model == 'Extended' else bytes.fromhex(row['can_id'].zfill(4))
        data_bytes = row['data'].encode('utf-8')
        data_len_byte = len(data_bytes).to_bytes(1, 'big')
        cyclic_bytes = int(row['cyclic']).to_bytes(2, 'big')

        frame = model_byte + can_id_bytes + data_len_byte + data_bytes + cyclic_bytes
        if ser and ser.is_open:
            try:
                ser.write(frame)
                print("Frame sent:", frame.hex())
            except Exception as e:
                print("UART Send Error:", e)
        else:
            print("UART not connected.")

    cursor.close()
    conn.close()
    return jsonify({'status': 'sent'})

@app.route('/delete_transmit/<int:transmit_id>', methods=['POST'])
def delete_transmit(transmit_id):
    conn = mysql.connector.connect(**db_config)
    cursor = conn.cursor()
    cursor.execute("DELETE FROM transmit WHERE id = %s", (transmit_id,))
    conn.commit()
    cursor.close()
    conn.close()
    return jsonify({'status': 'deleted'})

@app.route('/delete_receive/<int:receive_id>', methods=['POST'])
def delete_receive(receive_id):
    conn = mysql.connector.connect(**db_config)
    cursor = conn.cursor()
    cursor.execute("DELETE FROM receive WHERE id = %s", (receive_id,))
    conn.commit()
    cursor.close()
    conn.close()
    return jsonify({'status': 'deleted'})

@app.route('/edit_transmit/<int:transmit_id>', methods=['POST'])
def edit_transmit(transmit_id):
    data = request.form['data'][:256]
    cyclic = int(request.form['cyclic'])
    conn = mysql.connector.connect(**db_config)
    cursor = conn.cursor()
    cursor.execute("UPDATE transmit SET data=%s, cyclic=%s WHERE id=%s", (data, cyclic, transmit_id))
    conn.commit()
    cursor.close()
    conn.close()
    return jsonify({'status': 'updated'})

if __name__ == '__main__':
    app.run(debug=True)

