### Image description

This image build kit creates an image containing the **APM IA Agent + MySQL and PHP Extension**, and at the same time will provide all files required to implant the **PHP remote agent probe** into a PHP application container at start-time. The **implant** does not require the PHP application image to be modified upfront.

When deploying, the image can run in 2 operation modes:
1. Start up an APM IA (Infrastructure Agent) collector. Downloaded from DX SaaS, it is pre-configured to connect to the latter.
2. **PHP Remote Agent Probe** implant mode. It will implant the PHP Probe before starting the configured ENTRYPOINT/CMD of the PHP Application.

Note that because most PHP installations connect to a database, an optional mysql extension implant support has been added.

**It is assumed that the person using this fieldpack knows the APMIA Agent and how to deploy it, also has some basic knowledge in building a Docker Image and deploying it.**




### Functional description of the PHP Agent/Probe

The below graph describes a general deployment scenario.   
![Overview](PHP_APMIA_Docker.png)

To monitor a PHP Application, one needs 3 components to be deployed/configured and integrated.

1. Deploy an APM IA (Infrastructure Agent)
This is the actual Agent which will act as a proxy to the DX APM SaaS backend.
2. Enable the **PHP Extension** inside the APM IA.   
This will enable the APM IA to know what to do with the received data.
3. Deploy the **PHP Remote Agent Probe** into one or more PHP containers.   
The **PHP Remote Agent Probe** will collect data from non Java applications and forward these to the PHP Extension enabled APM IA Agent for processing.

**Note**: One APM IA (Infrastructure Agent) with the PHP Extension can receive monitoring data from multiple PHP remote agent Probes (And other remote agents probes).   
To save resources you can therefore deploy only one IA with the PHP extension to serve multiple PHP Remote Agent Probes.
(The IA is heavyweight (Java) and the PHP Remote Agent Probe is lightweight, about ~3MBytes).   
**Note 2**: Reduce the network path/hops the PHP remote Agent Probe has to use to transfer its metrics to the IA (Infrastructure Agent). The network induced I/O can have an impact on the overall network performance on highly loaded WebServers. 

It would be best to have a APM IA agent already deployed on each physical host of the docker cluster. That way one will receive metrics of the actual host running the docker environment.


#### Dependency

Requires the download links to PHP-APMIA Agent, and MySQL-apmia agent.

#### Restrictions

- The APM IA image will share its /opt/apmia (default installation path inside the docker container) directory to other containers.   This means that the configuration files, log-files etc. can be modified by any instance accessing it. So be careful which instance modifies any file in the /opt/apmia directory

### How to use this image

The idea is to use the APMIA Agent and the Probes in the same environment, while only one APMIA Agent is required, many PHP remote agent probes can report their data to it.

Using the Docker environment, a dedicated subnet will be created connecting the APMIA and the probes together. Hence there will be no requirement to open any ports as all required incoming connections will already be inside the docker LAN, and all other communication flows will be initiated by the APMIA Agent to the outside world, in this case the DX APM SaaS Instance.   

To correctly use this image, the following steps must be undertaken:   
1. Deploy one APMIA as an Agent. It can be on the host or inside a docker container. This image only supports the docker-container deployment mode!
2. Install the **APM PHP Remote Agent Probe** one or more times. 
   This requires:   
   - mount the /opt/apmia volume of the deployed APMIA Agent into the deployed container hosting the PHP application
   - Identify the ENTRYPOINT of the PHP application
   - Subseed the ENTRYPOINT with the PHP Application image provided one



**Note: You will first need to build the image for your environment.**




### Deploying as running APMIA Agent

Deploying as APMIA Agent container is pretty straight forward.

