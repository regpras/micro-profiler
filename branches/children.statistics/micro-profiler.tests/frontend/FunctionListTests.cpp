#include <frontend/function_list.h>

#include <frontend/symbol_resolver.h>
#include <collector/com_helpers.h>

#include <functional>
#include <list>
#include <utility>
#include <string>
#include <sstream>
#include <cmath>
#include <memory>
#include <locale>
#include <algorithm>

namespace std 
{
	using namespace tr1;
	using namespace tr1::placeholders;
}

using namespace std;
using namespace wpl::ui;
using namespace micro_profiler;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace 
		{
			__int64 test_ticks_resolution = 1;

			template <typename T>
			wstring to_string(const T &value)
			{
				wstringstream s;
				s << value;
				return s.str();
			}

			void assert_row(
				const functions_list &fl, 
				size_t row, 
				const wchar_t* name, 
				const wchar_t* times_called, 
				const wchar_t* inclusive_time, 
				const wchar_t* exclusive_time, 
				const wchar_t* avg_inclusive_time, 
				const wchar_t* avg_exclusive_time, 
				const wchar_t* max_reentrance,
				const wchar_t* max_call_time = L"")
			{
				wstring result;
				result.reserve(1000);
				fl.get_text(row, 0, result); // row number
				Assert::IsTrue(result == to_string(row));
				fl.get_text(row, 1, result); // name
				Assert::IsTrue(result == name);
				fl.get_text(row, 2, result); // times called
				Assert::IsTrue(result == times_called);
				fl.get_text(row, 3, result); // exclusive time
				Assert::IsTrue(result == exclusive_time);
				fl.get_text(row, 4, result); // inclusive time
				Assert::IsTrue(result == inclusive_time);
				fl.get_text(row, 5, result); // avg. exclusive time
				Assert::IsTrue(result == avg_exclusive_time);
				fl.get_text(row, 6, result); // avg. inclusive time
				Assert::IsTrue(result == avg_inclusive_time);
				fl.get_text(row, 7, result); // max reentrance
				Assert::IsTrue(result == max_reentrance);
				fl.get_text(row, 8, result); // max reentrance
				Assert::IsTrue(result == max_call_time);
			}

			class i_handler
			{
			public:
				typedef listview::model::index_type index_type;
				typedef list<index_type> counter;

				void bind2(listview::model &to) { _connection = to.invalidated += bind(&i_handler::handle, this, _1); }

				size_t times() const { return _counter.size(); }

				counter::const_iterator begin() const { return _counter.begin(); }
				counter::const_iterator end() const { return _counter.end(); }

				counter::const_reverse_iterator rbegin() {return _counter.rbegin(); }
				counter::const_reverse_iterator rend() {return _counter.rend(); }
			private:
				void handle(index_type count)
				{
					_counter.push_back(count);
				}
		
				wpl::slot_connection _connection;
				counter _counter;
			};

			class sri : public symbol_resolver
			{
			public:
				virtual wstring symbol_name_by_va(const void *address) const
				{
					return to_string(address);
				}
			};

			// convert decimal point to current(default) locale
			wstring dp2cl(const wchar_t *input, wchar_t stub_char = L'.')
			{
				static wchar_t decimal_point = use_facet< numpunct<wchar_t> > (locale("")).decimal_point();

				wstring result = input;
				replace(result.begin(), result.end(), stub_char, decimal_point);
				return result;
			}
		}


		[TestClass]
		public ref class FunctionListTests
		{
		public: 
			[TestMethod]
			void CanCreateEmptyFunctionList()
			{
				// INIT / ACT
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> sp_fl(functions_list::create(test_ticks_resolution, resolver));
				functions_list &fl = *sp_fl;

				// ACT / ASSERT
				Assert::IsTrue(fl.get_count() == 0);
			}


			[TestMethod]
			void FunctionListAcceptsUpdates()
			{
				// INIT
				function_statistics s[] = {
					function_statistics(19, 0, 31, 29),
					function_statistics(10, 3, 7, 5),
				};
				FunctionStatisticsDetailed data[2] = { 0 };
				shared_ptr<symbol_resolver> resolver(new sri);

				copy(make_pair((void *)1123, s[0]), data[0].Statistics);
				copy(make_pair((void *)2234, s[1]), data[1].Statistics);
				
				// ACT
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				fl->update(data, 2);

				// ASSERT
				Assert::IsTrue(fl->get_count() == 2);
			}


			[TestMethod]
			void FunctionListCanBeClearedAndUsedAgain()
			{
				// INIT
				function_statistics s1(19, 0, 31, 29);
				FunctionStatisticsDetailed ms1 = { 0 };
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));

				copy(make_pair((void *)1123, s1), ms1.Statistics);

				// ACT
				i_handler ih;
				ih.bind2(*fl);
				fl->update(&ms1, 1);

				// ASSERT
				Assert::IsTrue(fl->get_count() == 1);
				Assert::IsTrue(ih.times() == 1);
				Assert::IsTrue(*ih.rbegin() == 1); //check what's coming as event arg

				// ACT
				shared_ptr<const listview::trackable> first = fl->track(0); // 2229

				// ASSERT
				Assert::IsTrue(first->index() == 0);

				// ACT
				fl->clear();

				// ASSERT
				Assert::IsTrue(fl->get_count() == 0);
				Assert::IsTrue(ih.times() == 2);
				Assert::IsTrue(*ih.rbegin() == 0); //check what's coming as event arg
				Assert::IsTrue(first->index() == functions_list::npos);

				// ACT
				fl->update(&ms1, 1);

				// ASSERT
				Assert::IsTrue(fl->get_count() == 1);
				Assert::IsTrue(ih.times() == 3);
				Assert::IsTrue(*ih.rbegin() == 1); //check what's coming as event arg
				Assert::IsTrue(first->index() == 0); // kind of side effect
			}


			[TestMethod]
			void FunctionListGetByAddress()
			{
				// INIT
				function_statistics s[] = {
					function_statistics(19, 0, 31, 29),
					function_statistics(10, 3, 7, 5),
					function_statistics(5, 0, 10, 7),
				};
				FunctionStatisticsDetailed data[3] = { 0 };
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				vector<functions_list::index_type> expected;

				copy(make_pair((void *)1123, s[0]), data[0].Statistics);
				copy(make_pair((void *)2234, s[1]), data[1].Statistics);
				copy(make_pair((void *)5555, s[2]), data[2].Statistics);
				
				// ACT
				fl->update(data, 3);

				functions_list::index_type idx1118 = fl->get_index((void *)1118);
				functions_list::index_type idx2229 = fl->get_index((void *)2229);
				functions_list::index_type idx5550 = fl->get_index((void *)5550);

				expected.push_back(idx1118);
				expected.push_back(idx2229);
				expected.push_back(idx5550);
				sort(expected.begin(), expected.end());
				
				// ASSERT
				Assert::IsTrue(fl->get_count() == 3);

				for (size_t i = 0; i < expected.size(); ++i)
					Assert::IsTrue(expected[i] == i);

				Assert::IsTrue(idx1118 != functions_list::npos);
				Assert::IsTrue(idx2229 != functions_list::npos);
				Assert::IsTrue(idx5550 != functions_list::npos);
				Assert::IsTrue(fl->get_index((void *)1234) == functions_list::npos);

				//Check twice. Kind of regularity check.
				Assert::IsTrue(fl->get_index((void *)1118) == idx1118);
				Assert::IsTrue(fl->get_index((void *)2229) == idx2229);
				Assert::IsTrue(fl->get_index((void *)5550) == idx5550);
			}


			[TestMethod]
			void FunctionListCollectsUpdates()
			{
				//TODO: add 2 entries of same function in one burst
				//TODO: possibly trackable on update tests should see that it works with every sorting given.

				// INIT
				function_statistics s[] = {
					function_statistics(19, 0, 31, 29, 3),
					function_statistics(10, 3, 7, 5, 4),
					function_statistics(5, 0, 10, 7, 6),
					function_statistics(15, 1024, 1011, 723, 215),
					function_statistics(1, 0, 4, 4, 4),
				};
				FunctionStatisticsDetailed data1[2] = { 0 };
				FunctionStatisticsDetailed data2[2] = { 0 };
				FunctionStatisticsDetailed data3[1] = { 0 };

				copy(make_pair((void *)1123, s[0]), data1[0].Statistics);
				copy(make_pair((void *)2234, s[1]), data1[1].Statistics);
				copy(make_pair((void *)1123, s[2]), data2[0].Statistics);
				copy(make_pair((void *)5555, s[3]), data2[1].Statistics);
				copy(make_pair((void *)1123, s[4]), data3[0].Statistics);

				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				fl->set_order(2, true); // by times called
				
				i_handler ih;
				ih.bind2(*fl);

				// ACT
				fl->update(data1, 2);
				shared_ptr<const listview::trackable> first = fl->track(0); // 2229
				shared_ptr<const listview::trackable> second = fl->track(1); // 1118

				// ASSERT
				Assert::IsTrue(fl->get_count() == 2);
				Assert::IsTrue(ih.times() == 1);
				Assert::IsTrue(*ih.rbegin() == 2); //check what's coming as event arg
		
				/* name, times_called, inclusive_time, exclusive_time, avg_inclusive_time, avg_exclusive_time, max_reentrance */
				assert_row(*fl, fl->get_index((void *)1118), L"0000045E", L"19", L"31s", L"29s", L"1.63s", L"1.53s", L"0", L"3s");
				assert_row(*fl, fl->get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");

				Assert::IsTrue(first->index() == 0);
				Assert::IsTrue(second->index() == 1);

				// ACT
				fl->update(data2, 2);
				
				// ASSERT
				Assert::IsTrue(fl->get_count() == 3);
				Assert::IsTrue(ih.times() == 2);
				Assert::IsTrue(*ih.rbegin() == 3); //check what's coming as event arg

				assert_row(*fl, fl->get_index((void *)1118), L"0000045E", L"24", L"41s", L"36s", L"1.71s", L"1.5s", L"0", L"6s");
				assert_row(*fl, fl->get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");
				assert_row(*fl, fl->get_index((void *)5550), L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s");

				Assert::IsTrue(first->index() == 0);
				Assert::IsTrue(second->index() == 2); // kind of moved down

				// ACT
				fl->update(data3, 1);
				
				// ASSERT
				Assert::IsTrue(fl->get_count() == 3);
				Assert::IsTrue(ih.times() == 3);
				Assert::IsTrue(*ih.rbegin() == 3); //check what's coming as event arg

				assert_row(*fl, fl->get_index((void *)1118), L"0000045E", L"25", L"45s", L"40s", L"1.8s", L"1.6s", L"0", L"6s");
				assert_row(*fl, fl->get_index((void *)2229), L"000008B5", L"10", L"7s", L"5s", L"700ms", L"500ms", L"3", L"4s");
				assert_row(*fl, fl->get_index((void *)5550), L"000015AE", L"15", L"1011s", L"723s", L"67.4s", L"48.2s", L"1024", L"215s");

				Assert::IsTrue(first->index() == 0);
				Assert::IsTrue(second->index() == 2); // stand still
			}


			[TestMethod]
			void FunctionListTimeFormatter()
			{
				// INIT
				// ~ ns
				function_statistics s1(1, 0, 31, 29, 29);
				function_statistics s1ub(1, 0, 9994, 9994, 9994);

				// >= 1us
				function_statistics s2lb(1, 0, 9996, 9996, 9996);
				function_statistics s2(1, 0, 45340, 36666, 36666);
				function_statistics s2ub(1, 0, 9994000, 9994000, 9994000);

				// >= 1ms
				function_statistics s3lb(1, 0, 9996000, 9996000, 9996000);
				function_statistics s3(1, 0, 33450030, 32333333, 32333333);
				function_statistics s3ub(1, 0, 9994000000, 9994000000, 9994000000);

				// >= 1s
				function_statistics s4lb(1, 0, 9996000000, 9996000000, 9996000000);
				function_statistics s4(1, 0, 65450031030, 23470030000, 23470030000);
				function_statistics s4ub(1, 0, 9994000000000, 9994000000000, 9994000000000);

				// >= 1000s
				function_statistics s5lb(1, 0, 9996000000000, 9996000000000, 9996000000000);
				function_statistics s5(1, 0, 65450031030567, 23470030000987, 23470030000987);
				function_statistics s5ub(1, 0, 99990031030567, 99990030000987, 99990030000987);
				
				// >= 10000s
				function_statistics s6lb(1, 0, 99999031030567, 99999030000987, 99999030000987);
				function_statistics s6(1, 0, 65450031030567000, 23470030000987000, 23470030000987000);

				FunctionStatisticsDetailed data[16] = { 0 };
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(10000000000, resolver)); // 10 * billion for ticks resolution

				copy(make_pair((void *)1123, s1), data[0].Statistics);
				copy(make_pair((void *)2234, s2), data[1].Statistics);
				copy(make_pair((void *)3123, s3), data[2].Statistics);
				copy(make_pair((void *)5555, s4), data[3].Statistics);
				copy(make_pair((void *)4555, s5), data[4].Statistics);
				copy(make_pair((void *)6666, s6), data[5].Statistics);

				copy(make_pair((void *)1995, s1ub), data[6].Statistics);
				copy(make_pair((void *)2005, s2lb), data[7].Statistics);
				copy(make_pair((void *)2995, s2ub), data[8].Statistics);
				copy(make_pair((void *)3005, s3lb), data[9].Statistics);
				copy(make_pair((void *)3995, s3ub), data[10].Statistics);
				copy(make_pair((void *)4005, s4lb), data[11].Statistics);
				copy(make_pair((void *)4995, s4ub), data[12].Statistics);
				copy(make_pair((void *)5005, s5lb), data[13].Statistics);
				copy(make_pair((void *)5995, s5ub), data[14].Statistics);
				copy(make_pair((void *)6005, s6lb), data[15].Statistics);
				
				// ACT
				fl->update(data, sizeof(data) / sizeof(data[0]));

				// ASSERT
				Assert::IsTrue(fl->get_count() == sizeof(data) / sizeof(data[0]));
		
				// columns: name, times called, inclusive time, exclusive time, avg. inclusive time, avg. exclusive time, max reentrance, max call time
				assert_row(*fl, fl->get_index((void *)1118), L"0000045E", L"1", L"3.1ns", L"2.9ns", L"3.1ns", L"2.9ns", L"0", L"2.9ns");
				assert_row(*fl, fl->get_index((void *)2229), L"000008B5", L"1", L"4.53us", L"3.67us", L"4.53us", L"3.67us", L"0", L"3.67us");
				assert_row(*fl, fl->get_index((void *)3118), L"00000C2E", L"1", L"3.35ms", L"3.23ms", L"3.35ms", L"3.23ms", L"0", L"3.23ms");
				assert_row(*fl, fl->get_index((void *)5550), L"000015AE", L"1", L"6.55s", L"2.35s", L"6.55s", L"2.35s", L"0", L"2.35s");
				assert_row(*fl, fl->get_index((void *)4550), L"000011C6", L"1", L"6545s", L"2347s", L"6545s", L"2347s", L"0", L"2347s");
				assert_row(*fl, fl->get_index((void *)6661), L"00001A05", L"1", L"6.55e+006s", L"2.35e+006s", L"6.55e+006s", L"2.35e+006s", L"0", L"2.35e+006s");
				
				// ASSERT (boundary cases)
				assert_row(*fl, fl->get_index((void *)1990), L"000007C6", L"1", L"999ns", L"999ns", L"999ns", L"999ns", L"0", L"999ns");
				assert_row(*fl, fl->get_index((void *)2000), L"000007D0", L"1", L"1us", L"1us", L"1us", L"1us", L"0", L"1us");
				assert_row(*fl, fl->get_index((void *)2990), L"00000BAE", L"1", L"999us", L"999us", L"999us", L"999us", L"0", L"999us");
				assert_row(*fl, fl->get_index((void *)3000), L"00000BB8", L"1", L"1ms", L"1ms", L"1ms", L"1ms", L"0", L"1ms");
				assert_row(*fl, fl->get_index((void *)3990), L"00000F96", L"1", L"999ms", L"999ms", L"999ms", L"999ms", L"0", L"999ms");
				assert_row(*fl, fl->get_index((void *)4000), L"00000FA0", L"1", L"1s", L"1s", L"1s", L"1s", L"0", L"1s");
				assert_row(*fl, fl->get_index((void *)4990), L"0000137E", L"1", L"999s", L"999s", L"999s", L"999s", L"0", L"999s");
				assert_row(*fl, fl->get_index((void *)5000), L"00001388", L"1", L"999.6s", L"999.6s", L"999.6s", L"999.6s", L"0", L"999.6s");
				assert_row(*fl, fl->get_index((void *)5990), L"00001766", L"1", L"9999s", L"9999s", L"9999s", L"9999s", L"0", L"9999s");
				assert_row(*fl, fl->get_index((void *)6000), L"00001770", L"1", L"1e+004s", L"1e+004s", L"1e+004s", L"1e+004s", L"0", L"1e+004s");
			}


			[TestMethod]
			void FunctionListSorting()
			{
				// INIT
				function_statistics s[] = {
					function_statistics(15, 0, 31, 29, 3),
					function_statistics(35, 1, 453, 366, 4),
					function_statistics(2, 2, 33450030, 32333333, 5),
					function_statistics(15233, 3, 65450, 13470, 6),
				};
				FunctionStatisticsDetailed data[4] = { 0 };
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				i_handler ih;

				copy(make_pair((void *)1995, s[0]), data[0].Statistics);
				copy(make_pair((void *)2005, s[1]), data[1].Statistics);
				copy(make_pair((void *)2995, s[2]), data[2].Statistics);
				copy(make_pair((void *)3005, s[3]), data[3].Statistics);

				ih.bind2(*fl);

				const size_t data_size = sizeof(data)/sizeof(data[0]);

				fl->update(data, data_size);

				shared_ptr<const listview::trackable> pt0 = fl->track(fl->get_index((void *)1990));
				shared_ptr<const listview::trackable> pt1 = fl->track(fl->get_index((void *)2000));
				shared_ptr<const listview::trackable> pt2 = fl->track(fl->get_index((void *)2990));
				shared_ptr<const listview::trackable> pt3 = fl->track(fl->get_index((void *)3000));
				
				const listview::trackable &t0 = *pt0;
				const listview::trackable &t1 = *pt1;
				const listview::trackable &t2 = *pt2;
				const listview::trackable &t3 = *pt3;

				// ACT (times called, ascending)
				fl->set_order(2, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 2);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 1, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); // s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); // s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); // s4

				Assert::IsTrue(t0.index() == 1);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 3);

				// ACT (times called, descending)
				fl->set_order(2, false);

				// ASSERT
				Assert::IsTrue(ih.times() == 3);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 2, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 2);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 0);

				// ACT (name, ascending; after times called to see that sorting in asc direction works)
				fl->set_order(1, true);

				// ASSERT
				Assert::IsTrue(ih.times() == 4);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4
				
				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 2);
				Assert::IsTrue(t3.index() == 3);

				// ACT (name, descending)
				fl->set_order(1, false);

				// ASSERT
				Assert::IsTrue(ih.times() == 5);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 1);
				Assert::IsTrue(t3.index() == 0);

				// ACT (exclusive time, ascending)
				fl->set_order(3, true);

				// ASSERT
				Assert::IsTrue(ih.times() == 6);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 2);

				// ACT (exclusive time, descending)
				fl->set_order(3, false);

				// ASSERT
				Assert::IsTrue(ih.times() == 7);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 1);

				// ACT (inclusive time, ascending)
				fl->set_order(4, true);

				// ASSERT
				Assert::IsTrue(ih.times() == 8);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 2);

				// ACT (inclusive time, descending)
				fl->set_order(4, false);

				// ASSERT
				Assert::IsTrue(ih.times() == 9);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 1);
				
				// ACT (avg. exclusive time, ascending)
				fl->set_order(5, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 10);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 1, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 1);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 0);

				// ACT (avg. exclusive time, descending)
				fl->set_order(5, false);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 11);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 2, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 2);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 3);

				// ACT (avg. inclusive time, ascending)
				fl->set_order(6, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 12);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 3, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 1, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 3);
				Assert::IsTrue(t3.index() == 1);

				// ACT (avg. inclusive time, descending)
				fl->set_order(6, false);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 13);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 0, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 2, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 0);
				Assert::IsTrue(t3.index() == 2);

				// ACT (max reentrance, ascending)
				fl->set_order(7, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 14);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 2);
				Assert::IsTrue(t3.index() == 3);

				// ACT (max reentrance, descending)
				fl->set_order(7, false);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 15);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 1);
				Assert::IsTrue(t3.index() == 0);

				// ACT (max call time, ascending)
				fl->set_order(8, true);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 16);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 0, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 1, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 2, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 3, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 0);
				Assert::IsTrue(t1.index() == 1);
				Assert::IsTrue(t2.index() == 2);
				Assert::IsTrue(t3.index() == 3);

				// ACT (max call time, descending)
				fl->set_order(8, false);
				
				// ASSERT
				Assert::IsTrue(ih.times() == 17);
				Assert::IsTrue(*ih.rbegin() == data_size); //check what's coming as event arg

				assert_row(*fl, 3, L"000007C6", L"15", L"31s", L"29s", L"2.07s", L"1.93s", L"0", L"3s"); //s1
				assert_row(*fl, 2, L"000007D0", L"35", L"453s", L"366s", L"12.9s", L"10.5s", L"1", L"4s"); //s2
				assert_row(*fl, 1, L"00000BAE", L"2", L"3.35e+007s", L"3.23e+007s", L"1.67e+007s", L"1.62e+007s", L"2", L"5s"); //s3
				assert_row(*fl, 0, L"00000BB8", L"15233", L"6.55e+004s", L"1.35e+004s", L"4.3s", L"884ms", L"3", L"6s"); //s4

				Assert::IsTrue(t0.index() == 3);
				Assert::IsTrue(t1.index() == 2);
				Assert::IsTrue(t2.index() == 1);
				Assert::IsTrue(t3.index() == 0);
			}

			[TestMethod]
			void FunctionListPrintItsContent()
			{
				// INIT
				function_statistics s[] = {
					function_statistics(15, 0, 31, 29, 2),
					function_statistics(35, 1, 453, 366, 3),
					function_statistics(2, 2, 33450030, 32333333, 4),
				};
				FunctionStatisticsDetailed data[3] = { 0 };
				const size_t data_size = sizeof(data)/sizeof(data[0]);
				shared_ptr<symbol_resolver> resolver(new sri);
				shared_ptr<functions_list> fl(functions_list::create(test_ticks_resolution, resolver));
				wstring result;

				copy(make_pair((void *)1995, s[0]), data[0].Statistics);
				copy(make_pair((void *)2005, s[1]), data[1].Statistics);
				copy(make_pair((void *)2995, s[2]), data[2].Statistics);

				// ACT
				fl->print(result);

				// ASSERT
				Assert::IsTrue(fl->get_count() == 0);
				Assert::IsTrue(result == L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n");

				// ACT
				fl->update(data, data_size);
				fl->set_order(2, true); // by times called
				fl->print(result);

				// ASSERT
				Assert::IsTrue(fl->get_count() == data_size);
				Assert::IsTrue(result == dp2cl(L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										L"00000BAE\t2\t3.23333e+007\t3.345e+007\t1.61667e+007\t1.6725e+007\t2\t4\r\n"
										L"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										L"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"));

				// ACT
				fl->set_order(5, true); // avg. exclusive time
				fl->print(result);

				// ASSERT
				Assert::IsTrue(fl->get_count() == data_size);
				Assert::IsTrue(result == dp2cl(L"Function\tTimes Called\tExclusive Time\tInclusive Time\t"
										L"Average Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n"
										L"000007C6\t15\t29\t31\t1.93333\t2.06667\t0\t2\r\n"
										L"000007D0\t35\t366\t453\t10.4571\t12.9429\t1\t3\r\n"
										L"00000BAE\t2\t3.23333e+007\t3.345e+007\t1.61667e+007\t1.6725e+007\t2\t4\r\n"));
			}
		};
	}
}