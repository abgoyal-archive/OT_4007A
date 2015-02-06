#!/bin/bash
# ##########################################################
# ALPS(Android2.3 based) build environment profile setting
# ##########################################################
# Overwrite JAVA_HOME environment variable setting if already exists
JAVA_HOME=/mtkoss/jdk/jdk1.6.0_23
export JAVA_HOME

# Overwrite ANDROID_JAVA_HOME environment variable setting if already exists
ANDROID_JAVA_HOME=/mtkoss/jdk/jdk1.6.0_23
export ANDROID_JAVA_HOME

# Overwrite PATH environment setting for JDK & arm-eabi if already exists
#modify by shuzhong.cui for compile need to export arm-eabi-4.4.3 on 2012.8.28
#PATH=/mtkoss/jdk/jdk1.6.0_23/bin:/mtkoss/arm-eabi/4.4.3/bin:./prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin:$PATH
PATH=/mtkoss/jdk/jdk1.6.0_23/bin:/mtkoss/arm-eabi/4.4.3/bin:$PWD/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin:$PATH
export PATH

# Add MediaTek developed Python libraries path into PYTHONPATH
if [ -z "$PYTHONPATH" ]; then
  PYTHONPATH=$PWD/mediatek/build/tools
else
  PYTHONPATH=$PWD/mediatek/build/tools:$PYTHONPATH
fi
export PYTHONPATH

