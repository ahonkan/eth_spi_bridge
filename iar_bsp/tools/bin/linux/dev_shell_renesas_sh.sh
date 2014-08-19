#!/bin/sh

#PS1=`\e]0;Nucleus Development Shell (RENESAS SH)\a`


# See if the script is called through link
SCRIPT_PATH=$(readlink -f "$0")

if [ -z "$SCRIPT_PATH" ] ; then
  # It is called directly
    SCRIPT_PATH=$0
fi

SCRIPT_PATH=$(dirname $SCRIPT_PATH)

# Move to the Nucleus directory.
cd $SCRIPT_PATH/../../../

# Function to setup environment for ReadyStart development shell
setup_env () {

    # Setup path for Nucleus utilities.

    PATH="$SCRIPT_PATH:$PATH"
    PATH="$SCRIPT_PATH/x86:$PATH"

    # Put the JRE in the path.
    PATH="$SCRIPT_PATH/../../../../Uninstall/jre/bin:$PATH"

    # ReadyStart - adding path to Codebench JRE
    PATH="$SCRIPT_PATH/../../../../codebench/jre/bin:$PATH"

    # Set toolset env var used by the make system.
    export TOOLSET="renesas_sh"

    # Display help for the make system.

    echo .
    echo "NOTE: Please type 'make help' to see the options for building."
    echo .

    return
}

print_welcome () {
    echo "**************************************************************************"
    echo " Welcome to Nucleus Development Shell for RENESAS SH                      "
    echo "**************************************************************************"
return
}

check_toolset () {

    # Check for toolset.

    optlnk > /dev/null 2>&1

    if [ $? -eq 0 ]
    then
	# print welcome message
    	print_welcome

	setup_env
    else
	echo "**************************************************************************"
	echo "* WARNING: The Renesas SH Tools do not seem to be in your path."
	echo "*          These tools must be installed in order to build."
	echo "**************************************************************************"

    fi

    return

}


try_other_place () {

    SH_TOOLS_PATH="/usr/bin/renesas/sh/9_4_1"

    if [ ! -d "%SH_TOOLS_PATH%" ]; then
	check_toolset
    else
	PATH="$SH_TOOLS_PATH/bin:$PATH"
	SHC_INC="$SH_TOOLS_PATH/include"
	SHC_LIB="$SH_TOOLS_PATH/bin"
	SHC_TMP="$TEMP"
    fi

    return
}

# If it exists, put the toolset in the path and define SHC_INC, SHC_LIB, and SHC_TMP.
# TODO Get default path for renesas tools on Linux
SH_TOOLS_PATH="/usr/binrenesas/sh/9_4_1"

if [ ! -d "SH_TOOLS_PATH" ];then
    try_other_place
else
    PATH="$SH_TOOLS_PATH/bin:$PATH"
    SHC_INC="$SH_TOOLS_PATH/include"
    SHC_LIB="$SH_TOOLS_PATH/bin"
    SHC_TMP=$TEMP

    check_toolset

fi

# Run shell to accept normal commands after script execution
exec $SHELL

