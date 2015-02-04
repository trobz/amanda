#!/bin/bash

echo "configure amanda"
./configure \
        --prefix=/usr \
        --bindir=/usr/sbin \
        --libexecdir=/usr/libexec/amanda \
        --without-amlibexecdir \
        --without-amperldir \
        --sysconfdir=/etc \
        --sharedstatedir=/var/lib \
        --localstatedir=/var/lib \
        --with-user=amandabackup \
        --with-group=disk \
        --with-debugging=/var/log/amanda \
        --with-gnutar-listdir=/var/lib/amanda/gnutar-lists \
        --with-index-server=localhost \
        --with-amandahosts \
        --with-ssh-security \
        --without-ipv6 \
        --with-config=instance-backup \
        --mandir=/usr/share/man

echo "install amanda"
sudo -u amandabackup -H make
make install

