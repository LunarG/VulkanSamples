add_library(XGL SHARED loader.c)
set_target_properties(XGL PROPERTIES SOVERSION 0)
target_link_libraries(XGL -ldl -lpthread)
