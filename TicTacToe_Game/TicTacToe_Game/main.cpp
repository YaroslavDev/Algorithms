#include "SocketClient.h"
#include "SocketServer.h"
#include <crtdbg.h>
#include <iostream>
#include <vector>
#include <list>
#include <cstdlib>
#include <time.h>
#include <iomanip>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

using namespace std;

#define SERVER_ADDRESS "127.0.0.1"
#define SAFE_DELETE( x ) { if( x ) { delete (x); (x)=NULL; } }
#define SAFE_DELETE_ARRAY( x ) { if( x ) { delete[] (x); (x)=NULL; } }
#define THREAD_FUNC unsigned int (__stdcall*)(void* )
#define CS_TIMEOUT 50000
#define WIN_COMBO 100000
#define TABLE_SIZE 10
#define COMBO_SIZE 5
#ifdef _DEBUG
#define AI_DEPTH   4
#else
#define AI_DEPTH   5
#endif
#define N_THREADS  8
#define ACTIONS_THRESHOLD 8
#define maxOf( a , b )  (a)>(b)?(a):(b) 
#define minOf( a , b )  (a)<(b)?(a):(b) 

struct Cursor_Pos
{
	Cursor_Pos()
		: x(0), y(0) {}
	int x;
	int y;
};

struct GameState
{
	GameState() : est(0)
	{
		x_combos.resize(COMBO_SIZE+1);
		o_combos.resize(COMBO_SIZE+1);
	}
	GameState& operator=(const GameState& other)
	{
		if( this!=&other )
		{
			t = other.t;
			est = other.est;
			nCount = other.nCount;
			x_turn = other.x_turn;
		}
		return *this;
	}
	//bool operator<(const GameState& other)
	//{
	//	return est<other.est;	//for sorting
	//}
	vector<vector<short>> t;	//table
	double est;					//estimation of state
	int nCount;					//number of filled cells
	bool x_turn;				//whos turn: X or O ?
	vector<double> x_combos;	//track number of x combos
	vector<double> o_combos;	//track number of o combos
};
bool X_less(const GameState& left, const GameState& right)
{
	return left.est>right.est;
}
bool O_less(const GameState& left, const GameState& right)
{
	return left.est<right.est;
}
struct MinimaxArg
{
	GameState state;
	int min_b;
	int max_b;
	int depth;
};

struct MinimaxArray
{
	vector<MinimaxArg> v;
};

//Global variables
int isGameOnline = -1;			//1 - PvC; 2 - PvP; 0 - Exit
int isClient = -1;				//in online game any player can be client or server
char serv_address[14];			//ip-address of server in text form
HANDLE g_hCon;					//handle to console to change textcolor using WinAPI
Cursor_Pos g_cursor;
GameState gameState;			//current game state
//bool X_Turn = false;			//whos turn: X or O ?
size_t gSize;					//game table size
size_t gComboSize;				//how many symbols you should put in row, column or diagonal to win
SocketClient *gClientSock = NULL;
SocketServer *gServSock = NULL;
MESSAGE msg;
short gAIDepth = AI_DEPTH;		//how smart is computer
//Global functions
int prepareOnlineGame(char* char_address);
static DWORD connectToServer(char* char_address);
static DWORD createServer();
int closeOnlineSession();
int playPvCGame();
int playPvPGame();
BOOL WINAPI Handler(DWORD CtrlEvent);
static DWORD ThreadMinimax(PVOID );
//Determines all possible "next" states for current state
list<GameState> getActions(GameState& state, int nMaxActions);
//Get computer's action according to current state
//Based on Alpha-Beta Search(with pruning)
GameState getAIAction(GameState& state, short depth);
GameState getThreadAIAction(GameState& state, short depth);
//Get another players action in online game
int getPlayerAction(GameState& state, int &x, int &y);
//Send your action to your opponent
int sendMyAction(int x, int y);
//Let the AI estimate how good is this state
//-4*COMBO_SIZE - O won
//+4*COMBO_SIZE - X won
// other values - Draw		   if filled
//				  Not finished if not filled
double getAIStateEstimation(GameState& state);	

//Minimax functions for imperfect decisions(using depth)
double MaxValue(GameState& state, double alpha, double beta, short depth);
double MinValue(GameState& state, double alpha, double beta, short depth);
bool isAdjacent(short i, short j, short toX, short toY);
void printTable(GameState& );
void cleanTable(GameState& );
void createTableFromFile(GameState& t, string filename);

