@echo OFF

title Nucleus Development Shell (CS GNU PPC)

@rem Move to the Nucleus directory.

cd %~dp0..\..\

:check_toolset

@rem Check for toolset.

@powerpc-eabi-gcc -v 2>NUL
if not errorlevel 1 goto :setup_env

echo **************************************************************************
echo * WARNING: The Code Sourcery Tools for PPC do not seem to be in your path.
echo *          These tools must be installed in order to build.
echo **************************************************************************

:setup_env

@rem Setup path for Nucleus utilities.

set PATH=%~dp0;%PATH%
set PATH=%~dp0winnt\x86;%PATH%

@rem Put the JRE in the path.

set PATH=%~dp0jre6\bin;%PATH%

@rem ReadyStart - adding path to Codebench JRE
set PATH=%~dp0..\..\..\codebench\jre\bin;%PATH%

@rem Set toolset env var used by the make system.

set TOOLSET=csgnu_ppc

@rem Display help for the make system.

echo.
echo NOTE: Please type 'make help' to see the options for building.
echo.

