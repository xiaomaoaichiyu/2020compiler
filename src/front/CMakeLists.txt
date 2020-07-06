include_directories(../ir ../util)

aux_source_directory(. DIR_FRONT)

add_library(front ${DIR_FRONT})