# Generated by Django 2.2 on 2019-04-24 13:34

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0009_auto_20190424_1326'),
    ]

    operations = [
        migrations.AlterField(
            model_name='cart',
            name='product_name',
            field=models.CharField(max_length=500),
        ),
    ]
