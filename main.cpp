#include <iostream>
#include <memory_resource>
#include "queue_pmr.hpp"

struct Point
{
	int x, y;
	Point(int x = 0, int y = 0) : x(x), y(y) {}
};

std::ostream &operator<<(std::ostream &os, const Point &p)
{
	return os << "(" << p.x << ", " << p.y << ")";
}

int main()
{
	DynamicVectorMemoryResource mr;

	pmr_queue<int> int_queue(&mr);
	int_queue.push(10);
	int_queue.push(20);
	int_queue.push(30);

	std::cout << "int queue: ";
	for (const auto &v : int_queue)
	{
		std::cout << v << " ";
	}
	std::cout << "\n";

	pmr_queue<Point> point_queue(&mr);
	point_queue.push(Point{1, 2});
	point_queue.push(Point{3, 4});
	point_queue.push(Point{5, 6});

	std::cout << "point queue: ";
	for (const auto &p : point_queue)
	{
		std::cout << p << " ";
	}
	std::cout << "\n";

	return 0;
}