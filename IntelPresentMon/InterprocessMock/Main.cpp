#include <iostream>
#include "../Interprocess/source/Interprocess.h"

int main()
{
	std::cout << pmon::ipc::f() << std::endl;
	return 0;
}