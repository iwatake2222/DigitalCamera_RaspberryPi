#!/bin/sh
sudo apt-get install libjpeg-dev
make
rm log_shell.txt
rm log_exe.txt
rm log_run.txt
chmod +x `pwd`/autorun.sh
echo "@reboot " `pwd`/autorun.sh " > log_shell.txt 2>&1 &" | crontab
sudo /etc/init.d/cron start


# crontab -l
# echo "*/1 * * * * sh " `pwd`/autorun.sh | crontab
# sudo /etc/init.d/cron restart
# cat /var/log/syslog | grep cron
# sudo /etc/init.d/cron status

