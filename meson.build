project('quickerGPGX','c','cpp',
  version: '1.0.0',
  default_options : ['cpp_std=c++20', 'default_library=shared', 'buildtype=release'],
  subproject_dir : 'extern'
)

# Loading dependencies
subdir('source')

# Do not build any targets if this is a subproject
if meson.is_subproject() == false

# Common application flags
commonCompileArgs = [ '-Wall', '-Wfatal-errors' ]

# Grabbing jaffarCommon dependency

jaffarCommonSubproject = subproject('jaffarCommon')
jaffarCommonDependency = jaffarCommonSubproject.get_variable('jaffarCommonDependency')

# Building playback tool

if get_option('buildPlayer') == true

  newGPGXPlayer = executable('newGPGXPlayer',
    'source/player.cpp',
    cpp_args            : [ commonCompileArgs, '-DNCURSES' ],
    dependencies        : [ newGPGXDependency,
                            jaffarCommonDependency
                          ],
    include_directories : include_directories(['source']),
    link_args           : [ '-lncurses' ],
  )

  baseGPGXPlayer = executable('baseGPGXPlayer',
    'source/player.cpp',
    cpp_args            : [ commonCompileArgs, '-DNCURSES' ],
    dependencies        : [ baseGPGXDependency,
                            jaffarCommonDependency,
                          ],
    include_directories : include_directories(['source']),
    link_args           : [ '-lncurses' ],
  )

endif

# Building tester tool for QuickerSnes9x

newGPGXTester = executable('newGPGXTester',
  'source/tester.cpp',
  cpp_args            : [ commonCompileArgs ], 
  dependencies        : [ newGPGXDependency, jaffarCommonDependency ],
)

# Building tester tool for the original snes9x

baseGPGXTester = executable('baseGPGXTester',
  'source/tester.cpp',
  cpp_args            : [ commonCompileArgs ],
  dependencies        : [ baseGPGXDependency, jaffarCommonDependency ],
)

# Building tests
subdir('tests')

endif # If not subproject
