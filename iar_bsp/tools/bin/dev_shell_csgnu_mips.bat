@echo OFF

set CS_MIPS_TOOLSET_NAME=codesourcery_mips_4.5.2

title Nucleus Development Shell (CS GNU MIPS)

@rem Move to the Nucleus directory.

cd %~dp0..\..\

@rem If it exist, put the toolset in the path.
if not exist "%~dp0..\%CS_MIPS_TOOLSET_NAME%" goto :try_other_place

set PATH=%~dp0..\%CS_MIPS_TOOLSET_NAME%\bin;%PATH%

goto :check_toolset

:try_other_place

if not exist "%~dp0..\..\..\tools\%CS_MIPS_TOOLSET_NAME%" goto :check_toolset

set PATH=%~dp0..\..\..\tools\%CS_MIPS_TOOLSET_NAME%\bin;%PATH%

:check_toolset

@rem Check for toolset.

@mips-sde-elf-gcc -v 2>NUL
if not errorlevel 1 goto :setup_env

echo **************************************************************************
echo * WARNING: The Code Sourcery Tools for MIPS do not seem to be in your path.
echo *          These tools must be installed in order to build.
echo **************************************************************************

:setup_env

@rem Setup path for Nucleus utilities.

set PATH=%~dp0;%PATH%
set PATH=%~dp0winnt\x86;%PATH%

@rem Put the JRE in the path.

set PATH=%~dp0jre6\bin;%~dp0..\..\..\edge\eclipse\jre\bin;%PATH%

@rem Set toolset env var used by the make system.

set TOOLSET=csgnu_mips

@rem Display help for the make system.

echo.
echo NOTE: Please type 'make help' to see the options for building.
echo.

