# Generated by Django 2.2 on 2019-04-24 15:07

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0015_remove_cart_user_id'),
    ]

    operations = [
        migrations.AddField(
            model_name='cart',
            name='user_id',
            field=models.IntegerField(null=True),
        ),
    ]