int main(int argc, char *argv[])
{
	if( argc>=4 )
	{
		gSize = atoi(argv[1]);
		gComboSize = atoi(argv[2]);
		strcpy(serv_address, argv[3]);
	}
	else
	{
		gSize = TABLE_SIZE;
		gComboSize = COMBO_SIZE;
		strcpy(serv_address, SERVER_ADDRESS);
	}
	list<vector<vector<short>>> possActions;
	int result;
	int wantNewSession = 0;
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	srand( time(0) );
	g_hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	if( !SetConsoleCtrlHandler(Handler, TRUE) )
		fprintf(stderr, "Can't create console handler!\n");
	//Game table initialization
	gameState.t.resize(gSize);
	for(int i=0; i<gameState.t.size(); i++)
		gameState.t.at(i).resize(gSize);
	/*createTableFromFile(gameState, "Table.txt");
	printTable(gameState);
	result = getAIStateEstimation(gameState);
	list<GameState> acts = getActions(gameState, 8);*/
	do
	{
		cleanTable(gameState);
		//Main menu:
		cout<<"		MAIN MENU\n"
			  "1 - Player vs. Computer\n"
			  "2 - Player vs. Player\n"
			  "0 - Exit\n";
		cin>>isGameOnline;
		//isGameOnline = 1;
		switch( isGameOnline )
		{
		case 2:
			//system("cls");
			COORD cur_pos;
			cur_pos.X = 0;
			cur_pos.Y = 0;
			SetConsoleCursorPosition(g_hCon, cur_pos);
			cout<<"		ONLINE GAME MENU\n"
				  "0 - Create new game\n"
				  "1 - Connect to game\n";
			cin>>isClient;
			if( isClient )
			{
				printf("IP-Address of server: ");
				cin>>serv_address;
			}
			if( !prepareOnlineGame(serv_address) )
				playPvPGame();
			else
				fprintf(stderr, "Initialization online game error!\n");
			closeOnlineSession();
			break;
		case 1:
			//Randomly generates decision who starts first: X or O ?
			if( rand()%2==1 )
				gameState.x_turn = true;		//X - player
			else gameState.x_turn = false;	//O - computer
			playPvCGame();
			break;
		case 0:
			goto P_Exit;
			break;
		default:
			break;
		};
		printf("Want to play new game? (1/0)\n");
		cin>>wantNewSession;
	} while( wantNewSession );

	P_Exit:
#ifdef _DEBUG
	system("Pause");
#endif
	SAFE_DELETE(gClientSock);
	SAFE_DELETE(gServSock);
	return 0;
}

static DWORD ThreadMinimax(PVOID ptr)
{
	MinimaxArray *arr = (MinimaxArray *)ptr;
	MinimaxArg* arg = NULL;
	for(int i=0; i<arr->v.size(); i++)
	{
		arg = &arr->v[i];
		arg->state.est = MaxValue(arg->state, arg->min_b, arg->max_b, arg->depth);
	}
	return 0;
}

GameState getThreadAIAction(GameState& state, short depth)
{
	list<GameState> actions = getActions(state, 8);
	list<GameState>::iterator it = actions.begin(), end = actions.end();
	GameState best_action;
	short min_best = -WIN_COMBO-5;
	short max_best = WIN_COMBO+5;
	short best_est = (state.x_turn ? min_best : max_best);
	//if( actions.size()>=8 ) depth--; //decrease depth if branching factor is too high
	//Prepare args for threading
	// 8 - number of threads
	int numberOfThreads = min(N_THREADS, actions.size());
	HANDLE *hThreads = new HANDLE[numberOfThreads];
	MinimaxArray* pArray = new MinimaxArray[numberOfThreads];
	int minArgsPerThread = actions.size()/numberOfThreads;
	int maxArgsPerThread = actions.size()%numberOfThreads;
	int k_action = 0;
	for(int i=0; i<numberOfThreads; i++)
	{
		if( maxArgsPerThread>0 )
		{
			pArray[i].v.resize(minArgsPerThread+1);
			maxArgsPerThread--;
		}
		else
		{
			pArray[i].v.resize(minArgsPerThread);
		}
		for(int j=0; j<pArray[i].v.size(); j++, k_action++)
		{
			pArray[i].v.at(j).state = *actions.begin();
			actions.pop_front();
			pArray[i].v.at(j).min_b = min_best;
			pArray[i].v.at(j).max_b = max_best;
			pArray[i].v.at(j).depth = depth;
		}
	}
	for(int i=0; i<numberOfThreads; i++)
	{
		hThreads[i] = (HANDLE)_beginthreadex(NULL,0,(THREAD_FUNC)ThreadMinimax,(void*)&pArray[i], 0, NULL);
	}
	WaitForMultipleObjects(numberOfThreads, hThreads, TRUE, INFINITE);
	for(int i=0; i<numberOfThreads; i++)
	{
		CloseHandle(hThreads[i]);
	}
	SAFE_DELETE_ARRAY(hThreads);
	int current_estimate = 0;
	for(int i=0; i<numberOfThreads; i++)
	{
		for(int j=0; j<pArray[i].v.size(); j++)
		{
			if( state.x_turn )
			{
				if( best_action.est<pArray[i].v.at(j).state.est )
				{
					best_action = pArray[i].v.at(j).state;
				}
			}
			else
			{
				if( best_action.est>pArray[i].v.at(j).state.est )
				{
					best_action = pArray[i].v.at(j).state;
				}
			}
		}
	}
	SAFE_DELETE_ARRAY(pArray);
	return best_action;
}

GameState getAIAction(GameState& state, short depth)
{
	list<GameState> actions = getActions(state, ACTIONS_THRESHOLD);
	list<GameState>::iterator it = actions.begin(), end = actions.end();
	GameState best_action;
	float min_best = -WIN_COMBO-5;
	float max_best = WIN_COMBO+5;
	best_action.est = (state.x_turn ? min_best : max_best);
	while( it!=end )
	{
		it->est = MaxValue(*it, min_best, max_best, depth-1);
		if( state.x_turn )
		{
			if( best_action.est<it->est )
			{
				best_action = *it;
			}
		}
		else
		{
			if( best_action.est>it->est )
			{
				best_action = *it;
			}
		}
		it++;
	}
	return best_action;
}

double MaxValue(GameState& state, double alpha, double beta, short depth)
{
	double util = state.est;//getAIStateEstimation(state);
	if( util==-WIN_COMBO || util==WIN_COMBO ) return util;	//Someone won
	if( state.nCount == gSize*gSize ) return util;					//Draw
	if( depth<=0 ) return util;										//Cutoff test

	double v = -WIN_COMBO-5;	//just some big negative value
	list<GameState> actions;
	actions = getActions(state, ACTIONS_THRESHOLD);
	list<GameState>::iterator it = actions.begin(), end = actions.end();
	while( it!=end )
	{
		v = maxOf(v, MinValue(*it, alpha, beta, depth-1));
		if( v >= beta ) return v;
		alpha = maxOf(alpha, v);
		it++;
	}
	return v;
}

