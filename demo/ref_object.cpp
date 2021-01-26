
#include "TaskPool.hpp"
#include "RefObject.hpp"

class faketype
{
public:
	faketype()
	{
		SIM_LINFO("faketype() "<<SIM_HEX(this));
	}
	~faketype()
	{
		SIM_LINFO("~faketype " << SIM_HEX(this));
	}
};
sim::RefObject<faketype> temp;
void* run(void* pd)
{
	sim::RefObject<faketype> ref = temp;
	SIM_LINFO("ref count=" << ref.getcount());
	return NULL;
}
int main(int argc, char* argv[])
{
	SIM_LOG_CONFIG(sim::LInfo, NULL, NULL);
	SIM_FUNC_DEBUG();

	sim::RefObject<faketype> ref(new faketype());
	SIM_LINFO("ref count=" << ref.getcount());
	sim::RefObject<faketype> ref1 = ref;
	temp = ref1;
	sim::TaskPool pool(8);
	for (int i = 0; i < 10000; ++i)
		pool.Post(run, NULL, NULL);
	while (ref.getcount() >3)
	{
		SIM_LINFO("ref count=" << ref.getcount());
	}
	getchar();
	return 0;
}

