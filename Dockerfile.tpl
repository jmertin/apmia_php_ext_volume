# Create the Docker-Container from the smallest possible source.
FROM ubuntu:18.04

# Set the Agent Version into a Var. This makes it easier to use it at different locations
ARG PHP_EXTENSION_VER
ARG PHP_EXTENSION_FILE

# Create the Agent target directory
RUN mkdir -p /opt/apmia

# Add the Tar-File to the Target Directory (*)
COPY $PHP_EXTENSION_FILE /opt/${PHP_EXTENSION_FILE}.bin
ADD php-probe.sh /opt/apmia/php-probe.sh
ADD run.sh /opt/apmia/run.sh
RUN apt update &&  apt -y install bash nano && apt clean && tar xf /opt/${PHP_EXTENSION_FILE}.bin -C /opt && rm -f /opt/${PHP_EXTENSION_FILE}.bin
%%EXTCOPY%%

# Make sure the group is able to write the files (required for OpenShift/Kubernetes)
RUN chmod g+w -R /opt/apmia && chmod 555 /opt/apmia/run.sh /opt/apmia/php-probe.sh


# This would be the Entrypoint - which returns a true statement
CMD /opt/apmia/run.sh