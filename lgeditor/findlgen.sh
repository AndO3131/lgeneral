#!/bin/sh
echo >&2 "Trying to find LGeneral automatically..."
x=$(find /usr | grep lgeneral/scenarios | sed -e 's/scenarios//')
if [ -z "$x" ]; then
	>&2 echo "********************************************************"
	>&2 echo "Oops, cannot find LGeneral. Did you install it?"
	>&2 echo "If so, please use configure option --with-lgendir=..."
	>&2 echo "to specify the location manually, e.g.,"
	>&2 echo "  ./configure --with-lgendir=/home/myself/tmp/lgeneral"
	>&2 echo "********************************************************"
	echo NOTFOUND
else
	y=$(find /usr | grep lgeneral/maps/pg.tdb)
	if [ -z "$y" ]; then
		>&2 echo "WARNING: LGeneral found but maps/pg.tdb is missing..."
		>&2 echo "WARNING: You need LGeneral data as well, run lgc-pg for this."
	fi
	echo $x
fi
