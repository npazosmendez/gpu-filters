#!/bin/bash

TARGET_FILE=`realpath $1`

true > ${TARGET_FILE}
echo "// This is generated source code to stringify OpenCL kernels, do not edit." >> ${TARGET_FILE}

(cd opencl && 
for file in *.cl; do
    echo "\n// ${file}" >> ${TARGET_FILE}
    xxd -i ${file} >> ${TARGET_FILE}
done
)