double MinValue(GameState& state, double alpha, double beta, short depth)
{
	double util = state.est;//getAIStateEstimation(state);
	if( util==-WIN_COMBO || util==WIN_COMBO ) return util;	//Someone won
	if( state.nCount == gSize*gSize ) return util;					//Draw
	if( depth<=0 ) return util;										//Cutoff test

	double v = WIN_COMBO+5;	//just some big positive value
	list<GameState> actions;
	actions = getActions(state, ACTIONS_THRESHOLD);
	list<GameState>::iterator it = actions.begin(), end = actions.end();
	while( it!=end )
	{
		v = minOf(v, MaxValue(*it, alpha, beta, depth-1));
		if( v <= alpha ) return v;
		beta = minOf(beta, v);
		it++;
	}
	return v;
}

list<GameState> getActions(GameState& state, int nMaxActions)
{
	list<GameState> possible_actions;
	vector<vector<short>> map(gSize);
	for(int i=0; i<gSize; i++)
		map.at(i).resize(gSize);
	GameState action;
	action = state;
	//Include in list of possible actions only those which put adjacent verteces to already existing
	for(int i=0; i<gSize; i++)
		for(int j=0; j<gSize; j++)
		{
			if(state.t.at(i).at(j)!=0)
			{
				for(int x=i-1; x<=i+1; x++)
					for(int y=j-1; y<=j+1; y++)
						if( isAdjacent(x,y, i,j) && state.t.at(x).at(y)==0 )
							map.at(x).at(y)=1;
			}
		}
	//Try only adjacent actions(experiment)
	for(int i=0; i<gSize; i++)
		for(int j=0; j<gSize; j++)
		{
			if( map.at(i).at(j)==1 )
			{
				action.t.at(i).at(j) = ( state.x_turn ? 1 : -1 );
				action.est = getAIStateEstimation(action);
				action.nCount = state.nCount + 1;
				action.x_turn = !state.x_turn;
				possible_actions.push_back(action);
				action = state;
			}
		}
	if( state.x_turn )
		possible_actions.sort(X_less);
	else
		possible_actions.sort(O_less);
	list<GameState>::iterator it1 = possible_actions.begin(), it2 = it1, itEnd = possible_actions.end();
	for(int i=0; i<nMaxActions && it2!=itEnd; i++, it2++);
	list<GameState> probable_actions;
	probable_actions.assign(it1, it2);
	return probable_actions;
}

