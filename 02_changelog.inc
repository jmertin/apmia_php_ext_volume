Mon Jul  6 11:08:36 CEST 2020
	- Added MySQL extension to APMIA Agent
	- New PHP remote agent probe implant using Entrypoint superseed method. Does not require to
	modify the applicaiton image entrypoint/cmd script.
	- Documentation updates
	- Image build script enhancements
	- Changed base image from Alpine to Ubuntu 18.04 LTS due to glibc issues
	- Github version upped to 2.0
	- Had to export MySQL Extension inclusion into build-file. requires Dockerfile.tpl file to generate final build file.
	
Fri Mar 27 10:52:53 CET 2020
	- Initial release
