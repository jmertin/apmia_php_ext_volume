#!/bin/bash
#
# Executs the php-probe
PROBE_DIR=${PHP_PROBE_DIR:-/opt/apmia}

# Some checks
if [ ! -d $PROBE_DIR ]
then
    echo "*** FATAL: Probedir $PROBE_DIR does not exist. Emergency Exit!"
fi

# Some checks
if [ ! -x ${PROBE_DIR}/php-probe.sh ]
then
    echo "*** FATAL: PHP Probe installernot found. Emergency Exit!"
fi

# Checking if some parts are defined
# Application Name
if [ -n "${PHP_APPNAME}" ]
then
    SETAPPNAME="-appname \"${PHP_APPNAME}\""
fi

# Displayed Hostname
if [ -n "${PHP_AGENT_DISPLAYED_HOSTNAME}" ]
then
    SET_ADH="-agenthostname \"${PHP_AGENT_DISPLAYED_HOSTNAME}\""
fi

# Infrastructure Management HOST Host/IP. If running somewhere else, define it
if [ -n "${PHP_IAHOST}" ]
then
    SET_IAHOST="-iahost \"${PHP_IAHOST}\""
else
    SET_IAHOST="-iahost localhost"
fi

# Infratructure Management Agent Port
if [ -n "${PHP_IAPORT}" ]
then
    SET_IAPORT="-iahost \"${PHP_IAPORT}\""
else			
    SET_IAPORT="-iaport 5005"
fi	

# =====================================================================
# Set the below only if the script does not find the required locations
# =====================================================================

# PHP Root directory [Optional] - required if non standard
if [ -n "$PHP_ROOT" ]
then
    SET_PHPROOT="-phproot ${PHP_ROOT}"
fi

# PHP Extensions directory [Optional] - required if non standard
if [ -n "$PHP_EXT" ]
then
    SET_EXT="-ext ${PHP_EXT}"
fi

# PHP Extension configuration directory [Optional] - required if non standard
if [ -n "$PHP_INI" ]
then
    SET_INI="-ini ${PHP_INI}"
fi

# PHP probe Log directory [Optional] - required if non standard
if [ -n "$PHP_LOGDIR" ]
then
    SET_LOGDIR="-logdir ${PHP_LOGDIR}"
fi

# Changing to the probe installation directory
cd $PROBE_DIR

echo
echo " * Calling php-probe installer with the following options:"
echo " => $SETAPPNAME $SET_ADH $SET_IAHOST $SET_IOPORT $SET_PHPROOT $SET_EXT $SET_INI $SET_LOGDIR"
echo

# Actual probe installation happens here
./installer.sh $SETAPPNAME $SET_ADH $SET_IAHOST $SET_IOPORT $SET_PHPROOT $SET_EXT $SET_INI $SET_LOGDIR

# cleaning up installer mess
PHPINI=`find /etc /usr -name "wily_php_agent.ini" | tail -1`
sed -i 's/\"\"/\"/g' $PHPINI


echo "`date`" > /.PHP_PROBE_INSTALLED
