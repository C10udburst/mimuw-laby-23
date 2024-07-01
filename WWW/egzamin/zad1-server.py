"""
Tego pliku nie należy ruszać

Powinno się go dać uruchomić za pomocą

fastapi run zad1-server.py --port 8001


"""

from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse
from faker import Faker
import asyncio

app = FastAPI()
faker = Faker()

template_str = """
<!DOCTYPE html>
<html>
<head>
    <title>Generated content</title>
</head>
<body>
  %s
</body>
</html>
"""


@app.get("/", response_class=HTMLResponse)
async def read_root(request: Request, seed: int | None = None):
    if seed:
        Faker.seed(seed)  # Set the seed for Faker
        await asyncio.sleep(10)
    else:
        Faker.seed(1337)
    content = ""
    paragraphs = []
    for _ in range(faker.random_int(2, 5)):
        sentences = []
        for _ in range(faker.random_int(3, 7)):  # Generate 3-7 sentences per paragraph
            sentence = faker.sentence()
            if faker.random_int(1, 5) == 1:  # 20% chance of adding a link
                local_seed = faker.random_int(1, 1000000000)
                sentence = f"<a href='/?seed={local_seed}'>{sentence}</a>"
            sentences.append(sentence)
        paragraphs.append(" ".join(sentences))
    content = "".join([f"<p>{p}</p>" for p in paragraphs])
    return template_str % content
