//#include "Tree.hpp"
#include <stdio.h>
#include "RbTree.hpp"
//#include "wepoll_rbtree.hpp"
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <string.h>
bool MyTreeTraverseFunc(sim::RbTreeNode<int>* Now, void*pdata)
{
	printf("MyTreeTraverseFunc now %lld\n", Now->Key);
	if(Now->Key<1000)
		return true;
	return false;
}
int main(int argc, char* argv[])
{
	std::ostream &out = std::cout;

	const int size = 1000000;
	clock_t t1 = ::clock();
	srand(::time(NULL));
	/*tree_t t;
	tree_init(&t);
	*/
	sim::RbTree<int>tree1;
	for (int i = 0; i < size; ++i)
	{
		//Ëæ»ú²åÈë
		//int p = rand() ;
		/*tree_node_t *pn = new tree_node_t();
		tree_node_init(pn);
		pn->data = i;
		pn->key = i;
		tree_add(&t, pn, i);*/
		tree1.Add(i, i);
		//if (i % 2 == 0&&i!=0)//Ëæ»úÉ¾³ý
		//	tree1.Del(rand() % i);
	}
	tree1.TraverseTree(MyTreeTraverseFunc, NULL, sim::TraverseTypeDLR);
	clock_t t2 = ::clock();
	printf("add %d use %ld ms\n", size, t2 - t1);
	
	//Ëæ»ú¶ÁÈ¡
	for (int i = 0; i < size; ++i)
	{
		int d=-1;
		int p = rand()% size;
		tree1.Find(p, &d);
		/*tree_node_t*nn=tree_find(&t, p);
		if (NULL == nn||nn->data!=p)
		{
			printf("find %d fail\n", p);
		}*/
	}
	clock_t t3 = ::clock();
	printf("find %d use %ld ms\n", size, t3 - t2);
	/*tree1.Clear();
	clock_t t4 = ::clock();
	printf("clear use %ld ms\n", t4 - t3);*/

	//Ëæ»úÉ¾³ý
	for (int i = 0; i < size-3; ++i)
	{
		int d = -1;
		int p = i;
		tree1.Del(p);
		/*tree_node_t*nn=tree_find(&t, p);
		if (NULL == nn||nn->data!=p)
		{
			printf("find %d fail\n", p);
		}*/
	}
	tree1.Add(0, 0);
	tree1.Add(1, 1);
	clock_t t4 = ::clock();
	printf("del %d use %ld ms\n", size, t4 - t2);
	getchar();
	return 0;
}