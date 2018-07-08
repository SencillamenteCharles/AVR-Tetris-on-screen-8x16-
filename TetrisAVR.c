/*
 * TetrisAVR.c
 *
 * Created: 03/11/2017 12:53:34 p. m.
 *  Author: Charles Suarez Diego / Charles Suarez Ruben
 */ 
#define  F_CPU 1000000UL       //CPU clock frecuency at 1MHz
#include <avr/interrupt.h>	   //This library  handles interrupts
#include <avr/io.h>		       //The MCU version of stdio.h
#include <util/delay.h>		   //Handles a delay time between instructions

#define  Buzzer PIND2
#define CountForInterrupt 100 // always constant, gives time of 25.6ms for timer interrput
#define  Start  5			  //button of start in  PIND5 of MCU
 
//******************************************************************************
//___________________Program functions____________________________________________________

/*
We define a indentation in the functions for define a dependency, 
meaning that the indented functions under another are daugther functions
that a called only inside them
*/
//--For printing the display---------------
void printScreen(uint8_t arr[8][20],uint8_t repeat);
    void print8Matrix( uint8_t arrtop[8],uint8_t arrbottom[8]);
void ClearArray(uint8_t arr[8][20],uint8_t lengthX,uint8_t lengthY);
void startTetris();     //
void printSadFace();    //Displays a GAME OVER condition in the dot matrix
	
//--Drawing of tetriminos-------------------
void initGameTetriminos();  


int8_t moveTetrimino(uint8_t someTetrimino,uint8_t direction, int8_t originX,int8_t originY,uint8_t GameEspace[8][20]);
	int8_t shiftBlockX(uint8_t someTetrimino,int8_t originY,uint8_t GameEspace[8][20]);
	      //uint8_t Read4PinsPortD();
	int8_t AllowShiftBlockInX(uint8_t someTetrimino, int8_t originX,int8_t originY,uint8_t GameEspace[8][20]);
	void erraseTetriminoOnScreen(uint8_t someTetrimino,uint8_t direction, int8_t originX,int8_t originY,uint8_t GameEspace[8][20]);
	void RotateTetrimino(uint8_t someTetrimino, int8_t originX,int8_t originY,uint8_t GameEspace[8][20]);
	      //uint8_t Read4PinsPortD();
	int8_t CheckIfTetriminoFits(uint8_t someTetrimino,uint8_t direction, int8_t originX,int8_t originY,uint8_t GameEspace[8][20]);//returns 1 if the blokc fits  autorizing its drawing on screen ,0 if not
    void drawTetrimino(uint8_t someTetrimino,uint8_t direction, int8_t originX,int8_t originY,uint8_t GameEspace[8][20]);

// Game puntuations
int8_t tetrisScore(uint8_t tetrisScreen[8][20]);
    void MakeScreenDescend(uint8_t tetrisScreen[8][20],int8_t fromLine);
int8_t gameOver(uint8_t tetrisScreen[8][20]);	

//-- Pure Hardware related------------------
void timer_initialization();// sets timer counter and its interrupts notice that theres a ISR for timer
uint8_t Read4PinsPortD();//Reads the buttons of the game

//---Pseudo Random funtions------------------
int ramdomNumber(uint8_t rangoInicial,uint8_t rangoFinal);

//---Global variables-----------------------
volatile int8_t makeBlockFall=15;//this flag makes the block fag is used by timer interrupt vector and moveTetrimino function
int8_t posInX=1;//this variable is manipulated only by f shiftblockX, define the x position of the tetrimino
uint8_t ActualTetrimino;
int8_t blockFallSpeed=20;
int8_t lastSpeed=20;//remebers last speed of the block, because down button
int8_t CounterForBlockFalls=0;
int8_t AntibounceFlag=0;
int8_t AllowNewFallOfBlock=1;
uint8_t Message[8][20]={ 	};
int8_t OnePressONeRotation=0;	


 struct
 {
	int8_t rotate:1;
	int8_t left:1;//unused
	int8_t rigth:1;
 }AkeyWasPressed;


	  
typedef struct 
{
	uint8_t blockMatrix[4][4];
}tetrimino;


tetrimino GameTetriminos[7]; //Creates an array with the dot information of the pieces
//hardware functions
void DeclareInputsOutputs(); //Declaration of the IO's of the MCU

