from django.shortcuts import render, redirect
from django.contrib import messages
from django.contrib.auth.decorators import login_required
from django.views.decorators.http import require_http_methods

from .forms import PlaceOrderForm, SelectProductForm, QueryOrderForm, S1Form
from .models import Product, Order, Cart, User
from django.db.models import Sum
import socket
from django.core.mail import EmailMessage


def send_data_to_server(payload):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('vcm-8252.vm.duke.edu', 45678))
    s.sendall(bytes(str(payload),  'utf-8'))
    s.close()


@login_required
def select_product(request):
    products = Product.objects.distinct('product_name')

    if request.method == 'POST' and 'add_to_cart' in request.POST:
        form = SelectProductForm(request.POST)

        if form.is_valid():
            product_name = form.cleaned_data['product_name']
            amount = form.cleaned_data['amount']

            cart = Cart(product_name=product_name, amount=amount, user_id=request.user.id)
            cart.save()

            carts = Cart.objects.filter(user_id=request.user.id)

            # get total_stock of this product from ALL warehouses
            total_stock = Product.objects.filter(product_name=product_name).aggregate(Sum('stock'))
            # if stock of existing product is less than demand, return error msg
            if not total_stock['stock__sum']:
                messages.success(request, f'No such item, creating....')
                return render(request, 'orders/shopping_cart.html', {'carts': carts})
            if total_stock['stock__sum'] and total_stock['stock__sum'] < amount:
                messages.success(request, f'Item in low stock, we will pack soon.')
                return render(request, 'orders/shopping_cart.html', {'form': form, 'products': products,
                                                                     'message': messages, 'carts': carts})
            else:
                return redirect('shopping_cart')

    elif request.method == 'POST' and 'search_product' in request.POST:
        form = SelectProductForm(request.POST)

        if form.is_valid():
            product_name = form.cleaned_data['product_name']
            product_names = Product.objects.distinct('product_name')

            for name in product_names:
                if name.product_name == product_name:
                    newform = SelectProductForm(initial={'product_name': product_name})
                    return render(request, 'orders/select_product.html', {'form': newform, 'message': 'item exists',
                                                                          'products': products})

            messages.success(request, f'No such item, creating....')
            return render(request, 'orders/select_product.html', {'form': form, 'products': products,
                                                                  'message': messages})

    form = SelectProductForm()
    form1 = S1Form()

    return render(request, 'orders/select_product.html', {'form1': form1, 'products': products})


@login_required()
def shopping_cart(request):
    carts = Cart.objects.filter(user_id=request.user.id)
    print(request.user.id)
    return render(request, 'orders/shopping_cart.html', {'carts': carts})


@login_required
def place_order(request, product_name, amount):
    if request.method == 'POST':
        form = PlaceOrderForm(request.POST)
        if form.is_valid():
            tracking_number = form.save().tracking_number
            try:
                send_data_to_server(str(tracking_number)+'/'+form.cleaned_data['ups_account'])
            except Exception as e:
                print(e)

            user_id = request.user.id
            user = User.objects.get(id=user_id)
            user_email = user.email
            subject = 'Order succeeded!'
            body = 'Congratulations! Your order has been successfully placed!'
            email = EmailMessage(subject, body, to=[user_email])
            email.send()

            # after the order is placed successfully, it should be deleted from shopping cart
            Cart.objects.filter(user_id=request.user.id).filter(product_name=product_name).delete()

            return redirect('place_order_done')
    else:
        form = PlaceOrderForm(initial={
            'user_id': request.user.id,
            'product_name': product_name,
            'amount': amount
        })

    return render(request, 'orders/place_order.html', {'form': form})


def delete_item(request, cid):
    item = Cart.objects.filter(user_id=request.user.id).filter(id=cid)
    item.delete()
    carts = Cart.objects.filter(user_id=request.user.id)
    return render(request, 'orders/shopping_cart.html', {'carts': carts})


@login_required
def place_order_done(request):
    return render(request, 'orders/place_order_done.html')


@login_required
@require_http_methods(["GET"])
def order_history(request):
    orders = Order.objects.filter(user_id=request.user.id)
    return render(request, 'orders/order_history.html', {'orders': orders})


@login_required
def query_order(request):
    if request.method == 'POST':
        form = QueryOrderForm(request.POST)
        if form.is_valid():
            tracking_number = form.cleaned_data['tracking_number']
            orders = Order.objects.filter(user_id=request.user.id).filter(tracking_number=tracking_number)
            print(request.user.id)
            if len(orders) == 0:
                return render(request, 'orders/query_order.html', {'form': form, 'message': 'No Results.'})
            else:
                return render(request, 'orders/query_order.html', {'form': form, 'orders': orders})

    else:
        form = QueryOrderForm()

    return render(request, 'orders/query_order.html', {'form': form,
                                                       'message': 'Please search by tracking numbers for results.'})