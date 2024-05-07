from django.core.management.base import BaseCommand, CommandError
from random import randint
from app.models import Image, Rectangle, Tag
from django.contrib.auth.models import User
from string import ascii_lowercase

def random_string(length):
    return ''.join(ascii_lowercase + ' ' for _ in range(length))

def random_color():
    return '#{:06x}'.format(randint(0, 0xFFFFFF))

class Command(BaseCommand):
    help = 'Generates random images'
    
    def add_arguments(self, parser):
        parser.add_argument('count', type=int)
        parser.add_argument('rectangles', type=int)
        parser.add_argument('editor', type=str)
        parser.add_argument('tag', type=str)
    
    def handle(self, *args, **options):
        count = options['count']
        editor = options['editor']
        tag = options['tag']
        rectangles = options['rectangles']
        
        try:
            editor = User.objects.get(username=editor)
        except User.DoesNotExist:
            raise CommandError(f"User {editor} does not exist")
        
        try:
            tag = Tag.objects.get(name=tag)
        except Tag.DoesNotExist:
            tag = Tag(name=tag)
            tag.save()
            
        
        for _ in range(count):
            w = randint(100, 1000)
            h = randint(100, 1000)
            
            image = Image(
                name=random_string(10),
                width=w,
                height=h,
                description=random_string(100),
            )
            image.save()
            image.tags.add(tag)
            image.editors.add(editor)
            
            # add rectangles
            for _ in range(rectangles):
                rect = Rectangle(
                    x=randint(0, w),
                    y=randint(0, h),
                    width=randint(10, w),
                    height=randint(10, h),
                    color=random_color(),
                )
                rect.save()
                image.rectangles.add(rect)
                