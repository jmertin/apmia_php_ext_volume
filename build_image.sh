#!/bin/sh

if [ -f ./03_gitcredentials ]
then
    source ./03_gitcredentials
fi

# Get installer data
if [ -f ./00_infosource.cfg ]
then
    source ./00_infosource.cfg
    source ./.build
   
fi

if [ -f build_image.cfg ]
then
    source ./build_image.cfg
else
    echo
    echo "*** ERROR: Missing build_image.cfg configuration file (normal on first run)!"
    echo "============================================================================================================="
    echo > build_image.cfg
    echo "# Go to the Agent Download Section, select to download the PHP Agent and open the \"Command line Download\"." | tee -a build_image.cfg
    echo "# and put it all in the below link - incl. the security Token. Exlude the wget command!" | tee -a build_image.cfg
    echo "FILE='<Insert here the download URL>'" | tee -a build_image.cfg
    exit 1
fi

#echo -n "Removing dangling docker images"
#docker rmi $(docker images -f "dangling=true" -q)

echo
echo -n ">>> Download latest APMIA/PHP archive [y/n]?: "
read DownLoad

if [ "$DownLoad" = "y" ]
   then
       rm -f PHP-apmia-*.tar

       ## Download repository
       wget --content-disposition $FILE
       if [ $? == 0 ]
       then
	   tar xf PHP-apmia-*.tar apmia/manifest.txt
	   PHPMONITVER=`grep php-monitor apmia/manifest.txt | cut -d ':' -f 2`
	   TMP=`ls PHP-apmia-*.tar`
	   FILENAME=`ls PHP-apmia-*.tar`
       else
	   echo "*** FATAL: Download failed. Exiting."
	   ecit 1
       fi
else
    tar xf PHP-apmia-*.tar apmia/manifest.txt
    PHPMONITVER=`grep php-monitor apmia/manifest.txt | cut -d ':' -f 2`
    TMP=`ls PHP-apmia-*.tar`
    FILENAME=`ls PHP-apmia-*.tar`   
fi

echo ">>> Found PHP Monitor $PHPMONITVER!"

# Checking for MySQL extension file. Take the most recent one if more than one exist.
EXTFILE=`ls -t extensions/mysql-*.tar.gz 2> /dev/null | head -1`

if [ -f "$EXTFILE" ]
then
    EXTNAME=`basename $EXTFILE`
    echo ">>> Found ${EXTNAME} APMIA Extension. Enabling in build."
    echo "*** Make sure the Extension is pre-configured for your MySQL DB you want to monitor!"
    EXTCOPY="COPY extensions\/${EXTNAME}\ \/opt\/apmia\/extensions\/deploy\/${EXTNAME}"
    cat Dockerfile.tpl | sed -e 's/%%EXTCOPY%%/'"${EXTCOPY}"'/g' > Dockerfile
else
    cat Dockerfile.tpl | sed -e '/%%EXTCOPY%%/d' > Dockerfile
fi 


echo
echo -n ">>> Build APMIA/PHP image [y/n]?: "
read Build

if [ "$Build" = "y" ]
   then

       echo
       echo "*** If you want to apply OS Update, don't use the cache."
       echo -n ">>> Use cache for build [y/n]?: "
       read Cache
       
       if [ "$Cache" == "y" ]
       then
           ### Build PCM
           docker build -t mertin/$FileBase --build-arg PHP_EXTENSION_VER=$PHPMONITVER --build-arg PHP_EXTENSION_FILE=$FILENAME .
       else
           ### Build PCM
           docker build --no-cache -t mertin/$FileBase --build-arg PHP_EXTENSION_VER=$PHPMONITVER --build-arg PHP_EXTENSION_FILE=$FILENAME .
       fi
       # Tag the built image
       echo "*** Tagging image to mertin/$FileBase:$PHPMONITVER" 
       docker tag mertin/$FileBase:latest mertin/$FileBase:$PHPMONITVER
       
fi
