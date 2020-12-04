/*********---------------------------------------------------------------------------
		电子智能时钟程序
		单 片 机：STC89C52RC
		晶    振：12MHz
		时钟芯片：DS1302
		液 晶 屏：LCM-12864-ST7920
		时    间：2010年10月10日修改完成
		LCM12864使用并口连接方式,PSB、RST接高电平
------------------------------------------------------------------------------*/

/*-------------------------------头文件---------------------------------------*/
#include <reg52.h>
#include <intrins.h>
#include "LCD12864.h"
#include "DS1307.h"
#include "nongli.h"
#include "displaytime.h"
//#include "jieqi.h"
#define uint  unsigned int
#define uchar unsigned char

/*---------------------函数声明------------------------------*/
sbit K1  = P2^0; //K1-设置
sbit K2  = P2^1; //K2-确认、返回
sbit K3  = P2^2; //K3-加
sbit K4  = P2^3; //K4-减

sbit K5  = P2^4; //液晶背光控制按键，按一下亮，再按一下灭
sbit LCD = P3^6; //液晶背光控制输出，低电平有效，PNP三极管控制。
sbit beep = P3^7;
unsigned char  yy,mo,dd,xq,hh,mm,ss, naoh,naom,month_moon,day_moon,week,tiangan,dizhi,moontemp1,moontemp2;//定义时间映射全局变量（专用寄存器）
void Disp_Img(unsigned char code *img);
void DelayM(uint);       	
void ds_w(void);
void Conver_week(bit c,uchar year,uchar month,uchar day);
/*-----------------------------定义全局变量------------------------------*/
bit q=0, w=0,BEEP_bit = 0,flag = 0;    //调时标志位
signed char address,item,max,mini;
/******************************************************************************************/
/*-----------------------------延时函数 1MS/次-------------------------------*/
void DelayM(uint a)       	
{
	uchar i;
	while( --a != 0) { for(i = 0; i < 125; i++); }   				   
}
/*-----------------------------日期、时间设置函数-----------------------------*/

void tiaozheng(void)
{
	yy = read_clock(0x06);//调用1307时钟数据中的年数据，从地址0x06中
	mo = read_clock(0x05);//调用1307时钟数据中的月数据，从地址0x05中
	dd = read_clock(0x04);//从1307芯片中读取日数据，从地址0x04中
	week = read_clock(0x03);//从1307芯片中读取星期数据，从地址0x8b中
	lcm_w_test(0,0x80);

	lcm_w_word("20");//显示内容字符20
	write_data(yy/16+0x30);//函数参数1，代表本行写数据，YY/16+0X30得出年十位数字的显示码地址，送显示	
	write_data(yy%16+0x30);//函数
	lcm_w_word("年");

	write_data(mo/16+0x30);
	write_data(mo%16+0x30);//与16取余数，得到月份的个位数，加0x30得到该数字的液晶内定显示码送显示
	lcm_w_word("月");//调用字符显示函数，显示文字 月

	write_data(dd/16+0x30);
	write_data(dd%16+0x30);//第一个1参数，表示本行写数据，日数据与16取余得个位数，加0x30得到显示码
	lcm_w_word("日");//显示字符 日

	if(read_clock(0x02) != hh){//如果程序中的小时与1302芯片中的不同，
		hh = read_clock(0x02);//刷新程序中的小时数据
	}
	lcm_w_test(0,0x91);//第一个参数0，表示本行写入LCM的是指令，指定显示位置91H（第三行左端）
	write_data(hh/16+0x30);//显示十位
	write_data(hh%16+0x30);//显示个位
	lcm_w_word("时");

	if(read_clock(0x01) != mm){//如果1307芯片中的分钟数据与程序中的分钟变量不相等		
		mm = read_clock(0x01);//刷新程序中的分钟数据
	}
	write_data(mm/16+0x30);//向液晶写数据，显示分钟的十位数
	write_data(mm%16+0x30);//向液晶写数据，显示分钟的个位数
	lcm_w_word("分");

	if(read_clock(0x00) != ss){//如果1307芯片中的分钟数据与程序中的秒钟变量不相等		
		ss = read_clock(0x00);//刷新程序中的秒钟数据
	}
	write_data(ss/16+0x30);//向液晶写数据，显示分钟的十位数
	write_data(ss%16+0x30);//向液晶写数据，显示分钟的个位数
	lcm_w_word("秒");

	if(read_clock(0x08) != ss){//如果1307芯片中的分钟数据与程序中的秒钟变量不相等		
	   ss = read_clock(0x08);//刷新程序中的秒钟数据
	}
/********************************添加闹钟程序2013-06*****************************************/
	lcm_w_test(0,0x88);
	lcm_w_word("闹钟：");
	if(read_clock(0x08) != naoh){//如果1307芯片中的H钟数据与程序中的闹钟时变量不相等		
	  naoh = read_clock(0x08);//刷新程序中的H钟数据
	}
	write_data(naoh/16+0x30);//向液晶写数据，显示时钟的十位数
	write_data(naoh%16+0x30);//向液晶写数据，显示时钟的个位数
	lcm_w_word("时");

	if(read_clock(0x09) != naom){//如果1307芯片中的分钟数据与程序中的闹钟分变量不相等		
	  naom = read_clock(0x09);//刷新程序中的秒钟数据
	}
	write_data(naom/16+0x30);//向液晶写数据，显示分钟的十位数
	write_data(naom%16+0x30);//向液晶写数据，显示分钟的个位数
	lcm_w_word("分");
}

