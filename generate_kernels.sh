#!/bin/bash

TARGET_FILE=`realpath $1`

true > ${TARGET_FILE}
echo "// This is generated source code to stringify OpenCL kernels, do not edit." >> ${TARGET_FILE}

(cd opencl && 
for file in *.cl; do
    name="${file%%.*}_cl"
    echo "\n// ${file}" >> ${TARGET_FILE}
    echo "char ${name}[] = {" >> ${TARGET_FILE}
    raw_code=$(cat ${file})

    for (( i=0; i<${#raw_code}; i++ )); do
        char=${raw_code:$i:1}
        printf "'$char'," >> ${TARGET_FILE};
    done
    printf "'\n'};" >> ${TARGET_FILE}
done
)