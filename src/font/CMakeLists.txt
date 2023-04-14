add_library(font font.cc)
target_include_directories(font PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(venv_path ${CMAKE_CURRENT_BINARY_DIR}/venv)

add_custom_command(
  OUTPUT venv.stamp
  DEPENDS requirements.txt
  COMMAND mkdir -p ${venv_path}
  COMMAND python3 -m venv ${venv_path}
  COMMAND ${venv_path}/bin/pip3 install -r
          ${CMAKE_CURRENT_LIST_DIR}/requirements.txt
  COMMAND touch venv.stamp)

add_custom_target(venv DEPENDS venv.stamp)

set(font_images font_5x8.png font_6x12.png font_8x16.png font_12x24.png
                font_16x32.png font_32x64.png)

foreach(image ${font_images})
  cmake_path(APPEND CMAKE_CURRENT_SOURCE_DIR ${image} OUTPUT_VARIABLE
             image_path)
  list(APPEND font_image_paths ${image_path})
endforeach()

add_custom_command(
  OUTPUT font_data.bin
  DEPENDS venv generate_fonts.py ${font_images}
  COMMAND
    ${venv_path}/bin/python3 "${CMAKE_CURRENT_SOURCE_DIR}/generate_fonts.py"
    ${font_image_paths} -o font_data.bin)

add_custom_command(
  OUTPUT font_data.o
  DEPENDS font_data.bin
  COMMAND arm-none-eabi-ld --relocatable --format=binary -z noexecstack
          --output font_data.o font_data.bin)

add_library(font_data font_data.o)
set_target_properties(font_data PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(font PRIVATE font_data)
