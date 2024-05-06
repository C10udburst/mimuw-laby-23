# W tym zadaniu należy przygotować serwis internetowy do przeglądania i publikacji obrazków.

# Obrazek to grafika w formacie SVG, która składa się z dowolnej ilości dowolnych prostokątów (mogą różnić się położeniem, wielkości i kolorem wypełnienia).

# Przykładowy kod poprawnego obrazka
# <svg width="100" height="100" viewBox="0 0 100 100" version="1.1" xmlns="http://www.w3.org/2000/svg">
#     <rect x="0" y="0" width="100" height="100" fill="black"></rect>
#     <rect x="25" y="25" width="50" height="50" fill="red"></rect>
#     <rect x="10" y="10" width="10" height="10" fill="blue"></rect>
# </svg>
# Użytkownicy
# Z serwisu korzystają trzy typy użytkowników:

# zwykły użytkownik
# artysta (użytkownik z kontem)
# administrator serwisu (użytkownik z kontem administracyjnym)
# Każdy kolejny typ ściśle rozszerza dostępne w ramach serwisu akcje poprzedniego typu.

# Zwykły użytkownik
# Po wejściu na stronę serwisu może:

# zobaczyć listę wszystkich opublikowanych obrazków

# lab5-image

# wybrać obrazek i zobaczyć jego podgląd bezpośrednio w przeglądarce

# lab5-image-1

# Artysta
# Może zalogować się do serwisu korzystając z konta utworzonego dla niego przez administratora strony. Ponad to co może zrobić zwykły użytkownik, artysta może edytować obrazki, które są mu przypisane przez administratora strony.

# lab5-image-2

# Edycja oznacza, że może on przerobić dowolny obrazek na dowolny inny. Najbardziej minimalistyczna wersja spełnienia tego wymagania to implementacja dwóch akcji:

# usunięcie wszystkich prostokątów z obrazka (poniżej każdy prostokąt można usunąć z osobna)
# lab5-image-3

# dodanie dowolnego prostokąta na koniec obrazka (tzn. wyświetlającego się na wierzchu)
# lab5-image-4

# Administrator strony
# Jako jedyny ma dostęp do admin panelu django, skąd może:

# zarządzać kontami dla artystów

# lab5-image-5

# tworzyć nowe obrazki i przypisywać je do edycji artystom

# lab5-image-6

# modyfikować wielkość obrazków


from django.db import models
from django.contrib.auth.models import User

class Rectangle(models.Model):
    x = models.IntegerField()
    y = models.IntegerField()
    width = models.IntegerField()
    height = models.IntegerField()
    color = models.CharField(max_length=100, default="black")
    
    def __str__(self):
        return f"Rectangle {self.id}: <rect x='{self.x}' y='{self.y}' width='{self.width}' height='{self.height}' fill='{self.color}' />"
    
    def as_svg(self):
        return f"<rect x='{self.x}' y='{self.y}' width='{self.width}' height='{self.height}' fill='{self.color}' />"

class Tag(models.Model):
    name = models.CharField(max_length=100)
    
    def __str__(self):
        return f"Tag {self.id}: {self.name}"

class Image(models.Model):
    name = models.CharField(max_length=100)
    width = models.IntegerField()
    height = models.IntegerField()
    editors = models.ManyToManyField(User, related_name="images", blank=True)
    description = models.TextField(blank=True)
    pub_date = models.DateTimeField(auto_now_add=True)
    tags = models.ManyToManyField('Tag', related_name="images", blank=True)
    rectangles = models.ManyToManyField(Rectangle, related_name="images", blank=True)
    
    def __str__(self):
        return f"Image {self.id}: {self.name}"
    
    def as_svg(self):
        rectangles = "\n".join([rectangle.as_svg() for rectangle in self.rectangles.all()])
        return f"<svg width='{self.width}' height='{self.height}' viewBox='0 0 {self.width} {self.height}' version='1.1' xmlns='http://www.w3.org/2000/svg'>\n{rectangles}\n</svg>"