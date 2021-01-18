/*
	红黑树
*/

#ifndef SIM_TREE_HPP_
#define SIM_TREE_HPP_

#include <stdlib.h>
#include <string.h>

namespace sim
{
	//键值类型
	typedef unsigned long long TreeKey;

	//遍历方法
	enum TraverseType
	{
		TraverseTypeDLR,//前序
		TraverseTypeLDR,//中序
		TraverseTypeLRD,//后序
	};

	template<typename T>
	struct TreeNode
	{
		TreeKey Key;
		T Data;
		TreeNode* pParent;
		TreeNode* pLeft;
		TreeNode* pRight;
		bool Red;
	};

	//内存分配函数
	typedef void* (*TreeMalloc)(size_t size);
	typedef void(*TreeFree)(void*);

	template<typename T>
	class Tree
	{
	public:
		Tree();

		virtual ~Tree();

		virtual bool isEmpty();

		bool Add(TreeKey key, T data, bool cover = true);

		bool Del(TreeKey key);

		bool Find(TreeKey key, T *data);

		bool Clear();

		virtual bool SetAlloc(TreeMalloc m, TreeFree f);

		
		typedef bool(*TreeTraverseFunc)(TreeNode<T>* Now, void*pdata);
		
		bool TraverseTree(TreeTraverseFunc func, void*pdata, 
			TraverseType type = TraverseTypeLDR);
	private:
		//遍历函数
		static bool TreeTraverseAndDelete(TreeNode<T>* Now, void*pdata);
	private:
		TreeNode<T>* NewNode(TreeKey key, const T& t);

		bool FreeNode(TreeNode<T>* pnode);
		
		static bool Traverse(TreeNode<T>* proot,
			TreeTraverseFunc func, void*pdata, TraverseType type = TraverseTypeLDR);

		//左旋转 对x进行左旋，意味着"将x变成一个左节点"。
		bool LRotate(TreeNode<T>* p);

		//右旋转
		bool RRotate(TreeNode<T>* proot);

		//查找节点 失败返回 NULL
		TreeNode<T>* FindNode(TreeKey key);

		//插入调整 INSERT-FIXUP
		bool InsertFixUp(TreeNode<T>* p);
	private:
		//根节点
		TreeNode<T>* pRoot;
	private:
		TreeMalloc qMalloc;
		TreeFree qFree;
	};



	/********************************实现*************************************/
	template<typename T>
	inline Tree<T>::Tree()
		:pRoot(NULL)
		, qMalloc(::malloc)
		, qFree(::free)
	{

	}

	template<typename T>
	inline Tree<T>::~Tree()
	{
		//释放
		Clear();
	}

	template<typename T>
	inline bool Tree<T>::isEmpty()
	{
		return NULL == pRoot;
	}

	template<typename T>
	inline bool Tree<T>::Add(TreeKey key, T data, bool cover)
	{
		/*
		RB-INSERT(T, z)
		y ← nil[T]                        // 新建节点“y”，将y设为空节点。
		x ← root[T]                       // 设“红黑树T”的根节点为“x”
		while x ≠ nil[T]                  // 找出要插入的节点“z”在二叉树T中的位置“y”
		do
				y ← x
				if key[z] < key[x]
				then
					x ← left[x]
				else
					x ← right[x]
		p[z] ← y                          // 设置 “z的父亲” 为 “y”
		if y = nil[T]
		then
			root[T] ← z               // 情况1：若y是空节点，则将z设为根
		else if key[z] < key[y]
		then
			left[y] ← z       // 情况2：若“z所包含的值” < “y所包含的值”，则将z设为“y的左孩子”
		else
			right[y] ← z      // 情况3：(“z所包含的值” >= “y所包含的值”)将z设为“y的右孩子”
		left[z] ← nil[T]                  // z的左孩子设为空
		right[z] ← nil[T]                 // z的右孩子设为空。至此，已经完成将“节点z插入到二叉树”中了。
		color[z] ← RED                    // 将z着色为“红色”
		RB-INSERT-FIXUP(T, z)             // 通过RB-INSERT-FIXUP对红黑树的节点进行颜色修改以及旋转，让树T仍然是一颗红黑树
		*/
		//加到根节点
		if (NULL == pRoot)
		{
			pRoot = NewNode(key, data);
			if (NULL == pRoot)
				return false;//内存错误
			pRoot->Red = false;//染成黑色
			return true;
		}

		//查找插入点
		TreeNode<T>* pn = pRoot;
		//新增的点
		TreeNode<T>* add_node = NULL;
		int add_status = 0;//0异常 1 左边 2 右边
		while (pn)
		{
			//找到了已存在的点
			if (pn->Key == key)
			{
				//不覆盖报错
				if (!cover)
					return false;
				//覆盖
				pn->Data = data;
				return true;//不需要调整
			}
			else if (key < pn->Key)
			{
				//左边
				if (pn->pLeft)
				{
					pn = pn->pLeft;
				}
				else
				{
					add_status = 1;
					break;

				}
			}
			else if (key > pn->Key)
			{
				//右边
				if (pn->pRight)
				{
					pn = pn->pRight;
				}
				else
				{
					add_status = 2;
					break;
				}
			}
		}

		if (add_status == 0)
			return false;
		//左边插入
		add_node = NewNode(key, data);
		if (NULL == add_node)
			return false;//内存错误
		if (add_status == 1)
			pn->pLeft = add_node;
		else if (add_status == 2)
			pn->pRight = add_node;
		add_node->pParent = pn;
		return InsertFixUp(add_node);
	}

