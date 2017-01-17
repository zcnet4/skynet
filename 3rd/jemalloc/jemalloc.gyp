{
  'targets': [
    {
      'target_name': 'jemalloc',
      'type': 'none',
	  'conditions': [
        ['OS=="win"', {
		
        }, { # OS != "win",
		  'actions': [
		  {
            'action_name': 'generate header',
			 'inputs': [],
             'outputs': [''],
            'action': ['./build.sh'],
		    'message': 'Generating manpage'
          },
         ],
        }
		]
      ],
	},
  ],
}
