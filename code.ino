#include<mega128.h>
#include<delay.h>
#include<stdio.h>
#include<stdbool.h>

//lcd관련 
#asm
.equ __lcd_port=0x12 //portD 사용
#endasm
#include<lcd.h>
                      
//시간관련 세그먼트 선택
#define Q7 PORTB.4   //msb
#define Q6 PORTB.5
#define Q5 PORTB.6
#define Q4 PORTB.7   //lsb

//점수관련 세그먼트 선택
#define Q3 PORTB.0   //msb
#define Q2 PORTB.1
#define Q1 PORTB.2
#define Q0 PORTB.3   //lsb

unsigned char fnd1[10]={0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xd8, 0x80, 0x98}; // 순서대로 0~9 (도트 미포함)
unsigned char fnd2[10]={0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x18}; // 순서대로 0~9 (도트 포함)
int list[29] = {1500, 900, 850, 800, 750, 700, 650, 600, 550, 500, 475, 450, 425, 400, 375, 350, 325, 300, 275, 250, 225, 200, 175, 150, 125, 100, 75, 50, 25};

int disp=3000,tcnt=0; //disp는 시간, tcnt는 0.01초 카운트용

interrupt [TIM2_OVF] void timer2_ovf_isr(void)  //타이머2를 이용한 인터럽트, TCNT가 6에서 256을 가면 인터럽트가 발생하고 이 인터럽트 함수로 들어온다.들어올때마다 tcnt를 1씩증가시키고
{        int i;                                             //tcnt가 10이되면 즉,0.01초가 되면 숫자 1을 증가시킨다.             
        tcnt+=1;
        if(tcnt==10)
        {
         disp--;  //0.01초 감소
         tcnt=0;
        }  
            
        
for(i=0; i<=28; i++)
{
   if(disp==list[i])
    {
      if(i%2==0){PORTF=0xfb;}
      else {PORTF=0xff;}
    }
}
        
        
        
                       
}

void FndDisplay1(unsigned int i)   // 점수 FND
{
    PORTA = fnd1[i/1000];
    Q3=1, Q2=0, Q1=0, Q0=0;
    delay_ms(1);
    PORTA = fnd1[(i%1000)/100];
    Q3=0, Q2=1, Q1=0, Q0=0;
    delay_ms(1);
    PORTA = fnd1[(i%100)/10];
    Q3=0, Q2=0, Q1=1, Q0=0;
    delay_ms(1);
    PORTA = fnd1[(i%10)/1];
    Q3=0, Q2=0, Q1=0, Q0=1 ;
    delay_ms(1);
}


void FndDisplay2(unsigned int disp) //시간 FND
{
    PORTC = fnd1[disp/1000];
    Q7=1, Q6=0, Q5=0, Q4=0;
    delay_ms(1);
    PORTC = fnd2[(disp%1000)/100];
    Q7=0, Q6=1, Q5=0, Q4=0;
    delay_ms(1);
    PORTC = fnd1[(disp%100)/10];
    Q7=0, Q6=0, Q5=1, Q4=0;
    delay_ms(1);
    PORTC = fnd1[(disp%10)/1];
    Q7=0, Q6=0, Q5=0, Q4=1 ;
    delay_ms(1);
}

void FndFAIL() //제한시간내에 못할시 시간세그먼트에 FAIL표시
{
    PORTC = 0x8e;
    Q7=1, Q6=0, Q5=0, Q4=0;
    delay_ms(1);
    PORTC = 0x88;
    Q7=0, Q6=1, Q5=0, Q4=0;
    delay_ms(1);
    PORTC = 0xf9;
    Q7=0, Q6=0, Q5=1, Q4=0;
    delay_ms(1);
    PORTC = 0xc7;
    Q7=0, Q6=0, Q5=0, Q4=1;
    delay_ms(1);
}

