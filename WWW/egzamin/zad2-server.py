from fastapi import FastAPI, WebSocket
from fastapi.responses import HTMLResponse
import asyncio
import random

app = FastAPI()

@app.get("/current")
async def read_current():
    global current
    return {"current": current}

current = 100
listeners = []

async def my_background_task():
    global current
    while True:
        await asyncio.sleep(1)
        if random.random() < 0.5:
            current *= 1.1
            await notify_listeners(1.1, "multiply")
        else:
            current /= 1.1
            await notify_listeners(1.1, "divide")

async def notify_listeners(value: float, method: str):
    for listener in listeners:
        await listener.send_json({"value": value, "method": method})

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    listeners.append(websocket)
    try:
        while True:
            # Tutaj można obsłużyć przychodzące wiadomości, ale w tym przypadku po prostu czekamy
            await websocket.receive_text()
    except Exception as e:
        # Obsługa błędów, np. gdy klient się rozłączy
        pass
    finally:
        listeners.remove(websocket)


@app.on_event("startup")
async def start_background_tasks():
    asyncio.create_task(my_background_task())

@app.get("/", response_class=HTMLResponse)
async def read_root():
    # return zad2.html content
    return(open("zad2.html", "r").read())

@app.get("/zad2.js", response_class=HTMLResponse)
async def read_js():
    # return zad2.js content
    return(open("zad2.js", "r").read())