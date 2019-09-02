#!/bin/bash

rm error*

# change 0x610661c3 to your semaphore and shared memory key
ipcrm -S 0x610602ed
ipcrm -M 0x610602ed

