#ifndef QUEUE_PMR_HPP
#define QUEUE_PMR_HPP

#include <memory_resource>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <forward_list>
#include <iterator>
#include <type_traits>
#include <algorithm>

class DynamicVectorMemoryResource : public std::pmr::memory_resource
{
private:
	struct BlockInfo
	{
		void *ptr;
		std::size_t size;
		std::size_t alignment;
		bool allocated;
	};

	std::vector<BlockInfo> blocks_;

	auto find_block(void *p)
	{
		return std::find_if(blocks_.begin(), blocks_.end(),
							[p](const BlockInfo &b)
							{ return b.ptr == p; });
	}

protected:
	void *do_allocate(std::size_t bytes, std::size_t alignment) override
	{
		void *ptr = ::operator new(bytes, std::align_val_t(alignment));
		blocks_.push_back({ptr, bytes, alignment, true});
		return ptr;
	}

	void do_deallocate(void *p, std::size_t bytes, std::size_t alignment) override
	{
		auto it = find_block(p);
		if (it != blocks_.end())
		{
			if (it->allocated)
			{
				::operator delete(p, std::align_val_t(it->alignment));
				it->allocated = false;
			}
		}
	}

	bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
	{
		return this == &other;
	}

public:
	~DynamicVectorMemoryResource()
	{
		for (auto &block : blocks_)
		{
			if (block.allocated)
			{
				::operator delete(block.ptr, std::align_val_t(block.alignment));
			}
		}
	}
};

template <typename T>
struct QueueNode
{
	T value;
	QueueNode *next;

	template <typename... Args>
	QueueNode(std::pmr::polymorphic_allocator<T> alloc, Args &&...args)
		: value(std::forward<Args>(args)...), next(nullptr) {}
};

template <typename T>
class QueueIterator
{
	QueueNode<T> *node_;

public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T *;
	using reference = T &;

	explicit QueueIterator(QueueNode<T> *node = nullptr) : node_(node) {}

	T &operator*() const { return node_->value; }
	T *operator->() const { return &(node_->value); }

	QueueIterator &operator++()
	{
		if (node_)
			node_ = node_->next;
		return *this;
	}

	QueueIterator operator++(int)
	{
		QueueIterator tmp = *this;
		++(*this);
		return tmp;
	}

	bool operator==(const QueueIterator &other) const
	{
		return node_ == other.node_;
	}

	bool operator!=(const QueueIterator &other) const
	{
		return !(*this == other);
	}
};

template <typename T>
class pmr_queue
{
private:
	using Node = QueueNode<T>;
	using Alloc = std::pmr::polymorphic_allocator<T>;
	using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;

	Node *head_ = nullptr;
	Node *tail_ = nullptr;
	std::size_t size_ = 0;
	mutable Alloc alloc_;

public:
	using value_type = T;
	using iterator = QueueIterator<T>;

	explicit pmr_queue(std::pmr::memory_resource *mr = std::pmr::get_default_resource())
		: alloc_(mr) {}

	~pmr_queue()
	{
		while (head_ != nullptr)
		{
			Node *tmp = head_;
			head_ = head_->next;
			NodeAlloc na(alloc_);
			std::allocator_traits<NodeAlloc>::destroy(na, tmp);
			std::allocator_traits<NodeAlloc>::deallocate(na, tmp, 1);
		}
	}

	void push(const T &value)
	{
		NodeAlloc na(alloc_);
		Node *newNode = std::allocator_traits<NodeAlloc>::allocate(na, 1);
		std::allocator_traits<NodeAlloc>::construct(na, newNode, alloc_, value);
		if (tail_ == nullptr)
		{
			head_ = tail_ = newNode;
		}
		else
		{
			tail_->next = newNode;
			tail_ = newNode;
		}
		++size_;
	}

	void push(T &&value)
	{
		NodeAlloc na(alloc_);
		Node *newNode = std::allocator_traits<NodeAlloc>::allocate(na, 1);
		std::allocator_traits<NodeAlloc>::construct(na, newNode, alloc_, std::move(value));
		if (tail_ == nullptr)
		{
			head_ = tail_ = newNode;
		}
		else
		{
			tail_->next = newNode;
			tail_ = newNode;
		}
		++size_;
	}

	void pop()
	{
		if (head_ == nullptr)
			throw std::runtime_error("pop from empty queue");
		Node *tmp = head_;
		head_ = head_->next;
		if (head_ == nullptr)
			tail_ = nullptr;
		NodeAlloc na(alloc_);
		std::allocator_traits<NodeAlloc>::destroy(na, tmp);
		std::allocator_traits<NodeAlloc>::deallocate(na, tmp, 1);
		--size_;
	}

	T &front()
	{
		if (head_ == nullptr)
			throw std::runtime_error("front of empty queue");
		return head_->value;
	}

	const T &front() const
	{
		if (head_ == nullptr)
			throw std::runtime_error("front of empty queue");
		return head_->value;
	}

	bool empty() const noexcept
	{
		return head_ == nullptr;
	}

	std::size_t size() const noexcept
	{
		return size_;
	}

	iterator begin() const { return iterator(head_); }
	iterator end() const { return iterator(nullptr); }

	pmr_queue(const pmr_queue &) = delete;
	pmr_queue &operator=(const pmr_queue &) = delete;
	pmr_queue(pmr_queue &&) = delete;
	pmr_queue &operator=(pmr_queue &&) = delete;
};

#endif