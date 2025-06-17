#!/bin/bash
gcc main.c -o tpfan -Os -s -fno-ident -fno-asynchronous-unwind-tables
cp ./tpfan /sbin/tpfan
cp ./tpfan.service /lib/systemd/system/tpfan.service
#systemctl start tpfan
#systemctl enable tpfan