"""Focused Moonlight desktop input and physical-pixel captures for live tests."""
import argparse
import ctypes
from ctypes import wintypes
from pathlib import Path
import time
from PIL import ImageGrab

u = ctypes.windll.user32
u.SetProcessDPIAware()


def window():
    windows = []
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    @callback_type
    def callback(hwnd, _):
        title = ctypes.create_unicode_buffer(512)
        u.GetWindowTextW(hwnd, title, len(title))
        if u.IsWindowVisible(hwnd) and title.value.endswith(' - Moonlight'):
            windows.append(hwnd)
        return True
    u.EnumWindows(callback, 0)
    if len(windows) != 1:
        raise RuntimeError(f'Expected one Moonlight stream, found {len(windows)}')
    return windows[0]


def key(vk, down=True):
    extended = 1 if vk in (0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x5b) else 0
    u.keybd_event(vk, u.MapVirtualKeyW(vk, 0), extended | (0 if down else 2), 0)


def chord(keys):
    try:
        for vk in keys:
            key(vk)
        time.sleep(.08)
    finally:
        for vk in reversed(keys):
            key(vk, False)


def type_ascii(value):
    for char in value:
        mapped = u.VkKeyScanW(ord(char))
        if mapped == -1:
            raise ValueError('Only keyboard-mappable text is supported')
        mods = []
        for bit, vk in ((1, 0x10), (2, 0x11), (4, 0x12)):
            if (mapped >> 8) & bit:
                mods.append(vk)
        chord(mods + [mapped & 255])
        time.sleep(.015)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('action', choices=['capture', 'chord', 'type', 'click', 'doubleclick'])
    parser.add_argument('value')
    parser.add_argument('--wait', type=float, default=.3)
    args = parser.parse_args()
    hwnd = window()
    u.ShowWindow(hwnd, 9)
    u.SetForegroundWindow(hwnd)
    time.sleep(.15)
    if u.GetForegroundWindow() != hwnd:
        raise RuntimeError('Moonlight did not acquire foreground focus')
    if args.action == 'chord':
        chord([int(v, 0) for v in args.value.split('+')])
    elif args.action == 'type':
        type_ascii(args.value)
    elif args.action in ('click', 'doubleclick'):
        x, y = map(int, args.value.split(','))
        rect = wintypes.RECT()
        u.GetWindowRect(hwnd, ctypes.byref(rect))
        if not (0 <= x < rect.right - rect.left and 0 <= y < rect.bottom - rect.top):
            raise ValueError('Click is outside the observed Moonlight window')
        u.SetCursorPos(rect.left + x, rect.top + y)
        for _ in range(2 if args.action == 'doubleclick' else 1):
            u.mouse_event(0x0002, 0, 0, 0, 0)
            time.sleep(.04)
            u.mouse_event(0x0004, 0, 0, 0, 0)
            time.sleep(.08)
    time.sleep(args.wait)
    if args.action == 'capture':
        rect = wintypes.RECT()
        u.GetWindowRect(hwnd, ctypes.byref(rect))
        path = Path(args.value)
        path.parent.mkdir(parents=True, exist_ok=True)
        ImageGrab.grab(bbox=(rect.left, rect.top, rect.right, rect.bottom)).save(path)
        print(f'{path}: {rect.right - rect.left}x{rect.bottom - rect.top}')


if __name__ == '__main__':
    main()
