== Testing without serial port ==

MOK signing of the tty0tty module is required if secure boot is enabled.
How to MOK: https://ursache.io/posts/signed-kernel-module-debian-2023/
```
git clone https://github.com/freemed/tty0tty
cd tty0tty-1.2/module
make
cd ..
/usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 ../mok/MOK.priv ../mok/MOK.der ./module/tty0tty.ko
sudo cp module/tty0tty.ko /lib/modules/$(uname -r)/kernel/drivers/misc/
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

Quick test rebuild of Mastodon, keeping the conf files in the .dsk: (you can extract CLISETTINGS and MASTSETTINGS after a successful login, using java -jar bin/ac.jar ...)
```
cd src/mastodon/ && make clean all; cd ../.. && make mastoperso.dsk && mame apple2c -window -flop1 mastoperso.dsk -resolution 560x384 -modem null_modem -bitb socket.localhost:2000 -nomouse
```
