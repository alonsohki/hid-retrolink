#!/bin/bash

# to create a valid package we need to be root
# see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=291320
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

# maybe an earlier run of this script failed
rm -rf hid-retrolink

# create working directory
mkdir -p hid-retrolink/etc/udev/rules.d
mkdir -p hid-retrolink/etc/modules-load.d
mkdir -p hid-retrolink/usr/src/hid-retrolink-1.0.0

# copy necessary files
cp -R DEBIAN hid-retrolink/
cp 99-hid-retrolink.rules hid-retrolink/etc/udev/rules.d/
cp hid-retrolink.conf hid-retrolink/etc/modules-load.d/
cp COPYING hid-retrolink/usr/src/hid-retrolink-1.0.0/
cp DETAILS.md hid-retrolink/usr/src/hid-retrolink-1.0.0/
cp dkms.conf hid-retrolink/usr/src/hid-retrolink-1.0.0/
cp hid-retrolink.c hid-retrolink/usr/src/hid-retrolink-1.0.0/
cp Makefile hid-retrolink/usr/src/hid-retrolink-1.0.0/
cp README.md hid-retrolink/usr/src/hid-retrolink-1.0.0/

# create package
dpkg-deb --build hid-retrolink

# cleanup
rm -rf hid-retrolink