int main(void)
{
	timer_initialization();  //enables the timer that runs at 25ms	, this timer is used for aleatory numbers and make blok
	DeclareInputsOutputs();
	AkeyWasPressed.rotate=0; //for remembering if rotate was pressed before 
	while(Read4PinsPortD()!=Start)
	{  //will end the loop , when start button is pressed
	    startTetris(); 
	}
    uint8_t gameBoard[8][20]={ 	}; //this array holds the tetris gameplay for the user
		
	initGameTetriminos();//sets figures
	makeBlockFall=15; 
    ActualTetrimino=ramdomNumber(0,7);
    while(1)
    {
        while (  gameOver(gameBoard)==0)
		{
			moveTetrimino(ActualTetrimino,0,posInX,makeBlockFall,gameBoard);
			//moveTetrimino(ActualTetrimino,0,posInX,makeBlockFall,gameBoard);
		    tetrisScore(gameBoard);
		}	
		while(Read4PinsPortD()!=Start)//game over, 
		   printSadFace();		
	      blockFallSpeed=20;
		  lastSpeed=20;
		
		
    }
}

void printScreen(uint8_t arr[8][20],uint8_t repeat)
{
	uint8_t screenTop[8];
	uint8_t screenBottom[8];
	
	for (uint8_t i=0; i<repeat;i++)
	{
		uint8_t i,j;
	//printing the bottom screen;
	uint8_t aux_impresion;
	for (i=0;i<8;i++)
	   { 
        aux_impresion=0;
		for (j=0;j<8;j++)
		    aux_impresion|=arr[i][j]<<j;
		 screenBottom[i]=aux_impresion;	
	   }			  
	
	// printing the top screen 
	for (i=0;i<8;i++)
	   { 
        aux_impresion=0;
		for (j=8;j<16;j++)
		    aux_impresion|=arr[i][j]<<(j-8);
		 screenTop[i]=aux_impresion;	
	   }			  
	print8Matrix(screenTop,screenBottom);
	}
}

void print8Matrix( uint8_t arrtop[8],uint8_t arrbottom[8])
{
	uint8_t i;
	PORTA=0;// for columns
	PORTB=255; //for rows in Matrix of top
	PORTC=255; //fot rows in Matrix of bottom
	for (i=0;i<8;i++)
	{
		PORTA=0;//sets al columns off, for watch
		PORTA|=1<<i;//only  one column at a time
	     PORTB=~arrtop[i];  //
		 PORTC=~arrbottom[i];
		 _delay_ms(2);
		
	    
	 }	
	PORTA=0;//initialize 
	PORTB=255;     
	PORTC=255;
	        
}

void DeclareInputsOutputs(){
	DDRA=255;//controls columns
	DDRB=255;
	DDRC=255;
	DDRD|=1<<PIND0|1<<PIND2|1<<PIND3;//led for indicate timer, and output for buzzer
	PORTD|=1<<PIND4|1<<PIND5|1<<PIND6|1<<PIND7;//for input/game buttons
}

uint8_t blockDidntFit=0;

int8_t moveTetrimino(uint8_t someTetrimino,uint8_t direction, int8_t originX,int8_t originY,uint8_t GameEspace[8][20]){
    int8_t shiftBlockXAux=shiftBlockX(someTetrimino, originY, GameEspace);
	   PORTD&=~(1<<Buzzer);
       RotateTetrimino(someTetrimino, shiftBlockXAux, originY, GameEspace);
  
  
   // int8_t shiftBlockXAux=shiftBlockX(someTetrimino, originY, GameEspace);
	   PORTD&=~(1<<Buzzer);
	   if(CheckIfTetriminoFits(someTetrimino, direction, shiftBlockXAux, originY, GameEspace)==1)//if the tetrimino does not fits the fall is over
	   {
		  blockDidntFit=0;  
		 drawTetrimino( someTetrimino, direction, shiftBlockXAux, originY, GameEspace);
	     printScreen(GameEspace,7);
		 erraseTetriminoOnScreen(someTetrimino, direction, shiftBlockXAux, originY, GameEspace);
		 AllowNewFallOfBlock=1;
		
       }	
	   //THE challenge is where to print the tetrimino when it does not fits
	   else 
	   {
		    blockDidntFit=1;
		   makeBlockFall=15;
		    OnePressONeRotation=1;//
		   ActualTetrimino=ramdomNumber(0,6);//asking a new tetrimino
		   OnePressONeRotation=0;
		   RotateTetrimino(ActualTetrimino, shiftBlockXAux, originY, GameEspace);
		   
		   posInX=3;
		  drawTetrimino( someTetrimino, direction, originX, originY+1, GameEspace);
		   printScreen(GameEspace,1);
	   }	
 
	return 0;
}

