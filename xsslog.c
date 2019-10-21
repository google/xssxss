// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "xsslog.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void StrAppend4(char target[], const char a[], const char b[],
                       const char c[], const char d[]) {
  stpcpy(stpcpy(stpcpy(stpcpy(target, a), b), c), d);
}

void CheckErrnoImpl(const bool condition, const char description[]) {
  if (condition) {
    return;
  }
  const int e = errno;
  char buf[128];
  fprintf(stderr, "%s: check %s failed: %s\n", kProjectName, description,
          strerror_r(e, buf, 128));
  _Exit(1);
}

void CheckRetImpl(const int e, const char description[]) {
  if (e == 0) {
    return;
  }
  char buf[128];
  fprintf(stderr, "%s: %s failed: %s\n", kProjectName, description,
          strerror_r(e, buf, 128));
  _Exit(1);
}

void LogDebug(const char* const format, ...) {
  const char colon_space[] = ": ";
  char* const prefixed_format =
      calloc(strlen(kProjectName) + strlen(colon_space) + strlen(format) + 2,
             sizeof(char));
  CHECK_ERRNO(prefixed_format != NULL);
  StrAppend4(prefixed_format, kProjectName, ": ", format, "\n");
  va_list args;
  va_start(args, format);
  vfprintf(stderr, prefixed_format, args);
  va_end(args);
  free(prefixed_format);
}
