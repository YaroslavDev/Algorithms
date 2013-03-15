#include <crtdbg.h>

#include <Windows.h>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>

using namespace std;

vector<vector<int>> table;
vector<vector<list<int>>> map;
string inFile, outFile;

void printTable(ostream& out);	//prints current result(current filled sudoku)
void excludeValue(vector<vector<list<int>>>& Map, int i, int j, int value);	//excludes wrong values
bool fillTable(vector<vector<int>> &table,vector<vector<list<int>>>& Map);	//puts value in cells
bool isComplete(vector<vector<int>> &table);
int findAugment(int &x, int &y);
bool isSudokuCorrect(vector<vector<int>> &table);

//Recursive Sudoku solver function
void solveSudoku(vector<vector<int>>& Table, vector<vector<list<int>>>& Map, bool &solved);
//Sudoku Solver strategies
bool NakedSingles(vector<vector<int>> &Table,vector<vector<list<int>>>& Map);
bool HiddenSingles(vector<vector<int>> &Table,vector<vector<list<int>>>& Map);
bool NakedPairs(vector<vector<int>> &Table,vector<vector<list<int>>>& Map);

int main(int argc, char* argv[])
{
#if defined(_DEBUG) || defined(DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	if( argc > 3 )
	{
		printf("Usage: Sudoku_Solver <infile>.txt <outfile>.txt\n"
			   "     : Sudoku_Solver <infile>.txt\n"
			   "     : Sudoku_Solver \n");
		exit(1);
	} else
		if( argc==3 )
		{
			inFile.assign(argv[1]);
			outFile.assign(argv[2]);
		} else
			if( argc==2 )
			{
				inFile.assign(argv[1]);
				outFile.assign("output_Sudoku.txt");
			} else
			  {
				 inFile.assign("input_Sudoku.txt");
				 outFile.assign("output_Sudoku.txt");
			  }
	//Setup data table
	table.resize(9);
	for(int i=0; i<table.size(); i++)
		table[i].resize(9);
	//Read data from file
	ifstream inData(inFile, ios::in);
	inData.seekg(0);
	for(int i=0; i<9; i++)
		for(int j=0; j<9; j++)
			inData>>table.at(i).at(j);
	inData.close();
#if defined(_DEBUG)
	printTable(cout);
#endif

	//Create initial mapping. It contains possible values for every cell. 9x9 cells
	//and in every can be 9 possible values. Totally 9x9x9 = 3^6 = 729 array.
	//Our scope is to eliminate all unnecessary values in that way so for every cell
	//we have the only one value!
	map.resize(9);
	for(int i=0; i<9; i++)
	{
		map.at(i).resize(9);
		for(int j=0; j<9; j++)
			for(int k=1; k<10; k++)
				map.at(i).at(j).push_back(k);
	}

	//Modify mapping in accordance with input table
	for(int i=0; i<9; i++)
		for(int j=0; j<9; j++)
			if( table.at(i).at(j)!=0 )
			{
				excludeValue(map, i, j, table.at(i).at(j));
			}
	bool isFilled = false;
	bool isDone = false;
	solveSudoku(table, map, isFilled);

	//Final verification if computed answer is correct sudoku
	bool isCorrect = isSudokuCorrect(table);

	ofstream outFile(outFile, ios::out);
	outFile.seekp(0);
	printTable(outFile);
	outFile<<"This SUDOKU is "<<( isCorrect ? "correct" : "incorrect" )<<"!\n";
	outFile.close();
#if defined(_DEBUG) || defined(DEBUG)
	system("Pause");
#endif
	return 0;
}

int findAugment(int &x, int &y)
{
	int fillData[10];
	int augment, smallest_count;
	ZeroMemory(fillData, 10*sizeof(int));
	for(int i=0; i<9; i++)
		for(int j=0; j<9; j++)
		{
			fillData[table.at(i).at(j)]++;
		}
	augment = 1;
	smallest_count = fillData[1];
	for(int k=2; k<10; k++)
	{
		if( smallest_count>fillData[k] )	//smallest_count>fillData[k] should be!
		{
			smallest_count = fillData[k];
			augment=k;
		}
	}
	//Augment found! Now find x,y coords to put it!
	list<int>::iterator it, end;
	int best_i = -1, best_j = -1; //special initial values
	smallest_count = 32767;
	for(int i=0; i<9; i++)
		for(int j=0; j<9; j++)
		{
			it = map.at(i).at(j).begin();
			end = map.at(i).at(j).end();
			while( it != end )
			{
				if( *it == augment )
					if( smallest_count>map.at(i).at(j).size() )
					{
						smallest_count = map.at(i).at(j).size();
						best_i = i;
						best_j = j;
					}
				it++;
			}
		}
	x = best_i;
	y = best_j;
	return augment;
}

