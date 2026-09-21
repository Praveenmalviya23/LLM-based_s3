idf_component_register(SRCS "main.c" "run.c"
                       INCLUDE_DIRS ".")

# This line tells the linker to embed the vocabulary and generate the _binary_tok512_bin_start symbol
target_add_binary_data(${COMPONENT_LIB} "tok512.bin" TEXT)