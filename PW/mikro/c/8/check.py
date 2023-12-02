from sys import stdin

while line := stdin.readline():
    if "six times a number" in line:
        before, after = line.split("six times a number")
        after = after.strip()
        after = after.split(" ")
        after = set(after)
        if len(after) != 1:
            print("Invalid: " + line.strip())
            exit(1)
    print(line.strip())