#!/bin/bash
#-f "scons" 
#-f "cmake" 
#-D"buildtype=Official"

# root target: -R "skynet_bin" 

current_dir=${PWD}
#echo $current_dir

python ../build/gyp_chromium -f"scons" -D"clang=0"  -D"linux_use_gold_flags=0" -D"clang_use_chrome_plugins=0" -D"linux_use_gold_binary=0"   -D"use_glib=0" -D"use_dbus=0" -D"use_aura=1" -D"linux_use_tcmalloc=0" -D"use_gconf=0" -D"use_gio=0" -D"toolkit_views=1" -D"use_x11=0" --generator-output "$current_dir" skynet.gyp
