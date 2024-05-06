from django.urls import path

from . import views

urlpatterns = [
    path('', views.index, name="index"),
    path('login/', views.login_page, name='login'),
    path('logout/', views.logout_page, name='logout'),
    path('image/<int:image_id>/edit/', views.image_edit, name='image_edit'),
    path('image/<int:image_id>/', views.image_detail, name='image_detail'),
    path('svg/<int:image_id>/', views.image_render, name='image_render')
]