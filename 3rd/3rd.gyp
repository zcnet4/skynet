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
		#'..',
		#'../common',
    ],
	'defines': [
		#'DEFINE_ZC_VALUE=ZC',
		#'NOMINMAX',
	],
	#去掉lib前缀。
	'product_prefix': '',
  },
  'includes': [
	'posix_win/posix.gypi',
	#'tar/tar.gypi',
	'pbc/pbc.gypi',
  ],
  'targets': [
    {
      'target_name': 'lua',
      'type': 'shared_library',
	  'product_name':'lua53',
      'include_dirs': [
		'../skynet-src',
      ],
      'sources': [
		"lua/lapi.c",
		"lua/lapi.h",
		"lua/lauxlib.c",
		"lua/lauxlib.h",
		"lua/lbaselib.c",
		"lua/lbitlib.c",
		"lua/lcode.c",
		"lua/lcode.h",
		"lua/lcorolib.c",
		"lua/lctype.c",
		"lua/lctype.h",
		"lua/ldblib.c",
		"lua/ldebug.c",
		"lua/ldebug.h",
		"lua/ldo.c",
		"lua/ldo.h",
		"lua/ldump.c",
		"lua/lfunc.c",
		"lua/lfunc.h",
		"lua/lgc.c",
		"lua/lgc.h",
		"lua/linit.c",
		"lua/liolib.c",
		"lua/llex.c",
		"lua/llex.h",
		"lua/llimits.h",
		"lua/lmathlib.c",
		"lua/lmem.c",
		"lua/lmem.h",
		"lua/loadlib.c",
		"lua/lobject.c",
		"lua/lobject.h",
		"lua/lopcodes.c",
		"lua/lopcodes.h",
		"lua/loslib.c",
		"lua/lparser.c",
		"lua/lparser.h",
		"lua/lprefix.h",
		"lua/lstate.c",
		"lua/lstate.h",
		"lua/lstring.c",
		"lua/lstring.h",
		"lua/lstrlib.c",
		"lua/ltable.c",
		"lua/ltable.h",
		"lua/ltablib.c",
		"lua/ltm.c",
		"lua/ltm.h",
		#"lua/lua.c",
		"lua/lua.h",
		"lua/lua.hpp",
		#"lua/luac.c",
		"lua/luaconf.h",
		"lua/lualib.h",
		"lua/lundump.c",
		"lua/lundump.h",
		"lua/lutf8lib.c",
		"lua/lvm.c",
		"lua/lvm.h",
		"lua/lzio.c",
		"lua/lzio.h",
      ],
      'direct_dependent_settings': {
        'include_dirs': [
		  'lua',
        ],
      },
	  'conditions': [
        ['OS=="win"', {
			'defines': [
				'LUA_BUILD_AS_DLL',
			],
			'include_dirs': [
				'posix_win',
			],
			'dependencies': [
				'posix_win',
			],
			'direct_dependent_settings': {
				'include_dirs': [
					'posix_win',
				],
			},
		}, { # OS != "win",
		  'defines': [
			'LUA_USE_LINUX'
		  ],
		  'libraries': [
			'-ldl',
			'-lrt',
			'-lm',
		  ],
		  'scons_variable_settings': {'SHLIBPREFIX':'lib',},
        }]
      ],
    },
	{
      'target_name': 'md5',
      'type': 'loadable_module',
      'dependencies': [
		'lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'lua',
      ],
      'sources': [
		"lua-md5/compat-5.2.c",
		"lua-md5/md5.c",
		"lua-md5/md5lib.c",
		"lua-md5/compat-5.2.h",
		"lua-md5/md5.h",
	  ],
	  'conditions': [
        ['OS=="win"', {
		  'sources': [
			"lua-md5/md5.def",
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'lpeg',
      'type': 'loadable_module',
      'dependencies': [
		'lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'lua',
      ],
      'sources': [
		"lpeg/lpeg.def",
		"lpeg/lpvm.c",
		"lpeg/lptree.c",
		"lpeg/lpprint.c",
		"lpeg/lpcode.c",
		"lpeg/lpcap.c",
	  ],
    },
	{
      'target_name': 'cjson',
      'type': 'loadable_module',
      'dependencies': [
		'lua',
      ],
      'defines': [
      ],
      'include_dirs': [
		'lua',
      ],
      'sources': [
		"lua-cjson/fpconv.c",
		"lua-cjson/lua_cjson.c",
		"lua-cjson/strbuf.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
		  'sources': [
			"lua-cjson/cjson.def",
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
  ],
  
}
