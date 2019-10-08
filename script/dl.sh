#!/bin/bash

scp pmr25@thoth.cs.pitt.edu:$REMOTE/\{$FILES\} .
/bin/cp -rf bzImage /boot/bzImage-devel
/bin/cp -rf System.map /boot/System.map-devel
lilo
