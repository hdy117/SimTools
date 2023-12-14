@echo off

SET vcpkg_root=E:\work\vcpkg
SET protoc=%vcpkg_root%\installed\x64-windows\tools\protobuf\protoc.exe
SET grpc_cpp_plugin=%vcpkg_root%\installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe
SET msg_dir=%~dp0


for %%G in (%msg_dir%\*.proto) do %protoc% --proto_path=%msg_dir% -I=%msg_dir% --cpp_out=%msg_dir% %%G

%protoc% -I=%msg_dir% --grpc_out=%msg_dir% --plugin=protoc-gen-grpc=%grpc_cpp_plugin% %msg_dir%\HelloService.proto