/**********************************************************************************************************/
//调整时间子函数，设置键、数据范围、上调加一，下调减一功能。
void Set_time(unsigned char sel){ //根据选择调整的相应项目加1并写入DS1307，函数参数是按动设置键的次数
  	uchar a = 0;
	write_com(0x30); write_com(0x06);

	lcm_w_test(0,0x98);//第一参数0表示本行写入指令，指定下面行的 调整 显示起始位置为98H
	lcm_w_word("★调整");//调用字符显示函数，显示 调整字样
	if(sel == 6){lcm_w_word("闹分");address=0x09; max=59;mini=0;
	tiaozheng();
   	ds_w();
   	tiaozheng();
	}
	if(sel == 5){lcm_w_word("闹时");address=0x08; max=23;mini=0;
	tiaozheng();
   	ds_w();
   	tiaozheng();
	}
    if(sel==4)  {lcm_w_word("分钟");address=0x01; max=59;mini=0;
   	tiaozheng();
   	ds_w();
   	tiaozheng();
  
	}		
	 	//分钟6，按动6次显示 调整分钟
	//并指定分钟数据写入1307芯片的地址是0x01，分钟数据的最大值是59，最小值是0
	if(sel==3)  {lcm_w_word("小时");address=0x02; max=23;mini=0;
  
    	tiaozheng();
    	ds_w();
   	    tiaozheng();
  
	}	//小时5，按动5次显示 调整小时
		//规定小时数据写入1307芯片的位置是0x02，小时数据最大值23，最小值是0
    	if(sel==2)  {lcm_w_word("日期");
		address=0x04; 
		mo = read_clock(0x05);//读月数据
		moontemp1=mo/16;
		moontemp2=mo%16;
		mo=moontemp1*10+moontemp2;//转换成10进制月份数据

        yy = read_clock(0x06);//读年数据
        moontemp1=yy/16;
		moontemp2=yy%16;
		yy=moontemp1*10+moontemp2;//转换成10进制年份数据 

        if(mo==2&&yy%4!=0){max=28;mini=1;}//平年2月28天
		if(mo==2&&yy%4==0){max=29;mini=1;}//闰年2月29天
		if(mo==1||mo==3||mo==5||mo==7||mo==8||mo==10||mo==12){max=31;mini=1;}//31天的月份
		if(mo==4||mo==6||mo==9||mo==11){max=30;mini=1;}//30天的月份
		tiaozheng();
		ds_w();
		tiaozheng(); //调用日期、时间调整函数
 
  }	//日3，按动3次显示 调整日期
	//规定日期数据写入1302的位置地址是0x86，日期最大值31，最小值是1
  if(sel==1)  {lcm_w_word("月份");address=0x05; max=12;mini=1;
     tiaozheng();
     ds_w();
     tiaozheng();
  
   }	//月2，按动2次显示 调整月份
        //规定月份写入1302的位置地址是0x88，月份最大值12，最小值1
  if(sel==0)  {lcm_w_word("年份");address=0x06; max=99; mini=0;
     tiaozheng();
     ds_w();		//被调数据加一或减一函数
     tiaozheng();	//调用日期、时间调整函数

   }	//年1，按动1次显示 调整年份，
		//规定年份写入1302的地址是0x8c,年份的最大值99，最小值0

}

/*****************************************************************************/
//被调数据加一或减一，并检查数据范围，写入1302指定地址保存
void ds_w(void){

	item=((read_clock(address))/16)*10 + (read_clock(address))%16;
	if(K3 == 0){//如果按动上调键
		item++;//数加 1  
	}
	if(K4 == 0){//如果按动下调键
  		item--;//数减 1 
	}
	if(item>max) item=mini;//查看数值是否在有效范围之内   
	if(item<mini) item=max;//如果数值小于最小值，则自动等于最大值           

	write_clock(address,(item/10)*16+item%10);//转换成16进制写入1307

}
/****************************************************************************/

