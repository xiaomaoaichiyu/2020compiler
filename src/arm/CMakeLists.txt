include_directories(./ ../front ../ir ../util)

aux_source_directory(. DIR_ARM)

add_library(arm ${DIR_ARM})