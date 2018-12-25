/*
 * @Snail@ - Code Execution Footprint Tracer & Explorer.
 *
 * Copyright (C) 2018 Haoran Luo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file snailviewer/liveness_pthread.cpp
 * @author Haoran Luo
 * @brief Implementation of liveness pointer based on posix thread.
 *
 * The implementation is based on posix thread, which enables the pointer to be shared 
 * among multithreads. See also the corresponding header for specification details.
 * @see snailviewer/liveness.hpp
 */
#include "snailviewer/liveness.hpp"
#include <pthread.h>
#include <stdexcept>

namespace snailviewer {

// Implementation of lifeline::lifeline().
lifeline::lifeline(void* object): object(object) {
	static_assert((64 - sizeof(void*)) >= sizeof(pthread_rwlock_t), 
		"The control block is too small to hold the read-write lock information.");
	
	if(pthread_rwlock_init(reinterpret_cast<pthread_rwlock_t*>(controlBlock), NULL) != 0) {
		throw std::runtime_error("Cannot create read-write lock!");
	}
}

// Implementation of lineline::~lifeline().
lifeline::~lifeline() noexcept {
	pthread_rwlock_destroy(reinterpret_cast<pthread_rwlock_t*>(controlBlock));
}

// Implementation of lifeline::waitForDeath().
void lifeline::waitForDeath() noexcept {
	pthread_rwlock_wrlock(reinterpret_cast<pthread_rwlock_t*>(controlBlock));
	object = nullptr;
	pthread_rwlock_unlock(reinterpret_cast<pthread_rwlock_t*>(controlBlock));
}

// Implementation of lifeline::retain().
void* lifeline::retain() noexcept {
	// We use try read lock to make it fail fast if it is already write-locking.
	if(pthread_rwlock_tryrdlock(reinterpret_cast<pthread_rwlock_t*>(controlBlock)) != 0) {
		return nullptr;
	} else {
		return object;
	}
}

// Implementation of lifeline::release().
void lifeline::release() noexcept {
	pthread_rwlock_unlock(reinterpret_cast<pthread_rwlock_t*>(controlBlock));
}

} // namespace snailviewer.