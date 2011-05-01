#include "Helpers.h"

#include <analyzer.h>

#include <map>

using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		[TestClass]
		public ref class AnalyzerTests
		{
		public: 
			[TestMethod]
			void NewAnalyzerHasNoFunctionRecords()
			{
				// INIT / ACT
				analyzer a;

				// ACT / ASSERT
				Assert::IsTrue(a.begin() == a.end());
			}


			[TestMethod]
			void EnteringNewFunctionDoesNotCreateARecord()
			{
				// INIT
				analyzer a;
				calls_collector::acceptor &as_acceptor(a);
				call_record trace[] = {
					{	(void *)1234, 12300	},
					{	(void *)2234, 12305	},
				};

				// ACT
				as_acceptor.accept_calls(1, trace, array_size(trace));
				as_acceptor.accept_calls(2, trace, array_size(trace));

				// ASSERT
				Assert::IsTrue(a.begin() == a.end());
			}


			[TestMethod]
			void EvaluateSeveralFunctionDurations()
			{
				// INIT
				analyzer a;
				call_record trace[] = {
					{	(void *)1234, 12300	},
					{	(void *)0, 12305	},
					{	(void *)2234, 12310	},
					{	(void *)0, 12317	},
					{	(void *)2234, 12320	},
					{	(void *)12234, 12322	},
					{	(void *)0, 12325	},
					{	(void *)0, 12327	},
				};

				// ACT
				a.accept_calls(1, trace, array_size(trace));

				// ASSERT
				map<void *, function_statistics> m(a.begin(), a.end());	// use map to ensure proper sorting

				Assert::IsTrue(3 == m.size());

				map<void *, function_statistics>::const_iterator i1(m.begin()), i2(m.begin()), i3(m.begin());

				++i2, ++++i3;

				Assert::IsTrue(1 == i1->second.times_called);
				Assert::IsTrue(5 == i1->second.inclusive_time);
				Assert::IsTrue(5 == i1->second.exclusive_time);

				Assert::IsTrue(2 == i2->second.times_called);
				Assert::IsTrue(14 == i2->second.inclusive_time);
				Assert::IsTrue(11 == i2->second.exclusive_time);

				Assert::IsTrue(1 == i3->second.times_called);
				Assert::IsTrue(3 == i3->second.inclusive_time);
				Assert::IsTrue(3 == i3->second.exclusive_time);
			}


			[TestMethod]
			void ProfilerLatencyIsTakenIntoAccount()
			{
				// INIT
				analyzer a(1);
				call_record trace[] = {
					{	(void *)1234, 12300	},
					{	(void *)0, 12305	},
					{	(void *)2234, 12310	},
					{	(void *)0, 12317	},
					{	(void *)2234, 12320	},
					{	(void *)12234, 12322	},
					{	(void *)0, 12325	},
					{	(void *)0, 12327	},
				};

				// ACT
				a.accept_calls(1, trace, array_size(trace));

				// ASSERT
				map<void *, function_statistics> m(a.begin(), a.end());	// use map to ensure proper sorting

				Assert::IsTrue(3 == m.size());

				map<void *, function_statistics>::const_iterator i1(m.begin()), i2(m.begin()), i3(m.begin());

				++i2, ++++i3;

				Assert::IsTrue(1 == i1->second.times_called);
				Assert::IsTrue(4 == i1->second.inclusive_time);
				Assert::IsTrue(4 == i1->second.exclusive_time);

				Assert::IsTrue(2 == i2->second.times_called);
				Assert::IsTrue(12 == i2->second.inclusive_time);
				Assert::IsTrue(8 == i2->second.exclusive_time);

				Assert::IsTrue(1 == i3->second.times_called);
				Assert::IsTrue(2 == i3->second.inclusive_time);
				Assert::IsTrue(2 == i3->second.exclusive_time);
			}


			[TestMethod]
			void DifferentShadowStacksAreMaintainedForEachThread()
			{
				// INIT
				analyzer a;
				map<void *, function_statistics> m;
				call_record trace1[] = {	{	(void *)1234, 12300	},	};
				call_record trace2[] = {	{	(void *)1234, 12313	},	};
				call_record trace3[] = {	{	(void *)0, 12307	},	};
				call_record trace4[] = {
					{	(void *)0, 12319	},
					{	(void *)1234, 12323	},
				};

				// ACT
				a.accept_calls(1, trace1, array_size(trace1));
				a.accept_calls(2, trace2, array_size(trace2));

				// ASSERT
				Assert::IsTrue(a.begin() == a.end());

				// ACT
				a.accept_calls(1, trace3, array_size(trace3));

				// ASSERT
				Assert::IsTrue(1 == distance(a.begin(), a.end()));
				Assert::IsTrue((void *)1234 == a.begin()->first);
				Assert::IsTrue(1 == a.begin()->second.times_called);
				Assert::IsTrue(7 == a.begin()->second.inclusive_time);
				Assert::IsTrue(7 == a.begin()->second.exclusive_time);

				// ACT
				a.accept_calls(2, trace4, array_size(trace4));

				// ASSERT
				Assert::IsTrue(1 == distance(a.begin(), a.end()));
				Assert::IsTrue((void *)1234 == a.begin()->first);
				Assert::IsTrue(2 == a.begin()->second.times_called);
				Assert::IsTrue(13 == a.begin()->second.inclusive_time);
				Assert::IsTrue(13 == a.begin()->second.exclusive_time);
			}


			[TestMethod]
			void ClearingRemovesPreviousStatButLeavesStackStates()
			{
				// INIT
				analyzer a;
				map<void *, function_statistics> m;
				call_record trace1[] = {
					{	(void *)1234, 12319	},
					{	(void *)0, 12323	},
					{	(void *)2234, 12324	},
					{	(void *)0, 12326},
					{	(void *)2234, 12330	},
				};
				call_record trace2[] = {	{	(void *)0, 12350	},	};

				a.accept_calls(2, trace1, array_size(trace1));

				// ACT
				a.clear();
				a.accept_calls(2, trace2, array_size(trace2));

				// ASSERT
				Assert::IsTrue(1 == distance(a.begin(), a.end()));
				Assert::IsTrue((void *)2234 == a.begin()->first);
				Assert::IsTrue(1 == a.begin()->second.times_called);
				Assert::IsTrue(20 == a.begin()->second.inclusive_time);
				Assert::IsTrue(20 == a.begin()->second.exclusive_time);
			}
		};
	}
}
