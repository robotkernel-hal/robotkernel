#
# Regular cron jobs for the robotkernel package
#
0 4	* * *	root	[ -x /usr/bin/robotkernel_maintenance ] && /usr/bin/robotkernel_maintenance
