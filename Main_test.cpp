#include <iostream>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <termios.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "colors.h"
#include "Matrix.h"

using namespace std;


/**************************************************************/
/**************** Linux System Functions **********************/
/**************************************************************/

char saved_key = 0;
int tty_raw(int fd);	/* put terminal into a raw mode */
int tty_reset(int fd);	/* restore terminal's mode */
  
/* Read 1 character - echo defines echo mode */
char getch() {
  char ch;
  int n;
  while (1) {
    tty_raw(0);
    n = read(0, &ch, 1);
    tty_reset(0);
    if (n > 0)
      break;
    else if (n < 0) {
      if (errno == EINTR) {
        if (saved_key != 0) {
          ch = saved_key;
          saved_key = 0;
          break;
        }
      }
    }
  }
  return ch;
}

void sigint_handler(int signo) {
  // cout << "SIGINT received!" << endl;
  // do nothing;
}

void sigalrm_handler(int signo) {
  alarm(1);
  saved_key = 's';
}

void registerInterrupt() {
  struct sigaction act, oact;
  act.sa_handler = sigint_handler;
  sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
  act.sa_flags = SA_INTERRUPT;
#else
  act.sa_flags = 0;
#endif
  if (sigaction(SIGINT, &act, &oact) < 0) {
    cerr << "sigaction error" << endl;
    exit(1);
  }
}

void registerAlarm() {
  struct sigaction act, oact;
  act.sa_handler = sigalrm_handler;
  sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
  act.sa_flags = SA_INTERRUPT;
#else
  act.sa_flags = 0;
#endif
  if (sigaction(SIGALRM, &act, &oact) < 0) {
    cerr << "sigaction error" << endl;
    exit(1);
  }
  alarm(1);
}

/**************************************************************/
/**************** Tetris Blocks Definitions *******************/
/**************************************************************/
#define MAX_BLK_TYPES 7
#define MAX_BLK_DEGREES 4

int T0D0[] = { 1, 1, 1, 1, -1 };
int T0D1[] = { 1, 1, 1, 1, -1 };
int T0D2[] = { 1, 1, 1, 1, -1 };
int T0D3[] = { 1, 1, 1, 1, -1 };


int T1D0[] = { 0, 1, 0, 1, 1, 1, 0, 0, 0, -1 };
int T1D1[] = { 0, 1, 0, 0, 1, 1, 0, 1, 0, -1 };
int T1D2[] = { 0, 0, 0, 1, 1, 1, 0, 1, 0, -1 };
int T1D3[] = { 0, 1, 0, 1, 1, 0, 0, 1, 0, -1 };

int T2D0[] = { 1, 0, 0, 1, 1, 1, 0, 0, 0, -1 };
int T2D1[] = { 0, 1, 1, 0, 1, 0, 0, 1, 0, -1 };
int T2D2[] = { 0, 0, 0, 1, 1, 1, 0, 0, 1, -1 };
int T2D3[] = { 0, 1, 0, 0, 1, 0, 1, 1, 0, -1 };

int T3D0[] = { 0, 0, 1, 1, 1, 1, 0, 0, 0, -1 };
int T3D1[] = { 0, 1, 0, 0, 1, 0, 0, 1, 1, -1 };
int T3D2[] = { 0, 0, 0, 1, 1, 1, 1, 0, 0, -1 };
int T3D3[] = { 1, 1, 0, 0, 1, 0, 0, 1, 0, -1 };

int T4D0[] = { 0, 1, 0, 1, 1, 0, 1, 0, 0, -1 };
int T4D1[] = { 1, 1, 0, 0, 1, 1, 0, 0, 0, -1 };
int T4D2[] = { 0, 1, 0, 1, 1, 0, 1, 0, 0, -1 };
int T4D3[] = { 1, 1, 0, 0, 1, 1, 0, 0, 0, -1 };

