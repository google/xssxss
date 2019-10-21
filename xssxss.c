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

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <X11/Xlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <unistd.h>

#include "xssbase.h"
#include "xsslog.h"

static const int kWakeupSeconds = 10;

// Runs `xdg-screensaver reset` every `kWakeupSeconds` seconds.
static void* WakerThreadMain(void* const ignored) {
  // We'd like the new process to start with all signals in the default state.
  // Set up the appropriate attributes to pass to `posix_spawn`.
  sigset_t sigset;
  posix_spawnattr_t attr;
  CHECK_RET(posix_spawnattr_init(&attr));
  CHECK_ERRNO(sigfillset(&sigset) == 0);
  CHECK_RET(posix_spawnattr_setsigdefault(&attr, &sigset));
  CHECK_ERRNO(sigemptyset(&sigset) == 0);
  CHECK_RET(posix_spawnattr_setsigmask(&attr, &sigset));
  CHECK_RET(posix_spawnattr_setflags(
      &attr, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK));

  while (true) {
    LogDebug("resetting screensaver");

    // Start the process to wake up xscreensaver. When it exits, we'll get
    // SIGCHLD, but we don't care; see notes in `SpawnWakerThread`.
    pid_t waker_pid;
    char* const waker[] = {"xdg-screensaver", "reset", NULL};
    const int e = posix_spawnp(&waker_pid, waker[0], /*file_action=*/NULL,
                               &attr, waker, environ);
    if (e != 0) {
      char buf[128];
      LogDebug("failed to spawn waker: %d", strerror_r(e, buf, 128));
    }
    sleep(kWakeupSeconds);
  }

  CHECK_RET(posix_spawnattr_destroy(&attr));
  return NULL;
}

// Starts a new thread at `WakerThreadMain`.
static pthread_t SpawnWakerThread(void) {
  // We'd like the new thread to start with all signals masked. The new thread
  // inherits the state of our signal mask, so we need to temporarily mask off
  // all signals before we call pthread_create.
  //
  // The new thread is going to be launching new processes, so you might think
  // we should attract `SIGCHLD` to it. If this process has left the `SIGCHLD`
  // handler at its default (i.e., to ignore it), this isn't necessary. If, on
  // the other hand, this process has some thread with an actual `SIGCHLD`
  // handler, we're screwed: that handler's going to get called spuriously when
  // the new thread's child exits, and attracting `SIGCHLD` to the new thread
  // won't solve that problem (the kernel will deliver the signal to an
  // arbitrary thread that accepts it, so the original `SIGCHLD` handler might
  // get called anyway). Hope that this process has no `SIGCHLD` handler active,
  // and just ignore it.
  sigset_t parent, child;
  CHECK_ERRNO(sigfillset(&child) == 0);
  CHECK_ERRNO(pthread_sigmask(SIG_SETMASK, &child, &parent) == 0);

  pthread_t thread;
  CHECK_ERRNO(pthread_create(&thread, /*attr=*/NULL, WakerThreadMain,
                             /*arg=*/NULL) == 0);

  CHECK_ERRNO(pthread_sigmask(SIG_SETMASK, &parent, /*oldset=*/NULL) == 0);
  return thread;
}

static void CancelThread(const pthread_t thread) {
  CHECK_ERRNO(pthread_cancel(thread) == 0);
  CHECK_ERRNO(pthread_join(thread, /*retval=*/NULL) == 0);
}

// This function must be thread-safe (it overrides a function that is also
// thread-safe.)
PUBLIC void XScreenSaverSuspend(Display* const display, const Bool suspend) {
  // These variables are guarded by the Xlib display lock. The real
  // XScreenSaverSuspend also locks the Xlib display, but the lock is a
  // recursive mutex, so the double-locking is safe.
  static void (*real_XScreenSaverSuspend)(Display*, Bool) = NULL;
  static int suspends_active = 0;
  static pthread_t waker_thread;

  LogDebug("%s requested", suspend ? "suspend" : "resume");

  XLockDisplay(display);

  if (real_XScreenSaverSuspend == NULL &&
      (real_XScreenSaverSuspend = dlsym(RTLD_NEXT, "XScreenSaverSuspend")) ==
          NULL) {
    // We couldn't get the real function. This isn't a huge deal, because we can
    // still shut off xscreensaver, but the user probably wants to know about
    // it.
    LogDebug("can't forward to real XScreenSaverSuspend: %s", dlerror());
  }

  if (real_XScreenSaverSuspend != NULL) {
    real_XScreenSaverSuspend(display, suspend);
  }

  if (suspends_active == 0) {
    if (suspend) {
      LogDebug("starting waker thread");
      waker_thread = SpawnWakerThread();
    } else {
      LogDebug("resume requested, but no suspends are active; ignoring");
    }
  } else if (suspends_active == 1 && !suspend) {
    LogDebug("canceling waker thread");
    CancelThread(waker_thread);
  }
  suspends_active += suspend ? 1 : -1;
  LogDebug("%d suspend%s now active", suspends_active,
           suspends_active == 1 ? " is" : "s are");

  XUnlockDisplay(display);
}
