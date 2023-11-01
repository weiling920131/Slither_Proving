protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS clap.proto)
add_library(clap.proto OBJECT ${PROTO_SRCS})