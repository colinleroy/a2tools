== Testing without serial port ==

```
git clone https://github.com/freemed/tty0tty
cd tty0tty-1.2/module
make
sudo cp tty0tty.ko /lib/modules/$(uname -r)/kernel/drivers/misc/
sudo depmod
sudo modprobe tty0tty
sudo chmod 666 /dev/tnt*
```
Read from /dev/tnt1, write to /dev/tnt0
