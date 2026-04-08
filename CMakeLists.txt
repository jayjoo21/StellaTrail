cmake_minimum_required(VERSION 3.10)
project(StellaTrail)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg를 통한 SDL2 찾기
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)

# main.cpp를 실행 파일로 빌드
add_executable(StellaTrail main.cpp)

# SDL2 라이브러리 링크
target_link_libraries(StellaTrail PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
)
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    )
else()
    target_include_directories(StellaTrail PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(StellaTrail PRIVATE ${SDL2_LIBRARIES})
endif()
