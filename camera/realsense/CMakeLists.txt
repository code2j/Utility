cmake_minimum_required(VERSION 3.10)
project(realsense_camera)

set(CMAKE_CXX_STANDARD 17)


# =======================================================
# 패키지 찾기
# =======================================================
find_package(realsense2 REQUIRED)
find_package(OpenCV REQUIRED)


# =======================================================
# 실행 파일 생성
# =======================================================
add_executable(realsense_camera
        realsense_camera.cpp
)


# =======================================================
# 라이브러리 링크
# =======================================================
target_link_libraries(realsense_camera
        ${realsense2_LIBRARY}
        ${OpenCV_LIBS}
)