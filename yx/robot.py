#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import platform
import subprocess
import shutil
import time

def isWindowsSystem():
    return 'Windows' in platform.system()
 
def isLinuxSystem():
    return 'Linux' in platform.system()

script_dir = os.path.split(os.path.abspath(sys.argv[0]))[0]
skynet_dir = os.path.dirname(script_dir)
build_dir = os.path.join(os.path.dirname(skynet_dir), "build")
if isLinuxSystem():
	build_dir = os.path.join(os.path.dirname(skynet_dir), "build_linux")

def prepare_env():
	if isWindowsSystem():
		#pthreadVCE2 = os.path.join(skynet_dir, "3rd", "pthreadVCE2.dll")

		#exe_dir = os.path.join(build_dir, "Debug")
		#exe_file = os.path.join(exe_dir, "pthreadVCE2.dll")
		#if not os.path.exists(exe_file) :
		#	if not os.path.exists(exe_dir):
		#		os.makedirs(exe_dir)	
		#	shutil.copyfile(pthreadVCE2, exe_file)
		##
		#exe_dir = os.path.join(build_dir, "Release")
		#exe_file = os.path.join(exe_dir, "pthreadVCE2.dll")
		#if not os.path.exists(exe_file) :
		#	if not os.path.exists(exe_dir):
		#		os.makedirs(exe_dir)	
		#	shutil.copyfile(pthreadVCE2, exe_file)
		pass
	else:
		files = ["memory.so", "skynet.so", "protobuf.so"]
		for f in files:
			src_file = os.path.join(build_dir, "Debug", "lib.target", f)
			if os.path.exists(src_file) :
				dst_file = os.path.join(build_dir, "Debug", f)
				shutil.copyfile(src_file, dst_file)
			src_file = os.path.join(build_dir, "Release", "lib.target", f)
			if os.path.exists(src_file) :
				dst_file = os.path.join(build_dir, "Release", f)
				shutil.copyfile(src_file, dst_file)
		pass

	## 设置skynet工作目录。
	print("skynet dir:{dir}".format(dir=skynet_dir))
	os.chdir(skynet_dir)

def get_skynet_path(opts):
	skynet = ""
	if isWindowsSystem():
		if opts.release :
			skynet = os.path.join(build_dir, "Release", "skynet2.exe")
		else:
			skynet = os.path.join(build_dir, "Debug", "skynet2.exe")
	else:
		if opts.release :
			skynet = os.path.join(build_dir, "Release", "skynet")
		else:
			skynet = os.path.join(build_dir, "Debug", "skynet")

	return skynet

def run_skynet(skynet, configs):
	ps = []
	if isWindowsSystem():
		for c in configs:
			p = subprocess.Popen(skynet + " " + c, creationflags = subprocess.CREATE_NEW_CONSOLE)
			time.sleep(3)
	else:
		for c in configs:
			p = subprocess.Popen([skynet, c])
			time.sleep(3)

def start_robot(opts):
	skynet = get_skynet_path(opts);
	print("skynet path:{skynet}".format(skynet=skynet))
	configs =["yx/conf/robot/robot.conf"]
	for i in range(1, 10 + 1):
		configs.append("yx/conf/robot/robot{n}.conf".format(n=i))
	run_skynet(skynet, configs)

def _parse_arguments():
	"""Parse the sys.argv command-line arguments, returning the options."""
	import optparse
	parser = optparse.OptionParser()
	parser.add_option('--r', action='store_true', dest='release', default=False)
	parser.add_option('--test', action='store_true', dest='test', default=False)
	(opts, args) = parser.parse_args()
	return opts

def main(opts):
	prepare_env();
	start_robot(opts);

def test_main(opts):
	prepare_env()
	skynet = get_skynet_path(opts);
	print("skynet path:{skynet}".format(skynet=skynet))
	conf = " fb/conf/test_mgr.conf"
	os.system(skynet + conf)

if __name__ == '__main__':
	opts = _parse_arguments()
	##
	if opts.test :
		test_main(opts)
	else:
		main(opts)
	##
	if isWindowsSystem():
		pass #os.system('pause') 
	else:
		os.system('echo "Press any key to continue..." && read')