void initGameTetriminos(){
	//first we set every tetrimino to cero
	for(uint8_t n=0;n<7;n++)
		for (uint8_t i=0;i<4;i++)
		  for (uint8_t j=0;j<4;j++)
			GameTetriminos[n].blockMatrix[i][j]=0;
	//INITIALIZARTION OF EVERY TETRIMINO OR BLOCK
	
	
	//l block
	GameTetriminos[0].blockMatrix[1][0]=1;	
	GameTetriminos[0].blockMatrix[2][0]=1;
	GameTetriminos[0].blockMatrix[1][1]=1;
	GameTetriminos[0].blockMatrix[1][2]=1;
	//then J block
	GameTetriminos[1].blockMatrix[0][0]=1;	
	GameTetriminos[1].blockMatrix[1][0]=1;
	GameTetriminos[1].blockMatrix[1][1]=1;
	GameTetriminos[1].blockMatrix[1][2]=1;
	// t block
	GameTetriminos[2].blockMatrix[1][1]=1;	
	GameTetriminos[2].blockMatrix[2][1]=1;
	GameTetriminos[2].blockMatrix[3][1]=1;
	GameTetriminos[2].blockMatrix[2][2]=1;
	//S block
	GameTetriminos[3].blockMatrix[0][0]=1;	
	GameTetriminos[3].blockMatrix[0][1]=1;
	GameTetriminos[3].blockMatrix[1][1]=1;
	GameTetriminos[3].blockMatrix[1][2]=1;
	//Z block
	GameTetriminos[4].blockMatrix[1][0]=1;	
	GameTetriminos[4].blockMatrix[1][1]=1;
	GameTetriminos[4].blockMatrix[0][1]=1;
	GameTetriminos[4].blockMatrix[0][2]=1;
	//o block
	GameTetriminos[5].blockMatrix[2][1]=1;	
	GameTetriminos[5].blockMatrix[2][2]=1;
	GameTetriminos[5].blockMatrix[1][1]=1;
	GameTetriminos[5].blockMatrix[1][2]=1;
	//I block
	GameTetriminos[6].blockMatrix[2][0]=1;	
	GameTetriminos[6].blockMatrix[2][1]=1;
	GameTetriminos[6].blockMatrix[2][2]=1;
	GameTetriminos[6].blockMatrix[2][3]=1;
	
}	

void ClearArray(uint8_t arr[8][20],uint8_t lengthX,uint8_t lengthY)
{
	uint8_t i,j;
	for (i=0;i<lengthX;i++)
	   for (j=0;j<lengthY;j++)
	      arr[i][j]=0;
}

void timer_initialization()
{
	/*
	1 Set timer (TIMER 1 OF 16 BITS COUNT) prescaler, for having his own clock speed, if we dont
	 the timer will run at the same speed of CPU, of 1MHz, we want the timer run at a speed that let us 
	 count at least 100ms 
	 
	 find out
	*/
	TCCR1B|=1<<CS12;//See datasheet page 317, and table 48, Setting a preescaler of 256
	TCCR1A=0;
	/*	2 set Timer 1 operation mode, in this case we need mode CTC, or Clear Timer on Compare Match
	This mode will allow us to say to the timer: Giveme a count of 1000 steps not 65535, and when you reach that value
	i want you start over, clearing your count, and then tell me you did that, with your powerfull interruption vector
	*/
	TCCR1B|=1<<WGM12;//see datasheet table 47 page 112, here we set the Wave Generation Mode 12, wich is mode CTC of the timer
	
	/*
	3. We want the timer, tell us when reach our specified count of 1000 steps
	*/
	OCR1A=CountForInterrupt;//OK timer give 1000 steps on your the channel A
	TIMSK|=1<< OCIE1A;//PAGE 115,OK, Timer , now i just enabled your interrupts in your the channel A
	TIFR|1<<OCF1A;
	sei();
	
}
//INTERRUPT SERVICE ROUTINE OF THE TIMER
ISR(TIMER1_COMPA_vect){
	/*	NOTE:
	THIS ISR is excecuted each 25.6 ms
	*/
	PORTD^=1<<PIND0;//Debug led
	//AkeyWasPressed.rigth=0;
	// automatic block fall
	CounterForBlockFalls= (CounterForBlockFalls+1)%(blockFallSpeed+1);
	if (CounterForBlockFalls>=(blockFallSpeed) && AllowNewFallOfBlock==1)
	{
		makeBlockFall--;
		AllowNewFallOfBlock=0;
		if (makeBlockFall<-4)
		makeBlockFall=15;
		
	}	
	if (Read4PinsPortD()==6 )
	{
		if (blockFallSpeed!=3)
		{
			lastSpeed= blockFallSpeed;
		}
		blockFallSpeed=3;
		
	}		
	else
	  blockFallSpeed=lastSpeed;
	
}

