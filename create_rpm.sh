#!/bin/bash

# maybe an earlier run of this script failed
rm -rf RPM/hid-retrolink

# create working directory
pushd RPM
./togo create hid-retrolink
mkdir -p hid-retrolink/root/etc/udev/rules.d
mkdir -p hid-retrolink/root/etc/modules-load.d
mkdir -p hid-retrolink/root/usr/src/hid-retrolink-1.0.0
cp header hid-retrolink/spec/
cp post hid-retrolink/spec/
cp preun hid-retrolink/spec/
popd

# copy necessary files
cp 99-hid-retrolink.rules RPM/hid-retrolink/root/etc/udev/rules.d/
cp hid-retrolink.conf RPM/hid-retrolink/root/etc/modules-load.d/
cp COPYING RPM/hid-retrolink/root/usr/src/hid-retrolink-1.0.0/
cp DETAILS.md RPM/hid-retrolink/root/usr/src/hid-retrolink-1.0.0/
cp dkms.conf RPM/hid-retrolink/root/usr/src/hid-retrolink-1.0.0/
cp hid-retrolink.c RPM/hid-retrolink/root/usr/src/hid-retrolink-1.0.0/
cp Makefile RPM/hid-retrolink/root/usr/src/hid-retrolink-1.0.0/
cp README.md RPM/hid-retrolink/root/usr/src/hid-retrolink-1.0.0/

pushd RPM/hid-retrolink # otherwise togo -f fails
# exclude ownership
../togo file exclude root/etc/udev/rules.d
../togo file exclude root/etc/modules-load.d
../togo file exclude root/usr/src

# create package
../togo build package
popd

# move rpm to root dir
mv RPM/hid-retrolink/rpms/hid-retrolink-1.0.0-1.noarch.rpm ./

# cleanup
rm -rf RPM/hid-retrolink
