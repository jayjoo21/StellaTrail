cmake_minimum_required(VERSION 3.10)
project(StellaTrail)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg를 통한 SDL2 및 SDL2_image 찾기
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)

# 실행 파일 생성
add_executable(StellaTrail main.cpp)

# 라이브러리 연결 (가장 확실한 방법)
target_link_libraries(StellaTrail PRIVATE 
    SDL2::SDL2 
    SDL2::SDL2main 
    SDL2_image::SDL2_image
)

# 인클루드 디렉토리 설정
target_include_directories(StellaTrail PRIVATE ${SDL2_INCLUDE_DIRS})