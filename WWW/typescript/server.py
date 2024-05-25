from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.websockets import WebSocket
from fastapi.responses import FileResponse
from random import uniform
from asyncio import sleep, Semaphore

# host /ws as a websocket route

app = FastAPI()

new_image = Semaphore(0)
new_image_name = ""

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    await sleep(1)
    while True:
        data = await websocket.receive_text()
        await sleep(1)
        cmd, *args = data.split(" ")
        if uniform(0, 1) < 0:
            # simulate an error
            await websocket.send_text("Error: Random error occurred")
            return
            
        if cmd == "save":
            key, *value = args
            global new_image_name
            with open(f"./data/{key}", "w") as f:
                f.write(" ".join(value))
            new_image_name = key
            new_image.release()
        elif cmd == "load":
            key = args[0]
            with open(f"./data/{key}", "r") as f:
                await websocket.send_text(f.read())

@app.websocket("/ws2")
async def websocket_endpoint2(websocket: WebSocket):
    await websocket.accept()
    if await websocket.receive_text() != "subscribe":
        return
    await new_image.acquire()
    global new_image_name
    with open(f"./data/{new_image_name}", "r") as f:
        await websocket.send_text(f.read())

@app.get("/")
async def main():
    return FileResponse("./public/index.html")

@app.get("/{file_path}")
async def main(file_path: str):
    if file_path == "favicon.ico":
        return ""
    return FileResponse(f"./public/{file_path}")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)