	template<typename T>
	inline bool Tree<T>::Del(TreeKey key)
	{
		////端节点
		//if (NULL == pnode->pLeft&& NULL == pnode->pRight)
		//{
		//	TreeNode<T>*pp = pnode->pParent;
		//	if (pp)
		//	{
		//		if (pp->pLeft == pnode)
		//			pp->pLeft = NULL;
		//		else if (pp->pRight == pnode)
		//			pp->pRight = NULL;
		//	}
		//	return FreeNode(pnode);
		//}

		TreeNode<T>* node = FindNode(key);
		if (NULL == node)
		{
			return false;
		}
		TreeNode<T>* parent = node->pParent;
		TreeNode<T>* left = node->pLeft;
		TreeNode<T>* right = node->pRight;
		TreeNode<T>* next=NULL;
		bool red=false;

		if (!left) {
			next = right;
		}
		else if (!right) {
			next = left;
		}
		else {
			next = right;
			while (next->pLeft)
				next = next->pLeft;
		}

		if (parent) {
			if (parent->pLeft == node)
				parent->pLeft = next;
			else
				parent->pRight = next;
		}
		else {
			pRoot = next;
		}

		if (left && right) {
			red = next->Red;
			next->Red = node->Red;
			next->pLeft = left;
			left->pParent = next;
			if (next != right) {
				parent = next->pParent;
				next->pParent = node->pParent;
				FreeNode(node);
				node = next->pRight;
				parent->pLeft = node;
				next->pRight = right;
				right->pParent = next;
			}
			else {
				next->pParent = parent;
				parent = next;
				FreeNode(node);
				node = next->pRight;
			}
		}
		else {
			red = node->Red;
			FreeNode(node);
			node = next;
		}

		if (node)
			node->pParent = parent;
		if (red)
			return true;
		if (node && node->Red) {
			node->Red = false;
			return true;
		}

		do {
			if (node == pRoot)
				break;
			//TREE__REBALANCE_AFTER_REMOVE(cis, trans)
			if (node == parent->pLeft) {
				//TREE__REBALANCE_AFTER_REMOVE(left, right)
				TreeNode<T>* sibling = parent->pRight;

				if (sibling&&sibling->Red) {

					sibling->Red = false;
					parent->Red = true;
					LRotate(parent);
					sibling = parent->pRight;
				}
				if ((sibling&&sibling->pLeft && sibling->pLeft->Red) ||
					(sibling&&sibling->pRight && sibling->pRight->Red))
				{
					if (!sibling->pRight || !sibling->pRight->Red) 
					{
						sibling->pLeft->Red = false;
						sibling->Red = true;
						RRotate(sibling);
						sibling = parent->pRight;
					}
					sibling->Red = parent->pRight;
					parent->Red = sibling->pRight->Red = false;
					LRotate( parent);
					node = pRoot;
					break;
				}
				if(sibling)
					sibling->Red = true;
			}
			else {
				//TREE__REBALANCE_AFTER_REMOVE(right, left)
				TreeNode<T>* sibling = parent->pLeft;

				if (sibling&&sibling->Red) {

					sibling->Red = false;
					parent->Red = true;
					RRotate(parent);
					sibling = parent->pLeft;
				}
				if ((sibling&&sibling->pLeft && sibling->pLeft->Red) ||
					(sibling&&sibling->pRight && sibling->pRight->Red))
				{
					if (!sibling->pLeft || !sibling->pLeft->Red) 
					{
						sibling->pRight->Red = false;
						sibling->Red = true;
						LRotate(sibling);
						sibling = parent->pLeft;
					}
					sibling->Red = parent->Red;
					parent->Red = sibling->pLeft->Red = false;
					RRotate( parent);
					node = pRoot;
					break;
				}
				if(sibling)
					sibling->Red = true;
			}
			node = parent;
			parent = parent->pParent;
		} while (!node->Red);

		if (node)
			node->Red = false;
		
		return false;
	}

