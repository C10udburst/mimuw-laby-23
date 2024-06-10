import asyncio
import websockets
import random
import json
import sys
from dataclasses import dataclass

random_tags = ["tag1", "tag2", "tag3", "tag4", "tag5", "abc", "def", "ghi", "jkl", "mno", "aaa", "bbb", "ccc", "ddd", "eee", "fff", "ggg", "hhh", "iii"]

@dataclass
class Rect:
    x: int
    y: int
    w: int
    h: int
    color: str = "#000000"
    
    def to_json(self):
        return {
            "x": self.x,
            "y": self.y,
            "w": self.w,
            "h": self.h,
            "color": self.color
        }
    
    @staticmethod
    def random(w, h):
        x = random.randint(0, w - 1)
        y = random.randint(0, h - 1)
        w = random.randint(1, w - x)
        h = random.randint(1, h - y)
        color = f"#{random.randint(0, 0xFFFFFF):06X}"
        return Rect(x, y, w, h, color)

@dataclass
class Image:
    w: int
    h: int
    rects: list[Rect]
    tags: list[str]

    @staticmethod
    def random(w, h, n):
        return Image(w, h, [Rect.random(w, h) for _ in range(n)], random.choices(random_tags, k=3))
    
    def to_json(self):
        return {
            "w": self.w,
            "h": self.h,
            "rects": [rect.to_json() for rect in self.rects]
        }

images = [Image.random(800, 600, 10) for _ in range(27)]

    
async def recv(websocket, path):
    msg = await websocket.recv()
    print(f"{websocket.remote_address}: {msg}")
    if msg == "tags":
        tagset = set()
        for img in images:
            tagset.update(img.tags)
        await websocket.send(json.dumps(list(tagset)))
        return
    if msg == "list":
        await websocket.send(json.dumps(list(range(len(images)))))
        return
    if msg.startswith("tag:"):
        tag = msg[4:]
        await websocket.send(json.dumps([i for i, img in enumerate(images) if tag in img.tags]))
        return
    try:
        int_msg = int(msg)
        if int_msg < 0 or int_msg >= len(images):
            raise ValueError
        await asyncio.sleep(2.137)
        await websocket.send(json.dumps(images[int_msg].to_json()))
    except ValueError:
        await websocket.send("Invalid command")

async def recv_stdin(ws, reader):
    cmd = await reader.readline()
    cmd = cmd.decode().strip()
    if cmd == "exit":
        sys.exit(0)
    elif cmd == "img":
        images.append(Image.random(800, 600, 10))
        i = len(images) - 1
        await ws.send(str(i))
        
    
    
async def main(websocket, path):
    print(f"conn from {websocket.remote_address}: {path}")
    if path == "/notify":
        loop = asyncio.get_event_loop()
        reader = asyncio.StreamReader()
        protocol = asyncio.StreamReaderProtocol(reader)
        await loop.connect_read_pipe(lambda: protocol, sys.stdin)
        while True:
            await recv_stdin(websocket, reader)
    else:
        await recv(websocket, path)
        await websocket.close()
    
    

if __name__ == "__main__":
    start_server = websockets.serve(main, "localhost", 8765)
    print("Server started at ws://localhost:8765")
    asyncio.get_event_loop().run_until_complete(start_server)
    asyncio.get_event_loop().run_forever()