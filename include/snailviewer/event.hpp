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
 * @file snailviewer/event.hpp
 * @author Haoran Luo
 * @brief Event based implicit invocation definitions.
 *
 * The implicit invocation is the facility that objects passes message
 * via events. The event is fired when some program status altered and
 * those who are interested in the specified events could capture and
 * process on the events.
 *
 * There're three concepts related to the event:
 * - Event: which type of event should be propagated on the system.
 * - EventBus: how will the event be captured and broadcasted.
 * - Handlers: how does objects react corresponding to the events.
 *
 * There's no restriction on whether an event must be broadcasted at 
 * the moment is raised, or at which thread will the event be broadcasted,
 * or even will an event happens twice be broadcasted twice.
 *
 * To support dynamic events which might be defined in loadable modules,
 * the events are identified by event unique id. All events classes must
 * have their own eventType::id() method defined statically.
 */
#include "snailviewer/uid.hpp"
#include <memory>
#include <type_traits>

namespace snailviewer {

/**
 * @brief Ensures that an event id is defined for some kind of event,
 * and wraps some convenient interfaces.
 */
template<typename eventType> struct eventConcept {
	static_assert(std::is_same<decltype(eventType::id()), uid>::value,
		"The event must return an unique id identifying itselves.");
	static uid id() noexcept { return eventType::id(); }
};

template<typename> struct eventHandler;

/**
 * @brief Defines the event bus which manages the broadcasting of events.
 *
 * The event bus must live longer than any subscribers. It defines how
 * at which moment can an event be broadcasted and which way of 
 * broadcasting is best.
 */
class eventBus {
	/// Only the event handler is permitted to invoke the subscribe
	/// and unsubscribe method.
	template<typename> friend class eventHandler;

	/**
	 * Since the lifecycle and broadcasting timing of an fired event
	 * is not defined for concrete type of event bus, we usually have
	 * to copy out the fired event and hold it here.
	 */
	class eventHolder {
		/// The inheriting class defines how to destroying events.
		virtual ~eventHolder() = 0;

		/// The inheriting class provides pointer to event.
		virtual const void* get() const = 0;
	};

	/**
	 * @brief Register that an event handler will listen on some type 
	 * of events.
	 *
	 * Whether exceptions should be thrown or what sort of exceptions 
	 * will be thrown are completely depends on the underlying event 
	 * bus implementation.
	 * 
	 * @param[in] id the unique id identifying the event.
	 * @param[in] h the instance of the event handler.
	 */
	virtual void subscribe(uid id, eventHandler<void*>& h) = 0;

	/**
	 * @brief Unregister the event handler from listening on specified 
	 * type of event.
	 *
	 * Whether exceptions should be thrown or what sort of exceptions 
	 * will be thrown are completely depends on the underlying event 
	 * bus implementation.
	 *
	 * @param[in] id the unique id identifying the event.
	 * @param[in] h the instance of the event handler.
	 */
	virtual void unsubscribe(uid id, eventHandler<void*>& h) = 0;

	/**
	 * @brief Broadcast some events to targeted handler.
	 *
	 * @param[in] id the unique id identifying the event.
	 * @param[in] evh the holder holding the copied event.
	 */
	virtual void broadcast(uid id, std::unique_ptr<eventHolder> evh) = 0;
public:
	/// The virtual destructor for the event bus abstract class.
	virtual ~eventBus() {}

	/// Broadcast a strongly typed event via the event bus.
	template<typename eventType> void broadcast(eventType&& event) {
		struct typedEventHolder : public eventHolder {
			/// The event that is held inside.
			eventType event;

			/// Constructor of the typed event holder.
			typedEventHolder(eventType&& event):
				event(std::forward(event)) {}

			/// Destructor of the typed event holder.
			virtual ~typedEventHolder() {}

			/// Retrieve the concrete pointer to the event.
			virtual const void* get() const { return &event; }
		};
		broadcast(eventConcept<eventType>::id(), std::unique_ptr<typedEventHolder>(
			new typedEventHolder(std::forward<eventType>(event))));
	}
};

/**
 * @brief Defines the untyped event handler.
 *
 * It just provides eye candies for invoking handle interfaces.
 */
template<> struct eventHandler<void*> {
	/// Provides the virtual destructor for an event handler.
	virtual ~eventHandler() {}

	/// Provides the pure virtual interface for broadcasting events.
	virtual void handle(const void* evptr) = 0;
};

/**
 * @brief Defines the general and strongly typed event handler.
 *
 * An event handler can ONLY be bind to one event bus, and should never alter 
 * through out their life cycle.
 */
template<typename eventType> class eventHandler : public eventHandler<void*> {
	/// The event bus which it is registered at the time it is created.
	eventBus& ebus;
public:
	/// Construct the event handler and register it self to the event bus.
	eventHandler(eventBus& ebus): ebus(ebus) {
		ebus.subscribe(eventConcept<eventType>::id(), this);
	}

	/// Unsubscribe the event handler from the event bus.
	virtual ~eventHandler() {
		ebus.unsubscribe(eventConcept<eventType>::id(), this);
	}

	/// The interface waiting to be implemented by the event handler.
	virtual void handle(const eventType& event) = 0;

	/// The interface implementing its parent interfaces.
	virtual void handle(const void* evptr) override {
		handle(*reinterpret_cast<const eventType*>(evptr));
	}
};

} // namespace snailviewer.
