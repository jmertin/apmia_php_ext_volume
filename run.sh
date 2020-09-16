#!/bin/bash
#
#
if [ "$APMIA_DEPLOY" = "true" ]
then
    INS_DIR=${APMIA_DIR:-/opt/apmia}

    if [ ! -d $INS_DIR ]
    then
	echo "*** FATAL: $INS_DIR directory does not exist. Emergency exit!"
    fi
    
    # Check if we want to add a mysql Monitor
    if [ "$MYSQL_MONITOR" = "true" ]
    then
	if [ -x ${APMIA_DIR:-/opt/apmia}/deploy_extension.sh ]
	then
	    echo "*** INFO: MySQL Extension active "
	fi
    else
	# We need to remove the mysql module from the deploy
	# directory, or the installer agent will deploy it anyway.
	echo "*** MySQL Extension not wanted. Purging..."
	rm -f ${APMIA_DIR:-/opt/apmia}/extensions/deploy/mysql-*.tar.gz
    fi

    
    # Implant configuration / defaults
    sync

    # Got to APMIA Install directory
    cd $INS_DIR
    # Cleaning leftovers or else APMIA restart of container/pod will fail without removal
    rm -f ./bin/apmia.pid
    # Execute APMIA
    ./APMIACtrl.sh force_start
else
    # In case we have an ENTRYPOINT?CMD defined, use that one instead
    if [ -n "$ENTRYPOINT" ]
    then
	# Deplpoy the probe.
	if [ "$PHP_DEPLOY" = "true" ]
	then
	    if [ -x ${PHP_PROBE_DIR:-/opt/apmia}/php-probe.sh ]
	    then
		echo " * Implanting PHP probe"
		# Execute php-probe.sh
		/bin/sh ${PHP_PROBE_DIR:-/opt/apmia}/php-probe.sh
	    else
		echo "*** FATAL: PHP deployment enabled, but probe-installer not found. Exiting!"
	    fi
	fi

	if [ ! -f $PHP_LOGDIR ]
	then
	    mkdir -p $PHP_LOGDIR
	    chown www-data.www-data $PHP_LOGDIR
	    chmod 775 $PHP_LOGDIR
	fi
	
	echo " * Handing over to real ENTRYPOINT: $ENTRYPOINT"
	# Execute real Entrypoint. Make sure it hooks itself to the shell.
	$ENTRYPOINT
    else
	# Exit with a true status
	/bin/true
    fi
fi
