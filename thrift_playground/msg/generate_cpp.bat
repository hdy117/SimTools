@echo off

SET vcpkg_root=E:\work\vcpkg
SET protoc=%vcpkg_root%\installed\x64-windows\tools\protobuf\protoc.exe
set thrift_gen=%vcpkg_root%\installed\x64-windows\tools\thrift\thrift.exe
SET msg_dir=%~dp0

for %%G in (%msg_dir%\*.thrift) do %thrift_gen% --gen cpp %%G
