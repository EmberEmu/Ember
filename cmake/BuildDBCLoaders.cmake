# Copyright (c) 2016 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


 function(build_dbc_loaders dbc_hdr dbc_src
                            definition_dirs
                            output_dir
                            template_dir
                            target_name
                            fverbosity)
    set(dbcparser "dbcparser")

    set(template_files
        "${template_dir}/DiskDefs.h_"
        "${template_dir}/DiskLoader.cpp_"
        "${template_dir}/Linker.cpp_"
        "${template_dir}/MemoryDefs.h_"
        "${template_dir}/Storage.h_"
    )

    set(headers
        "${output_dir}/DiskDefs.h"
        "${output_dir}/MemoryDefs.h"
        "${output_dir}/Storage.h"
    )

    set(sources
        "${output_dir}/DiskLoader.cpp"
        "${output_dir}/Linker.cpp"
    )

    set_source_files_properties(
        ${headers}
        ${sources}
        PROPERTIES GENERATED TRUE
    )

    set(${dbc_hdr} ${headers} PARENT_SCOPE)
    set(${dbc_src} ${sources} PARENT_SCOPE)

    set(input_dbcs "")

    foreach(dir ${definition_dirs})
        file(GLOB input_dbcs ${input_dbcs} ${dir}/*.xml)
    endforeach()

    add_custom_command(
        OUTPUT ${headers} ${sources}
        COMMAND ${dbcparser}
        -d ${definition_dirs}
        -t ${template_dir}
        -o ${output_dir}
        --fverbosity ${fverbosity}
        --disk
        DEPENDS ${dbcparser} ${input_dbcs} ${template_files}
        COMMENT "Generating DBC loaders..."
    )

    add_custom_target(
        ${target_name}
        DEPENDS ${dbcparser} ${headers} ${sources} ${additional_dependencies}
    )

    set(${dbc_hdr} ${headers} PARENT_SCOPE)
    set(${dbc_src} ${sources} PARENT_SCOPE)
    set_target_properties(${target_name} PROPERTIES FOLDER "Code Generation")
endfunction()