#! /bin/bash

lsmod | grep encoder > /dev/null

if [ $? -eq 0 ]; then
    echo "encoder module loaded already."
    echo "Unloading..."
    sudo rmmod encoder
fi

echo "sudo insmod encoder.ko encoder0_gpio=0x00030402 encoder1_gpio=0x00070805 encoder2_gpio=0x000A0B09 encoder3_gpio=0x000F140E"
sudo insmod encoder.ko encoder0_gpio=0x00030402 encoder1_gpio=0x00070805 encoder2_gpio=0x000A0B09 encoder3_gpio=0x000F140E