bool isComplete(vector<vector<int>> &Table)
{
	bool filled = true;
	for(int i=0; i<9 && filled; i++)
		for(int j=0; j<9 && filled; j++)
			if(Table.at(i).at(j)==0) 
			{
				filled = false;
				break;
			}
	return filled;
}

bool fillTable(vector<vector<int>> &Table,vector<vector<list<int>>>& Map)
{
	bool wasPutSmth = false;
	//Here is Solver's AI. The way solver puts values
	//Level 1: The most obvious. Just find cells for which we have the only possible value
	//for(int i=0; i<9; i++)
	//	for(int j=0; j<9; j++)
	//	{
	//		if( Map.at(i).at(j).size() == 1 )
	//		{
	//			Table.at(i).at(j) = Map.at(i).at(j).front();
	//			//map.at(i).at(j).clear();
	//			excludeValue(Map, i,j, Table.at(i).at(j));
	//			wasPutSmth = true;
	//		}
	//	}
	wasPutSmth = NakedSingles(Table, Map);

	//Level 2: Try to find cells with specific values which other cells don't have
	//Find in horizontal rows.
	//struct counter
	//{
	//	int count_val;
	//	int count_cell;
	//	int pos;
	//};
	//counter counts[10];
	//list<int>::iterator it, end;
	//for(int i=0; i<9; i++)
	//{
	//	ZeroMemory(counts, 10*sizeof(counter));
	//	for(int j=0; j<9; j++)
	//	{
	//		it = Map.at(i).at(j).begin();
	//		end = Map.at(i).at(j).end();
	//		while( it != end )
	//		{
	//			counts[ *it ].count_val++;
	//			counts[ *it ].pos = j;
	//			it++;
	//		}
	//	}
	//	for(int k=1; k<10; k++)
	//		if( counts[k].count_val==1 )	//values which have count 1 are possible candidates
	//		{
	//			counts[ counts[k].pos+1 ].count_cell++;
	//		}

	//	for(int k=1; k<10; k++)
	//		if( counts[k].count_val==1 && counts[counts[k].pos+1].count_cell==1 )
	//		{
	//			Table.at(i).at(counts[k].pos) = k;
	//			excludeValue(Map, i, counts[k].pos, Table.at(i).at(counts[k].pos));
	//			wasPutSmth = true;
	//		}
	//}
	////Find in vertical columns
	//for(int i=0; i<9; i++)
	//{
	//	ZeroMemory(counts, 10*sizeof(counter));
	//	for(int j=0; j<9; j++)
	//	{
	//		it = Map.at(j).at(i).begin();
	//		end = Map.at(j).at(i).end();
	//		while( it != end )
	//		{
	//			counts[ *it ].count_val++;
	//			counts[ *it ].pos = j;
	//			it++;
	//		}
	//	}
	//	for(int k=1; k<10; k++)
	//		if( counts[k].count_val==1 )	//values which have count 1 are possible candidates
	//		{
	//			counts[ counts[k].pos+1 ].count_cell++;
	//		}

	//	for(int k=1; k<10; k++)
	//		if( counts[k].count_val==1 && counts[counts[k].pos+1].count_cell==1 )
	//		{
	//			Table.at(counts[k].pos).at(i) = k;
	//			excludeValue(Map, counts[k].pos, i, Table.at(counts[k].pos).at(i));
	//			wasPutSmth = true;
	//		}
	//}
	////Find in local squares
	//int global_x, global_y;
	//int i,j;
	//int i_put, j_put;
	//for(int M=0; M<9; M++)
	//{
	//	ZeroMemory(counts, 10*sizeof(counter));
	//	i = M%3;
	//	j = M/3;
	//	for(int x=0; x<3; x++)
	//		for(int y=0; y<3; y++)
	//		{
	//			global_x = i*3 + x;
	//			global_y = j*3 + y;
	//			it = Map.at(global_x).at(global_y).begin();
	//			end = Map.at(global_x).at(global_y).end();
	//			while( it != end )
	//			{
	//				counts[ *it ].count_val++;
	//				counts[ *it ].pos = x*3+y;
	//				it++;
	//			}
	//		}
	//	for(int k=1; k<10; k++)
	//		if( counts[k].count_val==1 )
	//		{
	//			counts[ counts[k].pos+1 ].count_cell++;
	//		}
	//	for(int k=1; k<10; k++)
	//		if( counts[k].count_val==1 && counts[k].count_cell==1 )
	//		{
	//			i_put = M%3 * 3 + counts[k].pos/3;
	//			j_put = M/3 * 3 + counts[k].pos%3;
	//			Table.at(i_put).at(j_put) = k;
	//			excludeValue(Map, i_put, j_put, Table.at(i_put).at(j_put));
	//			wasPutSmth = true;
	//		}
	//}
	wasPutSmth = HiddenSingles(Table, Map);

	wasPutSmth = NakedPairs(Table, Map);

	return !wasPutSmth;
}