	template<typename T>
	inline bool Tree<T>::Find(TreeKey key, T * data)
	{
		if (NULL == data)
			return false;

		TreeNode<T>* pn = FindNode(key);
		if (NULL == pn)
			return false;
		*data = pn->Data;
		return true;
	}

	template<typename T>
	inline bool Tree<T>::Clear()
	{

#if _DEBUG
		unsigned long long num = 0;
#endif
		TreeNode<T>*pn = pRoot;
		pRoot = NULL;
		//return Traverse(temp, TreeTraverseAndDelete, this);
		while (pn)
		{
			if (pn->pLeft)
			{
				pn = pn->pLeft;
			}
			else if (pn->pRight)
			{
				pn = pn->pRight;
			}
			else
			{
				//没有子节点了
				TreeNode<T>*t = pn;
				pn = pn->pParent;
				if (pn)
				{
					if (pn->pRight == t)
						pn->pRight = NULL;//已经删除了
					else
						pn->pLeft = NULL;//已经删除了
				}
				FreeNode(t);
#if _DEBUG
				++num;
#endif
			}
		}
#if _DEBUG
		printf("clear %lld node\n", num);
#endif
		return true;
	}

	template<typename T>
	inline bool Tree<T>::SetAlloc(TreeMalloc m, TreeFree f)
	{
		if (m && f)
		{
			qMalloc = m;
			qFree = f;
		}
		return false;
	}

	template<typename T>
	inline bool Tree<T>::TraverseTree(TreeTraverseFunc func, void * pdata, TraverseType type)
	{
		return Traverse(pRoot,func,pdata,type);
	}

	template<typename T>
	inline bool Tree<T>::TreeTraverseAndDelete(TreeNode<T>* Now, void * pdata)
	{
		Tree<T>*p = (Tree<T>*)pdata;
		if (NULL == p)
			return false;
		//printf("free %lld\n", Now->Key);
		return p->FreeNode(Now);
	}

	template<typename T>
	inline TreeNode<T>* Tree<T>::NewNode(TreeKey key, const T & t)
	{
		if (NULL == qMalloc)
		{
			return NULL;
		}
		//申请一个新节点
		TreeNode<T>* newnode = (TreeNode<T>*)qMalloc(sizeof(TreeNode<T>));
		if (NULL == newnode)
			return NULL;
		//初始化
		::memset(newnode, 0, sizeof(TreeNode<T>));
		newnode->Data = t;
		newnode->Key = key;
		newnode->Red = true;//新节点是红色的
		return newnode;
	}

	template<typename T>
	inline bool Tree<T>::FreeNode(TreeNode<T>* pnode)
	{
		if (NULL == qFree && NULL == pnode)
		{
			return false;
		}
		qFree(pnode);
		return true;
	}