Add the following into the docker-compose file:
```
  # Using the php-extension volume
  apmia-php-vol:
    image: mertin/apmia_php_ext_volume:latest
    container_name: apmia-php-vol   # The name used inside the PHP App to send probe data
    environment:
      # APMIA Configuration. Flag set to false creates a passive volume!
      - APMIA_DEPLOY=true
      - APMENV_INTROSCOPE_AGENT_AGENTNAME=apmia
      - APMENV_INTROSCOPE_AGENT_HOSTNAME=calypso-apmia-dev
      - APMENV_INTROSCOPE_AGENT_APPLICATION_NAME=php-collector
      - APMENV_LOG4J_LOGGER_INTROSCOPEAGENT=WARN, logfile
      # If activating the APMIA Extension, set below to true and fill in the required fields.
      # These entries will overwrite the pre-configuration from the Download.
      - MYSQL_MONITOR=true
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_PROFILES=db1
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_QUERY_SNAPSHOTS=false
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_QUERYINTERVAL=15
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_TRACEINTERVAL=30
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_PROFILES_DB1_INSTANCENAME=DatabaseName
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_PROFILES_DB1_HOSTNAME=DatabaseHostname
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_PROFILES_DB1_PORT=3306
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_PROFILES_DB1_USERNAME=monitoruser
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_PROFILES_DB1_PASSWORD=monitoruserPWD
      - APMENV_INTROSCOPE_AGENT_DBMONITOR_MYSQL_PROFILES_DB1_VERSION=
    volumes:
      - /opt/apmia
```
**Note 1**: The volume "/opt/apmia" is very important if one wants to use the same image for deploying the probes. See next chapter.   
**Note 2**: Do not use spaces in the app names. The **APM PHP Remote Agent Probe** installer does not like it.   


In the configuration, the following can be influenced:
- APMIA_DEPLOY [true|**false**]: If set to **true** the deployment process will launch the APM IA Agent with configured extensions (Default PHP Extension and if wanted, MySQL Extension).   
  
  If set to **false**, this image will remain passive (no application will run) but exit to provide the volume for implanting the **APM PHP Remote Agent Probe**.  
  **NOTE**: In passive mode, the container will exist and show an "Exit 0" status. This is normal! Do not delete it!  
  