void excludeValue(vector<vector<list<int>>>& Map, int i, int j, int value)
{
	int val = value - 1; //normalized value(usually used 1-9, but in array we have 0-8)
	//Step 1: exclude all entries in vertical column
	list<int>::iterator it, end;
	for(int x=0; x<9; x++)
	{
		it = Map.at(x).at(j).begin();
		end = Map.at(x).at(j).end();
		while( it != end )
		{
			if( *it == value ) 
			{
				Map.at(x).at(j).erase(it);
				break;
			}
			it++;
		}
	}

	//Step 2: exclude all entries from horizontal row
	for(int x=0; x<9; x++)
	{
		it = Map.at(i).at(x).begin();
		end = Map.at(i).at(x).end();
		while( it != end )
		{
			if( *it == value )
			{
				Map.at(i).at(x).erase(it);
				break;
			}
			it++;
		}
	}

	//Step 3: exclude all entries from local 3x3 square
	int local_x = i/3,
		local_y = j/3,
		global_x, global_y;
	for(int x=0; x<3; x++)
		for(int y=0; y<3; y++)
		{
			global_x = local_x*3 + x;
			global_y = local_y*3 + y;
			it = Map.at(global_x).at(global_y).begin();
			end = Map.at(global_x).at(global_y).end();
			while( it != end )
			{
				if( *it == value )
				{
					Map.at(global_x).at(global_y).erase(it);
					break;
				}
				it++;
			}
		}

	//Step 4: finally, exclude all entries from this (i,j) cell since it's already occupied
	Map.at(i).at(j).clear();
}

void printTable(ostream& out)
{
	system("cls");
	for(int i=0; i<table.size(); i++)
	{
		for(int j=0; j<table[i].size(); j++)
		{
			if( table.at(i).at(j)!=0 )
				out<<table.at(i).at(j);
			else
				out<<" ";
			out<<" ";
			if( (j+1)%3==0 && j<8 ) out<<" | ";
		}
		out<<endl;
		if( (i+1)%3==0 && i<8 )
		{
			for(int j=0; j<table[i].size(); j++)
			{
				out<<"--";
				if( (j+1)%3==0 && j<8 ) out<<"---";
			}
			out<<endl;
		}
	}
}

bool isSudokuCorrect(vector<vector<int>> &Table)
{
	bool isCorrect = true;
	int sum = 0;	//check sum is 45 = 1+2+..+8+9
	//horizontal check-sums
	for(int i=0; i<9; i++)
	{
		sum = 0;
		for(int j=0; j<9; j++)
			sum += Table.at(i).at(j);
		if( sum!=45 )
		{
			isCorrect = false;
			goto EXIT;
		}
	}
	//vertical check-sums
	for(int i=0; i<9; i++)
	{
		sum = 0;
		for(int j=0; j<9; j++)
			sum += Table.at(j).at(i);
		if( sum!=45 )
		{
			isCorrect = false;
			goto EXIT;
		}
	}
	//local 3x3 square check-sums
	int local_x, local_y, global_x, global_y;
	for(int M=0; M<9; M++)
	{
		sum = 0;
		local_x = M%3;
		local_y = M/3;
		for(int i=0; i<3; i++)
			for(int j=0; j<3; j++)
			{
				global_x = 3 * local_x + i;
				global_y = 3 * local_y + j;
				sum += Table.at(global_x).at(global_y);
			}
		if( sum!=45 )
		{
			isCorrect = false;
			goto EXIT;
		}
	}

EXIT:
	return isCorrect;
}

