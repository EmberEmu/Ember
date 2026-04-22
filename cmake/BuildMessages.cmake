# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Runs protogen over every message JSON in 'messages_dir' to produce C++
# headers under '${CMAKE_BINARY_DIR}/generated/', plus a 'Packets.h' which
# includes all other packet headers for convenience
#
# Adds a 'PROTOGEN_GENERATE' target that other targets can depend on, exposing
# the output root via the 'PROTOGEN_GENERATED_DIR' cache variable so
# consumers can add it to their include path

function(build_messages
         messages_dir
         types_path
         types_schema
         message_schema)

    set(protogen "protogen")
    set(output_root "${CMAKE_BINARY_DIR}/generated")
    set(templates_dir "$<TARGET_PROPERTY:${protogen},PROTOGEN_TEMPLATES_DIR>")

    file(GLOB message_jsons CONFIGURE_DEPENDS "${messages_dir}/*.json")
    file(GLOB templates CONFIGURE_DEPENDS
         "${CMAKE_SOURCE_DIR}/src/tools/protogen/templates/*.h_")

    set(aggregator "${output_root}/Packets.h")

    add_custom_command(
        OUTPUT ${aggregator}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${output_root}
        COMMAND ${protogen}
                --schema ${message_schema}
                --types-schema ${types_schema}
                --types ${types_path}
                --messages ${messages_dir}
                --output ${output_root}
                --templates ${templates_dir}
        DEPENDS ${protogen} ${types_path} ${message_jsons} ${templates}
        COMMENT "Generating message headers under ${output_root}"
        VERBATIM
    )

    add_custom_target(PROTOGEN_GENERATE DEPENDS ${aggregator})

    set(PROTOGEN_GENERATED_DIR "${CMAKE_BINARY_DIR}" CACHE PATH
        "Include root for generated message headers (use #include <generated/...>)")
    set_source_files_properties(${aggregator} PROPERTIES GENERATED TRUE)
endfunction()