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

set(bdf_prefix "${spleen-font_SOURCE_DIR}/spleen-")
set(sizes 5x8 6x12 8x16 12x24 16x32 32x64)
set(bdf_suffix ".bdf")

foreach(size ${sizes})
  list(APPEND bdf_paths "${bdf_prefix}${size}${bdf_suffix}")
endforeach()

add_custom_command(
  OUTPUT font_data.bin
  DEPENDS venv generate_fonts.py ${font_images}
  COMMAND
    ${venv_path}/bin/python3 "${CMAKE_CURRENT_SOURCE_DIR}/generate_fonts.py"
    ${bdf_paths} -o font_data.bin)

add_custom_command(
  OUTPUT font_data.o
  DEPENDS font_data.bin
  COMMAND arm-none-eabi-ld --relocatable --format=binary -z noexecstack
          --output font_data.o font_data.bin)

add_library(font_data font_data.o)
set_target_properties(font_data PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(font PRIVATE font_data pico_platform)