//Exponential algorithm - extremely slow!!!
void solveSudoku(vector<vector<int>>& Table, vector<vector<list<int>>>& Map, bool &solved)
{
	vector<vector<int>> _Table; /*= Table;*/ //local table
	vector<vector<list<int>>> _Map; /*= Map;*/ //local map
	list<int>::iterator it, end;
	//PART 1: Try to solve analytically
	bool isDone = false;
	while( !isDone )
	{
		isDone = fillTable(Table, Map);	//was _<var> , i.e. local
#if defined(_DEBUG)
		printTable(cout);
#endif
		/*for(int i=0; i<9; i++)
			for(int j=0; j<9; j++)
				if( Table.at(i).at(j)!=0 )
				{
					excludeValue(Map, i, j, Table.at(i).at(j));
				}*/
	}
	if( isComplete(Table) )
	{
		if( isSudokuCorrect(Table) )
		{
			table = Table;
			solved = true;//if true then change global variable and return
		}
	}
	else
	{
		_Table = Table;
		_Map = Map;
		//PART 2: Brute-Force search(BACKTRACKING)
		for(int i=0; i<9 && !solved; i++)
			for(int j=0; j<9 && !solved; j++)
				if( _Map.at(i).at(j).size() > 1 )
				{
					it = Map.at(i).at(j).begin();
					end = Map.at(i).at(j).end();
					while( it != end && !solved )
					{
						_Table.at(i).at(j) = *it; 
						excludeValue(_Map, i,j, _Table.at(i).at(j));
						solveSudoku(_Table, _Map, solved);

						_Table = Table;	//backup after returning of recursion, probably not found solution
						_Map = Map;	//but if found correct sudoku, then solved=true won't let any recursion calls happen

						it++;
					}
				}
	}
}

//===========================================================================
//Level 1 - "Naked singles"
// The most obvious. Just find cells for which we have the only possible value
//===========================================================================
bool NakedSingles(vector<vector<int>> &_Table,vector<vector<list<int>>>& _Map)
{
	bool wasPutSmth = false;
	for(int i=0; i<9; i++)
		for(int j=0; j<9; j++)
		{
			if( _Map.at(i).at(j).size() == 1 )
			{
				_Table.at(i).at(j) = _Map.at(i).at(j).front();
				//map.at(i).at(j).clear();
				excludeValue(_Map, i,j, _Table.at(i).at(j));
				wasPutSmth = true;
			}
		}
	return wasPutSmth; 
}
//===========================================================================
//Level 2 - "Hidden singles"
//Try to find cells with specific values which other cells don't have
//===========================================================================
bool HiddenSingles(vector<vector<int>> &Table,vector<vector<list<int>>>& Map)
{
	bool wasPutSmth = false;
	struct counter
	{
		int count_val;
		int count_cell;
		int pos;
	};
	counter counts[10];
	list<int>::iterator it, end;
	for(int i=0; i<9; i++)
	{
		ZeroMemory(counts, 10*sizeof(counter));
		for(int j=0; j<9; j++)
		{
			it = Map.at(i).at(j).begin();
			end = Map.at(i).at(j).end();
			while( it != end )
			{
				counts[ *it ].count_val++;
				counts[ *it ].pos = j;
				it++;
			}
		}
		for(int k=1; k<10; k++)
			if( counts[k].count_val==1 )	//values which have count 1 are possible candidates
			{
				counts[ counts[k].pos+1 ].count_cell++;
			}

		for(int k=1; k<10; k++)
			if( counts[k].count_val==1 && counts[counts[k].pos+1].count_cell==1 )
			{
				Table.at(i).at(counts[k].pos) = k;
				excludeValue(Map, i, counts[k].pos, Table.at(i).at(counts[k].pos));
				wasPutSmth = true;
			}
	}
	//Find in vertical columns
	for(int i=0; i<9; i++)
	{
		ZeroMemory(counts, 10*sizeof(counter));
		for(int j=0; j<9; j++)
		{
			it = Map.at(j).at(i).begin();
			end = Map.at(j).at(i).end();
			while( it != end )
			{
				counts[ *it ].count_val++;
				counts[ *it ].pos = j;
				it++;
			}
		}
		for(int k=1; k<10; k++)
			if( counts[k].count_val==1 )	//values which have count 1 are possible candidates
			{
				counts[ counts[k].pos+1 ].count_cell++;
			}

		for(int k=1; k<10; k++)
			if( counts[k].count_val==1 && counts[counts[k].pos+1].count_cell==1 )
			{
				Table.at(counts[k].pos).at(i) = k;
				excludeValue(Map, counts[k].pos, i, Table.at(counts[k].pos).at(i));
				wasPutSmth = true;
			}
	}
	//Find in local squares
	int global_x, global_y;
	int i,j;
	int i_put, j_put;
	for(int M=0; M<9; M++)
	{
		ZeroMemory(counts, 10*sizeof(counter));
		i = M%3;
		j = M/3;
		for(int x=0; x<3; x++)
			for(int y=0; y<3; y++)
			{
				global_x = i*3 + x;
				global_y = j*3 + y;
				it = Map.at(global_x).at(global_y).begin();
				end = Map.at(global_x).at(global_y).end();
				while( it != end )
				{
					counts[ *it ].count_val++;
					counts[ *it ].pos = x*3+y;
					it++;
				}
			}
		for(int k=1; k<10; k++)
			if( counts[k].count_val==1 )
			{
				counts[ counts[k].pos+1 ].count_cell++;
			}
		for(int k=1; k<10; k++)
			if( counts[k].count_val==1 && counts[k].count_cell==1 )
			{
				i_put = M%3 * 3 + counts[k].pos/3;
				j_put = M/3 * 3 + counts[k].pos%3;
				Table.at(i_put).at(j_put) = k;
				excludeValue(Map, i_put, j_put, Table.at(i_put).at(j_put));
				wasPutSmth = true;
			}
	}
	return wasPutSmth;
}

