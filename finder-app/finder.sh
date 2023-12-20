#!/bin/sh
# finder.sh filesdir searchstr

if [ $# -eq 2 ]
then
    if [ -d ${1} ]
    then
	numfiles=0
	nummatches=0
        for i in ${1}/* 
	do
	    if [ -d $i ]
	    then
		for ii in $i/*
		do
                    numfiles=$((numfiles+1))
		    sfound=$(grep ${2} $ii | wc -l)
                    if [ $sfound -gt 0 ]
		    then
		        nummatches=$((nummatches+1))
		    fi

		done
	    else
                numfiles=$((numfiles+1))
		sfound=$(grep ${2} $i | wc -l)
		if [ $sfound -gt 0 ]
		then
		    nummatches=$((nummatches+1))
		fi
	    fi
	done

	#numfiles=$(ls -rl ${1} | grep "^-" | wc -l)
        #nummatches=$(grep -Ir ${2} ${1} | wc -l)
        #nummatches=$(find } ${1} | wc -l)
    
   	echo "The number of files are $numfiles and the number of matching lines are $nummatches"
    else 
	echo "${1} is not a valid directory on the filesystem"    
	exit 1
    fi

else
    if [ $# -lt 2 ]
    then
        echo "Useage : finder dir searchstr"
	exit 1
    else
	echo "Too many arguments"
	exit 1
    fi
fi
