#!/bin/bash
#
#
if [ $APMIA_DEPLOY = "true" ]
then
    INS_DIR=${APMIA_DIR:-/opt/apmia}

    if [ ! -d INS_DIR ]
    then
	echo "*** FATAL: $INS_DIR directory does not exist. Emergency exit!"
    fi
    
    # Set some defaults
    AADH=${APMIA_AGENT_DISPLAYED_HOSTNAME:-apmia-container}
    ANAME=${APMIA_APP_NAME:-php-collector}
    ALOGLVL=${APMIA_LOG_LEVEL:-WARNING}
    AIAAG=${APMIA_INTROSCOPE_AGENTNAME:-apmia}

    # Implant configuration / defaults
    sed -i 's/^introscope.agent.agentName=.*/introscope.agent.agentName\='${AIAAG}'/g' ${INS_DIR}/core/config/IntroscopeAgent.profile
    sed -i 's/.*introscope.agent.application.name=.*/introscope.agent.application.name\='${ANAME}'/g' ${INS_DIR}/core/config/IntroscopeAgent.profile
    sed -i 's/^log4j.logger.IntroscopeAgent=.*/log4j.logger.IntroscopeAgent=INFO\='$ALOGLVL:=WARNING'\, console/g' ${INS_DIR}/core/config/IntroscopeAgent.profile
    echo "introscope.agent.hostName=${AADH}" >> ${INS_DIR}/core/config/IntroscopeAgent.profile
    sync

    cd $INS_DIR
    ./APMIACtrl.sh force_start
else
    # Exit with a true status
    /bin/true
fi
