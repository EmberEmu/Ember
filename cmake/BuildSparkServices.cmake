# Copyright (c) 2021 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

function(build_spark_services
         service_schemas
         template_dir
         bfbs_dir
         output_dir)

    foreach(schema_path ${service_schemas})
		message(${schema_path})
        cmake_path(GET schema_path FILENAME out)
        cmake_path(REMOVE_EXTENSION out)
        set(input_files ${input_files} ${bfbs_dir}/${out}.bfbs)
        set(output_files ${output_files} ${output_dir}/${out}Service_generated.h)
    endforeach()
  
	# concat the filenames into a format usable by the tool
    foreach(filename ${input_files})
        set(input_name_str "${filename}" ${input_name_str})
    endforeach()

    set_source_files_properties(${${output_files}} PROPERTIES GENERATED TRUE)

    add_custom_command(
		TARGET FB_SCHEMA_COMPILE
        COMMAND rpcgen -t ${template_dir} -s ${input_name_str} -o ${output_dir}
        DEPENDS rpcgen
        COMMENT "Generating Spark RPC service stubs..."
    )

    set(${output_files} ${${output_files}} PARENT_SCOPE)
endfunction()