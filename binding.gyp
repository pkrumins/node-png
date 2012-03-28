{
    "targets": [
        {
            "target_name": "png",
            "sources": [
                "src/common.cpp",
                "src/png_encoder.cpp",
                "src/png.cpp",
                "src/fixed_png_stack.cpp",
                "src/dynamic_png_stack.cpp",
                "src/module.cpp",
                "src/buffer_compat.cpp",
            ],
            "include_dirs" : [ "gyp/include" ]
        }
    ]
}

