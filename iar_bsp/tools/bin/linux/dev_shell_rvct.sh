#!/bin/sh

#PS1=`\e]0;Nucleus Development Shell (RVCT)\a`


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
    export TOOLSET="rvct"

    # Display help for the make system.

    echo .
    echo "NOTE: Please type 'make help' to see the options for building."
    echo .

    return
}

print_welcome () {
    echo "**************************************************************************"
    echo " Welcome to Nucleus Development Shell for RVCT                            "
    echo "**************************************************************************"
return
}

# Check for toolset.

armcc > /dev/null 2>&1

if [ $? -eq 0 ]
then
    # print welcome message
    print_welcome

    setup_env
else

    echo "**************************************************************************"
    echo "* WARNING: The Real View Tools for ARM do not seem to be in your path."
    echo "*          These tools must be installed in order to build."
    echo "**************************************************************************"

fi

# Run shell to accept normal commands after script execution
exec $SHELL
