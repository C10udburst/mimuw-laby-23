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