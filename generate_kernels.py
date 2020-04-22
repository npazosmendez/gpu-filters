#!/usr/bin/env python3

import os
import json

with open("generated/kernels.h", "w") as target_file:
    target_file.write("// This is generated source code to stringify OpenCL kernels, do not edit.\n")
    for filename in os.listdir("src/filters/opencl"):
        if filename.endswith(".cl"):
            name = filename[:-3] + "_cl" # canny.cl -> canny_cl
            target_file.write(f"//{filename}\n")
            with open(f"src/filters/opencl/{filename}") as source_file:
                string_literal = json.dumps(source_file.read()).strip('"') # Escape string
                target_file.write(f"char {name}[] = \"{string_literal}\";\n")
        else:
            continue
