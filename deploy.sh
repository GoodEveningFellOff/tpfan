#!/bin/bash
gcc main.c -o tpfan -Os -s -fno-ident -fno-asynchronous-unwind-tables
cp ./tpfan /sbin/tpfan
cp ./scripts/tpfan.service /lib/systemd/system/tpfan.service
cp ./scripts/tpfan-wakeup.service /lib/systemd/system/tpfan-wakeup.service
cp ./scripts/tpfan-sleep.service /lib/systemd/system/tpfan-sleep.service
systemctl start tpfan
systemctl enable tpfan