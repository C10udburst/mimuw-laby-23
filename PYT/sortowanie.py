from json import loads, dump
from sys import argv
import os

klucz = "klucz"
chunk_size = 1000

if len(argv) < 2:
    print("Nie podano pliku")
    exit(1)
    
src = open(argv[1], "r")

if src.read(1) != "[":
    print("Plik nie jest listą")
    src.close()
    exit(1)

src.read(1) # skip \n

def make_temp_file(id):
    return open("temp" + str(id) + "", "w")
    

chunk_count = 0
while True:
    chunk = []
    while len(chunk) < chunk_size:
        line = src.readline()
        if line == "]":
            break
        if line == "":
            print("Niepoprawny plik")
            src.close()
            exit(1)
        chunk.append(loads(line))

    chunk.sort(key=lambda x: x[klucz])
    tmp = make_temp_file(chunk_count)
    for item in chunk:
        dump(item, tmp)
        tmp.write("\n")
    tmp.close()
    
    if line == "]":
        break
    
src.close()
    
chunks = [open("temp" + str(i), "r") for i in range(chunk_count)]
chunk_top = [loads(chunks[i].readline()) for i in range(chunk_count)]

out = open("src.json", "w+")
out.write("[\n")

def all_empty():
    for i in range(chunk_count):
        if chunk_top[i] != "":
            return False
    return True

while not all_empty():
    min = chunk_top[0]
    min_id = 0
    for i in range(chunk_count):
        if chunk_top[i][klucz] < min[klucz]:
            min = chunk_top[i]
            min_id = i
    dump(min, out)
    out.write("\n")
    chunk_top[min_id] = loads(chunks[min_id].readline())
    
out.write("]")
out.close()

for i in range(chunk_count):
    chunks[i].close()
    os.remove("temp" + str(i))
    