double getAIStateEstimation(GameState& state)
{
	vector<vector<short>>& t = state.t;
	int combo_counter_O = 0,
		combo_counter_X = 0,
		longest_combo_O = 0,
		longest_combo_X = 0;
	int combo_begin = 0, combo_end = 0;
	int adj1 = 0, adj2 = 0;
	//Horizontal rows analysis
	int hor_combo_O = 0,
		hor_combo_X = 0;
	vector<double>& n_X_combos = state.x_combos;
	vector<double>& n_O_combos = state.o_combos;
	fill(n_X_combos.begin(), n_X_combos.end(), 0);
	fill(n_O_combos.begin(), n_O_combos.end(), 0);
	for(int k=0; k<gSize; k++)
	{
		combo_counter_O = 0;
		combo_counter_X = 0;
		int i=0;
		for(i=0; i<gSize; i++)
		{
			switch( t.at(k).at(i) )
			{
			case 1:
				combo_counter_X++;
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(k).at(i-combo_counter_O-1)!=1 )
						{
							hor_combo_O = combo_counter_O;
							n_O_combos[hor_combo_O]+=0.5;
						}
					}
					combo_counter_O = 0;
				}
				break;
			case 0:
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(k).at(i-combo_counter_X-1)!=-1 )
						{
							hor_combo_X = combo_counter_X;
							n_X_combos[hor_combo_X]++;
						}
						else
						{
							hor_combo_X = combo_counter_X;
							n_X_combos[hor_combo_X]+=0.5;
						}
					}
					else
					{
						hor_combo_X = combo_counter_X;
						n_X_combos[hor_combo_X]+=0.5;
					}
					combo_counter_X = 0;
				}
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(k).at(i-combo_counter_O-1)!=1 )
						{
							hor_combo_O = combo_counter_O;
							n_O_combos[hor_combo_O]++;
						}
						else
						{
							hor_combo_O = combo_counter_O;
							n_O_combos[hor_combo_O]+=0.5;
						}
					}
					else
					{
						hor_combo_O = combo_counter_O;
						n_O_combos[hor_combo_O]+=0.5;
					}
					combo_counter_O = 0;
				}
				break;
			case -1:
				combo_counter_O++;
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(k).at(i-combo_counter_X-1)!=-1 )
						{
							hor_combo_X = combo_counter_X;
							n_X_combos[hor_combo_X]+=0.5;
						}
					}
					combo_counter_X = 0;
				}
				break;
			default: break;
			}
		}
		if( combo_counter_X>0 )
		{
			if( combo_counter_X>=gComboSize ) return WIN_COMBO;
			if( gSize-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
			{
				if( t.at(k).at(gSize-combo_counter_X-1)!=-1 )
				{
					hor_combo_X = combo_counter_X;
					n_X_combos[hor_combo_X]+=0.5;
				}
			}
		}
		if( combo_counter_O>0 )
		{
			if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
			if( gSize-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
			{
				if( t.at(k).at(gSize-combo_counter_O-1)!=1 )
				{
					hor_combo_O = combo_counter_O;
					n_O_combos[hor_combo_O]+=0.5;
				}
			}
		}
	}
	//Vertical columns analysis
	int ver_combo_O = 0,
		ver_combo_X = 0;
	for(int k=0; k<gSize; k++)
	{
		combo_counter_O = 0;
		combo_counter_X = 0;
		int i=0;
		for(i=0; i<gSize; i++)
		{
			switch( t.at(i).at(k) )
			{
			case 1:
				combo_counter_X++;
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_O-1).at(k)!=1 )
						{
							ver_combo_O = combo_counter_O;
							n_O_combos[ver_combo_O]+=0.5;
						}
					}
					combo_counter_O = 0;
				}
				break;
			case 0:
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_X-1).at(k)!=-1 )
						{
							ver_combo_X = combo_counter_X;
							n_X_combos[ver_combo_X]++;
						}
						else
						{
							ver_combo_X = combo_counter_X;
							n_X_combos[ver_combo_X]+=0.5;
						}
					}
					else
					{
						ver_combo_X = combo_counter_X;
						n_X_combos[ver_combo_X]+=0.5;
					}
					combo_counter_X = 0;
				}
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_O-1).at(k)!=1 )
						{
							ver_combo_O = combo_counter_O;
							n_O_combos[ver_combo_O]++;
						}
						else
						{
							ver_combo_O = combo_counter_O;
							n_O_combos[ver_combo_O]+=0.5;
						}
					}
					else
					{
						ver_combo_O = combo_counter_O;
						n_O_combos[ver_combo_O]+=0.5;
					}
					combo_counter_O = 0;
				}
				break;
			case -1:
				combo_counter_O++;
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_X-1).at(k)!=-1 )
						{
							ver_combo_X = combo_counter_X;
							n_X_combos[ver_combo_X]+=0.5;
						}
					}
					combo_counter_X = 0;
				}
				break;
			default: break;
			}
		}
		if( combo_counter_X>0 )
		{
			if( combo_counter_X>=gComboSize ) return WIN_COMBO;
			if( gSize-combo_counter_X-1>=0 )
			{
				if( t.at(gSize-combo_counter_X-1).at(k)!=-1 )
				{
					ver_combo_X = combo_counter_X;
					n_X_combos[ver_combo_X]+=0.5;
				}
			}
		}
		if( combo_counter_O>0 )
		{
			if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
			if( gSize-combo_counter_O-1>=0 )
			{
				if( t.at(gSize-combo_counter_O-1).at(k)!=1 )
				{
					ver_combo_O = combo_counter_O;
					n_O_combos[ver_combo_O]+=0.5;
				}
			}
		}
	}
	//Secondary diagonal analysis: Upper part
	int sec_combo_O = 0,
		sec_combo_X = 0;
	for(int k=gComboSize-1; k<gSize; k++)
	{
		int i=k;
		combo_counter_O = 0;
		combo_counter_X = 0;
		int j=0;
		for(j=0; i>=0 && j<gSize; i--, j++)
		{
			switch( t.at(i).at(j) )
			{
			case 1:
				combo_counter_X++;
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i+combo_counter_O+1<gSize && j-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i+combo_counter_O+1).at(j-combo_counter_O-1)!=1 )
						{
							sec_combo_O = combo_counter_O;
							n_O_combos[sec_combo_O]+=0.5;
						}
					}
					combo_counter_O = 0;
				}
				break;
			case 0:
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i+combo_counter_X+1<gSize && j-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i+combo_counter_X+1).at(j-combo_counter_X-1)!=-1 )
						{
							sec_combo_X = combo_counter_X;
							n_X_combos[sec_combo_X]++;
						}
						else
						{
							sec_combo_X = combo_counter_X;
							n_X_combos[sec_combo_X]+=0.5;
						}
					}
					else
					{
						sec_combo_X = combo_counter_X;
						n_X_combos[sec_combo_X]+=0.5;
					}
					combo_counter_X = 0;
				}
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i+combo_counter_O+1<gSize && j-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i+combo_counter_O+1).at(j-combo_counter_O-1)!=1 )
						{
							sec_combo_O = combo_counter_O;
							n_O_combos[sec_combo_O]++;
						}
						else
						{
							sec_combo_O = combo_counter_O;
							n_O_combos[sec_combo_O]+=0.5;
						}
					}
					else
					{
						sec_combo_O = combo_counter_O;
						n_O_combos[sec_combo_O]+=0.5;
					}
					combo_counter_O = 0;
				}
				break;
			case -1:
				combo_counter_O++;
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i+combo_counter_X+1<gSize && j-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i+combo_counter_X+1).at(j-combo_counter_X-1)!=-1 )
						{
							sec_combo_X = combo_counter_X;
							n_X_combos[sec_combo_X]+=0.5;
						}
					}
					combo_counter_X = 0;
				}
				break;
			default: break;
			}
		}
		if( combo_counter_X>0 )
		{
			if( combo_counter_X>=gComboSize ) return WIN_COMBO;
			if( i+combo_counter_X+1<gSize && j-combo_counter_X-1>=0 )
			{
				if(t.at(i+combo_counter_X+1).at(j-combo_counter_X-1)!=-1)
				{
					sec_combo_X = combo_counter_X;
					n_X_combos[sec_combo_X]+=0.5;
				}
			}
		}
		if( combo_counter_O>0 )
		{
			if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
			if( i+combo_counter_O+1<gSize && j-combo_counter_O-1>=0 )
			{
				if(t.at(i+combo_counter_O+1).at(j-combo_counter_O-1)!=1)
				{
					sec_combo_O = combo_counter_O;
					n_O_combos[sec_combo_O]+=0.5;
				}
			}
		}
	}
	//Secondary diagonal analysis: Lower part
	for(int k=1; k<=gSize-gComboSize; k++)
	{
		int i=k;
		combo_counter_O = 0;
		combo_counter_X = 0;
		int j=0;
		for(j=gSize-1; j>=0 && i<gSize; j--, i++)
		{
			switch( t.at(i).at(j) )
			{
			case 1:
				combo_counter_X++;
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i-combo_counter_O-1>=0 && j+combo_counter_O+1<gSize )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_O-1).at(j+combo_counter_O+1)!=1 )
						{
							sec_combo_O = combo_counter_O;
							n_O_combos[sec_combo_O]+=0.5;
						}
					}
					combo_counter_O = 0;
				}
				break;
			case 0:
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i-combo_counter_X-1>=0 && j+combo_counter_X+1<gSize )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_X-1).at(j+combo_counter_X+1)!=-1 )
						{
							sec_combo_X = combo_counter_X;
							n_X_combos[sec_combo_X]++;
						}
						else
						{
							sec_combo_X = combo_counter_X;
							n_X_combos[sec_combo_X]+=0.5;
						}
					}
					else
					{
						sec_combo_X = combo_counter_X;
						n_X_combos[sec_combo_X]+=0.5;
					}
					combo_counter_X = 0;
				}
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i-combo_counter_O-1>=0 && j+combo_counter_O+1<gSize )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_O-1).at(j+combo_counter_O+1)!=1 )
						{
							sec_combo_O = combo_counter_O;
							n_O_combos[sec_combo_O]++;
						}
						else
						{
							sec_combo_O = combo_counter_O;
							n_O_combos[sec_combo_O]+=0.5;
						}
					}
					else
					{
						sec_combo_O = combo_counter_O;
						n_O_combos[sec_combo_O]+=0.5;
					}
					combo_counter_O = 0;
				}
				break;
			case -1:
				combo_counter_O++;
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i-combo_counter_X-1>=0 && j+combo_counter_X+1<gSize )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_X-1).at(j+combo_counter_X+1)!=-1 )
						{
							sec_combo_X = combo_counter_X;
							n_X_combos[sec_combo_X]+=0.5;
						}
					}
					combo_counter_X = 0;
				}
				break;
			default: break;
			}
		}
		if( combo_counter_X>0 )
		{
			if( combo_counter_X>=gComboSize ) return WIN_COMBO;
			if( i-combo_counter_X-1>=0 && j+combo_counter_X+1<gSize )
			{
				if(t.at(i-combo_counter_X-1).at(j+combo_counter_X+1)!=-1)
				{
					sec_combo_X = combo_counter_X;
					n_X_combos[sec_combo_X]+=0.5;
				}
			}
		}
		if( combo_counter_O>0 )
		{
			if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
			if( i-combo_counter_O-1>=0 && j+combo_counter_O+1<gSize )
			{
				if(t.at(i-combo_counter_O-1).at(j+combo_counter_O+1)!=1)
				{
					sec_combo_O = combo_counter_O;
					n_O_combos[sec_combo_O]+=0.5;
				}
			}
		}
	}
	//Main diagonal analysis: Lower part
	int main_combo_X = 0,
		main_combo_O = 0;
	for(int k=0; k<=gSize-gComboSize; k++)
	{
		int i=k;
		combo_counter_O = 0;
		combo_counter_X = 0;
		int j=0;
		for(j=0; i<gSize && j<gSize; i++, j++)
		{
			switch( t.at(i).at(j) )
			{
			case 1:
				combo_counter_X++;
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i-combo_counter_O-1>=0 && j-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_O-1).at(j-combo_counter_O-1)!=1 )
						{
							main_combo_O = combo_counter_O;
							n_O_combos[main_combo_O]+=0.5;
						}
					}
					combo_counter_O = 0;
				}
				break;
			case 0:
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i-combo_counter_X-1>=0 && j-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_X-1).at(j-combo_counter_X-1)!=-1 )
						{
							main_combo_X = combo_counter_X;
							n_X_combos[main_combo_X]++;
						}
						else
						{
							main_combo_X = combo_counter_X;
							n_X_combos[main_combo_X]+=0.5;
						}
					}
					else
					{
						main_combo_X = combo_counter_X;
						n_X_combos[main_combo_X]+=0.5;
					}
					combo_counter_X = 0;
				}
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i-combo_counter_O-1>=0 && j-combo_counter_O-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_O-1).at(j-combo_counter_O-1)!=1 )
						{
							main_combo_O = combo_counter_O;
							n_O_combos[main_combo_O]++;
						}
						else
						{
							main_combo_O = combo_counter_O;
							n_O_combos[main_combo_O]+=0.5;
						}
					}
					else
					{
						main_combo_O = combo_counter_O;
						n_O_combos[main_combo_O]+=0.5;
					}
					combo_counter_O = 0;
				}
				break;
			case -1:
				combo_counter_O++;
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i-combo_counter_X-1>=0 && j-combo_counter_X-1>=0 )	//check if combo is not blocked by opponent
					{
						if( t.at(i-combo_counter_X-1).at(j-combo_counter_X-1)!=-1 )
						{
							main_combo_X = combo_counter_X;
							n_X_combos[main_combo_X]+=0.5;
						}
					}
					combo_counter_X = 0;
				}
				break;
			default: break;
			}
		}
		if( combo_counter_X>0 )
		{
			if( combo_counter_X==gComboSize ) return WIN_COMBO;
			if( i-combo_counter_X-1>=0 && j-combo_counter_X-1>=0 )
			{
				if(t.at(i-combo_counter_X-1).at(j-combo_counter_X-1)!=-1)
				{
					main_combo_X = combo_counter_X;
					n_X_combos[main_combo_X]+=0.5;
				}
			}
		}
		if( combo_counter_O>0 )
		{
			if( combo_counter_O==gComboSize ) return -WIN_COMBO;
			if( i-combo_counter_O-1>=0 && j-combo_counter_O-1>=0 )
			{
				if(t.at(i-combo_counter_O-1).at(j-combo_counter_O-1)!=1)
				{
					main_combo_O = combo_counter_O;
					n_O_combos[main_combo_O]+=0.5;
				}
			}
		}
	}
	//Main diagonal analysis: Upper part
	for(int k=gComboSize-1; k<gSize-1; k++)
	{
		int i=k;
		combo_counter_O = 0;
		combo_counter_X = 0;
		int j=0;
		for(j=gSize-1; j>=0 && i>=0; i--, j--)
		{
			switch( t.at(i).at(j) )
			{
			case 1:
				combo_counter_X++;
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i+combo_counter_O+1<gSize && j+combo_counter_O+1<gSize )	//check if combo is not blocked by opponent
					{
						if( t.at(i+combo_counter_O+1).at(j+combo_counter_O+1)!=1 )
						{
							main_combo_O = combo_counter_O;
							n_O_combos[main_combo_O]+=0.5;
						}
					}
					combo_counter_O = 0;
				}
				break;
			case 0:
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i+combo_counter_X+1<gSize && j+combo_counter_X+1<gSize )	//check if combo is not blocked by opponent
					{
						if( t.at(i+combo_counter_X+1).at(j+combo_counter_X+1)!=-1 )
						{
							main_combo_X = combo_counter_X;
							n_X_combos[main_combo_X]++;
						}
						else
						{
							main_combo_X = combo_counter_X;
							n_X_combos[main_combo_X]+=0.5;
						}
					}
					else
					{
						main_combo_X = combo_counter_X;
						n_X_combos[main_combo_X]+=0.5;
					}
					combo_counter_X = 0;
				}
				if( combo_counter_O>0 )
				{
					if( combo_counter_O>=gComboSize ) return -WIN_COMBO;
					if( i+combo_counter_O+1<gSize && j+combo_counter_O+1<gSize )	//check if combo is not blocked by opponent
					{
						if( t.at(i+combo_counter_O+1).at(j+combo_counter_O+1)!=1 )
						{
							main_combo_O = combo_counter_O;
							n_O_combos[main_combo_O]++;
						}
						else
						{
							main_combo_O = combo_counter_O;
							n_O_combos[main_combo_O]+=0.5;
						}
					}
					else
					{
						main_combo_O = combo_counter_O;
						n_O_combos[main_combo_O]+=0.5;
					}
					combo_counter_O = 0;
				}
				break;
			case -1:
				combo_counter_O++;
				if( combo_counter_X>0 )
				{
					if( combo_counter_X>=gComboSize ) return WIN_COMBO;
					if( i+combo_counter_X+1<gSize && j+combo_counter_X+1<gSize )	//check if combo is not blocked by opponent
					{
						if( t.at(i+combo_counter_X+1).at(j+combo_counter_X+1)!=-1 )
						{
							main_combo_X = combo_counter_X;
							n_X_combos[main_combo_X]+=0.5;
						}
					}
					combo_counter_X = 0;
				}
				break;
			default: break;
			}
		}
		if( combo_counter_X>0 )
		{
			if( combo_counter_X==gComboSize ) return WIN_COMBO;
			if( i+combo_counter_X+1<gSize && j+combo_counter_X+1<gSize )
			{
				if(t.at(i+combo_counter_X+1).at(j+combo_counter_X+1)!=-1)
				{
					main_combo_X = combo_counter_X;
					n_X_combos[main_combo_X]+=0.5;
				}
			}
		}
		if( combo_counter_O>0 )
		{
			if( combo_counter_O==gComboSize ) return -WIN_COMBO;
			if( i+combo_counter_O+1<gSize && j+combo_counter_O+1<gSize )
			{
				if(t.at(i+combo_counter_O+1).at(j+combo_counter_O+1)!=1)
				{
					main_combo_O = combo_counter_O;
					n_O_combos[main_combo_O]+=0.5;
				}
			}
		}
	}
	state.est = -(n_O_combos[2]+n_O_combos[3]*100+n_O_combos[4]*10000)+
		(n_X_combos[2]+n_X_combos[3]*100+n_X_combos[4]*10000);
	return state.est;
}

