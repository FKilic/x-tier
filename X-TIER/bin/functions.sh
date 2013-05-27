#!/bin/bash

declare -r TRUE=1
declare -r FALSE=0

OUTPUT_DATA=""
DEBUG=0

###################################################
#       -> Functions
###################################################
function setColor
{
    case $1 in
        "red") echo -en "\e[31m" ;;
        "green") echo -en "\e[32m" ;;
        *) echo -en "\e[0m" ;;
    esac
}

function setFontStyle
{
    case $1 in
        "bold") echo -en "\e[1m" ;;
        "underlined") echo -en "\e[4m" ;;
        *) echo -en "\e[0m" ;;
    esac
}

function checkError
{
        if  [[ $? -ne 0 ]]
        then
                # Finish step
                setFontStyle "bold"
                echo -en "["
                setColor "red"
                echo -en " FAIL "
                setColor "default"
                setFontStyle "bold"
                echo -e "]"
                setFontStyle "normal"

                # Print Error
                if $1
                then
                    setFontStyle "bold"
                    echo -en "\t[ "
                    setColor "red"
                    echo -en "ERROR"
                    setColor "default"
                    setFontStyle "bold"
                    echo -en " ]"
                    setFontStyle "normal"
                    echo -en " A "
                    setFontStyle "underlined"
                    echo -en "fatal"
                    setFontStyle "normal"
                    echo " error occurred. Aborting."
                    echo ""

                    setFontStyle "bold"
                    echo -ne "\t["
                    setColor "red"
                    echo -ne " Information "
                    setColor "default"
                    setFontStyle "bold"
                    echo -e "]"
                    setFontStyle "normal"
                    echo $OUTPUT_DATA
                    exit -1
                else
                    echo -en "\t        Error "
                    setFontStyle "underlined"
                    echo -en "not"
                    setFontStyle "normal"
                    echo -e " fatal. Continuing."

                    if [[ $DEBUG -eq 1 ]]
                    then
                        setFontStyle "bold"
                        echo -ne "\t["
                        setColor "red"
                        echo -ne " Information "
                        setColor "default"
                        setFontStyle "bold"
                        echo -e "]"
                        setFontStyle "normal"
                        echo $OUTPUT_DATA
                    fi
                fi

                return $TRUE
        fi

        return $FALSE
}

# $1 is step
# $2 number of steps
# $3 message
function printSubStep
{
    setFontStyle "bold"
    echo -en "\t["
    setColor "red"
    printf "%02d" $1 
    setColor "default"
    setFontStyle "bold"
    printf "/%02d" $2
    printf "] %-60s" "$3..."
    setFontStyle "normal"
}

# $1 defines whether an error would be critical
# $2 contains the output of the command
function finishSubStep
{
    # Check for errors
    checkError $1 $2

    if [[ $? -eq $TRUE ]]
    then
        return $TRUE
    fi

    setFontStyle "bold"
    echo -en "["
    setColor "green"
    echo -en " PASS "
    setColor "default"
    setFontStyle "bold"
    echo -e "]"
    setFontStyle "normal"

    if [[ $DEBUG -eq 1 ]]
    then
        setFontStyle "bold"
        echo -ne "\t["
        echo -ne " Information "
        echo -e "]"
        setFontStyle "normal"
        echo $OUTPUT_DATA
    fi
}

# $1 message
function printStep
{
    setFontStyle "bold"
    echo $1...
    setFontStyle "normal"
}

function finishStep
{
    setFontStyle "bold"
    setColor "green"
    echo -e "DONE!\n"
    setFontStyle "normal"
}

function executeCommand
{
    OUTPUT_DATA=$(eval "$1" 2>&1)
}