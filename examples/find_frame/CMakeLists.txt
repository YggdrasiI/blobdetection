
find_package( OpenCV REQUIRED core highgui imgproc)

include_directories( ${OpenCV_INCLUDE_DIRS})
link_directories ( ${OpenCV_LIBRARY_PATH})

add_executable(find_frame find_frame.cpp )
target_link_libraries(find_frame ${OpenCV_LIBS})

target_link_libraries(find_frame threshtree depthtree) 

