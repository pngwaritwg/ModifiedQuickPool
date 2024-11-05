#!/bin/bash

# Check for required parameters
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <rider_count> <driver_count>"
    exit 1
fi

# Use the first and second parameters for rider and driver counts
rider_count=$1
driver_count=$2

# Set up any required environment or configuration

# Create a log directory
log_dir=~/application_logs
mkdir -p "$log_dir"

# Execute the application and redirect output to the log file
./build/src/Intersection_app -r "$rider_count" -d "$driver_count" > "$log_dir/output.log" 2>&1

# Check if the application executed successfully
if [ $? -eq 0 ]; then
    echo "Application executed successfully."
else
    echo "Application failed. Check the log for details."
fi
