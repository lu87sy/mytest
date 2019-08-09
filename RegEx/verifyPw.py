import re

test = input('请输入密码\n')
if re.match(r'^[a-zA-Z][a-zA-Z0-9\_]{5, 19}', test):
    print('符合密码要求！')
else:
    print('密码设置不符合要求')