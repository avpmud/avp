#include <vector>
#include <iostream>

#include "types.h"
#include "rooms.h"
#include "lexilist.h"
#include "WeakPtr.h"

#define MAX_SEARCH_NODES	256

struct fooptr : public WeakPointable
{
	int i;
};


void test()
{
	typedef Lexi::List<Lexi::String> List;
	List foo;
	
	foo.push_back("hi");
	foo.insert(foo.end(), "bye");
	foo.push_front("lo");
	foo.push_back("bar");
	foo.push_back("baz");
	foo.push_back("foo");
	for (List::iterator i = foo.begin(); i != foo.end(); ++i)
	{
		Lexi::String x = *i;
		*i = x;
		
		
		for (List::iterator i2 = foo.begin(); i2 != foo.end(); ++i2)
		{
			if (*i == "bar" && i == i2)
				foo.erase(i2);
		}
	}
	
	foo.sort();
	for (List::iterator i = foo.begin(); i != foo.end(); ++i)
	{
		std::cout << i->c_str() << std::endl;
	}
	
	List::const_iterator ci = foo.begin();
	List::const_iterator ci2 = ci;
//	List::iterator cinc = ci;
	
	for (List::reverse_iterator i = foo.rbegin(); i != foo.rend(); ++i)
	{
		Lexi::String x = *i;
		*i = x;
	}
	
	WeakPtr<fooptr>	ptrToFoo = 0;
	fooptr x;
	ptrToFoo = &x;
	WeakPtr<fooptr>	ptrToFoo2;
	ptrToFoo2 = (ptrToFoo);
	
	((fooptr *)ptrToFoo2)->i = 0;
}

#if 0

class SearchNode
{
public:
					SearchNode();
					
	void			Init(RoomData *pRoom, SearchNode *pParent, Direction dir, float cost);
	void			Release();

	RoomData *		m_pRoom;
	SearchNode *	m_pParent;
	Direction		m_Direction;
	float			m_Cost;	//	Cost from start
};


SearchNode::SearchNode() :
	m_pRoom(NULL),
	m_pParent(NULL),
	m_Direction(INVALID_DIRECTION),
	m_Cost(0)
{
}


void SearchNode::Init(RoomData *pRoom, SearchNode *pParent, Direction dir, float cost)
{
	m_pRoom = pRoom;
	m_pParent = pParent;
	m_Direction = dir;
	m_Cost = cost;
	m_pRoom->m_pSearchNode = this;
}


void SearchNode::Release()
{
	m_pRoom->m_pSearchNode = NULL;
}



class SearchNodePool
{
public:
	static SearchNode *	CreateNode(RoomData *pRoom, SearchNode *pParent, Direction dir, float cost);
	static void			Reset();

private:
	static SearchNode	ms_Nodes[MAX_SEARCH_NODES];
	static int			ms_NextNode;
};


SearchNode	SearchNodePool::ms_Nodes[MAX_SEARCH_NODES];
int			SearchNodePool::ms_NextNode = 0;


SearchNode *SearchNodePool::CreateNode(RoomData *pRoom, SearchNode *pParent, Direction dir, float cost)
{
	if (ms_NextNode < MAX_SEARCH_NODES)
	{
		SearchNode *pNode = &ms_Nodes[ms_NextNode++];
		pNode->Init(pRoom, pParent, dir, cost);
		return pNode;
	}
	
	return NULL;
}


void SearchNodePool::Reset()
{
	while (ms_NextNode > 0)
	{
		ms_Nodes[--ms_NextNode].Release();
	}
}






//	Binary Heap, for unsorted notes
class SearchNodeHeap
{
public:
					SearchNodeHeap();

	void			Add(SearchNode *pNode);
	void			Remove(int index);
	SearchNode *	Pop();
	
	bool			IsEmpty() { return m_NumNodes == 1; }
	void			Clear() { m_NumNodes = 1; }
	
private:
	int				m_NumNodes;
	SearchNode *	m_pNodes[MAX_SEARCH_NODES + 1];
};


SearchNodeHeap::SearchNodeHeap() :
	m_NumNodes(1)
{
}


void SearchNodeHeap::Add(SearchNode * pNode)
{
	if ( m_NumNodes > MAX_SEARCH_NODES )
		return;	//	Fail, we're full!
	
	int	i = m_NumNodes++;
	int	p = (i >> 1);
	
	while ( (i > 1) && pNode->m_Cost < m_pNodes[p]->m_Cost)
	{
		m_pNodes[i] = m_pNodes[p];
		
		i = p;
		p = i >> 1;
	}
	
	m_pNodes[i] = pNode;
}


void SearchNodeHeap::Remove( int i )
{
	SearchNode * pTemp = m_pNodes[--m_NumNodes];

	while ( (i << 1) < m_NumNodes)
	{
		int j = i << 1;
	
		if ( ((j+1) < m_NumNodes) && (m_pNodes[j + 1]->m_Cost < m_pNodes[j]->m_Cost) )
		{
			++j;
		}

		if ( pTemp->m_Cost < m_pNodes[j]->m_Cost)
			break;
		
		
		m_pNodes[i] = m_pNodes[j];
		i = j;
	}
	
	m_pNodes[i] = pTemp;
}


SearchNode *SearchNodeHeap::Pop()
{
	SearchNode *pNode = m_pNodes[1];
	Remove(1);
	return pNode;
}



class PathfindSolution
{
public:
	virtual bool		operator()(RoomData *room) = 0;
};


class PathfindCostFunction
{
public:
	virtual float		operator()(RoomData *room, Direction dir) = 0;
};


void Pathfind(RoomData *room, PathfindSolution &predicate, PathfindCostFunction &costFunc, int depth, Flags flags)
{
	SearchNodeHeap		nodes;
	std::vector<SearchNode *>	solution;
	SearchNode *		pNode;
	SearchNode *		pSolutionNode = NULL;
	
	
	solution.reserve(MAX_SEARCH_NODES);
	
	//	Push ROOM to the start
	pNode = SearchNodePool::CreateNode(room, NULL, INVALID_DIRECTION, 0);
	nodes.Add(pNode);
	
	while (!nodes.IsEmpty())
	{
		pNode = nodes.Pop();
		
		//	Check if this is our node
		if (0 /* Is our solution node */)
		{
			pSolutionNode = pNode;
			break;
		}
		
		//	Otherwise, push the children
		RoomData *pRoom = pNode->m_pRoom;
		for (int i = 0; i < NUM_OF_DIRS; ++i)
		{
			RoomExit *pDirection = pRoom->m_Exits[i]/*.Get()*/;
			
			if (!pDirection || pDirection->ToRoom() == NULL)
				continue;
			
			RoomData *pRoom = pDirection->ToRoom();
			
			if (pRoom->m_pSearchNode)	//	Already added it!
				continue;
			
			if (0 /* Can't traverse... */)
				continue;
			
			SearchNode *newNode = SearchNodePool::CreateNode(pRoom, pNode, (Direction)i, pNode->m_Cost + 1);
			
			if (!newNode)
				break;	//	Ran out of nodes!
			
			nodes.Add(newNode);
		}
	}
	
	if (pSolutionNode)
	{
		//	Build a path backwards!
	}
	
	SearchNodePool::Reset();
}

#endif
