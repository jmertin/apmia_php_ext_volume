#!/bin/bash
#

if [ $DEVEL ]
then
    source ./dev_env.sh
fi

# Catch prefix to run
prefix=$1

# Executs the php-probe
PROBE_DIR=${PHP_PROBE_DIR:-/opt/apmia}

# Extension directory
EXT_DIR=${PHP_EXT_DIR:-/opt/apmia/extensions}

# Define default error level
errlvl=0

cd ${EXT_DIR}/deploy

if [ -f ${prefix}*.tar.gz ]
then
    # Cycle through all tar-files
    for file in `ls ${prefix}*.tar.gz`
    do
	DIRNAME=`basename $file .tar.gz`
	echo ">>> Processing $DIRNAME"
	mkdir -p ${EXT_DIR}/${DIRNAME}
	err=$?
	if [ $err -ne 0 ]
	then
	    errlvl=$err # catch error level in case we have an issue
	fi
	tar zxf $file -C ${EXT_DIR}/${DIRNAME}
	err=$?
	if [ $err -ne 0 ]
	then
	    errlvl=$err # catch error level in case we have an issue
	fi

	# Create .deployment file
	echo "Name: $DIRNAME" > ${EXT_DIR}/${DIRNAME}/.deployment
	echo "InstallDate: `date -u`" >> ${EXT_DIR}/${DIRNAME}/.deployment
	echo "Checksum: `md5sum $file | cut -d ' ' -f 1 `" >> ${EXT_DIR}/${DIRNAME}/.deployment
	echo "Status: $errlvl" >> ${EXT_DIR}/${DIRNAME}/.deployment
	echo
	echo "================================================================================="
	echo "You can configure the following properties in the docker-compose/yaml file"
	echo "Prepend a: "

	grep "introscope.agent.dbmonitor.${prefix}." ${EXT_DIR}/${DIRNAME}/bundle.properties | grep -v ^# | sed -e 's/\./_/g' | cut -d '=' -f 1 
	echo
	# Backing up bundle-properties file
	# mv ${EXT_DIR}/${DIRNAME}/bundle.properties ${EXT_DIR}/${DIRNAME}/bundle.properties.bak
	echo "# $DIRNAME `date -u` by extension installer " > ${EXT_DIR}/${DIRNAME}/bundle.properties

	# Cycle through existing configuration in Environment and extract
	# it, convert it Upper to Lower and replace the _ by ., finaly
	# write it to configuration file.
	for env in `set | grep introscope_agent_dbmonitor_${prefix}`
	do
	    varname=`echo $env | cut -d '=' -f 1`
	    var=`echo $env | cut -d '=' -f 2`
	    newvarname=`echo $varname | sed -e 's/_/\./g'`
	    echo "${newvarname}=${var}" >> ${EXT_DIR}/${DIRNAME}/bundle.properties
	done
	
	# Tell APMIA which exension to load
	if [ ! $DEVEL ]
	then
	    sed -i 's/^introscope\.agent\.extensions\.bundles\.load=/introscope\.agent\.extensions\.bundles\.boot\.load='${DIRNAME}'/g' /opt/apmia/extensions/Extensions.profile
	fi
    done
else
    echo "ERROR: Extension $prefix not found. "
    echo "       You may want to add the MySQL Extension to the build"
    echo "       or set the MYSQL_MONITOR=false flag."
fi