/*********************************************************************/
void Timer0_Init(void) //定时器0初始（主函数中被调用一次）
{
	TMOD=0x01;//定时器方式设置（定时器工作方式1-16位定时）
	TH0=(65535-50000)/256;//50MS
	TL0=(65535-50000)%256;
	EA=1;//开总中断
	ET0=1;//开定时器0中断允许 
	TR0=1;//启动定时器0
}
/*********************************************************************************/
/*主函数---------------------------------------------------------------------*/
void main()
{
                         
	uchar e = 0,a = 0;
	K1=1;K2=1;K3=1;K4=1;K5=1;LCD=1;beep=1;
    init();	
	lcm_init();			//液晶初始化
    Init_1307();		//时钟芯片初始化
	welcome();    		//调用欢迎信息
	DelayM(200); 		//延时
	lcm_clr();
	Clean_12864_GDRAM();	 //清屏
	Timer0_Init(); 
	while(1)
	{
		if (w == 0) 		 //正常走时
		{ 
			displaydate();	 //显示日期
			nongli();		 //显示农历
			displaytime();	 //显示时间
			displaylamptemp(); //显示温度		 
			displayxq();	 //显示星期
			
		}	
/*----------------------------设置时间--------------------------------------*/		
		if (K1 == 0)         
		{
			DelayM(20);	               //按键消抖
			if(K1 == 0 && w == 1)      //当是调时状态 本键用于调整下一项
			{
				e++;
				if (e >= 7 ) {e = 0;}
		   	while(! K1 );            //等待键松开 
				Set_time(e);           //调整				
			}			
			if(K1 == 0 && w == 0)      //当是正常状态时就进入调时状态
			{
				lcm_clr();            
				Clean_12864_GDRAM();   //清屏
				w=1;	               //进入调时
				Set_time(e);
			}
		   	while(K1 == 0);            //等待键松开 
		}
/*--------------------------------------------------------------------------*/		
		if (K2 == 0)                   // 当在调时状态时就退出调时
		{
		   	BEEP_bit =1; 
			DelayM(20);
			if(K2 == 0 && w == 1)
			{				
				w = 0;	               //退出调时
				e = 0;		           //"下一项"计数器清0								
			}
			if(K2 == 0 && w == 0) 
			{				
				lcm_clr(); Clean_12864_GDRAM();
				write_com(0x30); write_com(0x06);
				welcome();				
				while(K2 == 0);  
			}
			lcm_clr(); Clean_12864_GDRAM();
			displaydate();
			displayxq(); 
			displaynl();
			displaytime();
			displaylamptemp();
		  
			while(K2 == 0);  
		}
/*加调整--------------------------------------------------------------------*/		
		if (K3 == 0 && w == 1)
		{ 
			DelayM(20);
			if(K3 == 0 && w == 1) { Set_time(e); }
			while(! K3 );
		}
/*减调整--------------------------------------------------------------------*/		
		if (K4 == 0 && w == 1) 
		{       	
			DelayM(20);
			if(K4 == 0 && w == 1) { Set_time(e); }
			while(! K4 );
		}
/*液晶背光控制，按一下亮，再按一下灭----------------------------------------*/
		if(K5 == 0)        
		{
		
			DelayM(20); q = ~q;      //标志位取反
		 	if(q){LCD = LCD| 1;}   
		 		else {LCD = LCD & 0;}   
			while(K5 == 0);           
		}
/**********************************************************************************/
       			   /*----------验证闹钟---------------*/
				   
	     if(read_clock(0x08) == read_clock(0x02)&&read_clock(0x09) == read_clock(0x01))
		 {
			     char i;  
		 		if(BEEP_bit == 1 && w == 0)
			    {
					flag = 1;


				
				for(i=0;i<4;i++)
				   {
					    if(i>4)i = 0;
					    beep = 0;  //打开峰鸣器
						DelayM(80);//延时
						beep = 1;  //关闭峰鸣器
					   	DelayM(125);
				   }			
				}
				else if(flag == 0)
				{
					BEEP_bit =1; 
					
				}
		 }
		 else
		 {
		 	  flag = 0;
		 }

    }
}
/**********************************************************************************************/	
void tiemr0()interrupt 1//定时器0初始（主函数中被调用一次）
{
	
	TH0=(65535-50000)/256;//50MS
	TL0=(65535-50000)%256;
	if(K4 == 0 && flag == 1)
	{
	   	DelayM(20);
		BEEP_bit = 0;
		beep = 1;
		while(!K4);
	}
}
