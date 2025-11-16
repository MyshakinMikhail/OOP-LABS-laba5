#include <gtest/gtest.h>
#include <memory_resource>
#include <vector>
#include <type_traits>
#include "queue_pmr.hpp"

// Тест: memory_resource наследует std::pmr::memory_resource
TEST(MemoryResourceTest, InheritsFromStdPMR)
{
	DynamicVectorMemoryResource mr;
	static_assert(std::is_base_of_v<std::pmr::memory_resource, DynamicVectorMemoryResource>,
				  "DynamicVectorMemoryResource must inherit from std::pmr::memory_resource");
}

// Тест: корректность выделения и освобождения памяти
TEST(MemoryResourceTest, AllocateDeallocateInt)
{
	DynamicVectorMemoryResource mr;

	void *p1 = mr.allocate(sizeof(int), alignof(int));
	ASSERT_NE(p1, nullptr);
	*static_cast<int *>(p1) = 42;

	mr.deallocate(p1, sizeof(int), alignof(int)); // должно пройти без краша

	// Повторное выделение — допустимо (не обязательно то же место)
	void *p2 = mr.allocate(sizeof(int), alignof(int));
	ASSERT_NE(p2, nullptr);
	mr.deallocate(p2, sizeof(int), alignof(int));
}

// Тест: автоматическая очистка при уничтожении
TEST(MemoryResourceTest, CleansUpOnDestruction)
{
	{
		DynamicVectorMemoryResource mr;
		static_cast<void>(mr.allocate(100, 8));
		static_cast<void>(mr.allocate(200, 16));
		// Память не освобождается — деструктор должен подчистить
	}
	SUCCEED();
}
// Тест: очередь использует polymorphic_allocator
TEST(QueueTest, UsesPolymorphicAllocator)
{
	DynamicVectorMemoryResource mr;
	pmr_queue<int> q(&mr);

	// Простая проверка: можно добавлять и извлекать
	q.push(10);
	q.push(20);
	EXPECT_EQ(q.size(), 2);
	EXPECT_EQ(q.front(), 10);
	q.pop();
	EXPECT_EQ(q.front(), 20);
}

// Тест: работа с int (простой тип)
TEST(QueueTest, WorksForInt)
{
	DynamicVectorMemoryResource mr;
	pmr_queue<int> q(&mr);

	for (int i = 1; i <= 5; ++i)
	{
		q.push(i);
	}

	std::vector<int> expected = {1, 2, 3, 4, 5};
	std::vector<int> actual;
	for (const auto &x : q)
	{
		actual.push_back(x);
	}
	EXPECT_EQ(actual, expected);
}

// Тест: работа со сложным типом
struct ComplexData
{
	int id;
	double value;
	std::string name;

	ComplexData(int id = 0, double v = 0.0, const std::string &n = "")
		: id(id), value(v), name(n) {}

	bool operator==(const ComplexData &other) const
	{
		return id == other.id && value == other.value && name == other.name;
	}
};

TEST(QueueTest, WorksForComplexType)
{
	DynamicVectorMemoryResource mr;
	pmr_queue<ComplexData> q(&mr);

	q.push(ComplexData{1, 3.14, "pi"});
	q.push(ComplexData{2, 2.71, "e"});

	auto it = q.begin();
	ASSERT_NE(it, q.end());
	EXPECT_EQ(*it, (ComplexData{1, 3.14, "pi"}));

	++it;
	ASSERT_NE(it, q.end());
	EXPECT_EQ(*it, (ComplexData{2, 2.71, "e"}));

	++it;
	EXPECT_EQ(it, q.end());
}

// Тест: итератор — forward_iterator
TEST(IteratorTest, IsForwardIterator)
{
	using It = QueueIterator<int>;
	static_assert(std::is_same_v<It::iterator_category, std::forward_iterator_tag>,
				  "Iterator must be a forward iterator");

	static_assert(std::is_default_constructible_v<It>);
	static_assert(std::is_copy_constructible_v<It>);
	static_assert(std::is_copy_assignable_v<It>);
	static_assert(std::is_destructible_v<It>);

	// Проверка операций
	DynamicVectorMemoryResource mr;
	pmr_queue<int> q(&mr);
	q.push(100);
	q.push(200);

	auto it1 = q.begin();
	auto it2 = it1;
	EXPECT_EQ(*it1, *it2);
	++it1;
	EXPECT_NE(it1, it2);
	EXPECT_EQ(*it1, 200);
	EXPECT_TRUE(it1 != q.end());
}

// Тест: корректная работа при пустой очереди
TEST(QueueTest, HandlesEmptyQueue)
{
	DynamicVectorMemoryResource mr;
	pmr_queue<int> q(&mr);

	EXPECT_TRUE(q.empty());
	EXPECT_EQ(q.size(), 0);

	// Эти операции должны выбрасывать исключение
	EXPECT_THROW(q.front(), std::runtime_error);
	EXPECT_THROW(q.pop(), std::runtime_error);

	// Итераторы begin == end
	EXPECT_EQ(q.begin(), q.end());
}

// Тест: деструктор очереди корректно освобождает узлы
TEST(QueueTest, QueueDestructorReleasesMemory)
{
	size_t alloc_count = 0;
	{
		DynamicVectorMemoryResource mr;
		pmr_queue<int> q(&mr);
		for (int i = 0; i < 100; ++i)
		{
			q.push(i);
		}
		// Ничего не удаляем вручную — деструктор очереди должен вызвать deallocate
	} // ← здесь всё освобождается
	SUCCEED(); // Если не упало — ок
}