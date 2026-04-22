# Copyright (c) 2021 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

function(build_spark_services
         service_schemas
         template_dir
         bfbs_dir
         output_dir)

    set(rpcgen "rpcgen")
    set(output_files "")

    foreach(schema_path IN LISTS service_schemas)
        cmake_path(GET schema_path FILENAME name)
        cmake_path(REMOVE_EXTENSION name)

        set(bfbs_file "${bfbs_dir}/${name}.bfbs")
        set(client_h "${output_dir}/${name}ClientStub.h")
        set(service_h "${output_dir}/${name}ServiceStub.h")

        add_custom_command(
            OUTPUT ${client_h} ${service_h}
            COMMAND ${rpcgen}
                    -t ${template_dir}
                    -s ${bfbs_file}
                    -o ${output_dir}
            DEPENDS ${rpcgen} FB_SCHEMA_COMPILE
            COMMENT "Generating Spark stubs for ${name}"
            VERBATIM
        )

        list(APPEND output_files ${client_h} ${service_h})
    endforeach()

	set(target_name SPARK_RPC_GENERATE)

    add_custom_target(
        ${target_name}
        DEPENDS ${output_files}
    )

    set_source_files_properties(${output_files} PROPERTIES GENERATED TRUE)
    set(${output_files} ${output_files} PARENT_SCOPE)
	set_target_properties(${target_name} PROPERTIES FOLDER "Code Generation")
endfunction()