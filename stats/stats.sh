#!/bin/bash
# Name: Siara Leininger
# Class: CS 344
# Assignment: Program 1 - Stats
# Description: Calculates mean and median of rows and columns from numbered input.

# Declare temp file variables
tempFile=tempInputFile$$

# Trap statement to catch interrupt, hangup, and terminate signals to remove temporary files if program terminates unexpectedly
trap "rm -f tempInputFile$$*; exit 1" INT HUP TERM

# Input validation: start with type of input
# Borrowed code from assignment description
if [ "$#" == "1" ]
then
	# Create temp file so that functions can read in values from file
	while read line
	do
		echo -e "$line" >> $tempFile
	done < /dev/stdin
	inputFile=$tempFile
# If there is a data file entered in the parameters, assign it to variable
elif [ "$#" = "2" ]
then
	inputFile="$2"
	# Check if able to read file
	if ! [[ -r "$inputFile" ]]
	then
		# Print error message and exit
		echo "Cannot read $inputFile" 1>&2
		exit 1
	fi
# If you get here, the wrong format was used when entering parameters
else
	echo "Error: Incorrect parameters." 1>&2
	echo "USAGE: ./stats {-rows | -cols} [fileName]" 1>&2
	exit 1
fi

# Read in data and store in temp file
# Will need to determine if necessary to store rows or columns
# I probably should have separated this into different functions but am too lazy at this point
# It validates -r* and -c* and performs all calculations and prints results.
# Reference: www.stackoverflow.com/questions/15108229/how-to-count-number-of-words-from-string-using-shell 
# Reference: https://ss64.com/bash/sort.html
# Reference: www.thegeekstuff.com/2012/12/linux-tr-command/

if [[ $1 == -r* ]];
then
	# Label for average and median
	echo -e "Average   Median"

	#Read info from file
	while read line
	do
		# Declare vaiables
		sum=0
		count=0
		average=0
	
		#Find number of elements in each line
		#numVal=$(echo $line | wc -w)
	
		#Find sum of each line and keep tally how how many values in each line
		for i in $line
		do
			sum=$(($sum + $i))
			# Counter to keep track of how many values in line
			(( count++ ))
	
		done
		
		# Calculate average using equation from assignment instructions
		average=$((($sum + ($count / 2)) / $count))

		# Sort the line to find correct median value
		sortLine=$(echo $line | tr " " "\n" | sort -g | tr "\n" " ")
		# Find middle value
		middle=$((($count / 2) + 1))
		# Cut value at middle position to get median value
		median=$(echo $sortLine | cut -d " " -f $middle)

		# Print results
		echo -e "$average\t\t$median" 
	done < "$inputFile"


# Now do the same thing, but if the user selected columns
# Must print the results in rows instead of columns
elif [[ $1 == -c* ]]
then

	# Find number of columns by calculating number of elements in row
	numCol=$(head -n 1 $inputFile | wc -w)

	# Find median and average of each column
	# While loop to read in data
	cur=1
	while [ $cur -le $numCol ]
	do
		# Initialize variables
		sum=0
		count=0
		average=0

		# Get column data from file and sort to help find median
		lineCol=$( cut -f $cur $inputFile | tr " " "\n" | sort -g)

		# Sum the column for finding the average
		for i in $lineCol
		do
			sum=$(($sum + $i))
			# Counter to keep track of how many values in line
			(( count++ ))
		done

		# Calculate column average
		average=$((($sum + ($count / 2)) / $count))
		
		# Find the middle value
		middle=$((($count / 2) + 1))
		# Cut value at middle position to get median value
		median=$(echo $lineCol | cut -d " " -f $middle)
		
		# Output answers in row form
		# Create string of results to print them on one row
		averageString+="$average\t"
		medianString+="$median\t"
		# Increment column counter
		(( cur ++ ))

	done < "$inputFile"
	
	# Output answers in row form
	echo -e "Averages: "
	echo -e "$averageString"
	echo -e "Medians: "
	echo -e "$medianString"
else
	# If you get here, user must not have entered -r* or -c*, send error
	>&2 echo "Invalid arguments entered. USAGE: ./stats {-rows | -cols} [fileName]"
	exit 1
fi
# Delete temp files
rm -f $tempFile
rm -f tempInputFile$$

exit 0 
