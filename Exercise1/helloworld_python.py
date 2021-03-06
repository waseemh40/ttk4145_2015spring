# Python 3.3.3 and 2.7.6
# python helloworld_python.py

from threading import Thread


i = 0

def td1_func():
    global i
    for n in range(0, 1000000):
        i+=1

def td2_func():
    global i
    for n in range(0, 1000000):
        i-=1

# Potentially useful thing:
#   In Python you "import" a global variable, instead of "export"ing it when you declare it
#   (This is probably an effort to make you feel bad about typing the word "global")
#    global i

def main():
    global i
    td1 = Thread(target = td1_func, args = (),)
    td2 = Thread(target = td2_func, args = (),)

    td1.start()
    td2.start()
    
    td1.join()
    td2.join()
    print("Hello from main! global i: ",i)


main()
