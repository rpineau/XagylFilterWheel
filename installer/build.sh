#!/bin/bash

mkdir -p ROOT/tmp/XagylFilterWheels_X2/
cp "../XagylFilterWheels.ui" ROOT/tmp/XagylFilterWheels_X2/
cp "../Xagylpng" ROOT/tmp/XagylFilterWheels_X2/
cp "../filterwheellist xagyl.txt" ROOT/tmp/XagylFilterWheels_X2/
cp "../build/Release/libXagylFilterWheels.dylib" ROOT/tmp/XagylFilterWheels_X2/

if [ ! -z "$installer_signature" ]; then
# signed package using env variable installer_signature
pkgbuild --root ROOT --identifier org.rti-zone.XagylFilterWheels_X2 --sign "$installer_signature" --scripts Scritps --version 1.0 XagylFilterWheels_X2.pkg
pkgutil --check-signature ./XagylFilterWheels_X2.pkg
else
pkgbuild --root ROOT --identifier org.rti-zone.XagylFilterWheels_X2 --scripts Scritps --version 1.0 XagylFilterWheels_X2.pkg
fi

rm -rf ROOT
