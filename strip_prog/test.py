#! python3 .
# Control LED strip over websocket example

import asyncio
import websockets
import numpy as np 
import sys
import argparse
import json

import matplotlib.pyplot as plt

from array import *

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

        # send theme command
        msg = json.dumps({"save":{"theme":args.theme, "numpixels":args.numpixel, "numframes":1}})
        print(msg)
        await websocket.send(msg)

        # send pixel table
        tab = cmap_tab(args.theme)
        print("pixel table:")
        print(tab)
        await websocket.send(tab.tobytes())
        print("done")

        # select theme
        msg = json.dumps({"trigger":{"mode":"infinite", "theme":args.theme, "delay":50}})
        print(msg)
        await websocket.send(msg)
        print("done")
        
		
			
parser = argparse.ArgumentParser()

parser.add_argument("hostname", nargs='?', default="esp32strip")
parser.add_argument("numpixel", nargs='?', default=150, type=int, help="number of pixels in your pixel strip")
parser.add_argument("theme", nargs='?', default="flag")

args = parser.parse_args()

print("hostname: ", args.hostname)
print("numpixel: ", args.numpixel)
print("theme: ", args.theme)

try:
	asyncio.get_event_loop().run_until_complete(mainfunc())
except KeyboardInterrupt:
    print("\r\nReceived exit, exiting")
