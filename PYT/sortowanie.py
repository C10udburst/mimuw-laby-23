from json import loads, dump
from sys import stderr
import argparse
import os

argparser = argparse.ArgumentParser(description="Sortuje plik JSON")
argparser.add_argument("plik", help="Plik do posortowania")
argparser.add_argument("-k", "--klucz", help="Klucz, według którego sortować", default="klucz")
argparser.add_argument("-s", "--chunk-size", help="Rozmiar chunka", default=1000, type=int)
argparser.add_argument("-o", "--output", help="Nazwa pliku wyjściowego", default=None, type=str)
args = argparser.parse_args()

klucz = args.klucz
chunk_size = args.chunk_size
    
src = open(args.plik, "r")

if src.read(1) != "[":
    print("Plik nie jest listą", file=stderr)
    src.close()
    exit(1)

src.read(1) # skip \n

tmpdir = os.popen("mktemp -d").read().strip()

def make_temp_file(id):
    return open(f"{tmpdir}/temp{id}", "w+")
    

chunk_count = 0
while True:
    chunk = []
    while len(chunk) < chunk_size:
        line = src.readline()
        if line == "]":
            break
        if line == "":
            print("Niepoprawny plik", file=stderr)
            src.close()
            exit(1)
        chunk.append(loads(line.strip().rstrip(",")))

    chunk.sort(key=lambda x: x[klucz])
    tmp = make_temp_file(chunk_count)
    for item in chunk:
        dump(item, tmp)
        tmp.write("\n")
    tmp.close()
    chunk_count += 1
    
    if line == "]":
        break
    
src.close()
    
chunks = [open(f"{tmpdir}/temp{i}", "r") for i in range(chunk_count)]
chunk_top = [loads(chunks[i].readline() or "false") for i in range(chunk_count)]

out = open(args.output or args.plik.replace(".json", "_sorted.json"), "w+")
out.write("[\n")

def all_empty():
    for i in range(chunk_count):
        if chunk_top[i]:
            return False
    return True

while not all_empty():
    min = chunk_top[0]
    min_id = 0
    for i in range(chunk_count):
        if not chunk_top[i]:
            continue
        if not min or chunk_top[i][klucz] < min[klucz]:
            min = chunk_top[i]
            min_id = i
    dump(min, out)
    out.write(",\n")
    chunk_top[min_id] = loads(chunks[min_id].readline() or "false")

out.seek(out.tell() - 2)
out.write("\n]")
out.close()

os.system(f"rm -rf {tmpdir}")
    