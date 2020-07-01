### Image description

This image build kit will create a passive image containing the PHP extension and/or APMIA Agent that can be started by a container at will.   

Depending on its deployment, it will implant the php-probe, or start up an APMIA collector already configured and connected to the DX APM SaaS environment it was downloaded from.

Note that because most PHP installations connect to a database, an optional mysql extension implant support has been added.

### Functional description of the PHP Agent/Probe

The below graph describes the setup in general.   
![Overview](PHP_APMIA_Docker.png)

The general approach to deploy the PHP Agent/Probe in an environment is as follows:

1. Deploy the Infrastructure Agent that 
    - sends monitoring data to the private or public cloud data collector
    - hosts the PHP extension
2. Deploy and activate the PHP extension inside the IA. The extension proxies data:
    - sends monitoring data to the IA (Infrastructure Agent)
    - receives monitoring data from the PHP probe (Installed at the WebServer Level/PHP section)
3. Deploy the PHP probe into one or more PHP installations. The probe captures monitoring data and sends them to the PHP extension (on port 5005 - by default)

**Note**: One APM IA (Infrastructure Agent) with the PHP Extension can receive monitoring data from multiple PHP Probes.   
To save resources you can therefore deploy only one IA with the PHP extension to serve multiple PHP probes.
Because the IA is heavyweight (Java) and the PHP probe is lightweight, about ~3MBytes).   
**Note 2**: Reduce the network path/hops the PHP Probe has to pass to transfer its findings to the IA (Infrastructure Agent). The network induce I/O can have an impact on the overall network infrastructure on highly loaded WebServers. The best approach is to use one IA per physical machine in the Docker cluster.

This documentation describes how to deploy an IA in a docker container and how to deploy a PHP probe in other docker containers for a resource efficient setup.



#### Dependency

None. Required the download links to PHP-APMIA Agent, and MySQL-apmia agent

#### Restrictions

- The APMIA image will share its /opt/apmia (default installation path inside the docker container) directory to other containers.   
This means that the configuration files, log-files etc. can be modified by any instance. So be careful which instance modifies any file in the /opt/apmia directory

### How to use this image

The idea is to use the APMIA Agent and the Probes in the same
environment, while only one APMIA Agent is required, many PHP probes
can report their data to it.

Using the Docker environment, a dedicated subnet will be created
connecting the APMIA and the probes together. Hence there will be no
requirement to open any ports. As all required incoming connections
will already be inside the docker LAN, and all other communication
flows will be initiated by the APMIA Agent to the outside world, in 
this case the DX APM SaaS Instance.   

To correctly use this image, the following steps must be undertaken:   
1. Deploy the APMIA as an Agent (Deploying as a APMIA Agent)   
   One APM IA needs to run to collect the data from the PHP Probes
2. Install the APM PHP Probe (Deploying as PHP Probe) one or more times. This requires:   
   - mount the /opt/apmia volume of the deployed APMIA Agent into the deployed container hosting the PHP application
   - Identify the ENTRYPOINT of the main application
   - Subseed the ENTRYPOINT with the apmia-image provided one


### Deploying as a APMIA Agent

Deploying as APMIA container is pretty straight forward.

Add the following into the docker-compose file:
```
  # Using the php-extension volume
  apmia-php-vol:
    image: mertin/apmia_php_ext_volume:latest
    container_name: apmia_container
    environment:
      # APMIA Configuration. Flag set to false creates a passive volume!
      - APMIA_DEPLOY=true
      - APMIA_AGENT_DISPLAYED_HOSTNAME=apmia-pod
      - APMIA_APP_NAME=php-collector
      - APMIA_LOG_LEVEL=WARN
      - APMIA_INTROSCOPE_AGENTNAME=apmia
    volumes:
      - /opt/apmia
```
**Note 1**: The volumes "/opt/apmia" is very important if one wants to use the same image for deploying the probes. See next chapter.   
**Note 2**: Do not use spaces in the app names. The installer does not like it.   


In the configuration, the following can be influenced:
- APMIA_DEPLOY [true|false]: Instructs the start script to either launch the APMIA Agent, or simply skip it and return a true exit value. Defaults to false
- APMIA_AGENT_DISPLAYED_HOSTNAME [default: apmia-container]: Hostname, default toi apmia-container. Docker will always create a random 16char hostname which looks bad in the Investigator.
- APMIA_APP_NAME [default: php-collector]: Application name, defaults to php-collector
- APMIA_LOG_LEVEL [default: WARN] Log level is set to WARN by default, to keep chattiness low.
- APMIA_INTROSCOPE_AGENTNAME [default: apmia]: Agent name

