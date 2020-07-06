include_directories(../front ../ir ../util)

aux_source_directory(. DIR_GEN)

add_library(gen ${DIR_GEN})