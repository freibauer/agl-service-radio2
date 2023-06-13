#
# Copyright (C) 2022 Konsulko Group
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

project (
  'agl-service-radio',
  ['c', 'cpp'],
  version : '2.0.0',
  license : 'Apache-2.0',
  meson_version : '>= 0.46.0',
  default_options :
    [
      'warning_level=1',
      'buildtype=debugoptimized',
      'c_std=gnu17',
      'cpp_std=c++17'
    ],
)

systemd_dep = dependency('systemd')

subdir('src')
subdir('systemd')