	template<typename T>
	inline bool Tree<T>::Traverse(TreeNode<T>* proot, TreeTraverseFunc func, void * pdata, TraverseType type)
	{
		if (NULL == func)
			return false;

		if (proot)
		{
			if (TraverseTypeLDR == type)//zhong
			{
				bool ret = Traverse(proot->pLeft, func, pdata, type);
				if (!ret)
					return ret;
				TreeNode<T>* r = proot->pRight;
				ret = func(proot, pdata);
				if (!ret)
					return ret;
				ret = Traverse(r, func, pdata, type);
				return ret;
			}
			else if (TraverseTypeDLR == type)//前
			{
				TreeNode<T>* r = proot->pRight;
				TreeNode<T>* l = proot->pLeft;
				bool ret = func(proot, pdata);
				if (!ret)
					return ret;
				ret = Traverse(l, func, pdata, type);
				if (!ret)
					return ret;
				ret = Traverse(r, func, pdata, type);
				return ret;
			}
			else if (TraverseTypeLRD == type)//后序
			{
				TreeNode<T>* r = proot->pRight;
				TreeNode<T>* l = proot->pLeft;
				bool ret =  Traverse(l, func, pdata, type);
				if (!ret)
					return ret;
				ret =  Traverse(r, func, pdata, type);
				if (!ret)
					return ret;
				ret = func(proot, pdata);
				return ret;
			}
			return false;
			//TreeNode<T>* pn = pRoot;
			//bool check_left=true;
			//while (true)
			//{
			//	//最左
			//	while (check_left&&pn->pLeft)
			//	{
			//		pn = pn->pLeft;
			//	}
			//	//有右节点
			//	if (pn->pRight)
			//	{
			//		pn = pn->pRight;
			//		check_left = true;
			//	}
			//	else
			//	{
			//		TreeNode<T>* temp = pn;
			//		TreeNode<T>* parent = pn->pParent;
			//		//最左子节点
			//		bool ret= func(temp, pdata);
			//		if (false == ret)
			//			return false;
			//		//根节点最后一个输出
			//		if (proot == pn)
			//			break;
			//		//导入父节点
			//		pn= parent;
			//		check_left = false;
			//	}
			//}

		}
		return true;
	}

	template<typename T>
	inline bool Tree<T>::LRotate(TreeNode<T>* p)
	{
		/*
		https://www.cnblogs.com/skywang12345/p/3245399.html
		LEFT-ROTATE(T, x)
			y ← right[x]            // 前提：这里假设x的右孩子为y。下面开始正式操作
			right[x] ← left[y]      // 将 “y的左孩子” 设为 “x的右孩子”，即 将β设为x的右孩子
			p[left[y]] ← x          // 将 “x” 设为 “y的左孩子的父亲”，即 将β的父亲设为x
			p[y] ← p[x]             // 将 “x的父亲” 设为 “y的父亲”

			if p[x] = nil[T]
			then
				root[T] ← y                 // 情况1：如果 “x的父亲” 是空节点，则将y设为根节点
			else if x = left[p[x]]
			then
				left[p[x]] ← y    // 情况2：如果 x是它父节点的左孩子，则将y设为“x的父节点的左孩子”
			else
				right[p[x]] ← y   // 情况3：(x是它父节点的右孩子) 将y设为“x的父节点的右孩子”

			left[y] ← x             // 将 “x” 设为 “y的左孩子”
			p[x] ← y                // 将 “x的父节点” 设为 “y”
		*/
		if (NULL == p)
			return false;
		//右孩子
		TreeNode<T>* r = p->pRight;
		TreeNode<T>* pp = p->pParent;

		//父节点
		if (NULL == r)
			return true;

		TreeNode<T>* rl = r->pLeft;

		//r作为p的父节点
		if (NULL == pp)
		{
			pRoot = r;
		}
		else if (pp->pLeft == p)
		{
			pp->pLeft = r;
		}
		else
		{
			pp->pRight = r;
		}
		r->pParent = pp;
		p->pParent = r;
		//p作为r的左节点
		r->pLeft = p;

		//r的左节点补全p的右节点
		p->pRight = rl;
		if (rl)
			rl->pParent = p;
		return true;
	}

	template<typename T>
	inline bool Tree<T>::RRotate(TreeNode<T>* p)
	{
		/*
		https://www.cnblogs.com/skywang12345/p/3245399.html
		RIGHT-ROTATE(T, y)
		x ← left[y]             // 前提：这里假设y的左孩子为x。下面开始正式操作
		left[y] ← right[x]      // 将 “x的右孩子” 设为 “y的左孩子”，即 将β设为y的左孩子
		p[right[x]] ← y         // 将 “y” 设为 “x的右孩子的父亲”，即 将β的父亲设为y
		p[x] ← p[y]             // 将 “y的父亲” 设为 “x的父亲”
		if p[y] = nil[T]
		then
			root[T] ← x                 // 情况1：如果 “y的父亲” 是空节点，则将x设为根节点
		else if y = right[p[y]]
		then
			right[p[y]] ← x   // 情况2：如果 y是它父节点的右孩子，则将x设为“y的父节点的左孩子”
		else
			left[p[y]] ← x    // 情况3：(y是它父节点的左孩子) 将x设为“y的父节点的左孩子”
		right[x] ← y            // 将 “y” 设为 “x的右孩子”
		p[y] ← x                // 将 “y的父节点” 设为 “x”
		*/
		if (NULL == p)
			return false;
		//右孩子
		TreeNode<T>* l = p->pLeft;
		TreeNode<T>* pp = p->pParent;

		//父节点
		if (NULL == l)
			return true;

		TreeNode<T>* lr = l->pRight;

		//r作为p的父节点
		if (NULL == pp)
		{
			pRoot = l;
		}
		else if (pp->pLeft == p)
		{
			pp->pLeft = l;
		}
		else
		{
			pp->pRight = l;
		}
		l->pParent = pp;
		p->pParent = l;
		//p作为l的右节点
		l->pRight = p;

		//l的右节点补全p的左节点
		p->pLeft = lr;
		if (lr)
			lr->pParent = p;
		return true;
	}

