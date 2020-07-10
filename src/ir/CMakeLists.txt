include_directories(./ ../front ../gen ../util)

aux_source_directory(. DIR_IR)

add_library(ir ${DIR_IR})