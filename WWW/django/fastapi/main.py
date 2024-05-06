from fastapi import FastAPI
from django_conn import DjangoConnection, Tag, Image, Optional
from typing import List

app = FastAPI()


@app.get("/tags")
def get_tags() -> List[Tag]:
    conn = DjangoConnection()
    return list(conn.get_tags())

@app.get("/images")
def get_images() -> List[Image]:
    conn = DjangoConnection()
    return list(conn.get_images())

@app.get("/tagged_images/{tag_id}")
def get_tagged_images(tag_id: int) -> List[Image]:
    conn = DjangoConnection()
    return list(conn.get_tagged_image(tag_id))

@app.delete("/delete_image/{image_id}")
def delete_image(image_id: int) -> Optional[Image]:
    conn = DjangoConnection()
    return conn.delete_image(image_id)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)