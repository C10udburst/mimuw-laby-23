from itertools import product
from random import shuffle, choice
from sys import argv

deck = list(product(list("23456789JQKA") + ["10"], "CDHS"))
types = "1234567"
first = "NESW"


def game():
    print(choice(types)+choice(first))
    shuffle(deck)
    i = 0
    for rank, suit in deck:
        print(rank + suit, end="")
        i += 1
        if i % 13 == 0:
            print()


if __name__ == "__main__":
    for _ in range(int(argv[1])):
        game()