uint8_t Read4PinsPortD()
{
  
	 if ((PIND&(1<<PIND7))==0)// && AkeyWasPressed.rigth==0)
	 {
		 
		 //AkeyWasPressed.rigth=1;
		 return 7;//rigth
	 }		 
	 else if ((PIND&(1<<PIND6))==0)
	 {
		 
		 return 6;//down
	 }		 
	 else if ((PIND&(1<<PIND5))==0 )// && AkeyWasPressed.rotate==0)
	 {
		 //AkeyWasPressed.rotate=1;//
		 return 5;//up rotate
	 }		 
	 else if ((PIND&(1<<PIND4))==0 )
	 {
		 //AkeyWasPressed.left=1;
		 return 4;	 //left
	 }		 
  //}		 
	 else
	 {
		// AkeyWasPressed.rotate=0;
		 //AkeyWasPressed.left=0;
		 //AkeyWasPressed.rigth=0;
		return	0;//means none of the buttons is being pressed
	 }	
	// AkeyWasPressed.rotate=((PIND&(1<<PIND5))==0)?1:0;//remembers logic state of button rotate	
}

int ramdomNumber(uint8_t rangoInicial,uint8_t rangoFinal)
{
	//uses the timer for gerenate ramdom numbers
	return (TCNT1%(rangoFinal+1)) + rangoInicial;
	
}

void drawTetrimino(uint8_t someTetrimino,uint8_t direction, int8_t originX,int8_t originY,uint8_t GameEspace[8][20])
{
	int8_t i2=0,j2=0;
	for(int8_t i=originX;i<originX+4;i++,i2++)
	{	
		j2=0;
	  for (int8_t j=originY ;j<originY+4;j++,j2++)
	      if( i<8 && j<16 && i>=0 && j>=0)//setting only the tetrimino figure
		  {
		     if(GameEspace[i][j]==0)
				 GameEspace[i][j]=GameTetriminos[someTetrimino].blockMatrix[i2][j2];
		  }			  
	}	  
	 
	 
	
}

int8_t CheckIfTetriminoFits(uint8_t someTetrimino,uint8_t direction, int8_t originX,int8_t originY,uint8_t GameEspace[8][20])
{
	
	uint8_t i2=0,j2=0;
	
	for(int8_t i=originX;i<originX+4;i++,i2++)
	{	
		j2=0;
	  for (int8_t j=originY ;j<originY+4;j++,j2++)
	  {
		 if (( (GameTetriminos[someTetrimino].blockMatrix[i2][j2]==1) &&  (i>7) )|| ( (GameTetriminos[someTetrimino].blockMatrix[i2][j2]==1) &&  (i<0) )  )
		           return 0;
		  if((GameTetriminos[someTetrimino].blockMatrix[i2][j2]==1) && (j<=-1))
		  {
			    makeBlockFall=15;
		         return 0;
			
		  }	
			 			   
		  if(i2>=0 && i2<=7 && j2>=0 && j2<=15 && i>=0 && i<=7 && j>=0 && j<=15 )//avoid checking on inexistent matrix ranges
			  if((GameTetriminos[someTetrimino].blockMatrix[i2][j2]==1)&& (GameEspace[i][j]==1))//if there is a bit on means that...
						return 0;					
	  }		   
	}	  
	return 1;

}  


