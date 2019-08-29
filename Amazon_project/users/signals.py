from django.db.models.signals import post_save
from django.contrib.auth.models import User
from django.dispatch import receiver
from .models import Profile


# create a profile every time a user is created without manually doing so in admin page.
# When a user is created, it sends a 'post_save' signal to receiver, receivers calls the
# create_profile() which takes in 4 paras that signal passed in, using the instance to create
# a profile object.
@receiver(post_save, sender=User)
def create_profile(sender, instance, created, **kwargs):
    if created:
        Profile.objects.create(user=instance)


@receiver(post_save, sender=User)
def save_profile(sender, instance, **kwargs):
    instance.profile.save()