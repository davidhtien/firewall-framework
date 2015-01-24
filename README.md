# Firewall Framework (in C)

A translation from Python to C of the framework for my firewall project from my networking class.

To try out this code download this VM from: https://drive.google.com/file/d/0Bwor7RzmbQy7THFLQ0FDaGN2WjA/view?usp=sharing.
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

1. install git and then clone this repog
2. or just download the files from https://drive.google.com/file/d/0Bwor7RzmbQy7VHMwSldwZGRXUlk/view?usp=sharing

cd into the folder where the files are and run
```
sudo ./main
```
to see it in action

Since this is just the framework not much is done with the packets. I did however mess around with integrating python into the C code
so you can do the packet processing if you want. WARNING processing packets in python makes the internet very slow...
For the sake of speed it is preferable to process the packets in C though it may be more tedious.

To get you started, the first 14 bytes of the packets are Ethernet, information concerning the IP and TCP headers (including length and src and dst all that good stuff) can be found on wikipedia or through google searches. 

If want to compare the files main.c corresponds to main.py. The basic jist is to set up sockets for the 'int' and 'ext' ethernet interfaces of the
virtual machine and then send packets from 'int' to 'ext' and vice versa to achieve connectivity.

![Alt text](/firewall-framework.png?raw=true)