int T5D0[] = { 0, 1, 0, 0, 1, 1, 0, 0, 1, -1 };
int T5D1[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, -1 };
int T5D2[] = { 0, 1, 0, 0, 1, 1, 0, 0, 1, -1 };
int T5D3[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, -1 };

int T6D0[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1 };
int T6D1[] = { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1 };
int T6D2[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1 };
int T6D3[] = { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1 };
  
int *setOfBlockArrays[] = {
  T0D0, T0D1, T0D2, T0D3,
  T1D0, T1D1, T1D2, T1D3,
  T2D0, T2D1, T2D2, T2D3,
  T3D0, T3D1, T3D2, T3D3,
  T4D0, T4D1, T4D2, T4D3,
  T5D0, T5D1, T5D2, T5D3,
  T6D0, T6D1, T6D2, T6D3,
};

void drawScreen(Matrix *screen, int wall_depth)
{
  int dy = screen->get_dy();
  int dx = screen->get_dx();
  int dw = wall_depth;
  int **array = screen->get_array();

  for (int y = 0; y < dy - dw + 3; y++) {
    for (int x = dw - 3; x < dx - dw + 3; x++) {
      if (array[y][x] == 0)
	      cout << "□ ";
      else if (array[y][x] == 1)
        cout << "■ ";
      else if (array[y][x] == 10)
	      cout << "◈ ";
      else if (array[y][x] == 20)
	      cout << "★ ";
      else if (array[y][x] == 30)
	      cout << "● ";
      else if (array[y][x] == 40)
	      cout << "◆ ";
      else if (array[y][x] == 50)
	      cout << "▲ ";
      else if (array[y][x] == 60)
	      cout << "♣ ";
      else if (array[y][x] == 70)
	      cout << "♥ ";
      else
	      cout << "X ";
    }
    cout << endl;
  }
}

Matrix *deleteFullLines(Matrix *screen, int wall_depth){
  int dy = screen->get_dy();
  int dx = screen->get_dx();
  int dw = wall_depth;
  int FullLine_y;
  Matrix *full_up_temp; //new Matrix();
  Matrix *fullcheck_temp; //new Matrix();

  for (int i=0; i<dy-dw; i++){
    fullcheck_temp = screen->clip(i, dw, i+1, dx-dw);
    if (fullcheck_temp->sum() == 10){
      FullLine_y = i;
      full_up_temp = screen->clip(0, 0, FullLine_y, dy);
      screen->paste(full_up_temp, 1, 0);
      delete full_up_temp;
      
    } 
    delete fullcheck_temp;
  }
  return screen;
}

/**************************************************************/
/******************** Tetris Main Loop ************************/
/**************************************************************/

#define SCREEN_DY  10
#define SCREEN_DX  10
#define SCREEN_DW  3

#define ARRAY_DY (SCREEN_DY + SCREEN_DW)
#define ARRAY_DX (SCREEN_DX + 2*SCREEN_DW)

int arrayScreen[ARRAY_DY][ARRAY_DX] = {
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};



