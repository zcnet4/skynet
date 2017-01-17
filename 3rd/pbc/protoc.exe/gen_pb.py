#!/usr/bin/env python
import sys
import os

def main(argv):
  pb_name = argv[1]
  os.system("protoc.exe -o {pb}.pb {pb}".format(pb=pb_name))

if __name__ == '__main__':
  sys.exit(main(sys.argv))
