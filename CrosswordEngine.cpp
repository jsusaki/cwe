#include "CrosswordEngine.h"


int main()
{
	cw::Engine engine;
	engine.Init("CrosswordEngine/grid_test_norm.txt", "CrosswordEngine/library.txt");
	engine.Search();
	
	return 0;
}