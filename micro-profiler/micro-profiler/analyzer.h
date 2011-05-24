//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include "calls_collector.h"
#include "primitives.h"

#include <hash_map>
#include <map>
#include <vector>

namespace micro_profiler
{
	class shadow_stack
	{
		struct call_record_ex;
		typedef stdext::hash_map<void *, int, address_compare> entrance_counter_map;

		__int64 _profiler_latency;
		std::vector<call_record_ex> _stack;
		entrance_counter_map _entrance_counter;

	public:
		shadow_stack(__int64 profiler_latency = 0);

		template <typename ForwardConstIterator, typename OutputMap>
		void update(ForwardConstIterator trace_begin, ForwardConstIterator trace_end, OutputMap &statistics);
	};


	struct shadow_stack::call_record_ex : call_record
	{
		call_record_ex(const call_record &from);
		call_record_ex(const call_record_ex &other);

		__int64 child_time;
	};


	class analyzer : public calls_collector::acceptor
	{
		typedef stdext::hash_map<void *, function_statistics, address_compare> statistics_container;
		typedef std::map<unsigned int /*threadid*/, shadow_stack> stacks_container;

		__int64 _profiler_latency;
		statistics_container _statistics;
		stacks_container _stacks;

	public:
		typedef statistics_container::const_iterator const_iterator;

	public:
		analyzer(__int64 profiler_latency = 0);

		void clear();
		const_iterator begin() const;
		const_iterator end() const;

		virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count);
	};


	// shadow_stack - inline definitions
	inline shadow_stack::shadow_stack(__int64 profiler_latency)
		: _profiler_latency(profiler_latency)
	{	}

	template <typename ForwardConstIterator, typename OutputMap>
	inline void shadow_stack::update(ForwardConstIterator i, ForwardConstIterator end, OutputMap &statistics)
	{
		for (; i != end; ++i)
			if (i->callee)
			{
				_stack.push_back(*i);
				++_entrance_counter[i->callee];
			}
			else
			{
				call_record_ex &current = _stack.back();
				__int64 inclusive_time = i->timestamp - current.timestamp;

				statistics[current.callee]
					.add_call(
						--_entrance_counter[current.callee],
						inclusive_time - _profiler_latency,
						inclusive_time - current.child_time - _profiler_latency);
				_stack.pop_back();
				if (!_stack.empty())
					_stack.back().child_time += inclusive_time + _profiler_latency;
			}
	}


	// shadow_stack::call_record_ex - inline definitions
	inline shadow_stack::call_record_ex::call_record_ex(const call_record &from)
		: call_record(from), child_time(0)
	{	}

	inline shadow_stack::call_record_ex::call_record_ex(const call_record_ex &other)
		: call_record(other), child_time(other.child_time)
	{	}


	// analyzer - inline definitions
	inline analyzer::analyzer(__int64 profiler_latency)
		: _profiler_latency(profiler_latency)
	{	}

	inline void analyzer::clear()
	{	_statistics.clear();	}

	inline analyzer::const_iterator analyzer::begin() const
	{	return _statistics.begin();	}

	inline analyzer::const_iterator analyzer::end() const
	{	return _statistics.end();	}

	inline void analyzer::accept_calls(unsigned int threadid, const call_record *calls, unsigned int count)
	{
		stacks_container::iterator i = _stacks.find(threadid);

		if (i == _stacks.end())
			i = _stacks.insert(std::make_pair(threadid, shadow_stack(_profiler_latency))).first;
		i->second.update(calls, calls + count, _statistics);
	}
}
