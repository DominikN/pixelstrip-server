#! python3 .
# Control LED strip over websocket example

# eg. use that command to program 3 themes in NVM memory: $python3.7 strip_prog.py esp32strip 150 -themes "autumn" "hsv" "Reds"

import asyncio
import websockets
import numpy as np 
import sys
import argparse
import json

import matplotlib.pyplot as plt

from array import *


NUMPIXELS = 150

def cmap_tab(theme):
	cmap = plt.get_cmap(theme)
	
	colors_i = np.linspace(0, 1., (int)((args.numpixel)/2))
	
	# convert float to uint8
	tab = np.uint8(255 * cmap(colors_i))
	
	# delete white channel
	tab = np.delete(tab, np.arange(3, tab.size, 4))
	
	# create mirrored pixels
	tab2 = np.reshape(tab,(-1,3))
	tab2 = np.flip(tab2,0)
	tab2 = tab2.flatten()
	
	# connect original and mirrored pixel table
	tab = np.concatenate((tab, tab2), axis=0)
	
	# delete NUMPIXEL+1'st pixel
	# tab = np.delete(tab, [0,1,2], 0)
	
	return tab

async def mainfunc():
	uri = "ws://" + args.hostname + ":8000/ws"
	print("connecting: " + uri)
	async with websockets.connect(uri) as websocket:
		#print("clear NVM")
		#msg = json.dumps({"clear": 1})
		#print(msg)
		#await websocket.send(msg)
		#await asyncio.sleep(0.5)
		
		for i in range(len(args.themes)):
			msg = json.dumps({"save":{"theme":args.themes[i], "numpixels":args.numpixel, "numframes":args.numpixel}})
			print(msg)
			await websocket.send(msg)
		
			a = cmap_tab(args.themes[i])

			print("pixel table:")
			print(a)
			print("programming sequence: ")
		
			for x in range(args.numpixel):	
				await websocket.send(a.tobytes())
				await asyncio.sleep(0.1)
			
				a = np.roll(a,3)
			
				sys.stdout.write('.')
				sys.stdout.flush()
		
			print("done")
		msg = json.dumps({"trigger":{"mode":"infinite", "theme":args.themes[0], "delay":50}})
		print(msg)
		await websocket.send(msg)
		print("done")
		
			
parser = argparse.ArgumentParser()

parser.add_argument("hostname", nargs='?', default="esp32strip")

parser.add_argument("numpixel", nargs='?', default=150, type=int, help="number of pixels in your pixel strip")

parser.add_argument("-themes", nargs='+', default="autumn", help="Themes to be displayed on a pixel strip. Available themes are listed here: https://matplotlib.org/3.1.0/tutorials/colors/colormaps.html . Default = autumn")

args = parser.parse_args()

print("hostname: ", args.hostname)
print("numpixel: ", args.numpixel)
print("themes: ", args.themes)

try:
	asyncio.get_event_loop().run_until_complete(mainfunc())
except KeyboardInterrupt:
    print("\r\nReceived exit, exiting")
