A translation from Python to C of the framework for my firewall project in my networking class.

To try out this code download this VM from: https://drive.google.com/file/d/0Bwor7RzmbQy7THFLQ0FDaGN2WjA/view?usp=sharing
Make sure you have VirtualBox installed (version 4.3 or higher suggested) and import the VM (don't change anything configurations!)

Once the VM is running login with
username: cs168
password: F1rewa!!

open the terminal and run
```
cd proj3
sudo ./main.py --mode bypass
```

This will give you access to the internet

At this point you can either
1. install git and then clone this repo
2. or just download the files from https://drive.google.com/file/d/0Bwor7RzmbQy7VHMwSldwZGRXUlk/view?usp=sharing

cd into the folder where the files are and run
```
sudo ./main
```
to see it in action

Since this is just the framework not much is done with the packets. I did however mess around with integrating python into the C code
so you can do the packet processing if you want. WARNING processing packets in python makes the internet very slow...
Preferable is to process the packets in C though it is more tedious.

If want to compare the files main.c corresponds to main.py. The basic jist is to set up sockets for the 'int' and 'ext' ethernet interfaces of the
virtual machine and then send packets from 'int' to 'ext' and vice versa to achieve connectivity.

![alt tag](https://lh4.googleusercontent.com/uZrItQ-uo3_G5ixJglWYlc2UH_fz0CNYfwdF0WE31Rbp6nAwHA9uVI95AP8miut2IybossdtE_s=w1342-h539)
