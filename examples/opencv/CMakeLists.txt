
find_package( OpenCV REQUIRED core highgui imgproc)

include_directories( ${OpenCV_INCLUDE_DIRS})
link_directories ( ${OpenCV_LIBRARY_PATH})

add_executable(opencv opencv.cpp )
target_link_libraries(opencv ${OpenCV_LIBS})

target_link_libraries(opencv threshtree depthtree) 

