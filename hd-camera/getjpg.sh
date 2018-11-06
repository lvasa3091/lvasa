#!/bin/bash
tt=$(date +"%T")
mkdir $tt
cd $tt
count=0
while [ $count -lt 600 ]
do
echo "$count"
gst-launch-1.0 v4l2src device=/dev/video13 num-buffers=1 ! jpegenc quality=100 ! filesink location="$count.jpg"
sleep 1
let "count+=1"
done
