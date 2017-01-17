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
		# 配置全局开发目录。
		#'..',
		#'../common',
    ],
	#目标工程预处理宏定义
	'defines': [
		#'DEFINE_ZC_VALUE=ZC',
		#'NOMINMAX',
	],
	#去掉lib前缀。
	'product_prefix': '',
  },
  'includes': [
	'skynet-src/skynet.gypi',
  ],
  'targets': [
    {
	  # 目标工程名
      'target_name': 'skynet_bin',
      'type': 'executable',
	  # 目标工程依赖工程
      'dependencies': [
	    '3rd/3rd.gyp:lua',
	    '3rd/3rd.gyp:md5',
		'3rd/3rd.gyp:cjson',
		#'3rd/3rd.gyp:tar',
		'3rd/3rd.gyp:protobuf',
		'skynet',
		'snlua',
		'logger',
		'harbor',
		'harbor2',
		'socketdriver',
      ],
	  #目标工程预处理宏定义
      'defines': [
      ],
	  #目标工程C++ include目录
      'include_dirs': [
		'.'
      ],
	  #目标工程源代码路径
      'sources': [
		"skynet-src/skynet_main.c",		
	  ],
	  'conditions': [
        ['OS=="win"', {
		  'product_name':'skynet2',
          'defines': [

          ],
		  'msvs_disabled_warnings': [
			'4204','4013','4996','4152','4047','4024','4133'
		  ],
		  'msvs_settings': {
              'VCLinkerTool': {
				'AdditionalLibraryDirectories':[
					'3rd',
				],
              },
            },
			'dependencies': [	
				'3rd/3rd.gyp:posix_win',
			],
        }, { # OS != "win",
		  'product_name':'skynet',
          'defines': [
            
          ],
        }]
      ],
    },
  ],
  
}
