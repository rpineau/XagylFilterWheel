#!/bin/bash

mkdir -p ROOT/tmp/XagylFilterWheel_X2/
cp "../XagylFilterWheel.ui" ROOT/tmp/XagylFilterWheel_X2/
cp "../Xagyl.png" ROOT/tmp/XagylFilterWheel_X2/
cp "../filterwheellist xagyl.txt" ROOT/tmp/XagylFilterWheel_X2/
cp "../build/Release/libXagylFilterWheel.dylib" ROOT/tmp/XagylFilterWheel_X2/

if [ ! -z "$installer_signature" ]; then
# signed package using env variable installer_signature
pkgbuild --root ROOT --identifier org.rti-zone.XagylFilterWheel_X2 --sign "$installer_signature" --scripts Scritps --version 1.0 XagylFilterWheel_X2.pkg
pkgutil --check-signature ./XagylFilterWheel_X2.pkg
else
pkgbuild --root ROOT --identifier org.rti-zone.XagylFilterWheel_X2 --scripts Scritps --version 1.0 XagylFilterWheel_X2.pkg
fi

rm -rf ROOT
