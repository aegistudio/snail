#pragma once
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
 * @file snailviewer/liveness.hpp
 * @author Haoran Luo
 * @brief Liveness pointer for snail viewer.
 *
 * One dilemma is that we wish to manage the object's lifecycle completely by itself,
 * which means we don't even want to share it with std::shared_ptr. But we want some
 * convenient polling methods to determine whether an object is alive and keep it alive
 * when we would like to use it, like std::weak_ptr.
 *
 * So this design comes into being, we split the objects into three roles:
 * - **Living**: The pointer embedded inside object itself, which has complete management 
 * right of the lifecycle of itself. With the living pointer, we can create an livness
 * pointer.
 * - **Liveness**: The pointer held by other objects which would like to determine the 
 * lifeness of the object pointed by living. But to hold the lifecycle, a holder pointer
 * must be created from the liveness pointer.
 * - **Holder**: The pointer held by the scope that would interact with the object, which
 * should be automatically destroyed and release the lock when it goes out of the scope.
 *
 * Only the cast from the living to liveness pointer and cast between liveness pointers
 * should be polymorphic. And both living and holder pointers are not copyable. (But is
 * movable). 
 */
#include <memory>
namespace snailviewer {

/**
 * @brief Shared and holds the life information of an object.
 *
 * The lineline must be allocated on heap to ensure the liveness pointers holds a valid
 * refernce to the lifeline. All methods beside the constructor and destructor are
 * multithread safe, and the construct and destructor are single thread exclusive.
 */
class lifeline {
	/// Forward those who can gain access to the lifeline methods.
	template<typename> friend class living_ptr;
	template<typename> friend class liveness_ptr;
	template<typename> friend class holder_ptr;
	
	/// The actual object the lifeline is pointing to.
	void* object;
	
	/// The implementation dependent control block.
	char controlBlock[64 - sizeof(void*)];
	
	/**
	 * @brief Initialize the instance of a lifeline.
	 *
	 * @param[in] object the initial instance of the object.
	 * @throw std::runtime_error when the control block fails to initialize.
	 */
	lifeline(void* object);
	
	/**
	 * @brief Mark the object as dead, potentially wait for all other holders to have 
	 * their jobs done.
	 *
	 * This method is invoked by the living pointer to mark this object killed or to be 
	 * killed. After exting this method, the object's pointer must be set to null. And 
	 * this method must guarantee that no lock is held by holder before setting the 
	 * object's pointer to null.
	 *
	 * This method is multithread safe and exclusive with other methods in this class.
	 */
	void waitForDeath() noexcept;
	
	/**
	 * @brief Begin to hold the object, prevent it from death.
	 *
	 * This method is multithread safe and exclusive with other methods in this class.
	 * @return The object that is fetched from the lifeline, which is actually immediately
	 * fetched and fail fast if the object has already held the write lock.
	 */
	void* retain() noexcept;
	
	/**
	 * @brief End to hold the object, making it possible for death.
	 *
	 * If the retain method returns nullptr, you don't need to invoke this method.
	 * This method is multithread safe and exclusive with other methods in this class.
	 */
	void release() noexcept;
public:
	/// Destroy the instance of a lifeline.
	~lifeline() noexcept;
};

/**
 * @brief The living pointer, which is held by the 'this' object.
 *
 * The living pointer can only be correctly initialized with living_ptr<T>(this). And it
 * must be destroyed (living_ptr<T>::kill()) just before executing any code of a 
 * destructor. If not so, it will leave your class in a semi-destructed state and is 
 * dangerous if other objects calls some member function of 'this' object.
 *
 * This pointer can not be copied, and should only move if the containing object is moved.
 * This pointer can also not be shared, you should use liveness pointer if you want to 
 * share 'this' object, which is created from the living pointer.
 */
template<typename objectType>
class living_ptr {
	/// The life object to control the shared lifecycle.
	mutable std::shared_ptr<lifeline> life;
	
	/// Befriending the liveness pointer to make it possible to build one from here.
	template<typename> friend class liveness_ptr;
public:
	/// Construct an instance of a living pointer. Exception might be thrown if the 
	/// lifeline cannot be allocated on stack, or cannot be initialized.
	living_ptr(objectType* thiz): life(new lifeline(thiz)) {}
	
	/// Construct an empty living pointer, which is an eye candy for empty or xvalue objects.
	living_ptr() noexcept: life() {}
	
	/// Destructor of this pointer, simply invokes the death waiting method.
	~living_ptr() noexcept { if(life) life -> waitForDeath(); }
	
	// Remove the copy constructor and assignment operator.
	living_ptr(const living_ptr&) = delete;
	
	/// Move constructor of the living pointer.
	living_ptr(living_ptr&& b) noexcept: life() {
		using std::swap;
		swap(life, b.life);
	}
	
	/// Copy-and-swap assignment of the living pointer.
	living_ptr& operator=(living_ptr b) noexcept {
		using std::swap;
		swap(life, b.life);
		return *this;
	}
};

/**
 * @brief The liveness pointer, which is polymorphic and held by interested objects.
 *
 * The liveness pointer can only be originated from a living pointer. But this class is
 * relatively polymorphic, which means it can be created from any living pointer that 
 * is subclass of this liveness pointer.
 */
template<typename polymorphicType>
class liveness_ptr {
	/// Still need to reference to the lifeline instance.
	mutable std::shared_ptr<lifeline> life;
	
	/// Befriending the liveness pointer to make it possible to build one from here.
	template<typename> friend class holder_ptr;
public:
	/// Construct from a living pointer that could be statically casted to this class.
	template<typename objectType, typename = 
		decltype(static_cast<polymorphicType&>(std::declval<objectType&>()))>
	explicit liveness_ptr(const living_ptr<objectType>& l): life(l.life) {}

	/// Allow polymorphic casting from another liveness pointer.
	template<typename objectType, typename = 
		decltype(static_cast<polymorphicType&>(std::declval<objectType&>()))>
	explicit liveness_ptr(const liveness_ptr<objectType>& lp): life(lp.life) {}
	
	/// Destroy the liveness pointer.
	~liveness_ptr() noexcept {}
};

/**
 * @brief The holder pointer, which is the actual pointer that is used by users.
 */
template<typename objectType>
class holder_ptr {
	/// Still need to reference to the lifeline instance.
	mutable std::shared_ptr<lifeline> life;
	
	/// Holds the pointer retrieved immediately from the life.
	mutable void* ptr;
public:
	/// Originate one holder pointer from a liveness pointer.
	holder_ptr(const liveness_ptr<objectType>& lp): life(lp.life), ptr(nullptr) {
		if(life) ptr = life -> retain();
	}
	
	/// Destroy the holder when it exits its scope.
	~holder_ptr() noexcept {
		if(life) if(ptr) life -> release();
	}
	
	// Cannot copy, if you want a new holder, please create it from liveness pointer.
	holder_ptr(const holder_ptr&) = delete;
	
	/// Move a holder pointer instance to this holder pointer.
	holder_ptr(holder_ptr&& hp) noexcept {
		using std::swap;
		swap(life, hp.life);
	}
	
	/// Copy-and-swap assignment of the holder pointer.
	holder_ptr& operator=(holder_ptr hp) noexcept {
		using std::swap;
		swap(life, hp.life);
		return *this;
	}
	
	/// Casting the holder pointer to the specified object type.
	operator objectType*() const noexcept {
		return reinterpret_cast<objectType*>(ptr);
	}
	
	/// Casting the holder pointer to the specified object type.
	operator const objectType*() const noexcept {
		return reinterpret_cast<const objectType*>(ptr);
	}
};

} // namespace snailviewer.