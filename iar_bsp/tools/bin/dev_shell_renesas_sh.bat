@echo OFF

title Nucleus Development Shell (RENESAS SH)

@rem Move to the Nucleus directory.

cd %~dp0..\..\

@rem If it exists, put the toolset in the path and define SHC_INC, SHC_LIB, and SHC_TMP.
set SH_TOOLS_PATH=c:\program files\renesas\hew\tools\renesas\sh\9_4_1

if not exist "%SH_TOOLS_PATH%" goto :try_other_place

set PATH=%SH_TOOLS_PATH%\bin;%PATH%
set SHC_INC=%SH_TOOLS_PATH%\include
set SHC_LIB=%SH_TOOLS_PATH%\bin
set SHC_TMP=%TEMP%

goto :check_toolset

:try_other_place

set SH_TOOLS_PATH=c:\program files (x86)\renesas\hew\tools\renesas\sh\9_4_1

if not exist "%SH_TOOLS_PATH%" goto :check_toolset

set PATH=%SH_TOOLS_PATH%\bin;%PATH%
set SHC_INC=%SH_TOOLS_PATH%\include
set SHC_LIB=%SH_TOOLS_PATH%\bin
set SHC_TMP=%TEMP%

:check_toolset

@rem Check for toolset.

@optlnk > NUL 2>&1
if not errorlevel 1 goto :setup_env

echo **************************************************************************
echo * WARNING: The Renesas SH Tools do not seem to be in your path.
echo *          These tools must be installed in order to build.
echo **************************************************************************

:setup_env

@rem Setup path for Nucleus utilities.

set PATH=%~dp0;%PATH%
set PATH=%~dp0winnt\x86;%PATH%

@rem Put the JRE in the path.

set PATH=%~dp0jre6\bin;%PATH%

@rem Set toolset env var used by the make system.

set TOOLSET=renesas_sh

@rem Display help for the make system.

echo.
echo NOTE: Please type 'make help' to see the options for building.
echo.

