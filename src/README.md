## Subdirectories:

./surl-server: the proxy
./mastodon: the Mastodon client
./stp: the FTP client
./telnet: the Telnet client
./quicktake: The Quicktake for Apple II program (does not require proxy, untestable via serial port emulation)
./homecontrol: My own thing for home automation, probably useless to you reading this file unless you hack it a lot to fit your usecase

The rest are more or less POCs or things of no interest.

## Testing locally with emulation

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

apt install ser2net
```

Put the following into /etc/ser2net.yaml:
```
connection: &con0096
    accepter: tcp,2000
    enable: on
    options:
      telnet-brk-on-sync: true
    connector: serialdev,
              /dev/tnt1,
              115200n81,local
```

Run the proxy:
```
A2_TTY=/dev/tnt0 ./src/surl-server
```
Run MAME:
```
mame apple2c -debug -window -flop1 dist/$(program).dsk -resolution 560x384 -modem null_modem -bitb socket.localhost:2000 -nomouse
```

Quick test rebuild of Mastodon, keeping the conf files in the .dsk: (you can extract CLISETTINGS and MASTSETTINGS after a successful login, using java -jar bin/ac.jar ...)
```
cd src/mastodon/ && make clean all; cd ../.. && make mastoperso.dsk && mame apple2c -window -flop1 mastoperso.dsk -resolution 560x384 -modem null_modem -bitb socket.localhost:2000 -nomouse
```
