# 3rd.gypi
{
  'targets': [
  {
	  # 目标工程名
      'target_name': 'posix_win',      
	  'conditions': [
        ['OS=="win"', {
		'type': 'shared_library',
		'include_dirs': [
			'.',
		],
          #目标工程源代码路径
		'sources': [
			"arpa/inet.h",
			"cpoll/cpoll.h",
			"cpoll/cpoll.cpp",
			"netinet/in.h",
			"netinet/tcp.h",
			"sys/file.h",
			"sys/socket.h",
			"atomic_lock.c",
			"atomic_lock.h",
			"dlfcn.c",
			"dlfcn.h",
			"pthread.h",
			"sched.h",
			"semaphore.h",
			"unistd.c",
			"unistd.h",
			"dirent.c",
			"dirent.h",
			"posix.def",
		],
		'direct_dependent_settings': {
			'include_dirs': [
				'posix_win',
			],
		},
		# Add the default import libs.
        'msvs_settings':{
          'VCLinkerTool': {
            'MinimumRequiredVersion' : '5.01',
            'AdditionalDependencies': [
              'User32.lib',
			  'Ws2_32.lib',
            ],
          },
        },
        }, { # OS != "win",
          'defines': [
            
          ],
		  'type': 'none',
        }]
      ],
    },
  ],
}
