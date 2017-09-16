#!/bin/bash

# Bash script with argument parsing skeleton
# With some help from: http://www.bahmanm.com/blogs/command-line-options-how-to-parse-in-bash-using-getopt

function usage(){
        echo "Usage: $0 [-n|--name <value>] [-p|--path <value>]"
}

function show_result(){
	echo "The following role dir strucure's been created:"
	exec tree $(pwd) 
	echo "End!"
}

####################################################################################

# “a” and “arga” have required arguments.
# “b” and “argb” have required arguments.

# read the options - in this case 'a|--arga' and '-b|--argb' are required
TEMP=`getopt -o n:p: --long name:,path: -n "$0.sh" -- "$@"`
eval set -- "$TEMP"

# check if args entered
if [ -z "$2" ]; then usage; exit 0; fi

# extract options and their arguments into variables.
while true ; do
    case "$1" in
        -n|--name)
            case "$2" in
                "") shift 2 ;;
                *) ARG_A=$2 ; shift 2 ;;
            esac ;;
        -p|--path)
            case "$2" in
                "") shift 2 ;;
                *) ARG_B=$2 ; shift 2 ;;
            esac ;;
        # and so on for agrc and successors...
	--) shift ; break ;;
        *) echo "Internal error!" ; exit 1 ;;
    esac
done

#####################################################################################

# Print parsed result
#echo "ARG_A = $ARG_A"
#echo "ARG_B = $ARG_B"


# Now do something with the parsed arguments
ROLE_NAME=$ARG_A
ROLE_PATH=$ARG_B

if [ $ROLE_PATH != "." ]; then
	mkdir $ROLE_PATH/$ROLE_NAME
	cd $ROLE_PATH/$ROLE_NAME
	mkdir files handlers meta templates tasks var
	show_result
else
	mkdir $ROLE_NAME
	cd $ROLE_NAME
	mkdir files handlers meta templates tasks vars
	show_result
fi
