@echo off

setlocal

set FUSE_HOME="%~dp0..\lib"

set CLASSPATH=%FUSE_HOME%\fuse.jar;%FUSE_HOME%\commons-cli.jar;%FUSE_HOME%\xerces.jar;%FUSE_HOME%\jruby-complete-1.4.0.jar

java -cp %CLASSPATH% com.mentor.nucleus.fuse.core.runners.CLIRunner %*

endlocal
