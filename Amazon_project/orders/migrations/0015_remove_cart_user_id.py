# Generated by Django 2.2 on 2019-04-24 15:07

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0014_cart_user_id'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='cart',
            name='user_id',
        ),
    ]
