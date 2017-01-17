# pbc.gypi
{
  'targets': [
  {
      'target_name': 'pbc',
      'type': 'static_library',
      'defines': [
      ],
      'include_dirs': [
		'../pbc/'
      ],
      'sources': [
		"pbc.h",
		"src/alloc.c",
		"src/alloc.h",
		"src/array.c",
		"src/array.h",
		"src/bootstrap.c",
		"src/bootstrap.h",
		"src/context.c",
		"src/context.h",
		"src/decode.c",
		"src/descriptor.pbc.h",
		"src/map.c",
		"src/map.h",
		"src/pattern.c",
		"src/pattern.h",
		"src/proto.c",
		"src/proto.h",
		"src/register.c",
		"src/rmessage.c",
		"src/stringpool.c",
		"src/stringpool.h",
		"src/varint.c",
		"src/varint.h",
		"src/wmessage.c",
	  ],
		'direct_dependent_settings': {
			'include_dirs': [
				'../pbc',
			],
		},
	  'conditions': [
        ['OS=="win"', {
		  'msvs_settings': {
            'VCCLCompilerTool': {
				'CompileAs':2,# CompileAsCpp
            },
          },
		  'sources': [
			#"lua-md5/md5.def",
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'protobuf',
      'type': 'shared_library',
      'dependencies': [
		'lua',
		'pbc',
      ],
      'include_dirs': [
		'lua',
		'../pbc/'
      ],
	  #目标工程源代码路径
      'sources': [
		"binding/lua53/pbc-lua53.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
		  'msvs_settings': {
            'VCCLCompilerTool': {
				'CompileAs':2,# CompileAsCpp
            },
          },
		  'sources': [
			"binding/lua53/protobuf.def",
		  ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
  ],
}
