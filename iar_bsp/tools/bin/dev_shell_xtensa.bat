@echo OFF

title Nucleus Development Shell (TENSILICA XTENSA)

set PATH=C:\usr\xtensa\XtDevTools\install\tools\RD-2011.2-win32\XtensaTools\bin;%PATH%

@rem Move to the Nucleus directory.

cd %~dp0..\..\

@rem Check for toolset.

@xt-ar -V > NUL 2>&1
if not errorlevel 1 goto :setup_env

echo **************************************************************************
echo * WARNING: The Tensilica Xtensa Tools do not seem to be in your path.
echo *          These tools must be installed in order to build.
echo **************************************************************************

:setup_env

@rem Setup path for Nucleus utilities.

set PATH=%~dp0;%PATH%
set PATH=%~dp0winnt\x86;%PATH%

@rem Put the JRE in the path.

set PATH=%~dp0jre6\bin;%PATH%

@rem Set toolset env var used by the make system.

set TOOLSET=tensilica

@rem Display help for the make system.

echo.
echo NOTE: Please type 'make help' to see the options for building.
echo.

