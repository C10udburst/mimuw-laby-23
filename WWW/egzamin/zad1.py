# ***




from fastapi import FastAPI, Request, HTTPException

description = """
API for counting sentences and links in a webpage.

Connects to a local server on port 8001 to get the content of the page.
"""

tags_metadata = [
    {
        "name": "Single page",
        "description": "Operations on a single page. Requires a seed parameter. Faster compared to rest."
    },
    {
        "name": "Multiple pages",
        "description": "Operations on multiple pages. Slower compared to single page."
    }
]


app = FastAPI(description=description,
              contact={"name": "***", "email": "***@students.mimuw.edu.pl"},
              openapi_tags=tags_metadata)

from typing import List
from bs4 import BeautifulSoup
from pydantic import BaseModel
import httpx
import asyncio
import re

host = "http://localhost:8001"

class SentenceCount(BaseModel):
    sentence_count: int

@app.get("/sentence_count", tags=["Single page"])
async def sentence_count(request: Request, seed: int) -> SentenceCount:
    """       
    Fetches a page with a given seed from a server simulating a webpage and returns the number of sentences on that page.
    
    Note: sentences always start with a capital letter and end with a dot.
    """
    page = httpx.get(f"{host}/?seed={seed}", timeout=None)
    soup = BeautifulSoup(page.text, "html.parser")
    count = 0
    for p in soup.find_all("p"):
        count += len(re.findall(r"[A-Z].*?\.", p.text))
    return SentenceCount(sentence_count=count)
    

class LinkCount(BaseModel):
    link_count: int
    
class Error(BaseModel):
    error: str


@app.get("/link_count", tags=["Single page"], response_model=LinkCount)
async def link_count(request: Request, seed: int):
    """   
    Returns the number of links on a page with a given seed. 
    Possible errors are handled correctly and return a 404 status code if one occurs.
    """
    try:
        page = httpx.get(f"{host}/?seed={seed}", timeout=None)
        if page.status_code != 200:
            return JSONResponse(status_code=404, content=Error(error="Page not found"))
        soup = BeautifulSoup(page.text, "html.parser")
        return LinkCount(link_count=len(soup.find_all("a")))
    except Exception as e:
        raise HTTPException(status_code=404, detail=f"Page not found: {e}")

class SeedList(BaseModel):
    seeds: List[int]

class PageMeta(BaseModel):
    seed: int
    sentence_count: int

class PageContentList(BaseModel):
    pages: List[PageMeta]


@app.post("/all_pages_and_sentences", tags=["Multiple pages"])
async def all_pages_and_sentences(request: Request, seeds: SeedList) -> PageContentList:
    """  
    Fetches pages with given seeds and all pages that can be reached from them with a single click.
    Returns a list of all fetched pages (with their seeds) and the number of sentences on each of them.
    """
    pages = []
    for seed in seeds.seeds:
        page = httpx.get(f"{host}/?seed={seed}", timeout=None)
        soup = BeautifulSoup(page.text, "html.parser")
        count = 0
        for p in soup.find_all("p"):
            count += len(re.findall(r"[A-Z].*?\.", p.text))
        pages.append(PageMeta(seed=seed, sentence_count=count))
        for a in soup.find_all("a"):
            seed = re.search(r"seed=(\d+)", a["href"])
            if not seed:
                continue
            seed = int(seed.group(1))
            page = httpx.get(f"{host}/?seed={seed}", timeout=None)
            soup = BeautifulSoup(page.text, "html.parser")
            count = 0
            for p in soup.find_all("p"):
                count += len(re.findall(r"[A-Z].*?\.", p.text))
            pages.append(PageMeta(seed=seed, sentence_count=count))
    return PageContentList(pages=pages)
    
    
class TimeoutError(BaseModel):
    error: str
    pages: List[PageMeta]


@app.get("/all_pages_and_sentences_k", tags=["Multiple pages"])
async def all_pages_and_sentences_k(
    request: Request, seed: int, k: int
) -> PageContentList:
    """    
    Download pages with given seeds and all pages that can be reached from them with a maximum of k clicks (k ≤ 3), eliminating duplicates.
    Returns a unique list of downloaded pages (with seeds) and the number of sentences on each page.
    
    Execution time limit: 60 seconds.
    """
    try:
        assert k <= 3
    except AssertionError:
        raise HTTPException(status_code=500, detail="k must be less than or equal to 3")
    page_queue = [(seed, 0)]
    pages = []
    page_seeds = set()
    try:
        async with asyncio.Timeout(5):
            async with httpx.AsyncClient() as client:
                while page_queue:
                    seed, depth = page_queue.pop(0)
                    if seed in page_seeds:
                        continue
                    page_seeds.add(seed)
                    page = await client.get(f"{host}/?seed={seed}", timeout=None)
                    soup = BeautifulSoup(page.text, "html.parser")
                    count = 0
                    for p in soup.find_all("p"):
                        count += len(re.findall(r"[A-Z].*?\.", p.text))
                    pages.append(PageMeta(seed=seed, sentence_count=count))
                    if depth < k:
                        for a in soup.find_all("a"):
                            seed = re.search(r"seed=(\d+)", a["href"])
                            if not seed:
                                continue
                            seed = int(seed.group(1))
                            page_queue.append((seed, depth + 1))
                return PageContentList(pages=pages)
    except asyncio.TimeoutError:
        raise HTTPException(status_code=408, detail="Timeout")


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="0.0.0.0", port=8000)