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

    set(${dbc_hdr} "")
    set(${dbc_src} "")

    list(APPEND ${dbc_hdr}
        ${output_dir}/DiskDefs.h
        ${output_dir}/MemoryDefs.h
        ${output_dir}/Storage.h
    )
    list(APPEND ${dbc_src}
        ${output_dir}/DiskLoader.cpp
        ${output_dir}/Linker.cpp
    )

    set_source_files_properties(${${dbc_hdr}} ${${dbc_src}} PROPERTIES GENERATED TRUE)

    set(input_dbcs "")
    foreach(dir ${definition_dirs})
        file(GLOB input_dbcs ${input_dbcs} ${dir}/*.xml)
    endforeach()

    add_custom_command(
        OUTPUT ${${dbc_hdr}} ${${dbc_src}}
        COMMAND ${dbcparser} 
        -d ${definition_dirs}
        -t ${template_dir} 
        -o ${output_dir} 
        --fverbosity ${fverbosity} 
        --disk
        DEPENDS ${dbcparser} ${input_dbcs}
        COMMENT "Generating DBC loaders..."
    )

    add_custom_target(
        ${target_name}
        DEPENDS ${dbcparser} ${${dbc_hdr}} ${${dbc_src}} ${additional_dependencies}
    )

    set(${dbc_hdr} ${${dbc_hdr}} PARENT_SCOPE)
    set(${dbc_src} ${${dbc_src}} PARENT_SCOPE)
endfunction()