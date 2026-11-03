import time
import struct
import pygame
import serial
import serial.tools.list_ports

LEFT_X, LEFT_Y, RIGHT_X, RIGHT_Y = 0, 1, 2, 3
LEFT_TRIGGER, RIGHT_TRIGGER = 4, 5
LEFT_STICK_TAP, RIGHT_STICK_TAP = 7, 8
L1_BTN, R1_BTN = 9, 10

START_BYTE = 0xAA

# Скорость передачи согласована с физической пропускной способностью LoRa (Ebyte 2.4 kbps, ~110-120мс Time-on-Air)
# Это устраняет рассинхронизацию между приемом RS-485 и отправкой в радиоэфир.
POLLING_INTERVAL = 0.120  # 120 мс (~8.3 Гц)

def find_serial_port() -> str:
    for p in serial.tools.list_ports.comports():
        if "usbserial" in p.device or "ttyusb" in p.device.lower():
            return p.device
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        raise RuntimeError("Serial ports haven't been found!")
    return ports[0].device

def scale_axis(axis: float) -> int:
    return max(0, min(127, round((axis + 1.0) / 2.0 * 127)))

def scale_trigger(trigger: float) -> int:
    return max(0, min(15, round((trigger + 1.0) / 2.0 * 15)))

def make_neutral_packet() -> bytes:
    """Генерация нейтрального пакета (стоп-кадр: стики по центру 64, курки 0, кнопки 0)"""
    return struct.pack("BBBBBBBB", START_BYTE, 64, 64, 64, 64, 0, 0, 0)

def send_failsafe_stop(ser: serial.Serial, count: int = 3):
    """Отправка аварийного стоп-пакета несколько раз для гарантированной остановки UGV"""
    stop_pkt = make_neutral_packet()
    for _ in range(count):
        try:
            ser.write(stop_pkt)
            ser.flush()
            time.sleep(0.02)
        except Exception as e:
            print(f"[FAILSAFE] Error sending stop packet: {e}")
            break
    print("[FAILSAFE] Neutral emergency stop sent to UGV.")

def wait_for_joystick():
    """Ожидание подключения джойстика"""
    print("[WAIT] Waiting for gamepad connection...")
    while True:
        pygame.event.pump()
        if pygame.joystick.get_count() > 0:
            try:
                js = pygame.joystick.Joystick(0)
                js.init()
                print(f"[INFO] Gamepad reconnected: {js.get_name()}")
                return js
            except pygame.error:
                pass
        time.sleep(0.5)

def main():
    pygame.init()
    pygame.joystick.init()
    
    js = None
    if pygame.joystick.get_count() > 0:
        js = pygame.joystick.Joystick(0)
        js.init()
        print(f"Gamepad ready: {js.get_name()}")
    else:
        js = wait_for_joystick()

    port = find_serial_port()
    print(f"Connection to RS-485: {port}")
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(2)

    latched_buttons = 0
    cnt = 0

    try:
        while True:
            t_start = time.perf_counter()
            joystick_disconnected = False

            # 1. Проверка событий Pygame
            for event in pygame.event.get():
                if event.type == pygame.JOYDEVICEREMOVED:
                    print("[WARNING] Gamepad disconnected event detected!")
                    joystick_disconnected = True
                elif event.type == pygame.JOYBUTTONDOWN:
                    if event.button == LEFT_STICK_TAP:
                        latched_buttons |= (1 << 0)
                    elif event.button == RIGHT_STICK_TAP:
                        latched_buttons |= (1 << 1)
                    elif event.button == L1_BTN:
                        latched_buttons |= (1 << 2)
                    elif event.button == R1_BTN:
                        latched_buttons |= (1 << 3)

            # 2. Проверка доступности контроллера
            if pygame.joystick.get_count() == 0 or joystick_disconnected:
                send_failsafe_stop(ser, count=4)
                js = wait_for_joystick()
                latched_buttons = 0
                continue

            # 3. Считывание осей с обработкой ошибок отключения
            try:
                lx = scale_axis(js.get_axis(LEFT_X))
                ly = scale_axis(js.get_axis(LEFT_Y))
                rx = scale_axis(js.get_axis(RIGHT_X))
                ry = scale_axis(js.get_axis(RIGHT_Y))

                l_trig = scale_trigger(js.get_axis(LEFT_TRIGGER))
                r_trig = scale_trigger(js.get_axis(RIGHT_TRIGGER))
            except pygame.error as e:
                print(f"[ERROR] Failed to read axes: {e}")
                send_failsafe_stop(ser, count=4)
                js = wait_for_joystick()
                latched_buttons = 0
                continue

            # 4. Формирование и передача пакета
            packet = struct.pack(
                "BBBBBBBB",
                START_BYTE,
                lx, ly, rx, ry,
                l_trig, r_trig,
                latched_buttons
            )

            ser.write(packet)
            ser.flush()

            latched_buttons = 0

            if cnt % 20 == 0:
                print(f"[{cnt}] L2:{l_trig:2d} | R2:{r_trig:2d} | Axis: ({lx},{ly}) | Rate: {1.0/POLLING_INTERVAL:.1f} Hz")
            cnt += 1

            dt = POLLING_INTERVAL - (time.perf_counter() - t_start)
            if dt > 0:
                time.sleep(dt)

    except KeyboardInterrupt:
        print("\nStopping...")
        send_failsafe_stop(ser, count=3)
    finally:
        ser.close()
        pygame.quit()

if __name__ == "__main__":
    main()