bool isAdjacent(short i, short j, short toX, short toY)
{
	if( i==toX && j==toY ) return false;
	if( i<0 || j<0 || i>=gSize || j>=gSize ) return false;
	if( abs(toX - i)==1 || abs(toY - j)==1 )return true;
	else return false;
}

int prepareOnlineGame(char* char_address)
{
	DWORD exitCode = 0;
	HANDLE hThread= NULL;
	if( isClient )
	{
		if( gClientSock==NULL)
			gClientSock = new SocketClient();
		printf("Connecting to server...\n");
		hThread = (HANDLE)_beginthreadex(NULL,0,(THREAD_FUNC)connectToServer, (void*)char_address, 0, NULL);
		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &exitCode);
		CloseHandle(hThread);
		if( exitCode==STILL_ACTIVE || exitCode==SOCKET_ERROR )
		{
			fprintf(stderr, "Can't connect to server\n");
			return 1;
		}
		printf("Connected!\n");
		//Client should receive from server who starts the game
		if( gClientSock->receiveMessage(&msg)!=0 )
		{
			fprintf(stderr, "Server disconnected!\n");
			return 1;
		}
		if( msg.record[0]=='$' )
			gameState.x_turn = ( msg.record[1]=='1' ? true : false );
	}
	else
	{
		if( gServSock==NULL )
			gServSock = new SocketServer();
		printf("Server runned. Waiting for client connections...\n");
		hThread = (HANDLE)_beginthreadex(NULL,0,(THREAD_FUNC)createServer,NULL,0,NULL);
		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &exitCode);
		CloseHandle(hThread);
		if( exitCode!=0 )
		{
			fprintf(stderr, "Can't create server\n");
			return 1;
		}
		//Server always decides who starts the game
		if( rand()%2==1 )
			gameState.x_turn = true;
		else
			gameState.x_turn = false;
		sprintf((char*)msg.record, "$%d", (gameState.x_turn ? 1 : 0));
		if( gServSock->sendMessage(&msg)!=0 )
		{
			fprintf(stderr, "Client disconnected!\n");
			return 1;
		}
	}
	return 0;
}

