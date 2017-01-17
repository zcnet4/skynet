# 3rd.gypi
{
  'targets': [
	{
      'target_name': 'tar',
      'type': 'static_library',
      'sources': [
		"tar.c",
		"tar.h",
	  ],
	  'conditions': [
        ['OS=="win"', {
		  'dependencies': [
			'posix_win',
		  ],
		  'defines': [],
		  'include_dirs': [
		   '../posix_win',
          ],
        }, { # OS != "win",
          'defines': [
            
          ],
        }]
      ],
    },
	{
      'target_name': 'tar_bin',
      'type': 'executable',
      'dependencies': [
		'tar',
      ],
      'defines': [
      ],
      'include_dirs': [
		'tar',
      ],
      'sources': [
		"main.c",
	  ],
	  'conditions': [
        ['OS=="win"', {
		  'dependencies': [ 'posix_win',],
		  'include_dirs': [ '../posix_win',],
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
        }]
      ],
    },
  ],
}
