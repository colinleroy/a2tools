# Repository of stuff I write for my Apple //c

I'm having fun with this antiquity like it's 1989*!

The Apple //c was my first computer and I loved it dearly
for years, but to be honest, it served little purpose these
last years, only decorating the office. But then my wife woke
up a monster by suggesting we do the Advent of Code, and we
started, and then I thought

_"I should do it with the Apple //c!"_

One thing led to another and there are now quite a few things
here:
- a full featured Mastodon client
- a media player (mp3, video, webradios, ...)
- a Peertube and Youtube client
- an FTP client
- a Telnet client
- an HomeAssistant frontend to control switches, heating, and view sensors' graphs.

## Building the Apple II programs

Install cc65:

```
git clone https://github.com/cc65/cc65.git
cd cc65
make
sudo make install
```

Build my things:

```
make
```

Create floppy images:

```
make dist
```

You can then transfer the images in dist/ using ADTPro.

## Building only the proxy

To build only the proxy, you can skip installing cc65. Install the build dependancies, then compile the proxy:

```
sudo apt-get install libcurl4-gnutls-dev libgumbo-dev libpng-dev libjq-dev \
  libsdl-image1.2-dev libavcodec-dev libavformat-dev libavfilter-dev libavutil-dev

cd src/surl-server
make
./surl-server
```

## Notes

The Apple 2 serial port is port 2 by default.

# Raspberry installation
For convenience, a pre-built surl-server is configured in the Raspberry image file,  surl-server-bullseye-YYYY-MM-DD-lite.img.gz , available in the releases.

Copy it to a microSD card:
```
gunzip surl-server-bullseye-YYYY-MM-DD-lite.img.gz
dd if=surl-server-bullseye-YYYY-MM-DD-lite.img of=/dev/mmcblk0 bs=4M status=progress
sync
```

Install the SD card into a Raspberry, connect Ethernet, connect USB/serial adapter and boot. Everything should be up and running if you have a DHCP server. 

You can ssh into the pi with the default Raspbian login, pi/raspberry. The Pi should get on the network as `surl-server.local`.

If you already have a running Raspberry that you'd want to use for this, you can instead add the following source and packages:

```
echo "deb https://apt-rpi.colino.net/debian bullseye main" | tee -a /etc/apt/sources.list.d/apt-rpi-colino-net.list
curl https://apt-rpi.colino.net/gpg.key | apt-key add -
apt update
apt install surl-server
```

*Yes, I know it's older than that. But I got mine in 1989, when I was nine.