//TODO:
//NakedPairs can be extended for NakedTriples also!
bool NakedPairs(vector<vector<int>> &Table,vector<vector<list<int>>>& Map)
{
	bool wasPutSmth = false;
	list<int>::iterator it, end, pair_it;
	size_t currSize;
	//Horizontal traversals
	for(int i=0; i<9; i++)
	{
		for(int h=0; h<9-1; h++)
			for(int j=h+1; j<9; j++)
				if( Map.at(i).at(h).size()==2 && Map.at(i).at(j).size()==2)
					if( Map.at(i).at(h)==Map.at(i).at(j) )
					{
						for(int k=0; k<9; k++)
						{
							if( Map.at(i).at(k)!=Map.at(i).at(h) && Map.at(i).at(k).size()>1)
							{
								pair_it = Map.at(i).at(h).begin();
								currSize = Map.at(i).at(k).size();
								Map.at(i).at(k).remove(*pair_it);
								pair_it++;
								Map.at(i).at(k).remove(*pair_it);
								if( Map.at(i).at(k).size() < currSize )
									wasPutSmth = true;
							}
						}
					}
	}
	//Vertical traversals
	for(int i=0; i<9; i++)
	{
		for(int h=0; h<9-1; h++)
			for(int j=h+1; j<9; j++)
				if( Map.at(h).at(i).size()==2 && Map.at(j).at(i).size()==2)
					if( Map.at(h).at(i)==Map.at(j).at(i) )
					{
						for(int k=0; k<9; k++)
						{
							if( Map.at(k).at(i)!=Map.at(h).at(i) && Map.at(k).at(i).size()>1)
							{
								pair_it = Map.at(h).at(i).begin();
								currSize = Map.at(k).at(i).size();
								Map.at(k).at(i).remove(*pair_it);
								pair_it++;
								Map.at(k).at(i).remove(*pair_it);
								if( Map.at(k).at(i).size() < currSize )
									wasPutSmth = true;
							}
						}
					}
	}
	//TODO: 3x3 traversals
	int col1, row1, col2, row2, col3, row3;
	for(int i=0; i<9; i++)
		for(int h=0; h<9-1; h++)
			for(int j=h+1; j<9; j++)
			{
				col1 = i%3 * 3 + h%3;	//1st local square coords
				row1 = i/3 * 3 + h/3;

				col2 = i%3 * 3 + j%3;	//2nd local square coords
				row2 = i/3 * 3 + j/3;

				if( Map.at(row1).at(col1).size()==2 && Map.at(row2).at(col2).size()==2 )
					if( Map.at(row1).at(col1)==Map.at(row2).at(col2) )
					{
						for(int k=0; k<9; k++)
						{
							col3 = i%3 * 3 + k%3;
							row3 = i/3 * 3 + k/3;
							if( Map.at(row3).at(col3)!=Map.at(row1).at(col1) && Map.at(row3).at(col3).size()>1 )
							{
								pair_it = Map.at(row1).at(col1).begin();
								currSize = Map.at(row3).at(col3).size();
								Map.at(row3).at(col3).remove(*pair_it);
								pair_it++;
								Map.at(row3).at(col3).remove(*pair_it);
								if( Map.at(row3).at(col3).size() < currSize )
									wasPutSmth = true;
							}
						}
					}

			}

	return wasPutSmth;
}