int main(int argc, char *argv[]) {
  char key;
  int blkType;
  int idxBlockDegree;
  int top = 0, left = 5;
  bool TouchDown = false;

  /*
  Matrix A((int*) arrayBlk, 3, 3);
  Matrix B(A);
  Matrix C(A);
  Matrix D;
  D = A + B + C;
  cout << D << endl;
  exit(0);
  */

  //테트리스 블럭 초기화
  Matrix *setofBlockObjects[MAX_BLK_TYPES][MAX_BLK_DEGREES];
  for (int i=0; i<MAX_BLK_TYPES; i++){ //0~6
    for (int j=0; j<MAX_BLK_DEGREES; j++){ //0~3
      int col = 3; int row = 3;
      if (i==0) {col=2; row=2;}
      else if (i==6) {col=4; row=4;}
      setofBlockObjects[i][j] = new Matrix(setOfBlockArrays[i*4 + j], col, row);
    }
  }
  
  //난수를 이용한 블럭타입 선택
  srand((unsigned int)time(NULL));
  blkType = rand() % MAX_BLK_TYPES;


  Matrix *iScreen = new Matrix((int *) arrayScreen, ARRAY_DY, ARRAY_DX);
  Matrix *currBlk = setofBlockObjects[blkType][0];
  Matrix *tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
  Matrix *tempBlk2 = tempBlk->add(currBlk);
  delete tempBlk;
  
  Matrix *oScreen = new Matrix(iScreen);
  oScreen->paste(tempBlk2, top, left);
  //delete tempBlk2;
  drawScreen(oScreen, SCREEN_DW); //테트리스 블럭출현
  delete oScreen;
  
  while ((key = getch()) != 'q') {
    switch (key) {
      case 'a': left--; break;
      case 'd': left++; break;
      case 's': top++; break;
      case 'w': 
        idxBlockDegree = (idxBlockDegree + 1) % 4;
        currBlk = setofBlockObjects[blkType][idxBlockDegree];
        break;
      case ' ': 
        while(1){
          top++; //스페이스 키 누르면 블록이 바닥까지 떨어지게
          delete tempBlk2;
          tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
          tempBlk2 = tempBlk->add(currBlk);
          delete tempBlk;
          
          //블록이 1(바닥이나 타 블록)과 충돌하면
          if (tempBlk2->anyGreaterThan(1)){
            top--; //(사후충돌처리) 위로 1 올리기
            if(top < 0){
              goto gameover;
            }
            TouchDown = true;
            break;
          }
        }          
        break;
      default: cout << "wrong key input" << endl;
    }
    delete tempBlk2;
    tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
    tempBlk2 = tempBlk->add(currBlk);
    delete tempBlk;

    //블록이 충돌하면
    if (tempBlk2->anyGreaterThan(1)){
      switch(key){
        case 'a': left++; break;
        case 'd': left--; break;
        case 's': 
          top--;
          if(top < 0){
            goto gameover;
          }
          TouchDown = true;
          break;
        case 'w': 
          //회전 시에 옆 벽면과 겹치면
          idxBlockDegree = (idxBlockDegree +3) % 4;
          currBlk = setofBlockObjects[blkType][idxBlockDegree];
          break;
        case ' ': break;
      }
      delete tempBlk2;
      tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
      tempBlk2 = tempBlk->add(currBlk);
      delete tempBlk;
    }
    
    //s, space키 바닥 닿았을 시 새로운 블럭 생성
    if (TouchDown == true){
      delete tempBlk2;
      tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
      tempBlk2 = tempBlk->add(currBlk);
      delete tempBlk;
      //바닥에 고정
      iScreen->paste(tempBlk2, top, left);
      delete tempBlk2;
      //new block
      iScreen = deleteFullLines(iScreen, SCREEN_DW);
      blkType = rand() % MAX_BLK_TYPES;
      currBlk = setofBlockObjects[blkType][0];
      top=0; left=6;
      tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
      tempBlk2 = tempBlk->add(currBlk);
      delete tempBlk;
      while(tempBlk2->anyGreaterThan(1)){
        if(top <= 0){
          goto gameover;
          }
        break;
      }
    }

    
    oScreen = new Matrix(iScreen);
    oScreen->paste(tempBlk2, top, left);
    drawScreen(oScreen, SCREEN_DW);
    delete oScreen;
  }
  
  gameover:
    cout << "===========GAME OVER===========" << endl;
    delete iScreen;
    delete tempBlk2;

    //블럭 객체 free
    for (int i=0; i<MAX_BLK_TYPES; i++){
      for (int j=0; j<MAX_BLK_DEGREES; j++){
        delete setofBlockObjects[i][j];
      }
    }

    cout << "(nAlloc, nFree) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ")" << endl;  
    cout << "Program terminated!" << endl;

    return 0;
}

