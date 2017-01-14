#!/bin/bash

TheSkyX_Install=~/Library/Application\ Support/Software\ Bisque/TheSkyX\ Professional\ Edition/TheSkyXInstallPath.txt
echo "TheSkyX_Install = $TheSkyX_Install"

if [ ! -f "$TheSkyX_Install" ]; then
    echo TheSkyXInstallPath.txt not found
    exit 1
fi


TheSkyX_Path=$(<"$TheSkyX_Install")
echo "Installing to $TheSkyX_Path"

if [ ! -d "$TheSkyX_Path" ]; then
    echo TheSkyX Install dir not exist
    exit 1
fi

cp "./filterwheellist xagyl.txt" "$TheSkyX_Path/Resources/Common/Miscellaneous Files/"
cp "./XagylFilterWheel.ui" "$TheSkyX_Path/Resources/Common/PlugIns/FilterWheelPlugIns/"
cp "./Xagyl.png" "$TheSkyX_Path/Resources/Common/PlugIns/FilterWheelPlugIns/"
cp "./libXagylFilterWheel.so" "$TheSkyX_Path/Resources/Common/PlugIns/FilterWheelPlugIns/"

app_owner=`/usr/bin/stat -n -f "%u" "$TheSkyX_Path" | xargs id -n -u`
if [ ! -z "$app_owner" ]; then
	chown $app_owner "$TheSkyX_Path/Resources/Common/Miscellaneous Files/filterwheellist xagyl.txt"
	chown $app_owner "$TheSkyX_Path/Resources/Common/PlugIns/FilterWheelPlugIns/XagylFilterWheel.ui"
	chown $app_owner "$TheSkyX_Path/Resources/Common/PlugIns/FilterWheelPlugIns/Xagyl.png"
	chown $app_owner "$TheSkyX_Path/Resources/Common/PlugIns/FilterWheelPlugIns/libXagylFilterWheel.so"
fi
chmod  755 "$TheSkyX_Path/Resources/Common/PlugIns/FilterWheelPlugIns/libXagylFilterWheel.so"
