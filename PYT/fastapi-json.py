import uvicorn
from fastapi import FastAPI, HTTPException
from starlette.responses import StreamingResponse
from pydantic import BaseModel, Field

app = FastAPI()

class JsonRequest(BaseModel):
    filename: str = Field(pattern=r"^[a-zA-Z0-9 ._-]+$") # regex na nazwę pliku, zapobiega path traversal vulnerability
    
@app.post("/json")
def json(request: JsonRequest):
    try:
        return StreamingResponse(open(f"{request.filename}.json", "rb"), media_type="application/json")
    except:
        raise HTTPException(status_code=404, detail="File not found")
    
if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
