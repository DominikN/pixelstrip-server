#! python3 .
# Control LED strip over websocket example

import asyncio
import websockets
import sys
import argparse
import json

async def mainfunc():
	uri = "ws://" + args.hostname + ":8000/ws"
	print("connecting: " + uri)
	async with websockets.connect(uri) as websocket:
		msg = json.dumps({"trigger":{"mode":args.mode, "theme":args.theme, "delay":args.delay}})
		print(msg)
		await websocket.send(msg)
		print("done")
		
			
parser = argparse.ArgumentParser()

parser.add_argument("hostname", nargs='?', default="esp32strip")
parser.add_argument("mode", nargs='?', default="infinite")
parser.add_argument("theme", nargs='?', default="autumn")
parser.add_argument("delay", nargs='?', default=50, type=int,)

args = parser.parse_args()

print("hostname: ", args.hostname)
print("mode: ", args.mode)
print("theme: ", args.theme)
print("delay: ", args.delay)

try:
	asyncio.get_event_loop().run_until_complete(mainfunc())
except KeyboardInterrupt:
    print("\r\nReceived exit, exiting")
