systemctl stop tpfan
systemctl disable tpfan
rm /sbin/tpfan
rm /lib/systemd/system/tpfan.service
rm /lib/systemd/system/tpfan-wakeup.service
rm /lib/systemd/system/tpfan-sleep.service