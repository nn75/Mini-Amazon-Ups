from django import forms
from django.contrib.auth.models import User
from django.contrib.auth.forms import UserCreationForm
from .models import Profile


class UserRegisterForm(UserCreationForm):
    email = forms.EmailField()

    class Meta:
        model = User
        fields = ['username', 'email', 'password1', 'password2']


class UserUpdateForm(forms.ModelForm):
    """
    a ModelForm allows us to create a form that will work with a specific database model
    "fields" are the title of things shown on page, first letter is automatically capitalized

    next 2 classes create different forms if user wants to update their profile.
    Because fields belong to different models.
    """
    email = forms.EmailField()
    username = forms.CharField(max_length=500, widget=forms.TextInput(attrs={'readonly': 'readonly'}))

    class Meta:
        model = User
        fields = ['username', 'email']


class ProfileUpdateForm(forms.ModelForm):
    class Meta:
        model = Profile
        fields = ['image']
