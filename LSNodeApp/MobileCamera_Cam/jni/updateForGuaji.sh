#!/bin/bash

cd $(dirname $0)

for i in ../libs/*; do
  if [[ -d $i && -e $i/androidvncserver ]];then
    echo Moving $i/androidvncserver to $i/libandroidvncserver.so;
    mv $i/androidvncserver $i/libandroidvncserver.so;
  fi
done
cp -frv nativeMethods/libs/* ../libs


echo libs/armeabi/liblocSDK3.so
cp -f  ../liblocSDK3.so   ../libs/armeabi/liblocSDK3.so

echo libs/armeabi-v7a/liblocSDK3.so
cp -f  ../liblocSDK3.so   ../libs/armeabi-v7a/liblocSDK3.so

echo libandroidvncserver_guaji_pie.so
cp -f  ../libandroidvncserver_guaji_pie.so   ../libs/armeabi-v7a/libandroidvncserver_pie.so


echo No need copy to Viewer Project£¡£¡£¡

echo Done.