int closeOnlineSession()
{
	SAFE_DELETE(gClientSock);
	SAFE_DELETE(gServSock);
	return 0;
}

static DWORD connectToServer(char* char_address)
{
	if( gClientSock )
		return gClientSock->connectToServer(char_address);
	else
		return -1;
}

static DWORD createServer()
{
	DWORD exitCode = 0;
	if( gServSock )
	{
		exitCode = gServSock->bindTo(INADDR_ANY);
		exitCode = gServSock->beginListen();
		exitCode = gServSock->acceptConnection();
	}
	else
		exitCode = -1;
	return exitCode;
}

void printTable(GameState& state)
{
	for(int i=0; i<gSize; i++)
	{
		for(int j=0; j<gSize; j++)
		{
			if( state.t.at(i).at(j)==1 )
				cout<<" X ";
			else if( state.t.at(i).at(j)==-1 )
				cout<<" O ";
			else cout<<"   ";
			cout<<'|';
		}
		cout<<endl;
		for(int j=0; j<gSize; j++)
			if( g_cursor.x==i && g_cursor.y==j )
			{
				SetConsoleTextAttribute(g_hCon, 9);
				cout<<"----";
				SetConsoleTextAttribute(g_hCon, 7);
			}
			else
				cout<<"----";
		cout<<endl;
	}
	if( !isGameOnline )
	{
		printf("X combos: 2 - %.1f\n"
					  "\t 3 - %.1f\n"
					  "\t 4 - %.1f\n"
			   "O combos: 2 - %.1f\n"
					  "\t 3 - %.1f\n"
					  "\t 4 - %.1f\n", state.x_combos.at(2), state.x_combos.at(3), state.x_combos.at(4),
					  state.o_combos.at(2), state.o_combos.at(3), state.o_combos.at(4));
	}
}