If the APMIA_DEPLOY tag is set to "true" - the APMIA Infrastructure Agent will deploy. If it is set to "false" this image will remain passive (no application will be run.   

**NOTE**: In passive mode, the container will exist and show an "Exit" status. This is normal! Do not delete it!   


### Deploying to implant the PHP Probe 

In a docker-compose file - set version to '2' - one needs to create a passive volume inclusion.

Even if you don't deploy the APMIA Container, it will have to exist in the docker-compose file as previous described (see "Deploying as a APMIA Agent"). The PHP Probes requires the image content to implant the probe binaries into the application php environment.   
If you don't want a APMIA Agent to run, set the "APMIA_DEPLOY=false". This will cause it to be a passive image.   


And in the application one wants to mount the passive image, add the following. Take into account that:
```
version: '2'
[...]
    entrypoint: /opt/apmia/run.sh    # This tells the docker-compose to use a different entry-points
    environment:                     # The environment to configure the PHP Probe are defined here.
      - PHP_DEPLOY=true
      - PHP_APPNAME=PCM_App
      - PHP_AGENT_DISPLAYED_HOSTNAME=pcm-dev
      - PHP_LOGDIR=/var/log
      - PHP_IAHOST=apmia_container
      - PHP_IAPORT=5005
      - ENTRYPOINT=/run.sh           # The entrypoint to use once the PHP Probe implant is done.
    volumes_from:
      - apmia-php-vol:rw
```
**Note**: In case the mount directory is not /opt/apmia - the installer script needs to be modified!   
**Note 2**: The PHP_IAHOST needs to match the container-name of the APMIA container or the APMIA Host!   
**Note 3**: The "apmia-php-vol" refers to APMIA Agent section (see "Deploying as a APMIA Agent").   
**Note 4**: Do not use spaces in the app names. The installer does not like it.   

To influence the installation of the PHP probe, one can use the following variables to be exposed in the docker-compose file:

- PHP_APPNAME: Sets the name of the Frontend application under which metrics will be reported. It is highly recommended to set an application name. Default value is "PHP App"
- PHP_AGENT_DISPLAYED_HOSTNAME: Sets custom agent hostname for PHP Probe agent
- PHP_IAHOST: Hostname/IP address of remote machine on which Infrastructure agent is running and listening for Probe agents.   
  If Infrastructure agent is running on the same machine as Probe agent this param can be ignored. Defaults to localhost if not set
- PHP_IAPORT: Port on which Infrastructure agent is listening for Probe agents (defaults to 5005 if not set)
- PHP_ROOT: PHP root directory
- PHP_EXT: PHP modules directory
- PHP_INI: PHP ini directory
- PHP_LOGDIR: PHP probe agent logs directory. WARNING: As this is a shared VOLUME, do not set the log directory to be in it!
- PHP_DEPLOY [true:false]: Trigger to install the probe
- PHP_PROBE_DIR: Directory the probe files exists. Defaults to /opt/apmia
- ENTRYPOINT: This points to the original entrypoint defined in the application image at build time. Identify it using the docker describe call with:   
```
$ docker inspect 900c9593e313 | grep ENTRYPOINT
                "ENTRYPOINT ["/run.sh"]"
```
Note: Sometimes the image uses a CMD instead of ENTRYPOINT. In that case, search for CMD and use that one instead.




### Building the apmia image

To create the APMIA Image, one need to provide the CLI Download link
of the PHP enabled APMIA package. Got to your DX APM SaaS Tenant, and
on the left side, into the "Agents" section => "Download
Agents". Under Applications select the "PHP" application.   

**NOTE**: The  **build_image.cfg** will be created on first run if non existent. The URL however needs to be provided by the user.   

Open the "Command line Download" entry - and extract the URL and apply it to the PHP_FILE Variable in the build_image.cfg file.

From the displayed:
```
wget --content-disposition 'https://apmgw.dxi-na1.saas.broadcom.com/acc/apm/acc/downloadpackage/43107?format=archive&layout=bootstrap_preferred&packageDownloadSecurityToken=233d4809807d21d1b24ae54639eede42f519dcbe35ca71d51bd051bca59c6a68'
```
extract only the URL
```
https://apmgw.dxi-na1.saas.broadcom.com/acc/apm/acc/downloadpackage/43107?format=archive&layout=bootstrap_preferred&packageDownloadSecurityToken=233d4809807d21d1b24ae54639eede42f519dcbe35ca71d51bd051bca59c6a68
```

Note: You can download the Agent manually but will have to upload it into the build directory - named as downloaded, for example:   
```
PHP-apmia-20200326_v1.tar
```

#### Adding the mysql extension

Downloading of the mysql extension with the PHP extension at the same time is not possible through the Admin interface.    

To have the mysql extension implanted, the build-script needs the APMIA with MySQL Extension CLI link. Got to your DX APM SaaS Tenant, and on the left side, into the "Agents" section => "Download Agents". Under Infrastructure  select the "MySQL" application.

Open the "Command line Download" entry - and extract the URL and apply it to the MYSQL_FILE Variable in the build_image.cfg file.

Note: You do not have to configure the mysql extension at the download page. It will be configured through the docker-environment variables.

Note: You can download the Agent manually but will have to upload it to the build directory - names as downloaded, for example:   
```
MySQL-apmia-20200701_v1.tar
```



### The actual build process

Launch the build script - it will ask which steps to perform:
```
jmertin@calypso:~/docker/apmia_php_ext_volume$ ./build_image.sh 

>>> Download latest APMIA/PHP/MySQL archive [y/n]?: n
>>> Found PHP Monitor 20.6.0.28!
>>> Found mysql-20.4.0.17-dd040260xt1.tar.gz APMIA Extension. Enabling in build.
*** Make sure the Extension is pre-configured for your MySQL DB you want to monitor!

>>> Build APMIA/PHP image [y/n]?: y

*** If you want to apply OS Update, don't use the cache.
>>> Use cache for build [y/n]?: y
Sending build context to Docker daemon  503.3MB
Step 1/12 : FROM ubuntu:18.04
 ---> c3c304cb4f22
Step 2/12 : ARG PHP_EXTENSION_VER
 ---> Using cache
 ---> a99e2f90b510
Step 3/12 : ARG PHP_EXTENSION_FILE
 ---> Using cache
 ---> 9560a082e86e
Step 4/12 : RUN mkdir -p /opt/apmia
 ---> Using cache
 ---> ed9d51952451
Step 5/12 : COPY $PHP_EXTENSION_FILE /opt/${PHP_EXTENSION_FILE}.bin
 ---> Using cache
 ---> c0607227c4b3
Step 6/12 : ADD php-probe.sh /opt/apmia/php-probe.sh
 ---> Using cache
 ---> 83633fdc7204
Step 7/12 : ADD run.sh /opt/apmia/run.sh
 ---> a462e62af81e
Step 8/12 : ADD extensions/deploy_extension.sh /opt/apmia/deploy_extension.sh
 ---> 88cb34197f92
Step 9/12 : RUN apt update &&  apt -y install bash nano && apt clean && tar xf /opt/${PHP_EXTENSION_FILE}.bin -C /opt && rm -f /opt/${PHP_EXTENSION_FILE}.bin
 ---> Running in ec5544cc387a

WARNING: apt does not have a stable CLI interface. Use with caution in scripts.                                                                                                                        

Get:1 http://security.ubuntu.com/ubuntu bionic-security InRelease [88.7 kB]
Get:2 http://archive.ubuntu.com/ubuntu bionic InRelease [242 kB]
Get:3 http://archive.ubuntu.com/ubuntu bionic-updates InRelease [88.7 kB]
Get:4 http://archive.ubuntu.com/ubuntu bionic-backports InRelease [74.6 kB]
Get:5 http://security.ubuntu.com/ubuntu bionic-security/main amd64 Packages [977 kB]
Get:6 http://archive.ubuntu.com/ubuntu bionic/universe amd64 Packages [11.3 MB]
Get:7 http://security.ubuntu.com/ubuntu bionic-security/multiverse amd64 Packages [9012 B]
Get:8 http://security.ubuntu.com/ubuntu bionic-security/restricted amd64 Packages [82.2 kB]
Get:9 http://security.ubuntu.com/ubuntu bionic-security/universe amd64 Packages [863 kB]
Get:10 http://archive.ubuntu.com/ubuntu bionic/main amd64 Packages [1344 kB]
Get:11 http://archive.ubuntu.com/ubuntu bionic/restricted amd64 Packages [13.5 kB]
Get:12 http://archive.ubuntu.com/ubuntu bionic/multiverse amd64 Packages [186 kB]
Get:13 http://archive.ubuntu.com/ubuntu bionic-updates/universe amd64 Packages [1403 kB]
Get:14 http://archive.ubuntu.com/ubuntu bionic-updates/multiverse amd64 Packages [13.6 kB]
Get:15 http://archive.ubuntu.com/ubuntu bionic-updates/main amd64 Packages [1293 kB]
Get:16 http://archive.ubuntu.com/ubuntu bionic-updates/restricted amd64 Packages [101 kB]
Get:17 http://archive.ubuntu.com/ubuntu bionic-backports/main amd64 Packages [8286 B]
Get:18 http://archive.ubuntu.com/ubuntu bionic-backports/universe amd64 Packages [8158 B]
Fetched 18.1 MB in 2s (11.4 MB/s)
Reading package lists...
Building dependency tree...
Reading state information...
5 packages can be upgraded. Run 'apt list --upgradable' to see them.

WARNING: apt does not have a stable CLI interface. Use with caution in scripts.                                                                                                                        

Reading package lists...
Building dependency tree...
Reading state information...
bash is already the newest version (4.4.18-2ubuntu1.2).
Suggested packages:
  spell
The following NEW packages will be installed:
  nano
0 upgraded, 1 newly installed, 0 to remove and 5 not upgraded.
Need to get 231 kB of archives.
After this operation, 778 kB of additional disk space will be used.
Get:1 http://archive.ubuntu.com/ubuntu bionic/main amd64 nano amd64 2.9.3-2 [231 kB]
debconf: delaying package configuration, since apt-utils is not installed
Fetched 231 kB in 0s (1978 kB/s)
Selecting previously unselected package nano.
(Reading database ... 4046 files and directories currently installed.)
Preparing to unpack .../nano_2.9.3-2_amd64.deb ...
Unpacking nano (2.9.3-2) ...
Setting up nano (2.9.3-2) ...
update-alternatives: using /bin/nano to provide /usr/bin/editor (editor) in auto mode
update-alternatives: warning: skip creation of /usr/share/man/man1/editor.1.gz because associated file /usr/share/man/man1/nano.1.gz (of link group editor) doesn't exist
update-alternatives: using /bin/nano to provide /usr/bin/pico (pico) in auto mode
update-alternatives: warning: skip creation of /usr/share/man/man1/pico.1.gz because associated file /usr/share/man/man1/nano.1.gz (of link group pico) doesn't exist

WARNING: apt does not have a stable CLI interface. Use with caution in scripts.

Removing intermediate container ec5544cc387a
 ---> 819a1b59b435
Step 10/12 : COPY extensions/mysql-20.4.0.17-dd040260xt1.tar.gz /opt/apmia/extensions/deploy/mysql-20.4.0.17-dd040260xt1.tar.gz
 ---> f80cad78cb23
Step 11/12 : RUN chmod g+w -R /opt/apmia && chmod 555 /opt/apmia/run.sh /opt/apmia/php-probe.sh /opt/apmia/deploy_extension.sh
 ---> Running in 9c1d051b5094
Removing intermediate container 9c1d051b5094
 ---> fbf0f3145203
Step 12/12 : CMD /opt/apmia/run.sh
 ---> Running in 76a02cf1869d
Removing intermediate container 76a02cf1869d
 ---> 5ac5bd62053a
Successfully built 5ac5bd62053a
Successfully tagged mertin/apmia_php_ext_volume:latest
*** Tagging image to mertin/apmia_php_ext_volume:20.6.0.28
```

The image will be created and the version tag applied to the image tag.
```english
jmertin@calypso:~/docker/apmia_php_ext_volume$ docker images | grep apmia_php
mertin/apmia_php_ext_volume   20.6.0.28                  5ac5bd62053a        3 minutes ago       619MB
mertin/apmia_php_ext_volume   latest                     5ac5bd62053a        3 minutes ago       619MB
mertin/apmia_php_ext_volume   20.4.0.17                  9389946ca153        3 hours ago         614MB
mertin/apmia_php_ext_volume   20.1.0.44                  d9789ba11ae3        3 months ago        639MB
```
