# Copyright (c) 2021 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

function(build_spark_services
         definition_dirs
         output_dir
         target_name
         fverbosity)

    # concat the paths into a format usable by the tool
    foreach(dir ${definition_dirs})
        set(definition_dir_str \"${dir}\" ${definition_dir_str})
    endforeach()

    add_custom_command(
        OUTPUT  "test.h"
        COMMAND rpcgen -s "Test.bfbs" -o ${output_dir} --fverbosity ${fverbosity}
        DEPENDS rpcgen
        COMMENT "Generating Spark service stubs..."
    )

    add_custom_target(${target_name} DEPENDS rpcgen ${additional_dependencies})

endfunction()