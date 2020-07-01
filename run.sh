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
    
    # Set some defaults
    AADH=${APMIA_AGENT_DISPLAYED_HOSTNAME:-apmia-container}
    ANAME=${APMIA_APP_NAME:-php-collector}
    ALOGLVL=${APMIA_LOG_LEVEL:-WARNING}
    AIAAG=${APMIA_INTROSCOPE_AGENTNAME:-apmia}

    # Check if we want to add a mysql Monitor
    if [ "$MYSQL_MONITOR" = "true" ]
    then
	if [ -x ${APMIA_DIR:-/opt/apmia}/deploy_extension.sh ]
	then
	    # executing mysql deployment
	    ${APMIA_DIR:-/opt/apmia}/deploy_extension.sh mysql
	else
	    echo "*** INFO: No mysql extension found. "
	fi
    else
	# We need to remove the mysql module from the deploy
	# directory, or the installer agent will deploy it anyway.
	echo "*** MySQL Extension not wanted. Purging..."
	rm -f ${APMIA_DIR:-/opt/apmia}/extensions/deploy/mysql-*.tar.gz
    fi

    
    # Implant configuration / defaults
    sed -i 's/^introscope.agent.agentName=.*/introscope.agent.agentName\='${AIAAG}'/g' ${INS_DIR}/core/config/IntroscopeAgent.profile
    sed -i 's/.*introscope.agent.application.name=.*/introscope.agent.application.name\='${ANAME}'/g' ${INS_DIR}/core/config/IntroscopeAgent.profile
    sed -i 's/^log4j.logger.IntroscopeAgent=.*/log4j.logger.IntroscopeAgent=INFO\='$ALOGLVL:=WARNING'\, console/g' ${INS_DIR}/core/config/IntroscopeAgent.profile
    echo "introscope.agent.hostName=${AADH}" >> ${INS_DIR}/core/config/IntroscopeAgent.profile
    sync

    # Got to APMIA Install directory
    cd $INS_DIR
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

	echo " * Handing over to real ENTRYPOINT: $ENTRYPOINT"
	# Execute real Entrypoint. Make sure it hooks itself to the shell.
	$ENTRYPOINT
    else
	# Exit with a true status
	/bin/true
    fi
fi
