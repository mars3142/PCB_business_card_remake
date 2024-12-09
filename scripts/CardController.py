import asyncio
import requests
import os
import json
from bleak import BleakClient, BleakScanner
import keyboard
import struct

# requires a config.py file with API_KEY and CITY_NAME defined
from config import API_KEY, CITY_NAME  # Import API_KEY and CITY_NAME from config

CONFIG_FILE = "ble_device_config.json"
CHARACTERISTIC_UUID = "23408888-1f40-4cd8-9b89-ca8d45f8a5b0"  # Replace with your characteristic UUID

debug_printed = False

# stringify the temperature
def temp_to_string(temp):
    return f"T:{temp:.1f}"  # Rounds to one decimal place

async def get_weather_temperature():
    """Fetches the current temperature from OpenWeatherMap."""
    url = f"http://api.openweathermap.org/data/2.5/weather?q={CITY_NAME}&appid={API_KEY}&units=metric"
    response = requests.get(url)
    if response.status_code == 200:
        data = response.json()
        return data['main']['temp']  # Returns temperature in Celsius
    else:
        print("Error fetching weather data.")
        return None

# Function to generate or load screen data
def generate_screen_data(text):
    # Define the font for simple characters (only uppercase for simplicity)
    font = {
        # Font for letters A-Z (5x3 pixels each)
        'A': [0x7, 0x5, 0x7, 0x5, 0x5],
        'B': [0x7, 0x5, 0x7, 0x5, 0x7],
        'C': [0x7, 0x4, 0x4, 0x4, 0x7],
        'D': [0x7, 0x5, 0x5, 0x5, 0x7],
        'E': [0x7, 0x4, 0x7, 0x4, 0x7],
        'F': [0x7, 0x4, 0x7, 0x4, 0x4],
        'G': [0x7, 0x4, 0x4, 0x5, 0x7],
        'H': [0x5, 0x5, 0x7, 0x5, 0x5],
        'I': [0x7, 0x2, 0x2, 0x2, 0x7],
        'J': [0x7, 0x1, 0x1, 0x1, 0x7],
        'K': [0x5, 0x5, 0x7, 0x5, 0x5],
        'L': [0x4, 0x4, 0x4, 0x4, 0x7],
        'M': [0x5, 0x7, 0x7, 0x5, 0x5],
        'N': [0x5, 0x7, 0x7, 0x5, 0x5],
        'O': [0x7, 0x5, 0x5, 0x5, 0x7],
        'P': [0x7, 0x5, 0x7, 0x4, 0x4],
        'Q': [0x7, 0x5, 0x5, 0x5, 0x7],
        'R': [0x7, 0x5, 0x7, 0x5, 0x5],
        'S': [0x7, 0x4, 0x7, 0x1, 0x7],
        'T': [0x7, 0x2, 0x2, 0x2, 0x2],
        'U': [0x5, 0x5, 0x5, 0x5, 0x7],
        'V': [0x5, 0x5, 0x5, 0x5, 0x2],
        'W': [0x5, 0x5, 0x7, 0x7, 0x5],
        'X': [0x5, 0x5, 0x2, 0x5, 0x5],
        'Y': [0x5, 0x5, 0x2, 0x2, 0x2],
        'Z': [0x7, 0x1, 0x2, 0x4, 0x7],

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

        # Font for dot (5x3 pixels)
        '.': [0x00, 0x00, 0x00, 0x00, 0x02],

        # Font for double quote (5x3 pixels)
        '"': [0x02, 0x02, 0x00, 0x00, 0x00],

        # Font for colon (5x3 pixels)
        ':': [0x00, 0x02, 0x00, 0x02, 0x00],
    }


    screen_data = []
    text = text.upper()  # Convert to uppercase
    text = text[:15]  # Limit to 15 characters to fit the screen

    # Clear the screen with black pixels
    for y in range(8):
        for x in range(15):
            screen_data.append(struct.pack("<I", 0x00000000))  # Black pixel

    # print("Text:", text)

    # Render the text
    char_x = 0  # Start x position for the first character
    for char in text:
        if char in font:
            char_data = font[char]
            # print(f"Rendering character '{char}' at position {char_x}:")
            for y, byte in enumerate(char_data):
                # print(f"  Row {y}: {byte:08b}")
                for bit in range(3):
                    if byte & (1 << (2 - bit)):  # Reverse the bit order to fix mirroring
                        # Calculate the pixel position
                        pixel_x = char_x + bit
                        pixel_y = y
                        # Ensure pixel is within screen bounds
                        if 0 <= pixel_x < 15 and 0 <= pixel_y < 5:
                            index = pixel_y * 15 + pixel_x
                            # print(f"    Setting pixel at ({pixel_x}, {pixel_y}) to white (Index: {index})")
                            # Set the pixel to white
                            screen_data[index] = struct.pack("<I", 0xFFFFFF)  # White pixel
            char_x += 4  # Move to the next character position with a space
        else:
            print(f"Character '{char}' not found in font.")
            char_x += 4  # Move to the next character position with a space

    # Fill the remaining rows with black pixels
    for y in range(5, 8):
        for x in range(15):
            index = y * 15 + x
            screen_data[index] = struct.pack("<I", 0x00000000)  # Black pixel

    return screen_data

async def send_screen_data(client, screen_data):
    global debug_printed

    # # Print the screen data for debugging, but only once
    # if not debug_printed:
    #     print("Screen Data:")
    #     for i, pixel in enumerate(screen_data):
    #         if i % 15 == 0:
    #             print()
    #         print(f"{struct.unpack('<I', pixel)[0]:08x}", end=" ")
    #     print("\n")
    #     debug_printed = True

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

class TemperatureDisplay:
    def __init__(self):
        self.temp_string = ""
        self.scroll_index = 0

    async def display_temperature_task(self, client):
        while True:
            if keyboard.is_pressed('q'):

                break

            # Generate screen data based on the current temperature
            screen_data = generate_screen_data(self.temp_string[self.scroll_index:self.scroll_index + 4])

            # Send the screen data
            await send_screen_data(client, screen_data)

            # Update the scroll index
            self.scroll_index += 1

            if self.scroll_index >= len(self.temp_string):
                self.scroll_index = 0

            await asyncio.sleep(0.3)  # Scroll every 0.3 seconds

    async def update_temperature(self, client, update_interval):
        while True:
            # Get the current temperature
            temperature = await get_weather_temperature()
            if temperature is not None:
                self.temp_string = temp_to_string(temperature)
            await asyncio.sleep(update_interval)

async def main():
    ble_address = await get_ble_address()
    if not ble_address:
        print("No device selected. Exiting.")
        return

    print(f"Connecting to device: {ble_address}")

    try:
        async with BleakClient(ble_address) as client:
            print(f"Connected: {await client.is_connected()}")
            print("Press 'q' to quit.")

            temperature_display = TemperatureDisplay()
            display_task = asyncio.create_task(temperature_display.display_temperature_task(client))
            update_task = asyncio.create_task(temperature_display.update_temperature(client, 10))

            await asyncio.gather(display_task, update_task)

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    asyncio.run(main())