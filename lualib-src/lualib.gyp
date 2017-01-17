{
  'variables': {
    'c99':1,
  },
  'target_defaults': {
    'conditions': [
      ['OS in "freebsd dragonflybsd linux openbsd solaris android"', {
        'cflags!': ['-fvisibility=hidden'],
		'cflags_cc!': ['-fvisibility-inlines-hidden'],
	  #	'scons_variable_settings': {'SHLIBPREFIX':'',},
      }],
    ],
	'include_dirs': [
    ],
	'defines': [
		#'DEFINE_ZC_VALUE=ZC',
		#'NOMINMAX',
	],
	#È¥µôlibÇ°×º¡£
	'product_prefix': '',
  },
  'targets': [
	{
      'target_name': 'memory',
      'type': 'shared_library',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
		'../3rd/jemalloc/jemalloc.gyp:jemalloc',
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"../skynet-src/skynet_malloc.h",
		"../skynet-src/malloc_hook.h",
		"../skynet-src/malloc_hook.c",
		"lua-memory.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [ 'NOUSE_JEMALLOC',],
		  'sources': [
			"memory.def",
		  ],
        }, { # OS != "win",
		  #'defines': [ 'NOUSE_JEMALLOC',],
          'include_dirs': [ "../3rd/jemalloc/include/jemalloc",],
		  'library_dirs': ['../skynet/3rd/jemalloc/lib',],
		  #libjemalloc_pic.a¾²Ì¬¿â
		  'libraries': [ '-ljemalloc_pic',],
        }]
      ],
    },
	{
      'target_name': 'bson',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
       'sources': [
		"lua-bson.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"bson.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  # Add the default import libs.
        'msvs_settings':{
          'VCLinkerTool': {
            #'MinimumRequiredVersion' : '5.01',
            'AdditionalDependencies': [
              #'User32.lib',
			  'Ws2_32.lib',
            ],
          },
        },
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
	  'target_name': 'crypt',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-crypt.c",
		"lsha1.c",
		"sha256.c",
		"sha256.h",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"crypt.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'debugchannel',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
      ],
      'defines': [
      ],
	  
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-debugchannel.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"debugchannel.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'mongo',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
		'memory',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-mongo.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"mongo.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'mysqlaux',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-mysqlaux.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"mysqlaux.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'netpack',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
		'memory',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-netpack.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"netpack.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'profile',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-profile.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"profile.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'sharedata',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-sharedata.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"sharedata.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'sproto',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"sproto/lsproto.c",
		"sproto/sproto.c",
		"sproto/sproto.h",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"sproto.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'stm',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
		'memory',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-stm.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"stm.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'multicast',
      'type': 'loadable_module',
      'dependencies': [
		'../3rd/3rd.gyp:lua',
		'memory',
      ],
      'defines': [
      ],
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua-multicast.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
          'defines': [
            
          ],
		  'sources': [
			"multicast.def",
		  ],
		  'msvs_disabled_warnings': [
			'4204'
		  ],
		  'dependencies': [	
			'../3rd/3rd.gyp:posix_win',
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
  ],
  
}