	template<typename T>
	inline TreeNode<T>* Tree<T>::FindNode(TreeKey key)
	{
#if _DEBUG
		unsigned long long num = 0;//查找的长度
#endif
		TreeNode<T>* pn = pRoot;
		while (pn)
		{
#if _DEBUG
			//if(!pn->Red)
				++num;//查找的长度
#endif
			//找到了
			if (pn->Key == key)
			{
#if _DEBUG&&0
				printf("find key %lld long %d\n", key, num);
#endif
				return pn;
			}
			else if (key < pn->Key)
			{
				//左边
				pn = pn->pLeft;
			}
			else if (key > pn->Key)
			{
				//右边
				pn = pn->pRight;
			}
		}
		return NULL;
	}

	template<typename T>
	inline bool Tree<T>::InsertFixUp(TreeNode<T>* p)
	{
		/**************平衡******************/
		/*
		RB-INSERT-FIXUP(T, z)
		while color[p[z]] = RED  // 若“当前节点(z)的父节点是红色”，则进行以下处理。
		do
			if p[z] = left[p[p[z]]]// 若“z的父节点”是“z的祖父节点的左孩子”，则进行以下处理。
			then
				y ← right[p[p[z]]]// 将y设置为“z的叔叔节点(z的祖父节点的右孩子)”
				if color[y] = RED    // Case 1条件：叔叔是红色
				then
					color[p[z]] ← BLACK   ▹ Case 1   //  (01) 将“父节点”设为黑色。
					color[y] ← BLACK      ▹ Case 1   //  (02) 将“叔叔节点”设为黑色。
					color[p[p[z]]] ← RED  ▹ Case 1   //  (03) 将“祖父节点”设为“红色”。
					z ← p[p[z]]           ▹ Case 1   //  (04) 将“祖父节点”设为“当前节点”(红色节点)
				else if z = right[p[z]] // Case 2条件：叔叔是黑色，且当前节点是右孩子
				then
					z ← p[z]			   ▹ Case 2   //  (01) 将“父节点”作为“新的当前节点”。
					LEFT-ROTATE(T, z)      ▹ Case 2   //  (02) 以“新的当前节点”为支点进行左旋。

				color[p[z]] ← BLACK        ▹ Case 3   // Case 3条件：叔叔是黑色，且当前节点是左孩子。(01) 将“父节点”设为“黑色”。
				color[p[p[z]]] ← RED       ▹ Case 3   //  (02) 将“祖父节点”设为“红色”。
				RIGHT-ROTATE(T, p[p[z]])    ▹ Case 3   //  (03) 以“祖父节点”为支点进行右旋。
				else (same as then clause with "right" and "left" exchanged)
				// 若“z的父节点”是“z的祖父节点的右孩子”，将上面的操作中“right”和“left”交换位置，然后依次执行。
				color[root[T]] ← BLACK
		*/

		//if (NULL == p)
		//{
		//	return false;
		//}
		//while (true)
		//{
		//	//父节点
		//	TreeNode<T>*pp = p->pParent;
		//	//当前节点存在而且为红色
		//	if (pp&&pp->Red)
		//	{
		//		//祖父节点
		//		TreeNode<T>*ppp = pp->pParent;
		//		if (NULL == ppp)
		//		{
		//			return false;//异常
		//		}
		//		//叔叔节点
		//		TreeNode<T>*pu = NULL;
		//		if (ppp->pLeft == pp)
		//		{
		//			//若“z的父节点”是“z的祖父节点的左孩子”，则进行以下处理。
		//			pu = ppp->pRight;
		//			//1当前节点(即，被插入节点)的父节点是红色，
		//			//且当前节点的祖父节点的另一个子节点（叔叔节点）也是红色。
		//			if (pu&&pu->Red)//NULL 是黑色的
		//			{
		//				/*
		//				(01) 将“父节点”设为黑色。
		//				(02) 将“叔叔节点”设为黑色。
		//				(03) 将“祖父节点”设为“红色”。
		//				(04) 将“祖父节点”设为“当前节点”(红色节点)；即，之后继续对“当前节点”进行操作。
		//				*/
		//				pp->Red = false;
		//				pu->Red = false;
		//				ppp->Red = true;
		//				p = ppp;
		//			}
		//			else
		//			{
		//				//当前节点的父节点是红色，叔叔节点是黑色，且当前节点是其父节点的右孩子
		//				if (pp->pRight == p)
		//				{
		//					//(01) 将“父节点”作为“新的当前节点”。
		//					//(02) 以“新的当前节点”为支点进行左旋。
		//					p = pp;
		//					if (!LRotate(p))
		//						return false;
		//				}
		//				//当前节点的父节点是红色，叔叔节点是黑色，且当前节点是其父节点的左孩子
		//				//(01) 将“父节点”设为“黑色”。
		//					//(02) 将“祖父节点”设为“红色”。
		//					//(03) 以“祖父节点”为支点进行右旋。
		//				pp->Red = false;
		//				ppp->Red = true;
		//				if (!RRotate(ppp))
		//					return false;
		//			}
		//		}
		//		else
		//		{
		//			pu = ppp->pLeft;
		//			//1当前节点(即，被插入节点)的父节点是红色，
		//			//且当前节点的祖父节点的另一个子节点（叔叔节点）也是红色。
		//			if (pu&&pu->Red)//NULL 是黑色的
		//			{
		//				/*
		//				(01) 将“父节点”设为黑色。
		//				(02) 将“叔叔节点”设为黑色。
		//				(03) 将“祖父节点”设为“红色”。
		//				(04) 将“祖父节点”设为“当前节点”(红色节点)；即，之后继续对“当前节点”进行操作。
		//				*/
		//				pp->Red = false;
		//				pu->Red = false;
		//				ppp->Red = true;
		//				p = ppp;
		//			}
		//			else
		//			{
		//				//当前节点的父节点是红色，叔叔节点是黑色，且当前节点是其父节点的右孩子
		//				if (pp->pLeft == p)
		//				{
		//					//(01) 将“父节点”作为“新的当前节点”。
		//					//(02) 以“新的当前节点”为支点进行左旋。
		//					p = pp;
		//					if (!RRotate(p))
		//						return false;
		//				}
		//				//当前节点的父节点是红色，叔叔节点是黑色，且当前节点是其父节点的左孩子
		//				//(01) 将“父节点”设为“黑色”。
		//					//(02) 将“祖父节点”设为“红色”。
		//					//(03) 以“祖父节点”为支点进行右旋。
		//				pp->Red = false;
		//				ppp->Red = true;
		//				if (!LRotate(ppp))
		//					return false;
		//			}
		//		}
		//		
		//	}
		//	else
		//	{
		//		break;
		//	}
		//}
		//pRoot->Red = false;

		TreeNode<T>* parent = p->pParent;
		//wepoll
		for (; parent && parent->Red; parent = p->pParent)
		{
			TreeNode<T>* grandparent = parent->pParent;
			if (parent == parent->pParent->pLeft)
			{
				//TREE__REBALANCE_AFTER_INSERT(left, right)
				TreeNode<T>* uncle = grandparent->pRight;

				if (uncle && uncle->Red) 
				{
					parent->Red = uncle->Red = false;
					grandparent->Red = true;
					p = grandparent;
				}
				else 
				{
					if (p == parent->pRight) {

						LRotate(parent);
						p = parent;
						parent = p->pParent;
					}
					parent->Red = false;
					grandparent->Red = true;
					RRotate(grandparent);
				}
			}
			else
			{
				//TREE__REBALANCE_AFTER_INSERT(right, left)
				//TREE__REBALANCE_AFTER_INSERT(left, right)
				TreeNode<T>* uncle = grandparent->pLeft;

				if (uncle && uncle->Red)
				{
					parent->Red = uncle->Red = false;
					grandparent->Red = true;
					p = grandparent;
				}
				else
				{
					if (p == parent->pLeft) {

						RRotate(parent);
						p = parent;
						parent = p->pParent;
					}
					parent->Red = false;
					grandparent->Red = true;
					LRotate(grandparent);
				}
			}
			
		}
		pRoot->Red = false;
		return true;
	}

}
#endif