void main(void)  //PORT관련 변수가 무조건 다른타입변수 아래 있어야함 (아니면 must declare first in block오류발생)
{
    int i, max=1000,str_max=0;
    bool button = false; // 버튼눌림감지 불리언
    bool start = false; // 입력감지 역할
     
      //LCD관련 변수
    int winner=3000,second_winner=3000,count=0;  // count는 맨처음 게임한사람이 무조건 1등이 되도록 하기위해 조건문 스위치 역할
    char sbuf1[16]; // LCD 1등 출력데이터 담을 변수
    char sbuf2[16]; // LCD 2등 출력데이터 담을 변수

    
    //센서 및 세그먼트 및 LCD 포트 초기화  
    unsigned int ADresult; // 압력센서 측정값 저장할 변수
    PORTA=0xff;  // 점수 세그먼트 불 꺼짐  
    DDRA=0xff;   // 점수 세그먼트 출력으로셋팅
    PORTB=0x00;  // 점수,시간 세그먼트 각각 4개 모두 선택 안함
    DDRB=0xff;   //하위 4비트는 점수fnd 선택, 상위 4비트는 시간fnd 선택
    PORTC=0xff;  // 시간 세그먼트 불 꺼짐
    DDRC=0xff;   // 시간 세그먼트 출력으로 세팅
    PORTF=0xfe;  //1111 1110 버튼,LED OFF
    DDRF=0xfe;   // 1111 1110   AD0(PF.0)는 압력센서핀으로 입력 PF.1는 버튼, PF.2,3은 LED로 출력셋팅
    PORTD=0xff;     // LCD를 모두ON
    DDRD=0xff;      // LCD이므로 PORTD 출력으로 셋팅
    lcd_init(16);   // 16비트 2줄 LCD관련 초기화 함수 , LCD 클리어 한 후 문자표시 위치 0,0으로 이동 

    //타이머관련 초기화
    TCCR2 = 0x03;   // 0000 0011  7은 항상0, 6,3은 normal모드(ocn과 상관없이 항상0~255까지 카운트하는)로 00, 5,4는 ocn쓰지않으므로 00,   2,1,0은 64분주비로하므로 011     
    TCNT2 = 6;   // 카운트 변수 초기치6으로 설정 이 변수가 6부터 1씩증가하면서 255까지 증가
    SREG = 0x80;    //global 인터럽트 인에이블인데 , 이게 맨앞이 1이어야  인터럽트 자체가 가능
    TIMSK = 0x00;   // 0000 0000  // 6번비트가(왼쪽에서두번째) 1이면 타이머2의 오버플로우 인터럽트를 활성시킴 (타이머 종류마다 다름)  초기치는 인터럽트 OFF

    //압력센서 초기화
    delay_ms(10);
    ADCSRA = 0x85; // 1000 0101
    ADMUX= 0x40;   // 0100 0000
    delay_us(150);

    while(1)
    {    
        if(start == true)  // 버튼누른 후 센서를 때려 게임시작된 경우
        {
             
            //압력센서 값전환부분 시작
            ADresult = 0;
            for(i=1; i<=8; i++)
            {
                ADCSRA = 0xD5;   // 1101 0101  좌측 1000 -> 1101
                while((ADCSRA & 0x10) == 0 ); // ADCSRA 좌측 LSB가 1되면 변환완료   라는 뜻이라 LSB가 1이면 넘어가라
                ADresult += (int)ADCL + ((int)ADCH<<8);   // 누적합
            
            }
            ADresult = (ADresult / 8);
            //압력센서 값전환부분 끝
             
            //압력센서에 손가락이 닿아있는 동안 들어온 값들 중 최대값을 저장하는 구간 시작 
            if((1023-ADresult)>str_max) str_max=(1023-ADresult);
            if((1023-ADresult)==0)
            {
                if((str_max)<3) str_max=0;
                else
                {
                    max=max-(str_max*4); 
                    str_max=0;
                } 
            }
            //압력센서에 손가락이 닿아있는 동안 들어온 값들 중 최대값을 저장하는 구간 끝 
                  
            FndDisplay1(max); // 세그먼트에 점수현황표시         
            FndDisplay2(disp);//  세그먼트에 진행시간표시
                

            if(disp <= 2 || max <= 0)     // 제한시간다되거나 점수 다 깎은경우  disp가 0일때 인터럽트를 멈추면 오류가 나서 2로 설정
            {
                TIMSK = 0x00; // 인터럽트 정지(타이머 정지)
                delay_ms(30); // delay를 넣어준 이유는 클록속도가 인터럽트차단 속도보다 빨라 시간이 계속가기 때문
                button=false; // 게임이 끝났으니 버튼을 누를 수 있는 상태로(false)
                start=false;  // 게임이 끝났으니 버튼을 누른후 입력감지를 받을 수 있도록(false)
                delay_us(10);
                lcd_clear();  // 순위를 표시하시전 LCD화면 초기화       
                   
                while(1)
                {   
                    if(button == false)  //게임이 끝난 후 버튼을 안눌렀을 때
                    {
                        if(disp<=2) //제한시간 다된경우
                        {
                            FndDisplay1(max);  //내 남은점수를 그대로 표시
                            FndFAIL();  // 시간 세그먼트에 FAIL표시 
                            
                            PORTF=0xfb;  //게임이 끝났으므로 LED에 빨간불
                        }
                        
                        if(max<=0)  // 점수다 깎은경우
                        {      
                            if(count==0)   // 맨처음게임 한사람의 점수는 무조건 1등으로 최고점수 업데이팅
                            {
                                if((3000-disp)<winner) winner=(3000-disp);
                            }                        
                            if(count==1) // 두번째 게임부터 1등 2등 업데이팅
                            { 
                                if((3000-disp)<winner){second_winner=winner; winner=(3000-disp);} 
                                else if((3000-disp)>winner && (3000-disp)<second_winner) second_winner=(3000-disp); 
                            }
                               
                            FndDisplay1(0);    // 내 점수는 0000으로 표시
                            FndDisplay2((3000-disp)); // 시간에 내 남은시간을 그대로 표시
                        }
                               

                                        
                        //LCD 코드 시작
                        lcd_gotoxy(0,0);
                        delay_us(10);
                        if(winner != 3000)
                        {sprintf(sbuf1,"1st : %d.%d",winner/100,winner%100);}
                        lcd_puts(sbuf1);
                        lcd_gotoxy(0,1);
                        delay_us(10);
                        if(second_winner != 3000)
                        {sprintf(sbuf2,"2st : %d.%d",second_winner/100,second_winner%100);}
                        lcd_puts(sbuf2);
                        //LCD코드 끝
                        PORTF=0xfb;  //게임이 끝났으므로 LED에 빨간불       
                    }                   
                    else // 게임끝난 후 버튼을 눌렀을 때
                    {
                        FndDisplay1(1000); // 세그먼트에 점수 1000으로 초기화
                        FndDisplay2(3000); // 세그먼트에 시간 30초로 초기화
                    }
                    
                    if(button==false && PINF.1 ==0 ) //게임끝난 후 버튼을 누를 경우 시간초와 점수, LCD화면을 초기화  
                    { 
                       button = true; disp=3000; max=1000; PORTF=0xf7; lcd_clear();
                    } 
 
                   
                    // 게임끝나고 버튼을 눌렀던 상태에서 센서입력받는 곳(시작)        
                    ADresult = 0;
                    for(i=1; i<=8; i++)
                    {
                        ADCSRA = 0xD5;   // 1101 0101  좌측 1000 -> 1101
                        while((ADCSRA & 0x10) == 0 ); // ADCSRA 좌측 LSB가 1되면 변환완료 라는 뜻, 즉 LSB가 1이면 넘어가라는 뜻
                        ADresult += (int)ADCL + ((int)ADCH<<8);   // 센서의 아날로그입력 8번 누적합                   
                    }
                    ADresult = (ADresult / 8); // 누적합을 8로나눠 평균을 구해 오차를 줄임
                    // 게임끝나고 버튼을 눌렀던 상태에서 센서입력받는 곳(끝)
                                   
                    //버튼을 눌렀던 상태에서 압력센서를 때리면 게임 시작
                    if(button==true && (1023-ADresult)>=3)
                    { 
                        TIMSK = 0x40; count=1;  
                        start=true;  break;
                    }
                           
                } // 작은 while
            } //if 게임이 끝났을때 or 시간이 다되었을때 
        } //if start
        else // 전원키고 맨 처음 게임시작 버튼을 누르지 않았을 때 표시할것들
        {   
            //LCD코드 시작
            lcd_gotoxy(4,0);
            delay_us(10);
            lcd_putsf("");
            //LCD코드 끝
            
            PORTF=0xf7; // 게임시작전이므로 초록불

            // 전원키고 맨처음 (셋/리셋)버튼 누른 후 센서입력받는 곳(시작)
            ADresult = 0;
            for(i=1; i<=8; i++)
            {
                ADCSRA = 0xD5;   // 1101 0101  좌측 1000 -> 1101
                while((ADCSRA & 0x10) == 0 ); // ADCSRA 좌측 4비트 LSB가 1되면 변환완료라는 뜻이라 LSB가 1이면 다음으로 넘어가라 라는 뜻 
                ADresult += (int)ADCL + ((int)ADCH<<8);   // 누적합        
            }
            ADresult = (ADresult / 8);
            // 전원키고 맨처음 (셋/리셋)버튼 누른 후 센서입력받는 곳(시작)

            if(button==false && PINF.1 ==0){  button = true;}      // 맨처음 전원키고 버튼을 눌렀다는 것을 알림                      
            if(button==true){FndDisplay1(max); FndDisplay2(disp);} // 맨처음 전원키고 버튼을 누른경우 세그먼트에 점수1000, 시간 30초 표시   
            if(button==true && (1023-ADresult)>=3)  // 맨처음 전원키고 버튼을 누른 후 압력센서를 때리면 게임시작
            {
                TIMSK = 0x40; start=true;
            }
         
        }
    } //큰 while
} //main void            
