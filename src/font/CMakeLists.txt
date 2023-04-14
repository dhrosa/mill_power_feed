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

set(font_includes "")
set(font_functions "")

add_library(font font.cc)
target_include_directories(font PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

foreach(font_image ${font_images})
  cmake_path(GET font_image STEM font_name)

  add_custom_command(
    OUTPUT ${font_name}.bin
    DEPENDS venv ${font_image} generate_font.py
    COMMAND
      ${venv_path}/bin/python3 ${CMAKE_CURRENT_SOURCE_DIR}/generate_font.py
      ${CMAKE_CURRENT_SOURCE_DIR}/${font_image}
      ${CMAKE_CURRENT_BINARY_DIR}/${font_name}.bin)
  add_custom_command(
    OUTPUT ${font_name}.o
    DEPENDS ${font_name}.bin
    COMMAND arm-none-eabi-ld --relocatable --format=binary -z noexecstack
            --output ${font_name}.o ${font_name}.bin)

  add_library(${font_name} ${font_name}.o)
  set_target_properties(${font_name} PROPERTIES LINKER_LANGUAGE C)

  configure_file(raw_data.h.in ${font_name}.h)
  string(APPEND font_includes "#include \"${font_name}.h\"\n")
  string(APPEND font_functions "${font_name}(),")

  target_link_libraries(font PRIVATE ${font_name})
endforeach()

configure_file(all_fonts.cc.in all_fonts.cc)
target_sources(font PRIVATE all_fonts.cc)
