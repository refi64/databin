databin = library('databin', 'databin.c', install : true)
install_headers('databin.h')

databin_inc = include_directories('.')
databin_dep = declare_dependency(include_directories : databin_inc,
                                 link_with : [databin])
