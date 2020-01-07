#!/bin/bash

#fallocate -l 10G test_10G.img

echo "create large file..." > file;
while [ $(stat -c "%s" file) -le 10485760000 ]; do 
    cat file >> newfile
	cat newfile >> file
done
