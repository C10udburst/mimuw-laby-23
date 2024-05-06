
from django.shortcuts import render, get_object_or_404, redirect
from django.http import HttpResponse
from django.contrib.auth import authenticate, login, logout
from .models import Image, Rectangle, Tag

per_page = 2

def index(request):
    curr_page = 1
    try:
        curr_page = int(request.GET.get('page', 1))
    except ValueError:
        pass
    tag = None
    try:
        tag = int(request.GET.get('tag') or 'none')
        # get list
        tag = Tag.objects.get(id=tag)
    except ValueError:
        pass
    asc = request.GET.get('asc', 'true') == 'true'
    images = Image.objects.all() if tag is None else Image.objects.filter(tags=tag)
    images = images.order_by('pub_date' if asc else '-pub_date')
    images = images[(curr_page-1)*per_page:curr_page*per_page]
    return render(request, 'app/index.html', {'images': images, 'page': curr_page, 'tag': tag, 'asc': str(asc).lower()})

def image_edit(request, image_id):
    image = get_object_or_404(Image, pk=image_id)
    
    if request.method == 'POST':
        print(request.POST)
        
        # check if user has permission to edit this image
        if request.user not in image.editors.all():
            return HttpResponse("You don't have permission to edit this image")
        
        if 'add_rectangle' in request.POST:
            x = request.POST.get('x')
            y = request.POST.get('y')
            width = request.POST.get('width')
            height = request.POST.get('height')
            color = request.POST.get('color')
            rectangle = Rectangle(x=x, y=y, width=width, height=height, color=color)
            rectangle.save()
            image.rectangles.add(rectangle)
        if 'delete_rectangle' in request.POST:
            rectangle_id = request.POST.get('delete_rectangle')
            rectangle = get_object_or_404(Rectangle, pk=rectangle_id)
            image.rectangles.remove(rectangle)
            rectangle.delete()
            pass
        
        
        return redirect('image_detail', image_id=image_id)
    
    return render(request, 'app/image_edit.html', {'image': image})

def image_render(request, image_id):
    image = get_object_or_404(Image, pk=image_id)
    return HttpResponse(image.as_svg(), content_type="image/svg+xml")

def image_detail(request, image_id):
    image = get_object_or_404(Image, pk=image_id)
    return render(request, 'app/image_detail.html', {'image': image})

def login_page(request):
    if request.method == 'POST':
        username = request.POST.get('username')
        password = request.POST.get('password')
        
        # use Django's built-in authentication system
        user = authenticate(request, username=username, password=password)
        if user is not None:
            login(request, user)
            return redirect('index')
        else:
            return HttpResponse("Invalid login")
        
        
    return render(request, 'app/login.html')

def logout_page(request):
    logout(request)
    return redirect('index')