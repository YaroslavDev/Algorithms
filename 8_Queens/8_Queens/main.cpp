#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <crtdbg.h>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

DWORD k_sol = 0; //total number of solutions!

vector<vector<int>> table;
vector<vector<int>> queens;
void putQueen(vector<vector<int>>& table, size_t col, size_t row);
void printTable();
void put8Queens(vector<vector<int>>& table,vector<vector<int>>& queens, int toPut, bool &found);
void printQueens(ostream &out);

int main(int argc, char* argv[])
{
#if defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF );
#endif 

	table.resize(8);
	for(size_t i=0; i<8; i++)
		table.at(i).resize(8);
	for(size_t i=0; i<8; i++)
		for(size_t j=0; j<8; j++)
			table.at(i).at(j) = 1;	// true - possible positions, false - impossible

	queens.resize(8);
	for(size_t i=0; i<8; i++)
		queens.at(i).resize(8);
	for(size_t i=0; i<8; i++)
		for(size_t j=0; j<8; j++)
			queens.at(i).at(j) = 0;	// 1 - queen put, 0 - free place

	bool found = false;
	put8Queens(table, queens, 7, found);
	printf("One of possible 8 queens arrangemens:\n");
	printQueens(cout);

#if defined(_DEBUG)
	system("Pause");
#endif
	return 0;
}

void printTable()
{
	for(size_t i=0; i<8; i++)
	{
		for(size_t j=0; j<8; j++)
			printf("%d ", table.at(i).at(j));
		printf("\n");
	}
}

void putQueen(vector<vector<int>>& table, size_t col, size_t row)
{
	//Horizontal traversal
	for(size_t i=0; i<8; i++)
		table.at(i).at(col) = 0;
	//Vertical traversal
	for(size_t j=0; j<8; j++)
		table.at(row).at(j) = 0;
	//1st diagonal traversal
	int _i,_j;
	if( row>col ) 
	{ 
		_i = row-col; 
		_j = 0; 
	}
	else		  
	{ 
		_i = 0; 
		_j = col-row; 
	}
	for(; _i<8&&_j<8; _i++,_j++)
		table.at(_i).at(_j) = 0;
	//2nd diagonal traversal
	if( row+col<=7 ) { _i = row+col; _j = 0; }
	else		  { _i = 7; _j = (row+col)-7; }
	for( ; _i>=0&&_j<8; _i--, _j++)
		table.at(_i).at(_j) = 0;
}

void put8Queens(vector<vector<int>>& Table,vector<vector<int>>& Queens, int toPut, bool &found)
{
	if( toPut == -1)
	{
		table = Table;
		queens = Queens;
		ostringstream os;
		os<<"Solutions\\solution_"<<k_sol<<".txt";
		ofstream outFile(os.str(), ios::out);
		outFile.seekp(0);
		printQueens(outFile);
		outFile.close();
		k_sol++;
		//found = true;
	}
	else
	{
		vector<vector<int>> _Table = Table; //local table
		vector<vector<int>> _Queens = Queens;
		for(int i=0; i<8 && !found; i++)
			for(int j=0; j<8 && !found; j++)
					if( _Table.at(i).at(j) ) //_Table.at(i).at(toPut)
					{
						putQueen(_Table, j, i); //putQueen(_Table, toPut, i); 
						_Queens.at(i).at(j) = 1; //_Queens.at(i).at(toPut) = 1;
						put8Queens(_Table, _Queens, toPut-1, found);
						_Table = Table;	//If we didn't find necessary combination, backup to previous version of table
						_Queens.at(i).at(j) = 0;
					}
	}
}

void printQueens(ostream &out)
{
	for(size_t i=0; i<queens.size(); i++)
	{
		for(size_t j=0; j<queens.at(i).size(); j++)
			out<<queens.at(i).at(j)<<" ";
		out<<endl;
	}
}