int playPvCGame()
{
	int result;
	bool goodInput;

	int input;
	result = getAIStateEstimation(gameState);
	while( abs(result)!=WIN_COMBO && gameState.nCount<gSize*gSize )	//while "not finished yet"
	{
		//system("cls");
		COORD cur_pos;
		cur_pos.X = 0;
		cur_pos.Y = 0;
		SetConsoleCursorPosition(g_hCon, cur_pos);
		printTable(gameState);
		cout<<"Current state estimation: "<<result<<endl;
		cout<<(gameState.x_turn ? "Your turn:" : "Computer is thinking...");
		if( gameState.x_turn )
		{
			goodInput = false;
			do 
			{
				//system("cls");
				COORD cur_pos;
				cur_pos.X = 0;
				cur_pos.Y = 0;
				SetConsoleCursorPosition(g_hCon, cur_pos);
				printTable(gameState);
				cout<<"Current state estimation: "<<result<<endl;
				cout<<(gameState.x_turn ? "Your turn:" : "Computer is thinking...");
				input = getch();
				switch(input)
				{
				case 52:	//Left
					g_cursor.y = (g_cursor.y>0 ? g_cursor.y-1 : g_cursor.y);
					break;
				case 54:	//Right
					g_cursor.y = (g_cursor.y<gSize-1 ? g_cursor.y+1 : g_cursor.y);
					break;
				case 56:	//Up
					g_cursor.x = (g_cursor.x>0 ? g_cursor.x-1 : g_cursor.x);
					break;
				case 50:	//Down
					g_cursor.x = (g_cursor.x<gSize-1 ? g_cursor.x+1 : g_cursor.x);
					break;
				case 13:
					if( gameState.t.at(g_cursor.x).at(g_cursor.y) )
					{
						cerr<<"Already occupied!\n";
						goodInput = false;
					}
					else
					{
						if( gameState.x_turn )
							gameState.t.at(g_cursor.x).at(g_cursor.y) = 1;
						else
							gameState.t.at(g_cursor.x).at(g_cursor.y) = -1;
						gameState.nCount++;
						gameState.x_turn = !gameState.x_turn;
						goodInput = true;
					}
					break;
				}
			} while( !goodInput );
			result=getAIStateEstimation(gameState);
		}
		else
		{
			if( gameState.nCount==0 )
			{
				gameState.t.at(gSize/2).at(gSize/2)=-1;
				gameState.x_turn = !gameState.x_turn;
			}
			else
				gameState = getAIAction(gameState, gAIDepth);
				//gameState = getThreadAIAction(gameState, X_Turn, gAIDepth);
			gameState.nCount++;
			result=getAIStateEstimation(gameState);
		}
	}
	system("cls");
	printTable(gameState);
	if( result==WIN_COMBO ) cout<<"X won!\n";
	else
		if( result==-WIN_COMBO ) cout<<"O won!\n";
		else
			if( result==0 ) cout<<"Draw!\n";
	return result;
}

