#include "Json.hpp"

int main(int argc, char *argv[])
{
	sim::JsonObjectPtr ptr=sim::JsonObject::NewObject();

	ptr->ObjectAddString("string", "value");
	ptr->ObjectAddNumber("double", 0.0000001);
	ptr->ObjectAddNumber("int", 1);
	ptr->ObjectAddBoolen("bool", true);
	
	sim::JsonObjectPtr child1 = sim::JsonObject::NewObject();
	ptr->ObjectAddObject("child1", child1);
	child1->ObjectAddString("string", "value");
	child1->ObjectAddNumber("double", 0.0000001);
	child1->ObjectAddNumber("int", 1);
	child1->ObjectAddBoolen("bool", true);

	sim::JsonObjectPtr child2 = sim::JsonObject::NewArray();
	ptr->ObjectAddObject("child2", child2);
	child2->ArrayAddNumber(1);
	child2->ArrayAddNumber(2);
	child2->ArrayAddNumber(2.2);
	child2->ArrayAddNumber(23);

	ptr->SaveFile("test.json", true);

	sim::JsonObject::Free(ptr);
	ptr = sim::JsonObject::ReadFile("test.json");
	return 0;
}