void erraseTetriminoOnScreen(uint8_t someTetrimino,uint8_t direction, int8_t originX,int8_t originY,uint8_t GameEspace[8][20])
{
	int8_t i2=0,j2=0;
	for(int8_t i=originX;i<originX+4;i++,i2++)
	{	
		j2=0;
	  for (int8_t j=originY ;j<originY+4;j++,j2++)
		if(i2>=0 && i2<=7 && j2>=0 && j2<=15 && i>=0 && i<=7 && j>=0 && j<=15 )//avoind checking on inexistent matrix ranges
	      if(GameTetriminos[someTetrimino].blockMatrix[i2][j2]==1)
	          GameEspace[i][j]=0;
	}	  
}

int8_t shiftBlockX(uint8_t someTetrimino,int8_t originY,uint8_t GameEspace[8][20])
{
	
	if(Read4PinsPortD()==4 )//if the user press letf on push button on pin PD4
	{
		 PORTD|=1<<Buzzer;
	  if(AllowShiftBlockInX( someTetrimino, posInX-1, originY, GameEspace))//check future position of x	
		{  
           posInX--;
		  if(posInX<-3)
		   posInX=-3;
		}		   
	}	   
	 else if(Read4PinsPortD()==7) //if the user press rigth 
	 {
		PORTD|=1<<Buzzer;
		 if (AllowShiftBlockInX( someTetrimino, posInX+1, originY, GameEspace))
		 {
			 
		   posInX++;
		   if(posInX>7)
			  posInX=7;
		 }			  
	 }
	 return posInX;		  
}

int8_t tetrisScore(uint8_t tetrisScreen[8][20])
{
  /*
  this function analyzes the  tetris screen
  and if finds a row full, increments the
  row score, if the row score reach s 
  four, the player is ascended to a new level
  
  */
     
  uint8_t rowScore=0;
    for (uint8_t j=0;j<16;j++)
		  {
			  
			   uint8_t PointsInrow=0;
			   for (uint8_t i=0;i<8;i++)
			   {
				
				   if(tetrisScreen[i][j]==1)
					   PointsInrow++;
				 if(PointsInrow>=8)
				 {
				 PORTD|=1<<PIND3;	  
			     MakeScreenDescend(tetrisScreen,j);
				 
				 
				 }
					 
			   }
			   rowScore=(PointsInrow==8)?rowScore+1:rowScore;
			
			   
		  }	 
	    
	if (rowScore>=5)
		PORTD|=1<<PIND3;	
	
			
	 
	//if the function reaches this part, means that there is a
	return 0;
}

int8_t AllowShiftBlockInX(uint8_t someTetrimino, int8_t originX,int8_t originY,uint8_t GameEspace[8][20])
{
	int8_t i2=0,j2=0;//this elements are used for the 4x4 of the n matrix that constainst a tetrimino
	
	for(int8_t i=originX;i<originX+4;i++,i2++)
	{	
		j2=0;
	  for (int8_t j=originY ;j<originY+4;j++,j2++)// j places  on a desired coordinate of the GameSpace
	  {
		  if (( (GameTetriminos[someTetrimino].blockMatrix[i2][j2]==1) &&  (i>7) )|| ( (GameTetriminos[someTetrimino].blockMatrix[i2][j2]==1) &&  (i<0) )  )
		           return 0;
		  if(i2>=0 && i2<=7 && j2>=0 && j2<=15 && i>=0 && i<=7 && j>=0 && j<=15 )//avoid checking on inexistent matrix ranges
			{  if((GameTetriminos[someTetrimino].blockMatrix[i2][j2]==1)&& (GameEspace[i][j]==1))//if there is a bit on means that...
						return 0;//deny shitf on that place
								
			}						
				
	  }		   
	}	  
	return 1;
}	





void RotateTetrimino(uint8_t someTetrimino,int8_t originX,int8_t originY,uint8_t GameEspace[8][20])
{
	
	if(Read4PinsPortD()==5 && OnePressONeRotation==0 )
	{
		OnePressONeRotation=1;
	//first we copy the matrix that contains a n tetrimino
		uint8_t x,y,CopyMatrix[4][4],aux;
		for (x=0;x<4;x++)
		{
			for (y=0;y<4;y++)
				CopyMatrix[x][y]=GameTetriminos[someTetrimino].blockMatrix[x][y];
		}				
		
		//then we overwrite the matrix that holds the tetrimino
		//by changin rows by column, expecting rotate the tetrimino to the left
		for (y=0,aux=3;y<4;y++,aux--)
			for (x=0;x<4;x++)
				GameTetriminos[someTetrimino].blockMatrix[aux][x]=CopyMatrix[x][y];	
				 
		//checking if the new rotation fits		 	
		if(CheckIfTetriminoFits( someTetrimino,0,  originX,originY,GameEspace)==0)//if the new rotation doesnot fit, we revert  the rotation
		{
			for (x=0;x<4;x++)
			{
				for (y=0;y<4;y++)
					GameTetriminos[someTetrimino].blockMatrix[x][y]=CopyMatrix[x][y];
			}
		}
	}				

  		if (Read4PinsPortD()==0)//if the button was not pressed autorize 1 rotation
  		{
			  OnePressONeRotation=0;
  		}
	
}