int playPvPGame()
{
	int result;
	bool goodInput;
	int input;
	int player_x, player_y;
	result = getAIStateEstimation(gameState);
	while( abs(result)!=WIN_COMBO && gameState.nCount<gSize*gSize )	//while "not finished yet"
	{
		//system("cls");
		COORD cur_pos;
		cur_pos.X = 0;
		cur_pos.Y = 0;
		SetConsoleCursorPosition(g_hCon, cur_pos);
		printTable(gameState);
		if( isClient )
			if( gameState.x_turn )
				cout<<"Opponent is thinking...\n";
			else
				cout<<"Your turn: \n";
		else
			if( gameState.x_turn )
				cout<<"Your turn: \n";
			else
				cout<<"Opponent is thinking...\n";
		if( (!gameState.x_turn && isClient) || (gameState.x_turn && !isClient) )
		{
			goodInput = false;
			do 
			{
				//system("cls");
				COORD cur_pos;
				cur_pos.X = 0;
				cur_pos.Y = 0;
				SetConsoleCursorPosition(g_hCon, cur_pos);
				printTable(gameState);
				cout<<"Your turn: ";
				input = getch();
				switch(input)
				{
				case 52:	//Left
					g_cursor.y = (g_cursor.y>0 ? g_cursor.y-1 : g_cursor.y);
					break;
				case 54:	//Right
					g_cursor.y = (g_cursor.y<gSize-1 ? g_cursor.y+1 : g_cursor.y);
					break;
				case 56:	//Up
					g_cursor.x = (g_cursor.x>0 ? g_cursor.x-1 : g_cursor.x);
					break;
				case 50:	//Down
					g_cursor.x = (g_cursor.x<gSize-1 ? g_cursor.x+1 : g_cursor.x);
					break;
				case 13:
					if( gameState.t.at(g_cursor.x).at(g_cursor.y) )
					{
						cerr<<"Already occupied!\n";
						goodInput = false;
					}
					else
					{
						if( gameState.x_turn )
							gameState.t.at(g_cursor.x).at(g_cursor.y) = 1;
						else
							gameState.t.at(g_cursor.x).at(g_cursor.y) = -1;
						gameState.nCount++;
						goodInput = true;
						if(	sendMyAction(g_cursor.x, g_cursor.y)!=0 )	//send my action to another player
						{
							fprintf(stderr, "Your opponent has been disconnected!\n");
							return 0;
						}
					}
					break;
				}
			} while( !goodInput );
			result=getAIStateEstimation(gameState);
		}
		else
		{
			player_x = -1;
			player_y = -1;
			if(	getPlayerAction(gameState, player_x, player_y)!=0 )
			{
				fprintf(stderr, "Opponent has been disconnected!\n");
				return 0;
			}
			if( player_x==-1 || player_y==-1 ) return 1;	//smth. wrong happened
			gameState.t.at(player_x).at(player_y) = (gameState.x_turn ? 1 : -1);
			gameState.nCount++;
			result=getAIStateEstimation(gameState);
		}
		gameState.x_turn = !gameState.x_turn;
	}
	system("cls");
	printTable(gameState);
	if( result==WIN_COMBO ) cout<<"X won!\n";
	else
		if( result==-WIN_COMBO ) cout<<"O won!\n";
		else
			if( result==0 ) cout<<"Draw!\n";
	return result;
}

int getPlayerAction(GameState& state, int &x, int &y)
{
	int res = 0;
	if( isClient )
		res = gClientSock->receiveMessage(&msg);
	else
		res = gServSock->receiveMessage(&msg);
	if( res==0 )
	{
		if( !strcmp((char*)msg.record, "$Quit") )
			return -1;	//opponent has been disconnected!
		int i=-1, j=-1;
		istringstream inStr((char*)msg.record, istringstream::in);
		inStr>>x>>y;
	}
	return res;
}

int sendMyAction(int x, int y)
{
	sprintf((char*)msg.record, "%d %d", x, y);
	if( isClient )
		return gClientSock->sendMessage(&msg);
	else
		return gServSock->sendMessage(&msg);
}

void cleanTable(GameState& state)
{
	for(int i=0; i<gSize; i++)
		for(int j=0; j<gSize; j++)
			state.t.at(i).at(j) = 0;
	state.nCount = 0;
}

BOOL WINAPI Handler(DWORD CtrlEvent)
{
	clog<<"Application has been terminated using Handler!\n";
	strcpy((char*)msg.record, "$Quit");
	if( isClient )
	{
		if( gClientSock )
			gClientSock->sendMessage(&msg);
	}
	else
	{
		if( gServSock )
			gServSock->sendMessage(&msg);
	}
	return TRUE;
}

void createTableFromFile(GameState& state, string filename)
{
	vector<vector<short>>& t = state.t;
	ifstream inFile(filename.c_str(), ios::in);
	if( !inFile )
	{
		cerr<<"Can't open file!\n";
		return;
	}
	inFile.seekg(0);

	inFile>>gSize;
	t.resize(gSize);
	char mark;
	for(int i=0; i<gSize;i++)
		t.at(i).resize(gSize);
	for(int i=0; i<gSize;i++)
		for(int j=0; j<gSize; j++)
		{
			inFile>>mark;
			switch(mark)
			{
			case 'X':
				t.at(i).at(j) = 1;
				break;
			case 'O':
				t.at(i).at(j) = -1;
				break;
			case '_':
				t.at(i).at(j) = 0;
				break;
			default:
				break;
			}
		}
	inFile.close();
}