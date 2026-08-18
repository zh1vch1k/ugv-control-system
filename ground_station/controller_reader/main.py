import pygame
import time
import serial
import struct

LEFT_X, LEFT_Y, RIGHT_X, RIGHT_Y= 0, 1, 2 ,3
LEFT_TRIGGER, RIGHT_TRIGGER = 4, 5
LEFT_STICK_TAP, RIGHT_STICK_TAP  = 7, 8
L1_BTN, R1_BTN = 9, 10

pygame.init()
pygame.joystick.init()
joystick = pygame.joystick.Joystick(0)
joystick.init()


ser = serial.Serial('/dev/cu.usbserial-595D0075621', 115200, timeout=1)
time.sleep(2)
polling_rate = 0.150

def scale_axis(axis) -> int:
    return max(0, min(127, round((axis + 1) /2 * 127)))

def scale_trigger(trigger) -> int:
    return max(0, min(15, round((trigger + 1) / 2 * 15)))

START_BYTE = 0xAA
cnt = 0
while True:
    btn_mask = 0
    for event in pygame.event.get():
        if event.type == pygame.JOYBUTTONDOWN:
            if event.button == LEFT_STICK_TAP:
                btn_mask |= (1 << 0)
            elif event.button == RIGHT_STICK_TAP:
                btn_mask |= (1 << 1)
            elif event.button == L1_BTN:
                btn_mask |= (1 << 2)
            elif event.button == R1_BTN:
                btn_mask |= (1 << 3)


    left_x = scale_axis(joystick.get_axis(LEFT_X))
    left_y = scale_axis(joystick.get_axis(LEFT_Y))
    right_x = scale_axis(joystick.get_axis(RIGHT_X))
    right_y = scale_axis(joystick.get_axis(RIGHT_Y))

    l_trigger = scale_trigger(joystick.get_axis(LEFT_TRIGGER))
    r_trigger = scale_trigger(joystick.get_axis(RIGHT_TRIGGER))

    packet = struct.pack("BBBBBBBB",
                START_BYTE,
                left_x, left_y, right_x, right_y,
                l_trigger, r_trigger,
                btn_mask)
    print(f'Sent packet: {cnt}')
    cnt += 1
    ser.write(packet)
    time.sleep(polling_rate)