void MakeScreenDescend(uint8_t tetrisScreen[8][20],int8_t fromLine)
{
	int8_t x,y;
	for (y=fromLine;y<16;y++)
	     for (x=0;x<8;x++)
	     {   
			if (y<15)
				 tetrisScreen[x][y]=tetrisScreen[x][y+1];
			else//when is 15
				 tetrisScreen[x][y]=0;
	     }
		       blockFallSpeed-=1;
			     lastSpeed-=1;
				 if (blockFallSpeed<=0 || lastSpeed<=0)
					 {
						blockFallSpeed=1;
					  lastSpeed=1; 
					 }
}
	
void startTetris()
{
  	//prints T E T R I S on screen
	ClearArray(Message,8,20);
	//s	  
    Message[1][0]=1;		
	Message[1][1]=1;	
	Message[1][2]=1;
    Message[1][3]=1; 
	Message[1][4]=1;
	//S	
	Message[4][0]=1;
    Message[5][0]=1; 
	Message[6][0]=1;	
	Message[6][1]=1;
    Message[6][2]=1; 
	Message[5][2]=1; 
	Message[4][2]=1; 
	Message[4][3]=1; 
	Message[4][4]=1;
	Message[5][4]=1;  
	//
	Message[6][4]=1; 
	//T
	Message[6][0]=1;	
	Message[1][6]=1;
    Message[1][7]=1; 
	Message[1][8]=1; 
	Message[1][9]=1; 
	Message[0][9]=1; 
	Message[2][9]=1;
	//R
	Message[4][6]=1;	
	Message[4][7]=1;
    Message[4][8]=1; 
	Message[4][9]=1; 
	Message[5][7]=1; 
	Message[5][9]=1; 
	Message[6][8]=1;
	Message[6][9]=1;
	Message[6][6]=1;
	//T
	Message[1][11]=1;	
	Message[1][12]=1;
    Message[1][13]=1; 
	Message[1][14]=1; 
	Message[1][15]=1; 
	Message[0][15]=1; 
	Message[2][15]=1;
	//E
	Message[4][11]=1;
	Message[5][11]=1;
    Message[6][11]=1;	
	Message[4][12]=1;
    Message[4][13]=1; 
	Message[4][14]=1; 
	Message[4][15]=1; 
	Message[5][15]=1;
	Message[6][15]=1;
	Message[5][13]=1;
	Message[6][13]=1;
	  printScreen(Message,1); 
}


int8_t countForGameOver=0;


int8_t gameOver(uint8_t tetrisScreen[8][20])
{
	
	if (makeBlockFall==15 && blockDidntFit==1)
	{
		countForGameOver++;
		if (countForGameOver>=2)
		{
			//you lose
			ClearArray(tetrisScreen,8,16);
			return 1;
		}
	}
	else
	   countForGameOver=0;
	   return 0;
}

void printSadFace()
{
	
	ClearArray(Message,8,20);	  
    Message[1][12]=1;		
	Message[2][12]=1;	
	Message[5][12]=1;
    Message[6][12]=1; 
	Message[1][13]=1;		
	Message[2][13]=1;	
	Message[5][13]=1;
    Message[6][13]=1; 
	
	Message[4][8]=1;	
	Message[3][8]=1;
	
    Message[3][7]=1; 
	Message[4][7]=1;	
	Message[5][7]=1;
    Message[2][7]=1;
	 
	Message[1][6]=1; 
	
	Message[6][6]=1; 
	Message[0][5]=1; 
	Message[7][5]=1;
		
	  printScreen(Message,1);   
}//666 THE NUMBER OF THE BEAST