# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

CPPFLAGS = -D_GNU_SOURCE
CFLAGS = -O2 -flto -Wall -Werror=format-security -fstack-protector-strong -fvisibility=hidden -fpic -pthread
LDFLAGS = -Wl,-O2 -flto -Wl,-z,relro -Wl,-z,now -fpic -pthread -shared -s
LDLIBS = -lX11 -ldl

.PHONY: all
all: libxssxss.so

libxssxss.so: xssbase.o xsslog.o xssxss.o
	$(LINK.o) $^ $(LDLIBS) -o $@

xssbase.o: xssbase.c xssbase.h
xsslog.o: xsslog.c xssbase.h xsslog.h
xssxss.o: xssxss.c xssbase.h xsslog.h

.PHONY: clean
clean:
	$(RM) xssbase.o xsslog.o xssxss.o libxssxss.so
