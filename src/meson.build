#
# Copyright (C) 2021 Collabora Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cpp = meson.get_compiler('cpp')
grpcpp_reflection_dep = cpp.find_library('grpc++_reflection')

gstreamer_dep = dependency('gstreamer-1.0')

radio_deps = [
    dependency('gobject-2.0'),
    dependency('gio-unix-2.0'),
    gstreamer_dep,
    dependency('protobuf'),
    dependency('grpc'),
    dependency('grpc++'),
    grpcpp_reflection_dep
]

protoc = find_program('protoc')
grpc_cpp = find_program('grpc_cpp_plugin')

protoc_gen = generator(protoc, \
                       output : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
                       arguments : ['--proto_path=@CURRENT_SOURCE_DIR@/../protos',
                         '--cpp_out=@BUILD_DIR@',
                         '@INPUT@'])
generated_protoc_sources = protoc_gen.process('../protos/radio.proto')

grpc_gen = generator(protoc, \
                     output : ['@BASENAME@.grpc.pb.cc', '@BASENAME@.grpc.pb.h'],
                     arguments : ['--proto_path=@CURRENT_SOURCE_DIR@/../protos',
                       '--grpc_out=@BUILD_DIR@',
                       '--plugin=protoc-gen-grpc=' + grpc_cpp.path(),
                       '@INPUT@'])
generated_grpc_sources = grpc_gen.process('../protos/radio.proto')

# FIXME: debug radio_impl_tef665x.c compile issues and add
executable (
    'agl-service-radio',
    [
        generated_protoc_sources,
        generated_grpc_sources,
        'main-grpc.cc',
	'RadioImpl.cc',
	'radio_impl_null.c',
	'radio_impl_kingfisher.c',
	'radio_impl_rtlsdr.c'
    ],
    dependencies : radio_deps,
    install : true,
    install_dir : get_option('sbindir')
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
helper_deps = [ gstreamer_dep, m_dep, dependency('librtlsdr'), dependency('libusb-1.0') ]
executable (
    'rtl_fm_helper',
    [
        'rtl_fm_helper.c',
	'radio_output_gstreamer.c',
	'rtl_fm.c',
	'convenience/convenience.c'
    ],
    dependencies : helper_deps,
    install : true,
    install_dir : get_option('sbindir')
)
