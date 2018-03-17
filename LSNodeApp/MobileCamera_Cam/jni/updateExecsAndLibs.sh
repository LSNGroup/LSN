#!/bin/bash

cd $(dirname $0)

echo libs/armeabi/liblocSDKx.so
cp -f  ../liblocSDK7a.so   ../libs/armeabi/liblocSDK7a.so

echo libs/armeabi-v7a/liblocSDKx.so
cp -f  ../liblocSDK7a.so   ../libs/armeabi-v7a/liblocSDK7a.so

echo Copy libs to Viewer Project...
cp -f  ../libs/armeabi/libshdir.so  ../../MobileCamera_Viewer/libs/armeabi/libshdir.so
cp -f  ../libs/armeabi/libavrtp.so  ../../MobileCamera_Viewer/libs/armeabi/libavrtp.so
cp -f  ../libs/armeabi/libup2p.so  ../../MobileCamera_Viewer/libs/armeabi/libup2p.so
cp -f  ../libs/armeabi-v7a/libshdir.so  ../../MobileCamera_Viewer/libs/armeabi-v7a/libshdir.so
cp -f  ../libs/armeabi-v7a/libavrtp.so  ../../MobileCamera_Viewer/libs/armeabi-v7a/libavrtp.so
cp -f  ../libs/armeabi-v7a/libup2p.so  ../../MobileCamera_Viewer/libs/armeabi-v7a/libup2p.so


echo Done.
