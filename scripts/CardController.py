import asyncio
import os
import json
from bleak import BleakClient, BleakScanner
import keyboard
import struct

CONFIG_FILE = "ble_device_config.json"
CHARACTERISTIC_UUID = "23408888-1f40-4cd8-9b89-ca8d45f8a5b0"  # Replace with your characteristic UUID

debug_printed = False

# Function to generate or load screen data
def generate_screen_data(text):
    # Define the font for simple characters (only uppercase for simplicity)
    font = {
        'A': [0x7E, 0x09, 0x09, 0x09, 0x7E],
        'B': [0xFF, 0x49, 0x49, 0x49, 0xFF],
        'C': [0x7E, 0x40, 0x40, 0x40, 0x7E],
        'D': [0xFF, 0x41, 0x41, 0x41, 0xFF],
        'E': [0xFF, 0x40, 0x40, 0x40, 0xFF],
        'F': [0xFF, 0x40, 0x40, 0x40, 0x00],
        'G': [0x7E, 0x40, 0x50, 0x40, 0x7E],
        'H': [0x99, 0x99, 0xFF, 0x99, 0x99],
        'I': [0xFF, 0x20, 0x20, 0x20, 0xFF],
        'J': [0x10, 0x10, 0x10, 0x7F, 0x10],
        'K': [0x99, 0xA4, 0xB2, 0xC9, 0xD6],
        'L': [0x40, 0x40, 0x40, 0x40, 0xFF],
        'M': [0x99, 0xA3, 0xA5, 0xA5, 0x99],
        'N': [0x99, 0xA1, 0xA2, 0xA4, 0x99],
        'O': [0x7E, 0x09, 0x09, 0x09, 0x7E],
        'P': [0xFF, 0x49, 0x49, 0x49, 0x41],
        'Q': [0x7E, 0x09, 0x19, 0x29, 0x46],
        'R': [0xFF, 0x49, 0x49, 0x49, 0xC3],
        'S': [0x7E, 0x40, 0x40, 0x40, 0x7E],
        'T': [0xFF, 0x20, 0x20, 0x20, 0x20],
        'U': [0x99, 0x99, 0x99, 0x99, 0x7F],
        'V': [0x99, 0x99, 0xA2, 0xA2, 0x81],
        'W': [0x99, 0x99, 0xA5, 0xA5, 0x99],
        'X': [0x99, 0xA2, 0x24, 0x24, 0xA2],
        'Y': [0x99, 0xA2, 0x24, 0x24, 0x12],
        'Z': [0xFF, 0x20, 0x60, 0xE0, 0xFF],
        ' ': [0x00, 0x00, 0x00, 0x00, 0x00],

        # Font for digits 0-9 (5x3 pixels each)
        '0': [0x7, 0x5, 0x5, 0x5, 0x7],
        '1': [0x2, 0x2, 0x2, 0x2, 0x2],
        '2': [0x7, 0x1, 0x7, 0x4, 0x7],
        '3': [0x7, 0x1, 0x7, 0x1, 0x7],
        '4': [0x5, 0x5, 0x7, 0x1, 0x1],
        '5': [0x7, 0x4, 0x7, 0x1, 0x7],
        '6': [0x7, 0x4, 0x7, 0x5, 0x7],
        '7': [0x7, 0x1, 0x1, 0x1, 0x1],
        '8': [0x7, 0x5, 0x7, 0x5, 0x7],
        '9': [0x7, 0x5, 0x7, 0x1, 0x7],
    }

    screen_data = []
    text = text.upper()  # Convert to uppercase
    text = text[:15]  # Limit to 15 characters to fit the screen

    # Clear the screen with black pixels
    for y in range(8):
        for x in range(15):
            screen_data.append(struct.pack("<I", 0x00000000))  # Black pixel

    print("Screen Dimensions: 15x8")
    print("Text:", text)

    # Render the text
    for char_index, char in enumerate(text):
        if char in font:
            char_data = font[char]
            print(f"Rendering character '{char}' at position {char_index}:")
            char_x = char_index * 3  # Start x position for this character (3 pixels wide)
            for y, byte in enumerate(char_data):
                print(f"  Row {y}: {byte:08b}")
                for bit in range(3):
                    if byte & (1 << (2 - bit)):  # Reverse the bit order to fix mirroring
                        # Calculate the pixel position
                        pixel_x = char_x + bit
                        pixel_y = y
                        # Ensure pixel is within screen bounds
                        if 0 <= pixel_x < 15 and 0 <= pixel_y < 5:
                            index = pixel_y * 15 + pixel_x
                            print(f"    Setting pixel at ({pixel_x}, {pixel_y}) to white (Index: {index})")
                            # Set the pixel to white
                            screen_data[index] = struct.pack("<I", 0xFFFFFF)  # White pixel
                        else:
                            print(f"    Pixel at ({pixel_x}, {pixel_y}) is out of bounds.")
        else:
            print(f"Character '{char}' not found in font.")

    # Fill the remaining rows with black pixels
    for y in range(5, 8):
        for x in range(15):
            index = y * 15 + x
            screen_data[index] = struct.pack("<I", 0x00000000)  # Black pixel

    return screen_data

async def send_screen_data(client, screen_data):
    global debug_printed

    # Print the screen data for debugging, but only once
    if not debug_printed:
        print("Screen Data:")
        for i, pixel in enumerate(screen_data):
            if i % 15 == 0:
                print()
            print(f"{struct.unpack('<I', pixel)[0]:08x}", end=" ")
        print("\n")
        debug_printed = True

    chunk_size = 120  # Adjust based on BLE MTU
    for i in range(0, len(screen_data), chunk_size):
        chunk = b''.join(screen_data[i:i+chunk_size])
        await client.write_gatt_char(CHARACTERISTIC_UUID, chunk)

async def select_device():
    devices = await BleakScanner.discover()
    if not devices:
        print("No BLE devices found.")
        return None

    print("Available BLE devices:")
    for i, device in enumerate(devices):
        print(f"{i}: {device.name} - {device.address}")

    while True:
        try:
            selection = int(input("Select device by number: "))
            if 0 <= selection < len(devices):
                return devices[selection].address
            else:
                print("Invalid selection. Please try again.")
        except ValueError:
            print("Please enter a valid number.")

def load_config():
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r") as f:
            return json.load(f)
    return {}

def save_config(config):
    with open(CONFIG_FILE, "w") as f:
        json.dump(config, f)

async def get_ble_address():
    config = load_config()
    saved_address = config.get("ble_address")

    if saved_address:
        print(f"Previously connected to device with address: {saved_address}")
        choice = input("Do you want to connect to the same device? (y/n): ").lower()
        if choice == 'y':
            return saved_address

    new_address = await select_device()
    if new_address:
        config["ble_address"] = new_address
        save_config(config)
    return new_address

async def main():
    ble_address = await get_ble_address()
    if not ble_address:
        print("No device selected. Exiting.")
        return

    print(f"Connecting to device: {ble_address}")
    async with BleakClient(ble_address) as client:
        print(f"Connected: {await client.is_connected()}")
        print("Press 'q' to quit.")

        text = "123"
        screen_data = generate_screen_data(text)

        while True:
            if keyboard.is_pressed('q'):
                print("Quitting...")
                break

            await send_screen_data(client, screen_data)

if __name__ == "__main__":
    asyncio.run(main())