from django.contrib import admin

# Register your models here.
from .models import Image, Rectangle, Tag

admin.site.register(Tag)
admin.site.register(Rectangle)
admin.site.register(Image)
