libcut = declare_dependency(include_directories : include_directories('../libcut'),
                            compile_args : ['-std=c11'])

test('libdatabin', executable('libdatabin-test', 'libdatabin-test.c',
                              dependencies : [libcut], link_with : [databin]))
