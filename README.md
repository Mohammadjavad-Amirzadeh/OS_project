# OS_project

### Execution Instructions:

1. Initially, you need to place the shopping list of each user in the `users_shopping_list/` directory. (Each user should have a file named `user{i}.txt` and the format for writing the file can be found in the same directory.)

2. Then, you can run the `store.c` file.

3. After a few seconds, when the file processing is complete, program waits until get the rate of products that user bought, then you should place the rating for each user in the `rates` directory. The format and file naming for this can also be found in that directory, where you need to save the rating in a text file.

4. Finally, when the rating files have been reviewed, the program will terminate.


---

the format of rate file must be like this:
```
  1
  2
  3
  4
  5
```
each number is customer vote to a product . ( first number for first product and second number for secend product and ... in its list. )

for giving rate you should enter your numbers in a `.txt` file out of `rates/` directory, then after you enter all of your rates, put your file in `rate/` directory. remember how your `.txt` file name must be.

the format of rating file of each user:
```
user{i}_list{j}.txt
```
where `i` is user number and `j` is the list number of user.

---

the format of user shopping list file must be like this:
```
list1:
Item : name of product you want, Quantity : count of product you want
Item : name of product you want, Quantity : count of product you want
Item : name of product you want, Quantity : count of product you want
Item : name of product you want, Quantity : count of product you want
Treshhold : a number

list2:
Item : name of product you want, Quantity : count of product you want
Item : name of product you want, Quantity : count of product you want
Item : name of product you want, Quantity : count of product you want
Item : name of product you want, Quantity : count of product you want
```

اول از همه نیاز است که شماره لیست مورد نظر را بنویسیم مانند مثال `:list1` سپس در خطوط بعدی در هر کدام به صورت زیر میتوان نوشت:
```
Item : name of product you want, Quantity : count of product you want
```
در مقابل `Item : ` نام محصولی که میخواهید و در مقابل `Quantity : ` تعدادی که از آن محصول میخواهید را مینویسید ، برای اینکه محصول جدیدی را به لیست اضافه کنید لازم است تا در خط دیگری همین عملیات را دوباره تکرار کنید.
در انتهای لیست خرید میتوانید treshhold مورد نظر خود را به صورت زیر وارد کنید :
```
Treshhold : a number
```
در روبه روی `Treshhold : ` یک عدد که سقف خرید را مشخص میکند باید بنویسید. اگر چیزی نوشته نوشد فرض میشود مقدار سقف خرید بینهایت است.
قبل از اینکه کد را اجرا کنید میتوانید برای user های متفاوت لیست خرید های متفاوت ایجاد کنید و آن ها را داخل `users_shopping_list/` قرار دهید . کد را اجرا کنید.

---

در ابتدای کد از شما تعداد یوزر ها را میخواهد که این عدد نشان میدهد که لیست های خرید کدام یوزر ها را بررسی کند . مثلا اگر عدد ۳ وارد شود . لیست های خرید یوزر ۱ و ۲ و ۳ بررسی میشوند و دقت کنید که حتما باید این فایل ها به تعداد مناسبی موجود باشند در دایرتوری مورد نظر.
سپس در الگوریتم های موجود در کد برای یک یوزر بهترین تمام لیست هایش بررسی میشوند و هر کدام از لیست هایش از بهترین فروشگاه ممکن خریداری میشوند و اگر یوزری از یک فروشگاه دوبار خرید کرد از دومین خریدش به بعد به او ۱۰ درصد تخفیف داده میشود. اگر یوزری لیست خریدش باطل شود دیگر نمیتواند به آن لیست خرید رای بدهد و آن لیست از بین میرود.


----

First of all, you need to write the number of the desired list like the example `:list1`. Then, in the following lines, you can write as follows:
```
Item : name of product you want, Quantity : count of product you want
```
In front of `Item : ` write the name of the product you want, and in front of `Quantity : ` write the number of that product you want. To add a new product to the list, simply repeat the same operation on another line.
At the end of the shopping list, you can enter your desired threshold as follows:
```
Treshhold : a number
```
Next to `Treshhold : ` you should write a number that specifies the purchase limit. If nothing is written, it is assumed that the purchase limit is infinite.
Before executing the code, you can create different shopping lists for different users and place them inside `users_shopping_list/`. Then run the code.

---

At the beginning of the code, it will ask you for the number of users, which indicates which users' shopping lists to check. For example, if the number 3 is entered, the shopping lists of user 1, 2, and 3 will be checked, and make sure that there are enough files available in the specified directory.
Then, in the algorithms present in the code, for a user, the best of all their lists will be reviewed, and each of their lists will be purchased from the best possible store. If a user buys from a store twice, they will receive a 10 percent discount from the second purchase onwards. If a user's shopping list is invalidated, they can no longer vote on that shopping list, and it will be discarded.
