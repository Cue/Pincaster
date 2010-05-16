/*
 * Copyright (c) 2008-2010 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVENT2_THREAD_H_
#define _EVENT2_THREAD_H_

/** @file thread.h

  Functions for multi-threaded applications using Libevent.

  When using a multi-threaded application in which multiple threads
  add and delete events from a single event base, Libevent needs to
  lock its data structures.

  Like the memory-management function hooks, all of the threading functions
  _must_ be set up before an event_base is created if you want the base to
  use them.

  A multi-threaded application must provide locking functions to
  Libevent via evthread_set_locking_callback().  Libevent will invoke
  this callback whenever a lock needs to be acquired or released.

  The total number of locks employed by Libevent can be determined
  via the evthread_num_locks() function.  An application must provision
  that many locks.

  If the owner of an event base is waiting for events to happen,
  Libevent may signal the thread via a special file descriptor to wake
  up.   To enable this feature, an application needs to provide a
  thread identity function via evthread_set_id_callback().

 */

#ifdef __cplusplus
extern "C" {
#endif

#include <event-config.h>

/** A flag passed to a locking callback when the lock was allocated as a
 * read-write lock, and we want to acquire or release the lock for writing. */
#define EVTHREAD_WRITE	0x04
/** A flag passed to a locking callback when the lock was allocated as a
 * read-write lock, and we want to acquire or release the lock for reading. */
#define EVTHREAD_READ	0x08
/** A flag passed to a locking callback when we don't want to block waiting
 * for the lock; if we can't get the lock immediately, we will instead
 * return nonzero from the locking callback. */
#define EVTHREAD_TRY    0x10

#ifndef _EVENT_DISABLE_THREAD_SUPPORT

/**
   @deprecated Use evthread_set_lock_callbacks instead.  This API will
     go away before Libevent 2.0.x-stable.
*/
void evthread_set_lock_create_callbacks(
    void *(*alloc_fn)(void), void (*free_fn)(void *));

/**
   @deprecated Use evthread_set_lock_callbacks instead.  This API will
     go away before Libevent 2.0.x-stable.
 */
void evthread_set_locking_callback(
    void (*locking_fn)(int mode, void *lock));

#define EVTHREAD_LOCK_API_VERSION 1

/** A recursive lock is one that can be acquired multiple times at once by the
 * same thread.  No other process can allocate the lock until the thread that
 * has been holding it has unlocked it as many times as it locked it. */
#define EVTHREAD_LOCKTYPE_RECURSIVE 1
/* A read-write lock is one that allows multiple simultaneous readers, but
 * where any one writer excludes all other writers and readers. */
#define EVTHREAD_LOCKTYPE_READWRITE 2

/** This structure describes the interface a threading library uses for
 * locking.   It's used to tell evthread_set_lock_callbacks how to use
 * locking on this platform.
 */
struct evthread_lock_callbacks {
	/** The current version of the locking API.  Set this to
	 * EVTHREAD_LOCK_API_VERSION */
	int lock_api_version;
	/** Which kinds of locks does this version of the locking API
	 * support?  A bitfield of EVTHREAD_LOCKTYPE_RECURSIVE and
	 * EVTHREAD_LOCKTYPE_READWRITE.
	 *
	 * (Note that RECURSIVE locks are currently mandatory, and
	 * READWRITE locks are not currently used.)
	 **/
	unsigned supported_locktypes;
	/** Function to allocate and initialize new lock of type 'locktype'.
	 * Returns NULL on failure. */
	void *(*alloc)(unsigned locktype);
	/** Funtion to release all storage held in 'lock', which was created
	 * with type 'locktype'. */
	void (*free)(void *lock, unsigned locktype);
	/** Acquire an already-allocated lock at 'lock' with mode 'mode'.
	 * Returns 0 on success, and nonzero on failure. */
	int (*lock)(unsigned mode, void *lock);
	/** Release a lock at 'lock' using mode 'mode'.  Returns 0 on success,
	 * and nonzero on failure. */
	int (*unlock)(unsigned mode, void *lock);
};

/** Sets a group of functions that Libevent should use for locking.
 * For full information on the required callback API, see the
 * documentation for the individual members of evthread_lock_callbacks.
 *
 * Note that if you're using Windows or the Pthreads threading library, you
 * probably shouldn't call this function; instead, use
 * evthread_use_windos_threads() or evthread_use_posix_threads() if you can.
 */
int evthread_set_lock_callbacks(const struct evthread_lock_callbacks *);

/**
   Sets the function for determining the thread id.

   @param base the event base for which to set the id function
   @param id_fn the identify function Libevent should invoke to
     determine the identity of a thread.
*/
void evthread_set_id_callback(
    unsigned long (*id_fn)(void));

#if defined(WIN32) && !defined(_EVENT_DISABLE_THREAD_SUPPORT)
/** Sets up Libevent for use with Windows builtin locking and thread ID
	functions.  Unavailable if Libevent is not built for Windows.

	@return 0 on success, -1 on failure. */
int evthread_use_windows_threads(void);
#define EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED 1
#endif

#if defined(_EVENT_HAVE_PTHREADS)
/** Sets up Libevent for use with Pthreads locking and thread ID functions.
	Unavailable if Libevent is not build for use with pthreads.  Requires
	libraries to link against Libevent_pthreads as well as Libevent.

	@return 0 on success, -1 on failure. */
int evthread_use_pthreads(void);
#define EVTHREAD_USE_PTHREADS_IMPLEMENTED 1

#endif

/** Enable debugging wrappers around the current lock callbacks.  If Libevent
 * makes one of several common locking errors, exit with an assertion failure.
 **/
void evthread_enable_lock_debuging(void);

#endif /* _EVENT_DISABLE_THREAD_SUPPORT */

struct event_base;
/** Make sure it's safe to tell an event base to wake up from another thread.
    or a signal handler.

	@return 0 on success, -1 on failure.
 */
int evthread_make_base_notifiable(struct event_base *base);

#ifdef __cplusplus
}
#endif

#endif /* _EVENT2_THREAD_H_ */
