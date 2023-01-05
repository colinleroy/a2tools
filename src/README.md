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

== Testing without apple // ==

```
apt install ser2net
```

```
connection: &con0096
    accepter: tcp,2000
    enable: on
    options:
      telnet-brk-on-sync: true
    connector: serialdev,
              /dev/tnt1,
              9600n81,local
```

```
mame apple2c -debug -window -flop1 net.dsk -resolution 560x384 -modem null_modem -bitb socket.localhost:2000 -nomouse
```