- The configuration of  the APM IA is done through the usual Variables with APMENV as prefix.
  You can use any of the properties in the **InfrastructureAgent.profile** to further configure your environment (See [Infrastructure Agent Properties Reference](https://techdocs.broadcom.com/content/broadcom/techdocs/us/en/ca-enterprise-software/it-operations-management/dx-apm-saas/SaaS/implementing-agents/infrastructure-agent/infrastructure-agent-properties-reference.html) for details). 
You must replace the periods in the property names with underscores. In addition,  you must prefix each property with apmenv_ (upper or lower-case).    
  For example, to configure the agent name and a proxy server for Docker Swarm, you would use the introscope.agent.agentName and introscope.agent.enterprisemanager.transport.http.proxy.host properties (optional values in bold) as follows:   
  
  ```
       - APMENV_INTROSCOPE_AGENT_AGENTNAME=example_agent_name
       - APMENV_INTROSCOPE_AGENT_ENTERPRISEMANAGER_TRANSPORT_HTTP_PROXY_HOST=example_proxy_server.com
  ```



If using docker-compose, running the agent will issue one message in the logs:

```
[cemadm@lvnprod005006 cemperf]$ docker-compose logs apmia-php-vol
Attaching to apmia-php-vol
apmia-php-vol    | APM Infrastructure Agent Force Start in Progress...
apmia-php-vol    | Running APM Infrastructure Agent in force_start mode... (To stop force_start use ctrl+c)
```

Once can check the APMIA logs that everything runs fine by going into the running container with:

```
docker exec -it apmia-php-vol /bin/bash
root@d75da5429526:/# cd /opt/apmia/logs/
root@d75da5429526:/opt/apmia/logs# cat IntroscopeAgent.log 
7/08/20 11:45:35 AM GMT [WARN] [IntroscopeAgent.InfrastructureAgent] Attempt to change fixed value of Configuration property (introscope.agent.hostName) to null ignored
7/08/20 11:45:35 AM GMT [WARN] [IntroscopeAgent.InfrastructureAgent] Attempt to change fixed value of Configuration property (introscope.agent.application.name) to "" ignored
7/08/20 11:45:35 AM GMT [WARN] [IntroscopeAgent.InfrastructureAgent] Attempt to change fixed value of Configuration property (introscope.agent.hostName) to null ignored
7/08/20 11:45:35 AM GMT [WARN] [IntroscopeAgent.InfrastructureAgent] Attempt to change fixed value of Configuration property (introscope.agent.application.name) to "" ignored
7/08/20 11:45:35 AM GMT [WARN] [IntroscopeAgent.InfrastructureAgent] Transaction Tracer user ID has not been configured.  User ID will not be reported with transactions.
7/08/20 11:45:35 AM GMT [WARN] [IntroscopeAgent.InfrastructureAgent] Attempt to change fixed value of Configuration property (introscope.agent.application.name) to "" ignored
7/08/20 11:45:35 AM GMT [WARN] [IntroscopeAgent.InfrastructureAgent] Transaction Tracer user ID has not been configured.  User ID will not be reported with transactions.
7/08/20 11:45:35 AM GMT [WARN] [IntroscopeAgent.BoundaryDetection] Automatic Boundary Detection Service is turned off 
7/08/20 11:47:40 AM GMT [WARN] [IntroscopeAgent.InfrastructureAgent] Absolute metric clamp of 100 reached, no more new SQL metrics can be created. All metrics will be reported under the default bucket. If you wish to increase the clamp value, change the property - introscope.agent.sqlagent.statement.clamp

```






### Deploying to implant the PHP remote agent Probe / passive APMIA volume 

In a docker-compose file - set version to '2' - one needs to create a passive volume inclusion. Adapt/convert to other versions.

Even if you don't intend to run the APM IA Agent, the container definition will have to exist in the docker-compose file in "passive mode" as previously described. The **PHP Remote Agent Probe** requires the image content to implant the probe binaries into the application php environment.   
If you don't want a APMIA Agent to run, set the "APMIA_DEPLOY=false". This will cause it to remain a passive container.   


In the container definition running the PHP Application, add the following tags:
```
version: '2'
[...]
    entrypoint: /opt/apmia/run.sh    # This tells the docker-compose to use a different entry-point
    depends_on:
      - apmia-php-vol                # The apmia-php-vol is required, as it hosts the probe files
    environment:                     # The environment to configure the PHP Probe are defined here
      - PHP_DEPLOY=true
      - PHP_APPNAME=PCM_App
      - PHP_AGENT_DISPLAYED_HOSTNAME=hostname
      - PHP_LOGDIR=/var/log
      - PHP_IAHOST=apmia-php-vol     # Defines the target APMIA Agent to send metrics
      - PHP_IAPORT=5005
      - ENTRYPOINT=/run.sh           # The entrypoint to use once the PHP Probe implant is done
    volumes_from:
      - apmia-php-vol:rw
```
**Note**: In case the mount directory is not /opt/apmia - the installer script needs to be modified!   
**Note 2**: The PHP_IAHOST needs to match the container-name of the APMIA container name or the APMIA Host!   
**Note 3**: The "apmia-php-vol" refers to APMIA Agent section (see "Deploying as running APMIA Agent").   
**Note 4**: Do not use spaces in the app names. The installer does not like it.   

To influence the installation of the PHP probe, one can use the following variables to be exposed in the docker-compose file. Note that not all are mandatory, as the implant script does a good job at identifying what needs to be used.

- PHP_APPNAME: Sets the name of the Frontend application under which metrics will be reported. It is highly recommended to set an application name. Default value is "PHP_App"
- PHP_AGENT_DISPLAYED_HOSTNAME: Sets custom agent hostname for PHP Probe agent
- PHP_IAHOST [Mandatory]: Hostname/IP address of remote machine on which Infrastructure agent is running and listening for Probe agents.   
  If Infrastructure agent is running on the same machine as Probe agent this param can be ignored. Defaults to localhost if not set
- PHP_IAPORT [Mandatory]: Port on which Infrastructure agent is listening for Probe agents (defaults to 5005 if not set)
- PHP_ROOT: PHP root directory
- PHP_EXT: PHP modules directory
- PHP_INI: PHP ini directory
- PHP_LOGDIR: PHP probe agent logs directory. WARNING: As this is a shared VOLUME, do not set the log directory to be in it!
- PHP_DEPLOY[Mandatory] [true:false]: Trigger to install the **PHP Remote Agent Probe**.
- PHP_PROBE_DIR: Directory the probe files exists. Defaults to /opt/apmia
- ENTRYPOINT [Mandatory]: This points to the original entrypoint defined in the application image at build time. Identify it using the docker describe call with:   
```
$ docker inspect 900c9593e313 | grep ENTRYPOINT
                "ENTRYPOINT ["/run.sh"]"
```
**Note**: Sometimes the image uses a CMD or Entrypoint instead of ENTRYPOINT.  You need to find the "real" entrypoint of the application. Sometimes, scripts are being used overriding the Entrypoint, or CMD is used. Last resort is to deploy the image as container, and check which entrypoint has been used. You will find the entrypoint as the process running with the PID == 1.  
For example:

```
[cemadm@lvnprod005006 cemperf]$ docker exec -it cemperf /bin/bash
[root@acc790fb507d app]# ps auxw
USER        PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root          1  0.0  0.0  11356  1404 ?        Ss   13:27   0:00 /bin/bash /run.sh <= PID == 1
root         58  0.0  0.0  11352  1340 ?        S    13:27   0:00 /bin/sh /usr/sbin/apachectl -DFOREGROUND
root         62  0.0  0.0 400172 15120 ?        Sl   13:27   0:00 /usr/sbin/httpd -DFOREGROUND
apache       64  0.0  0.0 400172  6400 ?        S    13:27   0:00 /usr/sbin/httpd -DFOREGROUND
apache       65  0.0  0.0 400172  6164 ?        S    13:27   0:00 /usr/sbin/httpd -DFOREGROUND
apache       66  0.0  0.0 400172  6164 ?        S    13:27   0:00 /usr/sbin/httpd -DFOREGROUND
apache       67  0.0  0.0 400172  6164 ?        S    13:27   0:00 /usr/sbin/httpd -DFOREGROUND
apache       68  0.0  0.0 400172  6164 ?        S    13:27   0:00 /usr/sbin/httpd -DFOREGROUND
apache       69  0.0  0.0 400172  6164 ?        S    13:27   0:00 /usr/sbin/httpd -DFOREGROUND
apache       70  0.0  0.0 400172  6164 ?        S    13:27   0:00 /usr/sbin/httpd -DFOREGROUND
apache       71  0.0  0.0 400172  6164 ?        S    13:27   0:00 /usr/sbin/httpd -DFOREGROUND
root         72  0.6  0.0 108368  1752 pts/0    Ss   13:31   0:00 /bin/bash
root         86  0.0  0.0 110244  1156 pts/0    R+   13:31   0:00 ps auxw
```



 

Once configured, you can start the PHP Application container and verify the **PHP Remote Agent Probe** implant worked by looking at the logs:

```
[cemadm@lvnprod005006 cemperf]$ docker-compose up -d
Creating network "cemperf_default" with driver "bridge"
Creating cemperf-db    ... done
Creating apmia-php-vol ... done
Creating cemperf       ... done
Creating haproxy       ... done
[cemadm@lvnprod005006 cemperf]$ docker-compose logs cemperf
Attaching to cemperf
cemperf          |  * Implanting PHP probe
cemperf          | 
cemperf          |  * Calling php-probe installer with the following options:
cemperf          |  => -appname "CEMPerfDB" -agenthostname "lvnprod005006" -iahost "apmia-php-vol"     -logdir "/var/log/httpd"
cemperf          | 
cemperf          | 
cemperf          |   Installing CA APM PHP Probe Agent...
cemperf          | 
cemperf          |   Looking for PHP Installations on this machine...
cemperf          | 
cemperf          |   Installer found following PHP installations on this machine
cemperf          | 
cemperf          |    1) /usr/bin
cemperf          | 
cemperf          |   Installing CA APM PHP Probe Agent at all discovered PHP installation(s)...
cemperf          | 
cemperf          | 
cemperf          | 
cemperf          |   Installation Overview:
cemperf          | 
cemperf          | 1) PHP Root : /usr/bin/php
cemperf          |    PHP Version : 5.3.3
cemperf          |    PHP Extensions directory : /usr/lib64/php/modules
cemperf          |    PHP Zend Thread Safety : Disabled
cemperf          |    PHP ini directory : /etc/php.d
cemperf          |    Installation Status : Success
cemperf          | 
cemperf          | 
cemperf          |    CA APM PHP Probe Agent is successfully installed on your machine.
cemperf          |    
cemperf          |    1) To modify default CA APM PHP Probe agent settings update file /etc/php.d/wily_php_agent.ini
cemperf          |    2) Restart your web server or your PHP-FPM process to see performance metrics.
cemperf          |      
cemperf          | chown: invalid user: `www-data.www-data'
cemperf          |  * Handing over to real ENTRYPOINT: /run.sh

```

The log will tell if everything went as it is supposed to.

From here on, if the application is used, metrics should start to show up in the DX APM SaaS UI




### Building the apmia image

the first build-attempt will cause an error message:

```
jmertin@calypso:~/docker/apmia_php_ext_volume$ ./build_image.sh 

*** ERROR: Missing build_image.cfg configuration file (normal on first run)!
=============================================================================================================
# Go to the Agent Download Section, select to download the PHP Agent and open the "Command line Download".
# and put it all in the below link - incl. the security Token. Exclude the wget command!
PHP_FILE='<Insert here the download CLI URL for the PHP APMIA Agent>'

# Go to the Agent Download Section, select to download the MYSQL Agent and open the "Command line Download".
# and put it all in the below link - incl. the security Token. Exclude the wget command!
MYSQL_FILE='<Insert here the download CLI URL for the MYSQL APMIA Agent>'
```

Running it the first time will create the template configuration file. Into that file, one needs to enter both the APMIA PHP and MYSQL Download link.

To create the APMIA Image, one need to provide the CLI Download link of the PHP enabled APMIA package. Got to your DX APM SaaS Tenant, and on the left side, into the "Agents" section => "Download Agents". Under Applications select the "PHP" application.   

Open the "Command line Download" entry - and extract the URL and apply it to the **PHP_FILE** Variable in the **build_image.cfg** file.

From the displayed:
```
wget --content-disposition 'https://apmgw.dxi-na1.saas.broadcom.com/acc/apm/acc/downloadpackage/43107?format=archive&layout=bootstrap_preferred&packageDownloadSecurityToken=99de9fe257187c2685875a2dc054dbb5656e9d5e0a25749fa7def1fccb32b4dc'
```
extract only the URL
```
https://apmgw.dxi-na1.saas.broadcom.com/acc/apm/acc/downloadpackage/43107?format=archive&layout=bootstrap_preferred&packageDownloadSecurityToken=99de9fe257187c2685875a2dc054dbb5656e9d5e0a25749fa7def1fccb32b4dc
```

and put it in the appropriate place in the  **build_image.cfg** file.

Note: You can download the Agent manually but will have to upload it into the build directory - named as downloaded, for example:   

```
PHP-apmia-20200701_v1.tar
```



#### Adding the mysql extension

Downloading of the mysql extension with the PHP extension at the same time is not possible through the Agent Download  interface.    

To have the mysql extension implanted, the build-script needs the APMIA with MySQL Extension CLI link. Got to your DX APM SaaS Tenant, and on the left side, into the "Agents" section => "Download Agents". Under Infrastructure  select the "MySQL" application.

Open the "Command line Download" entry - and extract the URL and apply it to the **MYSQL_FILE** Variable in the build_image.cfg file.

**Note**: You do not have to configure the mysql extension at the download page. It will be configured through the docker-environment variables.

**Note**: You can download the Agent manually but will have to upload it to the build directory - names as downloaded, for example:   

```
MySQL-apmia-20200701_v1.tar
```




### The actual build process

Launch the build script - it will ask which steps to perform (below example does not download the files and uses cache to make it shorter)
```
jmertin@calypso:~/docker/apmia_php_ext_volume$ ./build_image.sh 

>>> Download latest APMIA/PHP/MySQL archive [y/n]?: n
>>> Found PHP Monitor 20.6.0.28!
>>> Found mysql-dd040260xt4-20.6.0.28.tar.gz APMIA Extension. Enabling in build.
*** Make sure the Extension is pre-configured for your MySQL DB you want to monitor!

>>> Build APMIA/PHP image [y/n]?: y

*** If you want to apply OS Update, don't use the cache.
>>> Use cache for build [y/n]?: y
Sending build context to Docker daemon  503.5MB
Step 1/11 : FROM ubuntu:18.04
 ---> c3c304cb4f22
Step 2/11 : ARG PHP_EXTENSION_VER
 ---> Using cache
 ---> a99e2f90b510
Step 3/11 : ARG PHP_EXTENSION_FILE
 ---> Using cache
 ---> 9560a082e86e
Step 4/11 : RUN mkdir -p /opt/apmia
 ---> Using cache
 ---> e7036bc11fbd
Step 5/11 : COPY $PHP_EXTENSION_FILE /opt/${PHP_EXTENSION_FILE}.bin
 ---> Using cache
 ---> acb7b2afedca
Step 6/11 : ADD php-probe.sh /opt/apmia/php-probe.sh
 ---> Using cache
 ---> 0e50136f980e
Step 7/11 : ADD run.sh /opt/apmia/run.sh
 ---> Using cache
 ---> e43f66282d4f
Step 8/11 : RUN apt update &&  apt -y install bash nano && apt clean && tar xf /opt/${PHP_EXTENSION_FILE}.bin -C /opt && rm -f /opt/${PHP_EXTENSION_FILE}.bin
 ---> Running in d0294b0827a5


Get:1 http://security.ubuntu.com/ubuntu bionic-security InRelease [88.7 kB]
Get:2 http://archive.ubuntu.com/ubuntu bionic InRelease [242 kB]
Get:3 http://archive.ubuntu.com/ubuntu bionic-updates InRelease [88.7 kB]
Get:4 http://archive.ubuntu.com/ubuntu bionic-backports InRelease [74.6 kB]
Get:5 http://security.ubuntu.com/ubuntu bionic-security/main amd64 Packages [1003 kB]
Get:6 http://security.ubuntu.com/ubuntu bionic-security/restricted amd64 Packages [89.0 kB]
Get:7 http://security.ubuntu.com/ubuntu bionic-security/universe amd64 Packages [869 kB]
Get:8 http://security.ubuntu.com/ubuntu bionic-security/multiverse amd64 Packages [9282 B]
Get:9 http://archive.ubuntu.com/ubuntu bionic/universe amd64 Packages [11.3 MB]
Get:10 http://archive.ubuntu.com/ubuntu bionic/multiverse amd64 Packages [186 kB]
Get:11 http://archive.ubuntu.com/ubuntu bionic/restricted amd64 Packages [13.5 kB]
Get:12 http://archive.ubuntu.com/ubuntu bionic/main amd64 Packages [1344 kB]
Get:13 http://archive.ubuntu.com/ubuntu bionic-updates/multiverse amd64 Packages [13.6 kB]
Get:14 http://archive.ubuntu.com/ubuntu bionic-updates/universe amd64 Packages [1406 kB]
Get:15 http://archive.ubuntu.com/ubuntu bionic-updates/main amd64 Packages [1302 kB]
Get:16 http://archive.ubuntu.com/ubuntu bionic-updates/restricted amd64 Packages [102 kB]
Get:17 http://archive.ubuntu.com/ubuntu bionic-backports/universe amd64 Packages [8158 B]
Get:18 http://archive.ubuntu.com/ubuntu bionic-backports/main amd64 Packages [8286 B]
Fetched 18.2 MB in 2s (11.8 MB/s)
Reading package lists...
Building dependency tree...
Reading state information...
8 packages can be upgraded. Run 'apt list --upgradable' to see them.

Reading package lists...
Building dependency tree...
Reading state information...
bash is already the newest version (4.4.18-2ubuntu1.2).
Suggested packages:
  spell
The following NEW packages will be installed:
  nano
0 upgraded, 1 newly installed, 0 to remove and 8 not upgraded.
Need to get 231 kB of archives.
After this operation, 778 kB of additional disk space will be used.
Get:1 http://archive.ubuntu.com/ubuntu bionic/main amd64 nano amd64 2.9.3-2 [231 kB]
debconf: delaying package configuration, since apt-utils is not installed
Fetched 231 kB in 0s (2009 kB/s)
Selecting previously unselected package nano.
(Reading database ... 4046 files and directories currently installed.)
Preparing to unpack .../nano_2.9.3-2_amd64.deb ...
Unpacking nano (2.9.3-2) ...
Setting up nano (2.9.3-2) ...
update-alternatives: using /bin/nano to provide /usr/bin/editor (editor) in auto mode
update-alternatives: warning: skip creation of /usr/share/man/man1/editor.1.gz because associated file /usr/share/man/man1/nano.1.gz (of link group editor) doesn't exist
update-alternatives: using /bin/nano to provide /usr/bin/pico (pico) in auto mode
update-alternatives: warning: skip creation of /usr/share/man/man1/pico.1.gz because associated file /usr/share/man/man1/nano.1.gz (of link group pico) doesn't exist

Removing intermediate container d0294b0827a5
 ---> c24fc6a38e2e
Step 9/11 : COPY extensions/mysql-dd040260xt4-20.6.0.28.tar.gz /opt/apmia/extensions/deploy/mysql-dd040260xt4-20.6.0.28.tar.gz
 ---> 454b897800e8
Step 10/11 : RUN chmod g+w -R /opt/apmia && chmod 555 /opt/apmia/run.sh /opt/apmia/php-probe.sh
 ---> Running in 89fe495d9d3a
Removing intermediate container 89fe495d9d3a
 ---> 2db4898bcb7b
Step 11/11 : CMD /opt/apmia/run.sh
 ---> Running in 4f03c7b6a656
Removing intermediate container 4f03c7b6a656
 ---> cbef09956891
Successfully built cbef09956891
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
