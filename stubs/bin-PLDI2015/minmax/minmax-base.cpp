/**********************************************************/
/* This code is for PLDI-15 Artifact Evaluation only      */ 
/* and will be released with further copyright information*/ 
/* File: Basic sequential recursive version of minmax     */
/**********************************************************/

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>

//#include "harness.h"
#ifdef BLOCK_PROFILE
#include "blockprofiler.h"
BlockProfiler profiler;
#endif

long work = 0;
int g_ncheck = 12;

using namespace std;

#define BOARD_SIZE 4
#define POS_SIZE 17//Corresponding to BOARD_SIZE
#define NOMOVE 100

int pos_weights[POS_SIZE]={};

void init_board( char pboard[] )
{
  int i ;
  for( i = 1; i <= (BOARD_SIZE*BOARD_SIZE); i++) pboard[i] = '_';
}

void print_board(char pboard[] )
{
  int i;
  for (i = 1; i <= (BOARD_SIZE*BOARD_SIZE); i++)
  {
    printf("%c ",pboard[i]);
    if (i % BOARD_SIZE == 0)
      printf("\n");
  }
}

void print_weights( )
{
  int count =1;
  printf("\n*******Weighted array*****\n");
  while(count < POS_SIZE)
  {
    printf("%d\t",pos_weights[count++]);
  }
  printf("\n");
}

int check_win(char pboard[])
{
  int ret = 0;
  if ((pboard[1] == 'X' && pboard[2] == 'X' && pboard[3] == 'X' && pboard[4] == 'X') ||
      (pboard[5] == 'X' && pboard[6] == 'X' && pboard[7] == 'X' && pboard[8] == 'X') ||
      (pboard[9] == 'X' && pboard[10] == 'X' && pboard[11] == 'X' && pboard[12] == 'X') ||
      (pboard[13] == 'X' && pboard[14] == 'X' && pboard[15] == 'X' && pboard[16] == 'X') ||
      (pboard[1] == 'X' && pboard[5] == 'X' && pboard[9] == 'X' && pboard[13] == 'X') ||
      (pboard[2] == 'X' && pboard[6] == 'X' && pboard[10] == 'X' && pboard[14] == 'X') ||
      (pboard[3] == 'X' && pboard[7] == 'X' && pboard[11] == 'X' && pboard[15] == 'X') ||
      (pboard[4] == 'X' && pboard[8] == 'X' && pboard[12] == 'X' && pboard[16] == 'X') ||
      (pboard[1] == 'X' && pboard[6] == 'X' && pboard[11] == 'X' && pboard[16] == 'X') ||
      (pboard[4] == 'X' && pboard[7] == 'X' && pboard[10] == 'X' && pboard[13] == 'X'))
  {
    ret = 1;
  }
  else if ((pboard[1] == 'O' && pboard[2] == 'O' && pboard[3] == 'O' && pboard[4] == 'O') ||
           (pboard[5] == 'O' && pboard[6] == 'O' && pboard[7] == 'O' && pboard[8] == 'O') ||
           (pboard[9] == 'O' && pboard[10] == 'O' && pboard[11] == 'O' && pboard[12] == 'O') ||
           (pboard[13] == 'O' && pboard[14] == 'O' && pboard[15] == 'O' && pboard[16] == 'O') ||
           (pboard[1] == 'O' && pboard[5] == 'O' && pboard[9] == 'O' && pboard[13] == 'O') ||
           (pboard[2] == 'O' && pboard[6] == 'O' && pboard[10] == 'O' && pboard[14] == 'O') ||
           (pboard[3] == 'O' && pboard[7] == 'O' && pboard[11] == 'O' && pboard[15] == 'O') ||
           (pboard[4] == 'O' && pboard[8] == 'O' && pboard[12] == 'O' && pboard[16] == 'O') ||
           (pboard[1] == 'O' && pboard[6] == 'O' && pboard[11] == 'O' && pboard[16] == 'O') ||
           (pboard[4] == 'O' && pboard[7] == 'O' && pboard[10] == 'O' && pboard[13] == 'O'))
  {
    ret = -1;
  }

  return ret;
}

int check_draw(char pboard[])
{
  int win,ret=0;
  win = check_win(pboard);
  if ((win == 0) && (pboard[1] != '_') && (pboard[2] != '_') &&
      (pboard[3] != '_') && (pboard[4] != '_') && (pboard[5] != '_') &&
      (pboard[6] != '_') && (pboard[7] != '_') && (pboard[8] != '_') &&
      (pboard[9] != '_') && (pboard[10] != '_') && (pboard[11] != '_') &&
      (pboard[12] != '_') && (pboard[13] != '_') && (pboard[14] != '_') &&
      (pboard[15] != '_') && (pboard[16] != '_'))
  {
    ret = 1;
  }
  return ret;
}


