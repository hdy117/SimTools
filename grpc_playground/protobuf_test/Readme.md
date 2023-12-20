# linking against different version of protobuf

> main is linking against libprotobuf.so.23
> libmsg_gen.so is linkding against libprotobuf.a (version 24.3)
> then we got segmentfault when run main
>
> everything works fine if main and libmsg_gen.so is linking against one specific version of protobuf