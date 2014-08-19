#!/bin/sh

#PS1=`\e]0;Nucleus Development Shell (CS GNU MIPS)\a`

CS_MIPS_TOOLSET_NAME="codesourcery_mips_4.5.2"

# See if the script is called through link
SCRIPT_PATH=$(readlink -f "$0")

if [ -z "$SCRIPT_PATH" ] ; then
  # It is called directly
    SCRIPT_PATH=$0
fi

SCRIPT_PATH=$(dirname $SCRIPT_PATH)

try_other_place () {

    if [ ! -d "$SCRIPT_PATH/../../../tools/$CS_MIPS_TOOLSET_NAME" ];then
	check_toolset
    else
	PATH="$SCRIPT_PATH/../../../tools/$CS_MIPS_TOOLSET_NAME/bin:$PATH"
    fi
    return
}

print_welcome () {
    echo "**************************************************************************"
    echo " Welcome to Nucleus Development Shell for CS GNU MIPS                     "
    echo "**************************************************************************"
return
}

check_toolset () {

# Check for toolset.

    mips-sde-elf-gcc -v 2>/dev/null
    if [ $? -eq 0 ]
    then
	# print welcome message
	print_welcome

	setup_env
    else
	echo "**************************************************************************"
	echo "* WARNING: The Code Sourcery Tools for MIPS do not seem to be in your path."
	echo "*          These tools must be installed in order to build."
	echo "**************************************************************************"
    fi
    return
}

setup_env () {

    # Setup path for Nucleus utilities.

    PATH="$SCRIPT_PATH/:$PATH"
    PATH="$SCRIPT_PATH/x86:$PATH"

    # Put the JRE in the path.
    PATH="$SCRIPT_PATH/../../../../Uninstall/jre/bin:$SCRIPT_PATH/../../../../codebench/jre/bin:$PATH"

    #Set toolset env var used by the make system.
    export TOOLSET="csgnu_mips"

    # Display help for the make system.
    echo "NOTE: Please type 'make help' to see the options for building."

    return
}

# Move to the Nucleus directory.
cd $SCRIPT_PATH/../../../

# If it exist, put the toolset in the path.
if [ ! -d "$SCRIPT_PATH/../$CS_MIPS_TOOLSET_NAME" ]; then
    try_other_place
else
    PATH="$SCRIPT_PATH/../$CS_MIPS_TOOLSET_NAME/bin:$PATH"
    check_toolset
fi

# Run shell to accept normal commands after script execution
exec $SHELL