int evaluationFunction(char pboard[],int *leaf_val,int depth,int player)
{
  int chk_endgame = 0;
  chk_endgame = check_win(pboard);

  if (chk_endgame == 1) {
    *leaf_val = 20 - depth;
  }
  else if (chk_endgame == (-1)) {
    *leaf_val = -(20 - depth);
  }
  else {
    chk_endgame = check_draw(pboard);

    if(chk_endgame)
    {
      *leaf_val = 0;
    }
  }

  return chk_endgame;
}

int chooseNextMove(int* position,char pboard[])
{
  int pos,ret=0;
  int currentpos = *position;
  *position = NOMOVE;

  for (pos = currentpos; pos <= (BOARD_SIZE*BOARD_SIZE); pos++)
  {
    if('_' == pboard[pos])
    {
      *position = pos;
      ret = 1;
      break;
    }
  }

  return ret;
}

int update_pos_weigh(int player,char board[])
{
  int minmaxpos = 1,i,ret;
  ret = chooseNextMove(&minmaxpos, board);
  if(!ret)
  {
    printf("\nPanic - cant choose next position*");
    return -1;
  }
  else
  {
    if(player % 2) //player 1
    {
      for( i =1;i <= (BOARD_SIZE*BOARD_SIZE);i++) //check
      {
        if( (pos_weights[i] > pos_weights[minmaxpos]) &&
           (board[i] == '_') )
          minmaxpos = i;
      }
    }
    else //player 2
    {
      for( i =1;i <= (BOARD_SIZE*BOARD_SIZE);i++) //check
      {
        if( (pos_weights[i] < pos_weights[minmaxpos]) &&
           (board[i] == '_'))
          minmaxpos = i;
      }
    }
  }
  return minmaxpos;
}


void minimax( int pos_weights1[],int player, char board[],
             int depth,int startpos)
{
  //Recursive function calls
  for (int pos = 1; pos <= g_ncheck; pos++)
  {

    if (board[pos] != '_') continue;

    int eval_ended,ret=0;
    int leaf_val = 0;
    char pboard[BOARD_SIZE * BOARD_SIZE + 1];
    int nplayer=0;

    if(depth == 0)
    {
      startpos = pos;
    }

    memcpy (pboard,board,BOARD_SIZE * BOARD_SIZE + 1);

    if(player == 2)
    {
      pboard[pos] = 'O';
      nplayer = 1;
    }
    else if(player == 1)
    {
      pboard[pos] = 'X';
      nplayer = 2;
    }

    //Check if the game is over or depth of analysis is reached
    eval_ended = evaluationFunction(pboard,&leaf_val,depth,player);

    if(eval_ended)
    {
      //Update the pos_weights array with the results
      if(leaf_val)
      {
        pos_weights[startpos] += leaf_val;
      }
      continue;
    }

    minimax(pos_weights,nplayer, pboard, depth + 1,startpos);
  }
}


/*Benchmark entrance called by harness*/
//int app_main(int argc, char** argv)
int main(int argc, char** argv)
{
  if (argc == 2) g_ncheck = atoi(argv[1]);
  printf("Run at most %d positions\n", g_ncheck);

  int pos, input, player=1,chk=0,ret=0;
  char board[BOARD_SIZE*BOARD_SIZE+1];
  init_board(board);
  print_board(board);


  //Harness::start_timing();
  for(pos = 1; pos <= (BOARD_SIZE*BOARD_SIZE); pos++)
  {
    if(pos == 1) input = 5;
    else input = ret;

    if (pos % 2 != 0) board[input] = 'X';
    else board[input] = 'O';

    /*Print the board after each turn*/
    printf("\n****Board:********\n");
    print_board(board);
    /*Check if game is over*/
    chk = check_win(board);

    if (chk == 1)
    {
      printf("Player X wins!\n");
      break;
    }
    else if (chk == -1)
    {
      printf("Player O wins!\n");
      break;
    }
    /*Compute next best move*/
    else if ((chk == 0) && (pos != BOARD_SIZE * BOARD_SIZE))
    {
      memset(pos_weights,0,sizeof(pos_weights));
      player = 1 + pos % 2;
      minimax(pos_weights,player, board, 0,0);
      print_weights();

    }
    else
    {
      printf("The game is tied!\n");
    }
    ret = update_pos_weigh(player,board);
    if(-1 == ret)
    {
      return -1;
    }
    printf("\nOptimal move for player %d is %d",player,ret);
  }

  //Harness::stop_timing();
#ifdef BLOCK_PROFILE
  profiler.output();
#endif
#ifdef TRACK_TRAVERSALS
  cout << "work: " << work << endl;
#endif

  return 0;
}


