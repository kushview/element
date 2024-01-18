res_gen = generator (bin2c,
    output : [ '@PLAINNAME@.cpp', '@PLAINNAME@.h'],
    arguments : [ '@INPUT@', '@BUILD_DIR@', 'res' ])
res_includes = include_directories ('.')
res_sources = res_gen.process (
    'acknowledgements.txt',
    'developers.txt'
)

subdir ('fonts')
subdir ('images')
