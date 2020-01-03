#! python3 .
# Control LED strip over websocket example

import asyncio
import websockets
import numpy as np 
import sys
import argparse
import json

from array import *

async def mainfunc():
	uri = "ws://" + args.hostname + ":8001"
	print("connecting: " + uri)
	async with websockets.connect(uri) as websocket:
		print("clear NVM")
		msg = json.dumps({"clear": 1})
		print(msg)
		await websocket.send(msg)
		await asyncio.sleep(0.5)

		theme = np.zeros((3*args.numpixel,), dtype=int)
		print(theme)
		msg = json.dumps({"save":{"theme":"off", "numpixels":args.numpixel, "numframes":1}})
		print(msg)
		await websocket.send(msg)
		await websocket.send(theme.tobytes())
		print("done")
		await asyncio.sleep(0.5)

		theme = np.ones((3*args.numpixel,), dtype=int)
		tab = np.uint8(255 * theme)
		print(tab)
		msg = json.dumps({"save":{"theme":"white", "numpixels":args.numpixel, "numframes":1}})
		print(msg)
		await websocket.send(msg)
		await websocket.send(tab.tobytes())
		print("done")
		
		
			
parser = argparse.ArgumentParser()

parser.add_argument("hostname", nargs='?', default="esp32strip")
parser.add_argument("numpixel", nargs='?', default=150, type=int, help="number of pixels in your pixel strip")

args = parser.parse_args()

print("hostname: ", args.hostname)
print("numpixel: ", args.numpixel)

try:
	asyncio.get_event_loop().run_until_complete(mainfunc())
except KeyboardInterrupt:
    print("\r\nReceived exit, exiting")
