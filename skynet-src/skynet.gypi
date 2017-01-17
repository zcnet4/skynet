#  skynet.gyp模板配置
# skynet.gypi
{
  'targets': [
    {
      'target_name': 'skynet',
      'type': 'shared_library',
      'dependencies': [
		'3rd/3rd.gyp:lua',
		'lualib-src/lualib.gyp:memory',
		'lualib-src/lualib.gyp:*',
      ],
	  'export_dependent_settings': [  
        #'lualib-src/lualib.gyp:memory',
      ],
      'defines': [
      ],
      'include_dirs': [
		'.'
      ],
      'sources': [
		"atomic.h",
		#"malloc_hook.h",
		#"malloc_hook.c",
		"rwlock.h",
		"skynet.h",
		"skynet_daemon.h",
		"skynet_daemon.c",
		"skynet_env.h",
		"skynet_env.c",
		"skynet_handle.h",
		"skynet_handle.c",
		"skynet_harbor.h",
		"skynet_harbor.c",
		"skynet_imp.h",
		"skynet_log.h",
		"skynet_log.c",
		"skynet_malloc.h",
		"skynet_module.h",
		"skynet_module.c",
		"skynet_monitor.h",
		"skynet_monitor.c",
		"skynet_mq.h",
		"skynet_mq.c",
		"skynet_server.h",
		"skynet_server.c",
		"skynet_socket.h",
		"skynet_socket.c",
		"skynet_timer.h",
		"skynet_timer.c",
		"socket_epoll.h",
		"socket_kqueue.h",
		"socket_poll.h",
		"socket_server.h",
		"socket_server.c",
		"spinlock.h",
		"skynet_error.c",
		#"skynet_main.c",
		"skynet_start.c",
		"../lualib-src/lua-seri.c",
		"../lualib-src/lua-seri.h",
		"../lualib-src/skynet_proto.c",
		"../lualib-src/skynet_proto.h",
		"../lualib-src/lua-skynet.c",		
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"skynet.def",
			"socket_cpoll.h",
		  ],
		  'msvs_disabled_warnings': [
			'4204','4013','4996','4152','4047','4024','4133'
		  ],
		  'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalDependencies': [
					'pthreadVCE2.lib',
					'Ws2_32.lib',
				],
				'AdditionalLibraryDirectories':[
					'3rd',
				],
              },
            },
			'dependencies': [	
				'3rd/3rd.gyp:posix_win',
			],
			'msvs_postbuild': r'copy "$(ProjectDir)\\3rd\\pthreadVCE2.dll" "<(DEPTH)\\build\\$(ConfigurationName)\\pthreadVCE2.dll"',
        }, { # OS != "win",
		  'scons_variable_settings': {'SHLIBPREFIX':'lib',},
        }]
      ],
    },
	{
      'target_name': 'snlua',
      'type': 'loadable_module',
      'dependencies': [
		'3rd/3rd.gyp:lua',
		'skynet',
		'lualib-src/lualib.gyp:memory',
      ],
      'include_dirs': [
		'.'
      ],
      'sources': [
		"../service-src/service_snlua.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"../service-src/snlua.def",
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'logger',
      'type': 'loadable_module',
      'dependencies': [
		'3rd/3rd.gyp:lua',
		'skynet',
		'lualib-src/lualib.gyp:memory',
      ],
      'include_dirs': [
		'.'
      ],
      'sources': [
		"../service-src/service_logger.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"../service-src/logger.def",
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'harbor',
      'type': 'loadable_module',
      'dependencies': [
		'3rd/3rd.gyp:lua',
		'skynet',
		'lualib-src/lualib.gyp:memory',
      ],
      'include_dirs': [
		'.'
      ],
      'sources': [
		"../service-src/service_harbor.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"../service-src/harbor.def",
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'harbor2',
      'type': 'loadable_module',
      'dependencies': [
		'3rd/3rd.gyp:lua',
		'skynet',
		'lualib-src/lualib.gyp:memory',
      ],
      'include_dirs': [
		'.'
      ],
      'sources': [
		"../service-src/service_harbor2.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"../service-src/harbor2.def",
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'gate',
      'type': 'loadable_module',
      'dependencies': [
		'3rd/3rd.gyp:lua',
		'skynet',
		'lualib-src/lualib.gyp:memory',
      ],
      'include_dirs': [
		'.'
      ],
      'sources': [
		"../service-src/service_gate.c",
		"../service-src/hashid.h",
		"../service-src/databuffer.h",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"../service-src/gate.def",
		  ],
		  'dependencies': [	
				'3rd/3rd.gyp:posix_win',
		   ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'socketdriver',
      'type': 'loadable_module',
      'dependencies': [
		'3rd/3rd.gyp:lua',
		'skynet',
		'lualib-src/lualib.gyp:memory',
      ],
      'include_dirs': [
		'.'
      ],
      'sources': [
		"../lualib-src/lua-socket.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"../lualib-src/socketdriver.def",
		  ],
		  'msvs_disabled_warnings': [
			'4090'
		  ],
		  'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalDependencies': [
					'Ws2_32.lib',
				],
              },
            },
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
  ],
}
